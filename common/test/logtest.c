/*
 * logtest.c
 *
 *  Created on: 2016年6月3日
 *      Author: Luo Guochun
 */

#include "../log.h"

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



int main(int argc, char **argv) {
	if(log_init(LOG_ALL, LOG_ALL,
	        "./", "logtest", 1024*10,
	        0, -1) != 0) {
		printf("log_init failed.\n");
		return -1;
	}

    REGISTER_SIGNAL(SIGTERM, sigterm, 0);//Kill信号
    REGISTER_SIGNAL(SIGINT, sigterm, 0);//终端CTRL-C信号
    REGISTER_SIGNAL(SIGUSR2, sigusr2, 0);//SIGUSR2信号

	srand(time(0));
	long long times = 0LL;
	while(1) {
		times++;
		LOG_DEBUG("this is a log test debug\n");
		LOG_INFO("this is a log test info\n");
		LOG_WARN("this is a log test warn\n");
		LOG_ERROR("this is a log test error\n");
		LOG_FATAL("this is a log test fatal\n");
		LOG_INFO("--------- %lld --------------\n", times);

		if(g_usr2) {
			LOG_FLUSH();
			g_usr2 = 0;
		}

		if(g_term) {
			break;
		}

		sleep(rand() % 60);
	}

	log_finish();

	return 0;
}

