/*
 * corotest.c
 *
 *  Created on: 2016/6/6
 *      Author: Luo Guochun
 */

#include <stdio.h>
#include <unistd.h>
#include "../coro.h"

static int testfunc(coro_t* coro)
{
	int sim_block_times = 0;
	const int BLOCK_STOP_TIME = 10;
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
	coro_t* co = coro_new(testfunc, NULL, 0);

	int ret = CORO_RESUME;

	while(ret != CORO_FINISH) {
		coro_resume(co);

		printf("main sleep 1 seconds\n");
		sleep(1);
	}

	return 0;
}
