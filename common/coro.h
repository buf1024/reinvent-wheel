/*
 * coro.h
 *
 *  Created on: 2016/6/6
 *      Author: Luo Guochun
 */

#ifndef __CORO_H__
#define __CORO_H__

#include <stddef.h>

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

typedef struct coro_schedule_s coro_schedule_t;
typedef int (*coro_schedule_done_t)(coro_schedule_t*, coro_t* coro);
typedef int (*coro_schedule_timer_t)(coro_schedule_t*, int);

// coro
coro_t* coro_new(coro_fun_t fun, void* data, size_t stack_size);
int coro_resume(coro_t* coro);
int coro_yield_value(coro_t* coro, int value);
int coro_state(coro_t* coro);
void* coro_data(coro_t* coro);
int coro_free(coro_t* coro);

// coro schedule
coro_schedule_t* coro_schedule_new(coro_schedule_done_t fun_done,
		coro_schedule_timer_t fun_timer, int timer_value, void* sche_data);
int coro_schedule_add(coro_schedule_t* sche, coro_t* coro);
int coro_schedule_del(coro_schedule_t* sche, coro_t* coro);
void* coro_schedule_data(coro_schedule_t* sche);
int coro_schedule_run(coro_schedule_t* sche);
int coro_schedule_stop(coro_schedule_t* sche);
int coro_schedule_free(coro_schedule_t* sche);

#ifdef __cplusplus
}
#endif

#endif /* __CORO_H__ */
