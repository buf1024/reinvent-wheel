/*
 * thinpxy.h
 *
 *  Created on: 2016/5/17
 *      Author: Luo Guochun
 */

#ifndef __SIMPLEPROXY_H__
#define __SIMPLEPROXY_H__

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dlfcn.h>


#include "tson.h"
#include "log.h"
#include "misc.h"
#include "sock.h"
#include "sock-ext.h"

#include "proxyutil.h"

#define TSON_READ_STR_MUST(t, k, v, st) \
	do { \
		if(tson_get(t, k, &v, &st) <= 0) { \
			return -1; \
		} \
	}while(0)

#define TSON_READ_STR_OPT(t, k, v, st) \
	do { \
		tson_get(t, k, &v, &st); \
	}while(0)

#define TSON_READ_INT_MUST(t, k, v, st) \
	do { \
		char* tmp = NULL; \
		if(tson_get(t, k, &tmp, &st) <= 0) { \
			return -1; \
		} \
		v = atoi(tmp); \
	}while(0)

#define TSON_READ_BOOL_MUST(t, k, v, st) \
	do { \
		char* tmp = NULL; \
		if(tson_get(t, k, &tmp, &st) <= 0) { \
			return -1; \
		} \
		if(strcasecmp(tmp, "yes") == 0 || \
				strcasecmp(tmp, "true") == 0 || \
				strcasecmp(tmp, "on") == 0 || \
				strcasecmp(tmp, "1") == 0) { \
			v = true; \
		}else { \
			v = false; \
		} \
	}while(0)


typedef struct proxy_plugin_s  proxy_plugin_t;
typedef struct connection_s    connection_t;
typedef struct buffer_s        buffer_t;
typedef struct simpleproxy_s   simpleproxy_t;
typedef struct proxy_session_s proxy_session_t;
typedef struct proxy_thread_s  proxy_thread_t;


typedef proxy_plugin_t* (*plugin_create_fun_t)();


enum {
	CONN_TYPE_SERVER,
	CONN_TYPE_CLIENT,
};

enum {
	CONN_STATE_LISTENING,
	CONN_STATE_CONNECTING,
	CONN_STATE_CONNECTED,
	CONN_STATE_BROKEN,
	CONN_STATE_RESOLVING,
};

enum {
	PROTO_IPV4 = 4,
	PROTO_IPV6 = 6,

	LISTEN_BACK_LOG      = 128,
	EPOLL_TIMEOUT        = 1000,
	DEFAULT_IDEL_TIMEOUT = 3600,
	DEFAULT_CORO_STACK   = 1024*8,
	DEFAULT_BUF_SIZE     = 1024
};

enum {
	THREAD_STATE_ACTIVE,
	THREAD_STATE_DEAD,
};

struct proxy_plugin_s
{
	// = 0, ok
	int (*init)(simpleproxy_t* proxy);
	int (*uninit)();

	proxy_session_t* (*session)(connection_t* con);
	// = 0, ok
	int (*proxy)(proxy_session_t* session);
};

struct proxy_session_s
{
	proxy_thread_t* thread;

	connection_t* req_con;
	connection_t* rsp_con;

	int state;
};

struct connection_s
{
	int fd;
	int type;
	int state;

	proxy_session_t* sess;

	buffer_t* rbuf;
	buffer_t* wbuf;
};
struct buffer_s
{
	int size;
	char* cache;
};

struct proxy_thread_s
{
	simpleproxy_t* proxy;
	int state;

	int fd[2];

	int epfd;


	int nfd;
	struct epoll_event* evts;

	pthread_t tid;
};

struct simpleproxy_s
{
	char* conf;

	char* log_head;
	char* log_path;
	int   log_level;
	int   log_buf_size;

	char* listen;
	int   port;
	int   proto_v;

	int   thread_num;
	int   idle_to;
	int   mode;

	char* plugin_name;
	proxy_plugin_t* plugin;

	size_t nfd;
    connection_t* conns;
    proxy_thread_t* threads;

    int fd;

    bool sig_term;
    bool sig_usr1;
    bool sig_usr2;
};

int parse_conf(simpleproxy_t* proxy);

int schedule_fd(simpleproxy_t* proxy, int fd);
int proxy_main_loop(simpleproxy_t* proxy);

int proxy_init(simpleproxy_t* proxy);
int proxy_uninit(simpleproxy_t* proxy);

void* proxy_task_routine(void* args);

int proxy_timer_task(proxy_thread_t* t);
int proxy_biz_task(proxy_thread_t* t, connection_t* con);

int proxy_coro_fun(coro_t* coro);


#endif /* __SIMPLEPROXY_H__ */
