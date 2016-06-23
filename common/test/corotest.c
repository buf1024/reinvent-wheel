/*
 * corotest.c
 *
 *  Created on: 2016/6/6
 *      Author: Luo Guochun
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "../coro.h"

static int testfunc(coro_t* coro)
{
	int sim_block_times = 0;
	const int BLOCK_STOP_TIME = 3;
	while(1) {
		sim_block_times++;
		if(sim_block_times > BLOCK_STOP_TIME) {
			printf("%s call end\n", __func__);
			break;
		}
		printf("%s call block, block times: %d\n", __func__, sim_block_times);
		coro_yield_value(coro, CORO_RESUME);
	}
	return CORO_FINISH;
}

int main(int argc, char **argv)
{
	coro_t* co1 = coro_new(testfunc, (void*)1, 0);
	coro_t* co2 = coro_new(testfunc, (void*)1, 0);

	char* buf = malloc(100);

	do {
		if(coro_get_state(co1) != CORO_FINISH) {
		    coro_resume(co1);
		}
		if(coro_get_state(co2) != CORO_FINISH) {
		    coro_resume(co2);
		}

		printf("main sleep 1 seconds\n");
		sleep(1);
	}while(coro_get_state(co1) != CORO_FINISH && coro_get_state(co2) != CORO_FINISH);
	free(buf);
	coro_free(co1);
	coro_free(co2);

	return 0;
}
