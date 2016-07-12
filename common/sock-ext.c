/*
 * sock-ext.c
 *
 *  Created on: 2016/6/9
 *      Author: Luo Guochun
 */

#include "sock-ext.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#ifndef RESOLVE_THREAD_COUNT
static const int __THREAD_COUNT = 4;
#else
static const int __THREAD_COUNT = RESOLVE_THREAD_COUNT;
#endif

#ifndef RESOLVE_SLEEP_TIME
static const int __SLEEP_TIME = 3;
#else
static const int __SLEEP_TIME = RESOLVE_SLEEP_TIME;
#endif

#define __ADDR_SIZE 64

// pipe 的操作是原子性的
typedef struct resov_thread_info_s
{
    int fd_req[2];
    int fd_rst[2];

    int count;
    pthread_t* thread;
}resov_thread_info_t;

typedef struct resov_data_s resov_data_t;

struct resov_data_s
{
    char* host;
    char* addr;
    bool resov;
    void* data;
};

static int __put_resolve_data(int fd, resov_data_t* d)
{
    intptr_t ptr = (intptr_t)d;

    int rv = write(fd, &ptr, sizeof(void*));

    return rv;
}
static resov_data_t* __get_resolve_data(int fd)
{
	resov_data_t* d = NULL;
    intptr_t ptr = 0;

    int rv = read(fd, &ptr, sizeof(void*));
    if(rv > 0) {
        d = (resov_data_t*)ptr;
    }

    return d;
}

static void* __resov_thread(void* d)
{
    resov_thread_info_t* t = (resov_thread_info_t*)d;

    while (true) {
        fd_set rd;
        FD_ZERO(&rd);
        FD_SET(t->fd_req[0], &rd);

        struct timeval tv = { 0 };
        tv.tv_sec = __SLEEP_TIME;
        tv.tv_usec = 0;

        int rv = select(t->fd_req[0] + 1, &rd, NULL, NULL, &tv);
        if (rv > 0) {

        	if(!FD_ISSET(t->fd_req[0], &rd)) {
        		continue;
        	}
        	resov_data_t* reso = __get_resolve_data(t->fd_req[0]);
            if (!reso)
                continue;

            printf("thread %ld start to resolve %s\n", pthread_self(), reso->host);

            bool state = false;
            reso->addr = malloc(__ADDR_SIZE);
            if (tcp_resolve(reso->host, reso->addr, __ADDR_SIZE) == 0) {
            	state = true;
            } else {
            	state = false;
            }
            printf("resov done: %s\n", reso->host);
            reso->resov = state;
            __put_resolve_data(t->fd_rst[1], reso);
        }else if(rv == 0) {
            //printf("thread %ld select timeout\n", pthread_self());
        }else{
            //printf("thread %ld select error, errno = %d\n", pthread_self(), errno);
            break;
        }
    }

    return d;
}

static resov_thread_info_t* __create_resov_thread(int thread)
{
	static resov_thread_info_t* resov = NULL;

	if (!resov) {
		resov = (resov_thread_info_t*)malloc(sizeof(*resov));
		memset(resov, 0, sizeof(*resov));
		pipe(resov->fd_req);
		pipe(resov->fd_rst);

		resov->count = __THREAD_COUNT;
        if (thread > 0) {
            resov->count = thread;
        }

		resov->thread = (pthread_t*)malloc(resov->count * sizeof(pthread_t));


		int i = 0;
		for (i = 0; i < resov->count; i++) {
			pthread_t tid;
			pthread_create(&tid, NULL, __resov_thread, resov);
			printf("thread %d started, thread id = %ld\n", i, tid);
			resov->thread[i] = tid;
		}
	}

    if (resov) {
        if (thread > resov->count) {
            int diff = thread - resov->count;
            resov->thread = realloc(resov->thread, thread * sizeof(pthread_t));
            int i = 0;
            for (i = 0; i < diff; i++) {
                pthread_t tid;
                pthread_create(&tid, NULL, __resov_thread, resov);
                printf("new thread %d started, thread id = %ld\n", i, tid);
                resov->thread[i + resov->count] = tid;
            }
            resov->count = thread;
        }
    }
    return resov;
}

int tcp_noblock_resolve(const char* host, void* data, int thread)
{
	resov_thread_info_t* resov = __create_resov_thread(thread);
	if(!resov) return -1;

    resov_data_t* d = (resov_data_t*)malloc(sizeof(*d));
    if(!d) return -1;
    memset(d, 0, sizeof(*d));

    d->host = strdup(host);
    d->data = data;

    if (__put_resolve_data(resov->fd_req[1], d) != 0) {
        return -1;
    }

    return resov->fd_rst[0];
}
int tcp_noblock_resolve_fd()
{
	resov_thread_info_t* resov = __create_resov_thread(0);
	if(!resov) return -1;

	return resov->fd_rst[0];
}

bool tcp_noblock_resolve_result(char** host, char** addr, void** data)
{
	if(addr) *addr = NULL;
	if(data) *data = NULL;

	int fd = tcp_noblock_resolve_fd();
	if(fd <= 0) return -1;

	resov_data_t* reso = __get_resolve_data(fd);

	if(!reso) return false;

	bool state = reso->resov;

	if(reso->resov) {
		if(addr) *addr = reso->addr;
	}
	if(data) *data = reso->data;
	if(host) *host = reso->host;
	free(reso);
	return state;
}
