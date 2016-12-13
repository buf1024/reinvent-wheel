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

#include "evt.h"
#include "sock.h"

int main(int argc, char **argv)
{
    event_t* evt = evt_new(0, 0);
    int fd = tcp_server("127.0.0.1", 8585, 128);
    evt_add(evt, fd, EVT_READ);
    printf("listening 127.0.0.1:8585 ...\n");

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
                }else{
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

