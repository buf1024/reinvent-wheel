/*
 * coro.c
 *
 *  Created on: 2016/6/6
 *      Author: Luo Guochun
 */

#include "coro.h"


#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <sys/resource.h>

#include <stddef.h>
#if defined(__x86_64__)
#include <stdint.h>
typedef uintptr_t coro_context_t[10];
#elif defined(__i386__)
#include <stdint.h>
typedef uintptr_t coro_context_t[7];
#else
#include <ucontext.h>
typedef ucontext_t coro_context_t;
#endif

enum
{
	EIP = 0,
	ESP = 7,
};
enum
{
	RDI = 6,
	RSI = 7,
	RIP = 8,
	RSP = 9,
};

enum {
	CORO_MIN_STACK_SIZE  = 4*1024,
};

enum {
	CORO_STATUS_READY    = 0,
	CORO_STATUS_RUNNING,
	CORO_STATUS_SUSPEND,
	CORO_STATUS_DEATH,
};

struct coro_s {
	coro_context_t caller;
	coro_context_t callee;
	coro_context_t context;

	coro_fun_t fun;
	void* data;

	int status;
	int yield_value;

	size_t stack_size;
	unsigned char* stack;
};


#if defined(__x86_64__)
void __attribute__((noinline))
coro_swap_context(coro_context_t *current, coro_context_t *other);
    asm(
    ".text\n\t"
    ".p2align 4\n\t"
#if defined(__APPLE__)
    ".globl _coro_swap_context\n\t"
    "_coro_swap_context:\n\t"
#else
    ".globl coro_swap_context\n\t"
    "coro_swap_context:\n\t"
#endif
    "mov    %rbx,0(%rdi)\n\t"
    "mov    %rbp,8(%rdi)\n\t"
    "mov    %r12,16(%rdi)\n\t"
    "mov    %r13,24(%rdi)\n\t"
    "mov    %r14,32(%rdi)\n\t"
    "mov    %r15,40(%rdi)\n\t"
    "mov    %rdi,48(%rdi)\n\t"
    "mov    %rsi,56(%rdi)\n\t"
    "mov    (%rsp),%rcx\n\t"
    "mov    %rcx,64(%rdi)\n\t"
    "lea    0x8(%rsp),%rcx\n\t"
    "mov    %rcx,72(%rdi)\n\t"
    "mov    72(%rsi),%rsp\n\t"
    "mov    0(%rsi),%rbx\n\t"
    "mov    8(%rsi),%rbp\n\t"
    "mov    16(%rsi),%r12\n\t"
    "mov    24(%rsi),%r13\n\t"
    "mov    32(%rsi),%r14\n\t"
    "mov    40(%rsi),%r15\n\t"
    "mov    48(%rsi),%rdi\n\t"
    "mov    64(%rsi),%rcx\n\t"
    "mov    56(%rsi),%rsi\n\t"
    "jmp    *%rcx\n\t");
#elif defined(__i386__)
void __attribute__((noinline))
coro_swap_context(coro_context_t *current, coro_context_t *other);
    asm(
    ".text\n\t"
    ".p2align 16\n\t"
#if defined(__APPLE__)
    ".globl _coro_swap_context\n\t"
    "_coro_swap_context:\n\t"
#else
    ".globl coro_swap_context\n\t"
    "coro_swap_context:\n\t"
#endif
    "movl   0x4(%esp),%eax\n\t"
    "movl   %ecx,0x1c(%eax)\n\t" /* ECX */
    "movl   %ebx,0x0(%eax)\n\t"  /* EBX */
    "movl   %esi,0x4(%eax)\n\t"  /* ESI */
    "movl   %edi,0x8(%eax)\n\t"  /* EDI */
    "movl   %ebp,0xc(%eax)\n\t"  /* EBP */
    "movl   (%esp),%ecx\n\t"
    "movl   %ecx,0x14(%eax)\n\t" /* EIP */
    "leal   0x4(%esp),%ecx\n\t"
    "movl   %ecx,0x18(%eax)\n\t" /* ESP */
    "movl   8(%esp),%eax\n\t"
    "movl   0x14(%eax),%ecx\n\t" /* EIP (1) */
    "movl   0x18(%eax),%esp\n\t" /* ESP */
    "pushl  %ecx\n\t"            /* EIP (2) */
    "movl   0x0(%eax),%ebx\n\t"  /* EBX */
    "movl   0x4(%eax),%esi\n\t"  /* ESI */
    "movl   0x8(%eax),%edi\n\t"  /* EDI */
    "movl   0xc(%eax),%ebp\n\t"  /* EBP */
    "movl   0x1c(%eax),%ecx\n\t" /* ECX */
    "ret\n\t");
