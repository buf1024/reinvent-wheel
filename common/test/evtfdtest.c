/*
 * evttest.c
 *
 *  Created on: 2016-7-2
 *      Author: Luo Guochun
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <sys/eventfd.h>
#include <signal.h>

#include "evt.h"
#include "sock.h"

int main(int argc, char **argv)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGUSR1);

    if(sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
    	printf("sigprocmask failed.\n");
    	return -1;
    }

    int sfd = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
    if(sfd <= 0) {
    	printf("signalfd failed.\n");
    	return -1;
    }

    int tfd1 = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    if(tfd1 <= 0) {
    	printf("timerfd 1 create failed.\n");
    	return -1;
    }

    int tfd2 = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC);
    if(tfd1 <= 0) {
    	printf("timerfd 2 create failed.\n");
    	return -1;
    }

    int evtfd = eventfd(0, 0);
    if(evtfd <= 0) {
    	printf("create event fd failed.\n");
    	return -1;
    }

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    struct itimerspec tm1, tm2;
    tm1.it_interval.tv_nsec = 0;
    tm1.it_interval.tv_sec = 2;
    tm1.it_value.tv_nsec = ts.tv_nsec;
    tm1.it_value.tv_sec = ts.tv_sec + 1;

    tm2.it_interval.tv_nsec = 0;
    tm2.it_interval.tv_sec = 5;
    tm2.it_value.tv_nsec = ts.tv_nsec;
    tm2.it_value.tv_sec = ts.tv_sec + 5;

    if(timerfd_settime(tfd1, TFD_TIMER_ABSTIME, &tm1, NULL) != 0) {
    	printf("timerfd_settime failed.\n");
    	return -1;
    }

    if(timerfd_settime(tfd2, TFD_TIMER_ABSTIME, &tm2, NULL) != 0) {
    	printf("timerfd_settime failed.\n");
    	return -1;
    }

    event_t* evt = evt_new(0, 0);
    int fd = tcp_server("127.0.0.1", 8585, 128);
    evt_add(evt, fd, EVT_READ);
    printf("listening 127.0.0.1:8585 ...\n");

    evt_add(evt, sfd, EVT_READ);

    evt_add(evt, tfd1, EVT_READ);
    evt_add(evt, tfd2, EVT_READ);
    evt_add(evt, evtfd, EVT_READ);



    poll_event_t* events = NULL;
    while(true) {
        int rv = evt_poll(evt,&events);
        if(errno == EINTR) {
            printf("interupt.\n");
            break;
        }
        if(rv > 0) {
            for(int i=0;i<rv; i++) {
                poll_event_t* e =&(events[i]);
                if(e->fd == fd) {
                    char ip[32] = {0};
                    int port = 0;
                    int s = tcp_accept(fd, ip, sizeof(ip), &port);
                    evt_add(evt, s, EVT_READ);

                    printf("tcp accept: ip=%s, port=%d\n", ip, port);
                }else if(e->fd == sfd) {
                	printf("signalfd received.\n");
                	struct signalfd_siginfo sfd_st;
                	int s = read(sfd, &sfd_st, sizeof(sfd_st));
                	if(s != sizeof(sfd_st)) {
                		printf("signal fd read failed.\n");
                	}
                	printf("signal: %d\n", sfd_st.ssi_signo);
                	if(sfd_st.ssi_signo == SIGINT) {
                		evt_free(evt);
                		return -1;
                	}
                	if(sfd_st.ssi_signo == SIGUSR1) {
                		uint64_t v = (uint64_t)rand();
                		int s = write(evtfd, &v, sizeof(v));
                		if(s != sizeof(v)) {
                			printf("evet write failed.\n");
                			evt_free(evt);
                			return -1;
                		}
                		printf("write to event, value = %ld\n", v);
                	}
                }else if(e->fd == evtfd){
                	uint64_t v ;
                	int s = read(evtfd, &v, sizeof(v));
					if (s != sizeof(v)) {
						printf("evet read failed.\n");
						evt_free(evt);
						return -1;
					}
            		printf("read from event, value = %ld\n", v);

                }else if(e->fd == tfd1 || e->fd == tfd2) {
                	printf("timer expired\n");
                	uint64_t t;
                	int s = read(e->fd, &t, sizeof(t));
                	if(s != sizeof(t)) {
                		printf("timer readsize != sizeof(ulong)\n");
                		evt_free(evt);
                		return -1;
                	}
                	printf("timer = %llu, fd=%d\n", (unsigned long long)t, e->fd);
                }else {
                    char buf[2048] = {0};
                    int r = read(e->fd, buf, sizeof(buf));
                    printf("read(%d) \n%s\n", r, buf);
                    write(e->fd, buf, r);
                    evt_del(evt, e->fd);
                    close(e->fd);
                }
            }
        }else if(rv == 0) {
        	printf("evt timeout, to=%d\n", evt->timeout);
        }
    }



    return 0;
}

