/*
 * lazy.h
 *
 *  Created on: 2016/6/14
 *      Author: Luo Guochun
 */

#ifndef __LAZY_H__
#define __LAZY_H__

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include "cmmhdr.h"
#include "log.h"
#include "coro.h"
#include "misc.h"
#include "sock.h"
#include "dict.h"
#include "coro.h"

#define OOM_CHECK(ptr, msg) \
	do { \
		if(!ptr) { \
			LOG_FATAL("oom, abort! %s\n", msg); \
			LOG_FINISH(); \
			abort(); \
		} \
	}while(0)



enum {
	MAX_HOST_SIZE = 256,
	MAX_ADDR_SIZE = 16,
	MAX_PATH_SIZE = 256,
	MAX_REQ_SIZE  = 4*1024,
};

enum {
	MAX_CONCURRENT_CONN = 1024,
	MAX_EPOLL_TIMEOUT   = 1000,
};

enum {
	CONN_TYPE_SERVER,
	CONN_TYPE_CLIENT,
};
enum {
	CONN_STATE_LISTENING,
	CONN_STATE_CONNECTING,
	CONN_STATE_CONNECTED,
	CONN_STATE_BROKEN,
	CONN_STATE_CLOSING,
	CONN_STATE_CLOSED,
};

typedef struct connection_s connection_t;
typedef struct http_req_s http_req_t;
typedef struct http_rsp_s http_rsp_t;
typedef struct lazy_proxy_s lazy_proxy_t;
typedef struct buffer_s buffer_t;

struct lazy_proxy_s
{
	char log_dir[MAX_PATH_SIZE];
	char host[MAX_HOST_SIZE];
	int port;
	int listen;


	bool sig_term;
	bool sig_usr1;
	bool sig_usr2;

	int epfd;
	int timout;
	int conn_size;
	connection_t* conn;
	struct epoll_event* events;
};

struct connection_s
{
	lazy_proxy_t* pxy;

	int fd;
	int type;
	int state;

	http_req_t* req;
	coro_t* coro;
};

struct buffer_s
{
	int cap;
	int size;
	char* buf;
};
struct http_req_s
{
	buffer_t* buf;
	dict* header;
};

struct http_rsp_s
{

};


int lazy_net_init(lazy_proxy_t* pxy);
int lazy_net_uninit(lazy_proxy_t* pxy);
int lazy_add_fd(int epfd, connection_t* con);
int lazy_del_fd(int epfd, connection_t* con);

int lazy_proxy_task(lazy_proxy_t* pxy);
int lazy_timer_task(lazy_proxy_t* pxy);

int lazy_spawn_coro(coro_switcher_t* switcher, connection_t* con);
int resume_coro_demand(coro_t* coro);
int lazy_http_req_coro(coro_t* coro);




#endif /* __LAZY_H__ */