#else
#define coro_swap_context(cur,oth) swapcontext(cur, oth)
#endif

static int coro_yield_value_status(coro_t* coro, int value, int status) {
 	coro->yield_value = value;
   	coro->status = status;
   	coro_swap_context(&coro->callee, &coro->caller);
   	return coro->yield_value;
}

static void coro_entry(coro_t* coro, coro_fun_t fun) {
	int yield_value = fun(coro);
   	coro_yield_value_status(coro, yield_value, CORO_STATUS_DEATH);
}



static void coro_make_context(coro_t *coro)
{
	unsigned char* stack = coro->stack;
    #if defined(__x86_64__)
        coro->context[RDI /* RDI */] = (uintptr_t) coro;
        coro->context[RSI /* RSI */] = (uintptr_t) coro->fun;
        coro->context[RIP /* RIP */] = (uintptr_t) coro_entry;
        coro->context[RSP /* RSP */] = (uintptr_t) stack + coro->stack_size;
    #elif defined(__i386__)
        /* Align stack and make room for two arguments */
        stack = (unsigned char *)((uintptr_t)(stack + coro->stack_size -
            sizeof(uintptr_t) * 2) & 0xfffffff0);

        uintptr_t *argp = (uintptr_t *)stack;
        *argp++ = 0;
        *argp++ = (uintptr_t)coro;
        *argp++ = (uintptr_t)coro->fun;

        coro->context[EIP /* EIP */] = (uintptr_t) coro_entry;
        coro->context[ESP /* ESP */] = (uintptr_t) stack;
    #else
        getcontext(&coro->context);

        coro->context.uc_stack.ss_sp = stack;
        coro->context.uc_stack.ss_size = coro->stack_size;
        coro->context.uc_stack.ss_flags = 0;
        coro->context.uc_link = NULL;

        makecontext(&coro->context, (void (*)())coro_entry, 2, coro, coro->fun);
    #endif
}


coro_t* coro_new(coro_fun_t fun, void* data, size_t stack_size)
{
	static size_t config_stack_size = 0;
	if (config_stack_size == 0) {
		config_stack_size = CORO_MIN_STACK_SIZE;
		struct rlimit r;
		if (getrlimit(RLIMIT_STACK, &r) == 0) {
			config_stack_size = r.rlim_cur;
		}
	}

	coro_t* coro = (coro_t*) malloc(sizeof(*coro));
	if(coro) {
		memset(coro, 0, sizeof(*coro));

		if (stack_size < CORO_MIN_STACK_SIZE) {
			stack_size = CORO_MIN_STACK_SIZE;
		}
		if (stack_size > config_stack_size) {
			stack_size = config_stack_size;
		}

		if (stack_size & 0xfff) {
			stack_size &= ~0xfff;
			stack_size += 0x1000;
		}
		coro->stack_size = stack_size;
		coro->stack = (unsigned char*)malloc(coro->stack_size);
		if(!coro->stack) {
			free(coro);
			return NULL;
		}
        coro->data = data;
        coro->fun = fun;
    }
    return coro;
}

int coro_resume(coro_t* coro)
{
	assert(coro->status == CORO_STATUS_READY ||
			coro->status == CORO_STATUS_SUSPEND);
	if(coro->status == CORO_STATUS_READY) {
		coro_make_context(coro);
		coro->status = CORO_STATUS_RUNNING;
	}
	if(coro->status == CORO_STATUS_SUSPEND)
	{
		coro->status = CORO_STATUS_RUNNING;
	}

	coro_swap_context(&coro->caller, &coro->context);

	if (coro->status != CORO_STATUS_DEATH) {
		memcpy(&coro->context, &coro->callee, sizeof(coro->context));
	}
	return coro->yield_value;
}

int coro_yield_value(coro_t* coro, int value) {
	assert(coro->status == CORO_STATUS_RUNNING);
	return coro_yield_value_status(coro, value, CORO_STATUS_SUSPEND);
}

int coro_state(coro_t* coro) {
	return coro->yield_value;
}
void* coro_data(coro_t* coro) {
	return coro->data;
}

int coro_free(coro_t* coro) {
    if(coro) {
    	if(coro->stack) {
    		free(coro->stack);
    	}
        free(coro);
    }
	return 0;
}

