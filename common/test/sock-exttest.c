/*
 * cororesolve.c
 *
 *  Created on: 2016/6/9
 *      Author: Luo Guochun
 */

#include "sock.h"
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


    resov_data_t* d = NULL;
    int fd = 0;

    d = (resov_data_t*)malloc(sizeof(*d));
    memset(d, 0, sizeof(*d));
    strcpy(d->host, "python.org");
    d->thread = 0;
    fd = tcp_noblock_resolve(d);

    d = (resov_data_t*)malloc(sizeof(*d));
    memset(d, 0, sizeof(*d));
    strcpy(d->host, "baidu.com");
    fd = tcp_noblock_resolve(d);

    d = (resov_data_t*)malloc(sizeof(*d));
    memset(d, 0, sizeof(*d));
    strcpy(d->host, "aaagoogleyyasdf.com");
    d->thread = 6;
    fd = tcp_noblock_resolve(d);

    int i=0;
    for(;i<100; i++) {

        d = (resov_data_t*)malloc(sizeof(*d));
        memset(d, 0, sizeof(*d));
        snprintf(d->host, sizeof(d->host) -1, "python%d.org", i);
        d->thread = 32;
        fd = tcp_noblock_resolve(d);
    }

    fd = tcp_noblock_resolve_pollfd();
    while (1) {
        fd_set rd;
        FD_ZERO(&rd);
        FD_SET(fd, &rd);

        struct timeval tv = { 0 };
        tv.tv_sec = 3;
        tv.tv_usec = 0;

        int rv = select(fd + 1, &rd, NULL, NULL, &tv);
        if (rv > 0) {

        	if(!FD_ISSET(fd, &rd)) {
        		continue;
        	}
        	resov_data_t* reso = tcp_noblock_resolve_result();
        	if(!reso) continue;

            if(reso->resov) {
            	printf("resolve success: %s -> %s\n", reso->host, reso->addr);
            }else{
            	printf("resolve failed : %s\n", reso->host);
            }
        }else if(rv == 0) {
            printf("select timeout\n");
        }else{
            printf("select error, errno = %d\n", errno);
            break;
        }
    }

	return 0;
}

