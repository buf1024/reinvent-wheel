/*
 * cororesolve.c
 *
 *  Created on: 2016/6/9
 *      Author: Luo Guochun
 */

#include "sock-ext.h"
#include "coro.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>


typedef void sigfunc(int);
static sigfunc* set_signal(int signo, sigfunc* func, int interupt);

#define REGISTER_SIGNAL(signo, func, interupt)            \
    do {                                                  \
        if(set_signal(signo, func, interupt) == SIG_ERR){ \
            printf("register signal %d failed:%s\n",      \
                    signo, strerror(errno));              \
            return -1;                                    \
        }                                                 \
    }while(0)


sigfunc* set_signal(int signo, sigfunc* func, int interupt)
{
    struct sigaction act, oact;
    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if (!interupt){
        act.sa_flags |= SA_RESTART;
    }
    if (sigaction(signo, &act, &oact) < 0) {
        return SIG_ERR ;
    }
    return oact.sa_handler;
}

static int g_term = 0;
static int g_usr2 = 0;

static void sigterm(int signo) {
	g_term = 1;
}

static void sigusr2(int signo) {
	g_usr2 = 1;
}

typedef struct resov_node_s
{
    coro_t* coro;
    SLIST_ENTRY(resov_node_s) next;
}resov_node_t;

typedef SLIST_HEAD(_queue, resov_node_s) resov_t;

int main(int argc, char **argv) {

    REGISTER_SIGNAL(SIGTERM, sigterm, 0);//Kill信号
    REGISTER_SIGNAL(SIGINT, sigterm, 0);//终端CTRL-C信号
    REGISTER_SIGNAL(SIGUSR2, sigusr2, 0);//SIGUSR2信号

    resov_t r;

    SLIST_INIT(&r);

    resov_data_t* d = tcp_resolve_set_data("python.org", false);
    coro_t* c = coro_new(tcp_noblock_resolve, d, 0);
    resov_node_t* n = (resov_node_t*)malloc(sizeof(*n));
    n->coro = c;
    SLIST_INSERT_HEAD(&r, n, next);

    d = tcp_resolve_set_data("baidu.com", false);
    c = coro_new(tcp_noblock_resolve, d, 0);
    n = (resov_node_t*)malloc(sizeof(*n));
    n->coro = c;
    SLIST_INSERT_HEAD(&r, n, next);

    d = tcp_resolve_set_data("aaagoogleyyasdf.com", false);
    c = coro_new(tcp_noblock_resolve, d, 0);
    n = (resov_node_t*)malloc(sizeof(*n));
    n->coro = c;
    SLIST_INSERT_HEAD(&r, n, next);

    int i=0;
    for(;i<100; i++) {
        char host[64] = {0};
        snprintf(host, sizeof(host) -1, "python%d.org", i);

        d = tcp_resolve_set_data(host, false);
        c = coro_new(tcp_noblock_resolve, d, 0);
        n = (resov_node_t*)malloc(sizeof(*n));
        n->coro = c;
        SLIST_INSERT_HEAD(&r, n, next);
    }

    while (1) {
    	if(g_term) break;
        SLIST_FOREACH(n, &r, next) {
            coro_t* c = n->coro;
            int state = coro_get_state(c);
            if(state == CORO_RESUME) {
                coro_resume(c);
            }else{
                resov_data_t* d = (resov_data_t*)coro_get_data(c);

                char host[64] = {0}, addr[64] = {0};

                tcp_resolve_data_host(d, host);
                tcp_resolve_data_addr(d, addr);

                if(state == CORO_FINISH) {
                    printf("state(%d) resolve, host %s  -> %s\n", state, host, addr);
                }

                if(state == CORO_ABORT) {
                    printf("fail to resolve, host %s\n", host);
                }
                tcp_resolve_data_free(d);
                coro_free(c);

                SLIST_REMOVE(&r, n, resov_node_s, next);

                free(n);
            }
        }
        if(SLIST_EMPTY(&r)) break;

        printf("sleep 1s, for iteration end.\n");
        sleep(1);

        fflush(stdout);
    }

    printf("sleep 60s\n");
    sleep(60);


	return 0;
}

