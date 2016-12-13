/*
 * sock-con.h
 *
 *  Created on: 2016/6/13
 *      Author: Luo Guochun
 */

#ifndef __SOCK_CON_H__
#define __SOCK_CON_H__

#ifdef __cplusplus
extern "C" {
#endif

// tcp
int tcp_resolve(char *host, char *ipbuf, size_t ipbuf_len);
int tcp_resolve_ip(char *host, char *ipbuf, size_t ipbuf_len);
int tcp_server(char *bindaddr, int port, int backlog);
int tcp6_server(char *bindaddr, int port, int backlog);
int tcp_accept(int serversock, char *ip, size_t ip_len, int *port);
int tcp_connect(char *addr, int port);
int tcp_noblock_connect(char *addr, int port);
int tcp_read(int fd, char *buf, int count, bool block, bool* ok);
int tcp_write(int fd, char *buf, int count, bool block, bool* ok);
int tcp_peer_name(int fd, char *ip, size_t ip_len, int *port);
int tcp_sock_name(int fd, char *ip, size_t ip_len, int *port);

enum {
	CONN_STATE_LISTENING,
	CONN_STATE_CONNECTING,
	CONN_STATE_CONNECTED,
	CONN_STATE_BROKEN,
};


typedef struct connection_s    connection_t;

int conn_init();
conn_listen(char *bindaddr, int port, int backlog, void* data);
conn_accept(int serversock, char *ip, size_t ip_len, int *port);
int conn_connect(char *addr, int port);
int conn_recv(connection_t* conn);
int conn_send(connection_t* conn);

int conn_recv_buf(connection_t* conn, char* buf);
int conn_send_buf(connection_t* conn, char* buf, int size);


int conn_free(int fd);

#ifdef __cplusplus
}
#endif

#endif /* __SOCK_CON_H__ */
