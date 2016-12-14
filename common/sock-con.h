/*
 * sock-con.h
 *
 *  Created on: 2016/6/13
 *      Author: Luo Guochun
 */

#ifndef __SOCK_CON_H__
#define __SOCK_CON_H__

#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	CONN_STATE_UNKNOWN,
	CONN_STATE_LISTENING,
	CONN_STATE_CONNECTING,
	CONN_STATE_CONNECTED,
	CONN_STATE_BROKEN,
};


typedef struct connection_s    connection_t;

struct connection_s
{
	int fd;
	int state;

	buffer_t* recv_buf;
	buffer_t* send_buf;

	char* addr;
	int port;

	void* data;
};

int conn_init(int size);

connection_t* conn_listen(char *bindaddr, int port, int backlog, void* data);
connection_t* conn_listen4(char *bindaddr, int port, int backlog, void* data);
connection_t* conn_listen6(char *bindaddr, int port, int backlog, void* data);
connection_t* conn_accept(connection_t* conn, int buf_size, void* data);
connection_t* conn_connect(char *addr, int port, int buf_size, void* data);
int conn_recv(connection_t* conn);
int conn_send(connection_t* conn);

connection_t* conn_get(int fd);
int conn_close(connection_t* conn);

int conn_free();

#ifdef __cplusplus
}
#endif

#endif /* __SOCK_CON_H__ */
