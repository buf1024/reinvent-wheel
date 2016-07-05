/*
 * thinpxy.h
 *
 *  Created on: 2016/5/17
 *      Author: Luo Guochun
 */

#ifndef __SIMPLEPROXY_H__
#define __SIMPLEPROXY_H__

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


#include "tson.h"
#include "coro.h"
#include "log.h"
#include "misc.h"
#include "sock.h"


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


#define DEF_IDLE_TO    60*60 /* 1 min */
#define DEF_PXY_ALGO   "default"


typedef struct proxy_backend_s proxy_backend_t;
typedef struct proxy_plugin_s  proxy_plugin_t;
typedef struct connection_s    connection_t;
typedef struct simpleproxy_s   simpleproxy_t;
typedef struct proxy_session_s proxy_session_t;
typedef struct proxy_thread_s  proxy_thread_t;


enum {
	WORK_MODE_CONNECT_NEW,
	WORK_MODE_CONNECT_PACKET,
};

enum {
	CONN_TYPE_SERVER,
	CONN_TYPE_CLIENT,
};

enum {
	CONN_STATE_LISTENING,
	CONN_STATE_CONNECTIN,
	CONN_STATE_CONNECTED,
	CONN_STATE_BROKEN,
};

enum {
	PROTO_IPV4 = 4,
	PROTO_IPV6 = 6,
	LISTEN_BACK_LOG = 128,
	EPOLL_TIMEOUT = 1000,
};

enum {
	THREAD_STATE_ACTIVE,
	THREAD_STATE_DEAD,
};

struct proxy_plugin_s
{
	// = 0, ok
	int (*init)(simpleproxy_t* proxy, tson_t* tson);
	int (*uninit)();

	// = 0, not finish, > 0, finish, < 0 failed.
	int (*parse)(const char* packet);
	// = 0, ok
	proxy_session_t* (*proxy)(simpleproxy_t* proxy, connection_t* con);
};

struct proxy_session_s
{
	connection_t* req_con;
	connection_t* psp_con;

	int state;
	coro_t* coro;
};

struct proxy_backend_s
{
	simpleproxy_t* proxy;

	char* name;
	char* addr;
	int   port;
	int   proto_v;
	int   weight;

	connection_t* conn;

	// static info
	int  attach_cnt;
    int  attach_total;

	unsigned long pxy_packet_cnt;
	unsigned long pxy_packet_succ_cnt;
	unsigned long pxy_packet_fail_cnt;
	unsigned long pxy_bytes;

	time_t up_time;
};

struct connection_s
{
	int fd;
	int type;
	int state;

	proxy_session_t* sess;
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

	char* log_path;
	int   log_level;
	int   log_buf_size;

	char* listen;
	int   port;
	int   proto_v;

	int   thread_num;
	int   idle_to;
	int   mode;

	int backend_cnt;
	proxy_backend_t* backends;


	char* algo;
	char* plugin_name;
	proxy_plugin_t* plugin;

	size_t nfd;
    connection_t* conns;
    proxy_thread_t* threads;

    int fd;

    bool sig_term;
    bool sig_usr1;
    bool sig_usr2;

    coro_switcher_t switcher;
};

int parse_conf(simpleproxy_t* proxy);

int schedule_fd(simpleproxy_t* proxy, int fd);
int proxy_main_loop(simpleproxy_t* proxy);

int proxy_init(simpleproxy_t* proxy);
int proxy_uninit(simpleproxy_t* proxy);

void* proxy_task_routine(void* args);

int epoll_add_fd(int epfd, int evt, connection_t* con);
int epoll_mod_fd(int epfd, int evt, connection_t* con);
int epoll_del_fd(int epfd, connection_t* con);

#endif /* __SIMPLEPROXY_H__ */
