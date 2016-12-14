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
#include <stdbool.h>

#include "evt.h"
#include "sock-con.h"

int main(int argc, char **argv)
{
    event_t* evt = evt_new(0, 0);
    conn_init(0);

    connection_t* conn = conn_listen("127.0.0.1", 8585, 128, NULL);
    if(!conn) {
    	printf("listen failed.\n");
    	return -1;
    }
    evt_add(evt, conn->fd, EVT_READ);
    printf("listening %s:%d ...\n", conn->addr, conn->port);

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
                connection_t* conn = conn_get(e->fd);
                if(conn->state == CONN_STATE_LISTENING) {
                    connection_t* clt_conn = conn_accept(conn, 1024, NULL);
                    evt_add(evt, clt_conn->fd, EVT_READ);

                    printf("tcp accept: ip=%s, port=%d\n", clt_conn->addr, clt_conn->port);
                }else{
                    conn_recv(conn);
                    char buf[1024] = {0};
                    int rd = buffer_read(conn->recv_buf, buf, 1024);
                    printf("read(%d) \n%s\n", rd, buf);
                    buffer_write(conn->send_buf, buf, rd);
                    conn_send(conn);
                    evt_del(evt, conn->fd);
                    conn_close(conn);
                }
            }
        }else if(rv == 0) {
        	printf("evt timeout, to=%d\n", evt->timeout);
        }
    }
    return 0;
}

