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
#include <ctype.h>
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


#define HTTP_END       (CRLF CRLF)
#define HTTP_END_SIZE  sizeof(HTTP_END)

#define HTTP_GET     "GET"
#define HTTP_POST    "POST"
#define HTTP_CONNECT "CONNECT"

#define HTTP_CONTENT_LEN  "Content-Length"
#define HTTP_METHOD_KEY "HTTP_METHOD"

#define HTTP_HOST    "Host"

enum {
	MAX_HOST_SIZE       = 256,
	MAX_ADDR_SIZE       = 16,
	MAX_PATH_SIZE       = 256,
	MAX_REQ_SIZE        = 4*1024,
	MAX_REQ_GROW_SIZE   = 512,
	MAX_CORO_STACK_SIZE = 160*1024,
};

enum {
	MAX_CONCURRENT_CONN      = 256,
	MAX_CONCURRENT_CONN_GROW = 64,
	MAX_EPOLL_TIMEOUT        = 1000,
};

enum {
	CONN_TYPE_SERVER,
	CONN_TYPE_CLIENT,
};
enum {
	CONN_STATE_NONE,
	CONN_STATE_LISTENING,
	CONN_STATE_CONNECTING,
	CONN_STATE_CONNECTED,
	CONN_STATE_BROKEN,
	CONN_STATE_CLOSING,
	CONN_STATE_CLOSED,
	CONN_STATE_RESOLVING,
	CONN_STATE_WAIT,
};

enum {
	HTTP_METHORD_NONE,
	HTTP_METHORD_GET,
	HTTP_METHORD_POST,
	HTTP_METHORD_CONNECT,
};

enum {
    HTTP_SSL_PORT = 443,
    HTTP_PORT     = 80,
};

typedef struct connection_s connection_t;
typedef struct lazy_proxy_s lazy_proxy_t;
typedef struct buffer_s buffer_t;
typedef struct http_proxy_s http_proxy_t;

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

	connection_t* dns_rsv_con;
	coro_switcher_t switcher;
};

struct connection_s
{
    lazy_proxy_t* lazy;

	int fd;
	int type;
	int state;

	coro_t* coro;
	http_proxy_t* pxy;
};

struct buffer_s
{
	int size;
	char cache[MAX_REQ_SIZE];
};

struct http_proxy_s
{
    connection_t* req_con;

    buffer_t* req_r;
    buffer_t* req_w;
    int   req_type;
    dict* req_head;

    connection_t* rsp_con;
};



int lazy_net_init(lazy_proxy_t* pxy);
int lazy_net_uninit(lazy_proxy_t* pxy);

int lazy_add_fd(int epfd, int evt, connection_t* con);
int lazy_mod_fd(int epfd, int evt, connection_t* con);
int lazy_del_fd(int epfd, connection_t* con);

int lazy_proxy_task(lazy_proxy_t* lazy);
int lazy_timer_task(lazy_proxy_t* lazy);

int lazy_spawn_http_req_coro(connection_t* con);

int lazy_http_req_coro(coro_t* coro);

int lazy_http_req_read(coro_t* coro, buffer_t* buf);
int lazy_http_rsp_write(coro_t* coro, buffer_t* buf);
int lazy_http_rsp_read(coro_t* coro, buffer_t* buf);
int lazy_http_req_write(coro_t* coro, buffer_t* buf);

int resume_http_req_coro(coro_t* coro);
int parse_http_req(http_proxy_t* pxy, const char* req, int size);
int parse_req_head(http_proxy_t* pxy, const char* head, int size);
int establish_proxy_tunnel(http_proxy_t* pxy);




#endif /* __LAZY_H__ */
