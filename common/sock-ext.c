/*
 * sock-ext.c
 *
 *  Created on: 2016/6/9
 *      Author: Luo Guochun
 */

#include "cmmhdr.h"
#include "coro.h"

#ifndef RESOLVE_THREAD_COUNT
static const int THREAD_COUNT = 4;
#else
static const int THREAD_COUNT = RESOLVE_THREAD_COUNT;
#endif

enum {
	RESOLV_HOST_SIZE = 128,
	RESOLV_ADDR_SIZE = 64,
};

typedef struct resov_data_s
{
	char* host;
	char* addr;
}resov_data_t;

struct thread_info_s
{

};

typedef struct thread_info_s thread_info_t;

__CONSTRCT(resolve, pthread)
{

}

int tcp_noblock_resolve(coro_t* coro)
{

}
resov_data_t* tcp_reslove_data(const char* host)
{

}
int tcp_reslove_data_free(resov_data_t* data)
{

}
