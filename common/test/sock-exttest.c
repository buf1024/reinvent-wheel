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


int main(int argc, char **argv) {

    REGISTER_SIGNAL(SIGTERM, sigterm, 1);//Kill信号
    REGISTER_SIGNAL(SIGINT, sigterm, 1);//终端CTRL-C信号
    REGISTER_SIGNAL(SIGUSR2, sigusr2, 1);//SIGUSR2信号


    int fd = 0;

    //fd = tcp_noblock_resolve("python.org", 0, 0);
    fd = tcp_noblock_resolve("baidu.com", 0, 0);
    //fd = tcp_noblock_resolve("aaagoogleyyasdf.com", 0, 0);

    int i=0;
    for(;i<100; i++) {

        char host[512] = {0};
        snprintf(host, sizeof(host) -1, "python%d.org", i);
        //fd = tcp_noblock_resolve(host, 0, 8);
    }

    fd = tcp_noblock_resolve_fd();
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
        	char* host = NULL;
        	char* addr = NULL;
        	bool reso = tcp_noblock_resolve_result(&host, &addr, NULL);
        	if(!reso) continue;

            if(reso) {
            	printf("resolve success: %s -> %s\n", host, addr);
            }else{
            	printf("resolve failed : %s\n", host);
            }
            free(host); free(addr);
        }else if(rv == 0) {
            printf("select timeout\n");
        }else{
            printf("select error, errno = %d\n", errno);
            break;
        }
    }

	return 0;
}

