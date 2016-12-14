/*
 * sock-con.c
 *
 *  Created on: 2016/6/13
 *      Author: Luo Guochun
 */

#include "sock-con.h"
#include "sock.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/resource.h>
#include <errno.h>


enum {
	MAX_OPEN_FILE = 1024*10,
	MAX_IP_SIZE = 64,
};

static int _size = 0;
static connection_t** _conn = NULL;

static connection_t* _conn_new(int fd, int state, void* data,
		int buf_size, char* addr, int port)
{
	if(fd <= 0 || fd >= _size) return NULL;

	connection_t* conn = _conn[fd];
	if(!conn) {
		conn = (connection_t*)calloc(1, sizeof(connection_t));
		if(!conn) return NULL;

		_conn[fd] = conn;
	}
	conn->fd = fd;
	conn->state = state;
	conn->data = data;

	if (buf_size <= 0) {
		if (conn->recv_buf) {
			buffer_free(conn->recv_buf);
			conn->recv_buf = NULL;
		}
		if (conn->send_buf) {
			buffer_free(conn->send_buf);
			conn->send_buf = NULL;
		}
	}else{
		if (!conn->recv_buf) {
			conn->recv_buf = buffer_new(buf_size);
			if(!conn->recv_buf) {
				free(conn);
				_conn[fd] = NULL;
				return NULL;
			}
		}
		conn->recv_buf->pos = 0;
		if (!conn->send_buf) {
			conn->send_buf = buffer_new(buf_size);
			if(!conn->send_buf) {
				buffer_free(conn->recv_buf);
				free(conn);
				_conn[fd] = NULL;
				return NULL;
			}
		}
		conn->send_buf->pos = 0;
	}
	if(conn->addr) {
		free(conn->addr);
	}
	conn->addr = strdup(addr);
	conn->port = port;

	return conn;
}

static connection_t* _conn_listen(int v, char *bindaddr, int port, int backlog, void* data)
{
	if(!_conn) return NULL;

	int fd = v == 4? tcp_server(bindaddr, port, backlog):tcp6_server(bindaddr, port, backlog);
	connection_t* conn = _conn_new(fd, CONN_STATE_LISTENING, data, 0, bindaddr, port);
	if(!conn) close(fd);

	return conn;
}
int conn_init(int size)
{
	if(_conn != NULL) return 0;

	if (size <= 0) {
		do {
			struct rlimit r;

			if (getrlimit(RLIMIT_NOFILE, &r) < 0) {
				printf("getrlimit failed\n");
				break;
			} else {
				if (r.rlim_max != r.rlim_cur) {
					if (r.rlim_max == RLIM_INFINITY) {
						r.rlim_cur = MAX_OPEN_FILE;
					} else if (r.rlim_cur < r.rlim_max) {
						r.rlim_cur = r.rlim_max;
					}
					if (setrlimit(RLIMIT_NOFILE, &r) < 0) {
						printf("setrlimit\n");
						break;
					}
				}
				size = (int) r.rlim_cur;

			}
		} while (0);

		if(size <= 0) {
			size = MAX_OPEN_FILE;
		}
	}

	_conn = (connection_t**)calloc(size, sizeof(connection_t*));
	if(_conn == NULL) return -1;
	_size = size;

	return 0;
}
connection_t* conn_listen(char *bindaddr, int port, int backlog, void* data)
{
	return _conn_listen(4, bindaddr, port, backlog, data);
}

connection_t* conn_listen4(char *bindaddr, int port, int backlog, void* data)
{
	return _conn_listen(4, bindaddr, port, backlog, data);
}
connection_t* conn_listen6(char *bindaddr, int port, int backlog, void* data)
{
	return _conn_listen(6, bindaddr, port, backlog, data);
}
connection_t* conn_accept(connection_t* conn, int buf_size, void* data)
{
	if(!conn) return NULL;

	int sfd = conn->fd;

	char addr[MAX_IP_SIZE] = {0};
	int port = 0;

	int fd = tcp_accept(sfd, addr, sizeof(addr), &port);

	connection_t* clt_conn = _conn_new(fd, CONN_STATE_CONNECTED, data, buf_size, addr, port);
	tcp_noblock(fd, true);

	if(!clt_conn) close(fd);

	return clt_conn;

}
connection_t* conn_connect(char *addr, int port, int buf_size, void* data)
{
	if(!_conn) return NULL;

	int fd = tcp_noblock_connect(addr, port);

	connection_t* conn = _conn_new(fd, errno == EINPROGRESS ? CONN_STATE_CONNECTING: CONN_STATE_CONNECTED,
			data, buf_size, addr, port);
	tcp_noblock(fd, true);

	if(!conn) close(fd);

	return conn;
}
int conn_recv(connection_t* conn)
{
	if(!conn || !conn->recv_buf
			|| conn->state != CONN_STATE_CONNECTED) return 0;

	buffer_t* buf = conn->recv_buf;

	int free_size = buffer_free_size(buf);
	if(free_size <= 0) return 0;

	char* pos = buffer_write_pos(buf);

	bool ok = false;
	int rd = tcp_read(conn->fd, pos, free_size, false, &ok);
	buffer_set_write_size(buf, rd);

	if(!ok) {
		conn->state = CONN_STATE_BROKEN;
	}
	return rd;
}
int conn_send(connection_t* conn)
{
	if(!conn || !conn->send_buf
			|| conn->state != CONN_STATE_CONNECTED) return 0;

	buffer_t* buf = conn->send_buf;

	int size = buffer_size(buf);
	if(size <= 0) return 0;

	char* pos = buffer_read_pos(buf);
	bool ok = false;
	int wr = tcp_write(conn->fd, pos, size, false, &ok);
	buffer_set_read_size(buf, wr);

	if(!ok) {
		conn->state = CONN_STATE_BROKEN;
	}
	return wr;
}


connection_t* conn_get(int fd)
{
	if(!_conn || fd > _size) return NULL;

	return _conn[fd];

}

int conn_close(connection_t* conn)
{
	if(!conn) return -1;

	conn->state = CONN_STATE_BROKEN;
	close(conn->fd);

	return 0;
}


int conn_free()
{
	if(!_conn) return 0;

	for(int i=0; i<_size; i++) {
		connection_t* con = _conn[i];
		if(con) {
			if(con->recv_buf) {
				buffer_free(con->recv_buf);
				con->recv_buf = NULL;
			}
			if(con->send_buf) {
				buffer_free(con->send_buf);
				con->send_buf = NULL;

			}
			free(con);
		}
	}
	free(_conn);

	return 0;
}
