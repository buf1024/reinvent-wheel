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

enum {
	CORO_DEF_STACK_SIZE = 8192,
};

struct coro_s {

	coro_switcher_t* switcher;

	ucontext_t context;

	void* data;

	bool call_end;
	int yield_value;

	bool new_tag;
	int stack_size;
	coro_fun_t fun;
};


static void __coro_entry(coro_t* coro, coro_fun_t fun) {
	int value = fun(coro);
	coro->call_end = true;
	coro_yield_value(coro, value);
}

static coro_t* __coro_new(coro_switcher_t* switcher, bool new_tag,
        coro_fun_t fun, void* data, int coro_stack) {
    int stack_size = coro_stack <= 0 ? CORO_DEF_STACK_SIZE : coro_stack;
    coro_t* coro = (coro_t*) malloc(sizeof(*coro) + stack_size);
    if (coro) {
        memset(coro, 0, sizeof(*coro)); // leave stack
        coro->data = data;
        coro->switcher = switcher;
        coro->new_tag = new_tag;
        coro->stack_size =  stack_size;
        coro->fun = fun;

        coro_reset(coro);
    }
    return coro;
}

coro_t* coro_new(coro_fun_t fun, void* data, int coro_stack)
{
    coro_switcher_t* switcher = malloc(sizeof(coro_switcher_t));
    if(!switcher) return NULL;

    coro_t* coro = __coro_new(switcher, true, fun, data, coro_stack);
    if(!coro) {
        free(switcher);
    }
    return coro;
}

coro_t* coro_new2(coro_switcher_t* switcher, coro_fun_t fun, void* data, int coro_stack) {
	return __coro_new(switcher, false, fun, data, coro_stack);
}

int coro_reset(coro_t* coro)
{
    if (coro) {
    	coro->call_end = false;
    	coro->yield_value = CORO_RESUME;

        getcontext(&coro->context);

        unsigned char* stack = (unsigned char*) (coro + 1);

        coro->context.uc_stack.ss_flags = 0;
        coro->context.uc_stack.ss_sp = (void*) stack;
        coro->context.uc_stack.ss_size = coro->stack_size;
        coro->context.uc_link = NULL;

        makecontext(&coro->context, (void (*)()) __coro_entry, 2, coro, coro->fun);
    }
    return 0;
}

int coro_resume(coro_t* coro) {
	assert(coro->call_end == false);
	if (swapcontext(&coro->switcher->caller, &coro->context) != 0) {
		return CORO_ABORT;
	}
	if (!coro->call_end) {
		memcpy(&coro->context, &coro->switcher->callee, sizeof(coro->context));
	}

	return coro->yield_value;
}

int coro_yield_value(coro_t* coro, int value) {
	coro->yield_value = value;
	if (swapcontext(&coro->switcher->callee, &coro->switcher->caller) != 0) {
		return CORO_ABORT;
	}
	return coro->yield_value;
}

int coro_get_state(coro_t* coro) {
	return coro->yield_value;
}
int coro_set_state(coro_t* coro, int state) {
	coro->yield_value = state;

	return coro->yield_value;
}

void* coro_get_data(coro_t* coro) {
	return coro->data;
}

void coro_set_data(coro_t* coro, void* data) {
	coro->data = data;
}

int coro_free(coro_t* coro) {
    if(coro) {
        if(coro->new_tag) {
            free(coro->switcher);
        }
        free(coro);
    }
	return CORO_FINISH;
}

