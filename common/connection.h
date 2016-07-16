/*
 * connection.h
 *
 *  Created on: 2016-7-16
 *      Author: Luo Guochun
 */

#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <stdbool.h>

#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    CONN_TYPE_PIPE,
    CONN_TYPE_DOMAIN,
    CONN_TYPE_SERVER,
    CONN_TYPE_CLIENT,
};

enum {
    CONN_STATE_LISTENING,
    CONN_STATE_CONNECTING,
    CONN_STATE_CONNECTED,
    CONN_STATE_CLOSING,
    CONN_STATE_CLOSED,
    CONN_STATE_BROKEN
};

struct connection_s
{
    int fd;
    int type;
    int state;

    buffer_t* rbuf;
    buffer_t* wbuf;

    time_t t_conn;
    time_t t_read;
    time_t t_write;
};
int con_new(int maxfd);
int con_free();

connection_t* con_init(int fd, int type, int rsize, int wsize);

connection_t* con_get(int fd);
int con_type(connection_t* con);

int con_state(connection_t* con);
int con_set_state(connection_t* con, int state);


int con_recv(connection_t* con);
int con_send(connection_t* con, bool fore);

int con_flush();

int con_tcp_server(char *bindaddr, int port, int backlog);
int con_tcp6_server(char *bindaddr, int port, int backlog);
int con_accept(int serversock);
int tcp_connect(char *addr, int port);
int tcp_noblock_connect(char *addr, int port);
int tcp_read(int fd, char *buf, int count, bool block, bool* ok);
int tcp_write(int fd, char *buf, int count, bool block, bool* ok);
int tcp_peer_name(int fd, char *ip, size_t ip_len, int *port);
int tcp_sock_name(int fd, char *ip, size_t ip_len, int *port);

#ifdef __cplusplus
}
#endif


#endif /* __CONNECTION_H__ */
