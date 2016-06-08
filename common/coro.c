/*
 * coro.c
 *
 *  Created on: 2016/6/6
 *      Author: Luo Guochun
 */

#include "coro.h"

#include <ucontext.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

enum {
	CORO_DEF_STACK_SIZE  = 8192,
};

struct coro_s
{
    ucontext_t callee, caller, context;

    void* data;

    bool call_end;
    int yield_value;
};

static void coro_entry(coro_t* coro, coro_fun_t func)
{
	int value = func(coro);
	coro->call_end = true;
	coro_yield_value(coro, value);
}

coro_t* coro_new(coro_fun_t fun, void* data, int coro_stack)
{
	int stack_size = coro_stack <= 0 ? CORO_DEF_STACK_SIZE : coro_stack;
	coro_t* coro = (coro_t*)malloc(sizeof(*coro) + stack_size);
	if (coro) {
		memset(coro, 0, sizeof(*coro)); // leave stack
		coro->data = data;

		getcontext(&coro->context);

		unsigned char* stack = (unsigned char*) (coro + 1);

		coro->context.uc_stack.ss_flags = 0;
		coro->context.uc_stack.ss_sp = (void*) stack;
		coro->context.uc_stack.ss_size = stack_size;
		coro->context.uc_link = NULL;

		makecontext(&coro->context, (void (*)()) coro_entry, 2, coro, fun);

	}

	return coro;
}

int coro_resume(coro_t* coro)
{
	if(swapcontext(&coro->caller, &coro->context) != 0) {
		return CORO_ABORT;
	}
	if(!coro->call_end) {
		memcpy(&coro->context, &coro->callee, sizeof(coro->context));
	}
	return coro->yield_value;
}

int coro_yield_value(coro_t* coro, int value)
{

	coro->yield_value = value;
	if(swapcontext(&coro->callee, &coro->caller) != 0) {
		return CORO_ABORT;
	}

	return coro->yield_value;
}

void* coro_data(coro_t* coro)
{
	return coro->data;
}
int coro_free(coro_t* coro)
{
	free(coro);
	return CORO_FINISH;
}

