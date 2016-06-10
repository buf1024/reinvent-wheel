/*
 * sock-ext.c
 *
 *  Created on: 2016/6/9
 *      Author: Luo Guochun
 */

#include "cmmhdr.h"
#include "coro.h"
#include "sock.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#ifndef RESOLVE_THREAD_COUNT
static const int __THREAD_COUNT = 4;
#else
static const int __THREAD_COUNT = RESOLVE_THREAD_COUNT;
#endif

#ifndef RESLOVE_SLEEP_TIME
static const int __SLEEP_TIME = 200;
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

typedef struct resov_queue_node_s
{
    coro_t* coro;
    SLIST_ENTRY(resov_queue_node_s) next;
}resov_queue_node_t;

typedef SLIST_HEAD(resov_queue, resov_queue_node_s) resov_queue_t;

typedef struct resov_thread_info_s
{
    pthread_mutex_t locker;
    resov_queue_t queue;
}resov_thread_info_t;

static resov_thread_info_t __resov;

static int __put_queue_data(resov_thread_info_t* t, coro_t* d)
{
    pthread_mutex_lock(&t->locker);

    resov_queue_node_t* n = (resov_queue_node_t*)malloc(sizeof(*n));
    if(!n) {
        pthread_mutex_unlock(&t->locker);
        return -1;
    }

    n->coro = d;

    SLIST_INSERT_HEAD(&t->queue, n, next);

    pthread_mutex_unlock(&t->locker);

    return 0;
}
static coro_t* __get_queue_data(resov_thread_info_t* t)
{
    coro_t* co = NULL;

    pthread_mutex_lock(&t->locker);

    if(!SLIST_EMPTY(&t->queue)) {

        resov_queue_node_t* n = SLIST_FIRST(&t->queue);
        if(n) {
            co = n->coro;
            SLIST_REMOVE_HEAD(&t->queue, next);

            free(n);
        }
    }

    pthread_mutex_unlock(&t->locker);

    return co;
}

static void* __resov_thread(void* d)
{
    resov_thread_info_t* t = (resov_thread_info_t*)d;
    while(true) {
        if(SLIST_EMPTY(&t->queue)) {
            usleep(__SLEEP_TIME);
            continue;
        }

        coro_t* co = __get_queue_data(t);
        if(!co) continue;

        resov_data_t* reso = (resov_data_t*)coro_get_data(co);
        if(!co) continue;

        if(tcp_resolve(reso->host, reso->addr, sizeof(reso->addr)) == 0) {
            coro_set_state(co, CORO_FINISH);
        }else{
            coro_set_state(co, CORO_ABORT);
        }
    }

    return d;
}

__CONSTRCT(resolve, pthread)
{
    memset(&__resov, 0, sizeof(__resov));

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
    while(coro_get_state(coro) != CORO_FINISH) {
        coro_yield_value(coro, CORO_RESUME);
    }

    return CORO_FINISH;
}
resov_data_t* tcp_reslove_data(const char* host)
{
    resov_data_t* d = (resov_data_t*)malloc(sizeof(*d));
    if(d) {
        memset(d, 0, sizeof(*d));

        strncpy(d->host, host, sizeof(d->host));
    }
    return d;
}

void tcp_reslove_data_free(resov_data_t* data)
{
    if(data) {
        free(data);
    }
}

int tcp_reslove_data_host(resov_data_t* data, char* host)
{
    if(!data) return -1;

    strncpy(host, data->host, sizeof(data->host));

    return 0;
}
int tcp_reslove_data_addr(resov_data_t* data, char* addr)
{
    if(!data) return -1;

    strncpy(addr, data->addr, sizeof(data->addr));

    return 0;
}
