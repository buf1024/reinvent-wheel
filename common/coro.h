/*
 * coro.h
 *
 *  Created on: 2016/6/6
 *      Author: Luo Guochun
 */

#ifndef __CORO_H__
#define __CORO_H__

#ifdef __cplusplus
extern "C" {
#endif

enum {
	CORO_ABORT     = -1,
	CORO_RESUME    = 0,
	CORO_FINISH    = 1,
};

typedef struct coro_s coro_t;

typedef int (*coro_fun_t)(coro_t* coro);

coro_t* coro_new(coro_fun_t fun, void* data, int stack_size);

int coro_resume(coro_t* coro);
int coro_yield_value(coro_t* coro, int value);

int coro_get_state(coro_t* coro);
int coro_set_state(coro_t* coro, int state);

void* coro_get_data(coro_t* coro);
void coro_set_data(coro_t* coro, void* data);

int coro_free(coro_t* coro);

#ifdef __cplusplus
}
#endif

#endif /* __CORO_H__ */
