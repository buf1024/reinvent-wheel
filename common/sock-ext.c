/*
 * sock-ext.c
 *
 *  Created on: 2016/6/9
 *      Author: Luo Guochun
 */

#include "cmmhdr.h"
#include "sock.h"
#include "sock-ext.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

#ifndef RESOLVE_THREAD_COUNT
static const int __THREAD_COUNT = 4;
#else
static const int __THREAD_COUNT = RESOLVE_THREAD_COUNT;
#endif

#ifndef RESLOVE_SLEEP_TIME
static const int __SLEEP_TIME = 60;
#else
static const int __SLEEP_TIME = RESLOVE_SLEEP_TIME;
#endif


enum {
    RESOLV_HOST_SIZE = 128,
    RESOLV_ADDR_SIZE = 64,
};

typedef struct resov_data_s
{
    char host[RESOLV_HOST_SIZE];
    char addr[RESOLV_ADDR_SIZE];
}resov_data_t;

typedef struct resov_thread_info_s
{
    pthread_mutex_t locker;
    int fd[2];
}resov_thread_info_t;

static resov_thread_info_t __resov;

static int __put_queue_data(resov_thread_info_t* t, coro_t* d)
{
    intptr_t ptr = (intptr_t)d;

    int rv = write(t->fd[1], &ptr, sizeof(void*));
    if(rv != sizeof(void*)) {
        return -1;
    }

    return 0;
}
static coro_t* __get_queue_data(resov_thread_info_t* t)
{
    coro_t* co = NULL;
    intptr_t ptr = 0;

    pthread_mutex_lock(&t->locker);
    int rv = read(t->fd[0], &ptr, sizeof(void*));
    if(rv > 0) {
        co = (coro_t*)ptr;
    }
    pthread_mutex_unlock(&t->locker);

    return co;
}

static void* __resov_thread(void* d)
{
    resov_thread_info_t* t = (resov_thread_info_t*)d;

    while (true) {
        fd_set rd;
        FD_ZERO(&rd);
        FD_SET(t->fd[0], &rd);

        struct timeval tv = { 0 };
        tv.tv_sec = __SLEEP_TIME;
        tv.tv_usec = 0;

        int rv = select(t->fd[0] + 1, &rd, NULL, NULL, &tv);
        if (rv > 0) {

            coro_t* co = __get_queue_data(t);
            if (!co)
                continue;

            resov_data_t* reso = (resov_data_t*) coro_get_data(co);
            if (!co)
                continue;

            printf("thread %ld start to resolve %s\n", pthread_self(),
                    reso->host);

            if (tcp_resolve(reso->host, reso->addr, sizeof(reso->addr)) == 0) {
                coro_set_state(co, CORO_FINISH);
            } else {
                coro_set_state(co, CORO_ABORT);
            }
        }else if(rv == 0) {
            printf("thread %ld select timeout\n", pthread_self());
        }else{
            printf("thread %ld select error, errno = %d\n", pthread_self(), errno);
            break;
        }
    }

    return d;
}

__CONSTRCT(resolve, pthread)
{
    memset(&__resov, 0, sizeof(__resov));
    pipe(__resov.fd);
    tcp_noblock(__resov.fd[0], true);

    int i = 0;
    for(i=0; i<__THREAD_COUNT; i++) {
        pthread_t tid;
        pthread_create(&tid, NULL, __resov_thread, &__resov);

        printf("thread %d started, thread id = %ld\n", i, tid);
    }
}

int tcp_noblock_resolve(coro_t* coro)
{
    if(__put_queue_data(&__resov, coro) != 0) {
        return CORO_ABORT;
    }
    while(coro_get_state(coro) == CORO_RESUME) {
        coro_yield_value(coro, CORO_RESUME);
    }

    return coro_get_state(coro);

    return 0;
}
resov_data_t* tcp_resolve_data(const char* host)
{
    resov_data_t* d = (resov_data_t*)malloc(sizeof(*d));
    if(d) {
        memset(d, 0, sizeof(*d));

        strncpy(d->host, host, sizeof(d->host));
    }
    return d;
}

void tcp_resolve_data_free(resov_data_t* data)
{
    if(data) {
        free(data);
    }
}

int tcp_resolve_data_host(resov_data_t* data, char* host)
{
    if(!data) return -1;

    strncpy(host, data->host, sizeof(data->host));

    return 0;
}
int tcp_resolve_data_addr(resov_data_t* data, char* addr)
{
    if(!data) return -1;

    strncpy(addr, data->addr, sizeof(data->addr));

    return 0;
}
