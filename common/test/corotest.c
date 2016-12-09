/*
 * corotest.c
 *
 *  Created on: 2016/6/6
 *      Author: Luo Guochun
 */

#define C_TEST_APP
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "../coro.h"
#include "ctest.h"

static int testfunc(coro_t* coro)
{
	void* data = coro_data(coro);
	int id = (int)data;
	int sim_block_times = 0;
	const int BLOCK_STOP_TIME = 3;
	while(1) {
		sim_block_times++;
		if(sim_block_times > BLOCK_STOP_TIME) {
			printf("%s call end\n", __func__);
			break;
		}
		printf("coro(%d) %s call block, block times: %d\n", id, __func__, sim_block_times);
		coro_yield_value(coro, CORO_RESUME);
		sleep(1);
	}
	return CORO_FINISH;
}
static int coro_done(coro_schedule_t* sche, coro_t* coro)
{
	void* data = coro_data(coro);
	int id = (int)data;
	printf("coro done, id = %d\n", id);

	return 0;
}
static int coro_timer(coro_schedule_t* sche, int timer_value)
{
	printf("coro timer, timer=%d\n", timer_value);
	return 0;
}


TEST(coro, coro_normal)
{
	coro_t* co1 = coro_new(testfunc, (void*)1, 0);
	coro_t* co2 = coro_new(testfunc, (void*)2, 0);

	char* buf = malloc(100);

	do {
		if(coro_state(co1) != CORO_FINISH) {
		    coro_resume(co1);
		}
		if(coro_state(co2) != CORO_FINISH) {
		    coro_resume(co2);
		}

	}while(coro_state(co1) != CORO_FINISH && coro_state(co2) != CORO_FINISH);
	free(buf);
	coro_free(co1);
	coro_free(co2);
}

TEST(coro, coro_sche)
{
	coro_schedule_t* sche = coro_schedule_new(coro_done, coro_timer, 1000, NULL);
	for(int i=0; i<10; i++) {
		coro_t* co = coro_new(testfunc, (void*)i, 0);
		coro_schedule_add(sche, co);
	}

	coro_schedule_run(sche);

	coro_schedule_free(sche);
}

int main(int argc, char **argv)
{
	INIT_TEST(argc, argv);
	return RUN_ALL_TEST();
}
