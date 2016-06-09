/*
 * cororesolve.c
 *
 *  Created on: 2016/6/9
 *      Author: Luo Guochun
 */

#include "sock-ext.h"

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

int biz(const char* host) {
	coro_t* co = coro_new(tcp_noblock_resolve, 0, 0);

	if(coro_resume(co) != CORO_FINISH) {

	}

}

int main(int argc, char **argv) {

    REGISTER_SIGNAL(SIGTERM, sigterm, 0);//Kill信号
    REGISTER_SIGNAL(SIGINT, sigterm, 0);//终端CTRL-C信号
    REGISTER_SIGNAL(SIGUSR2, sigusr2, 0);//SIGUSR2信号

    char addr[1024] = {0};

    while(1) {
    	printf("Input RESOV addr:\n");
    	scanf("%s", addr);



    	coro_t* co = coro_new(tcp_noblock_resolve, 0, 0);

    	coro_resume(co);
    }


	return 0;
}

