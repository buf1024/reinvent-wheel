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

#include "tson.h"
#include "coro.h"
#include "log.h"


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
typedef struct proxy_pair_s    proxy_pair_t;


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
};

struct proxy_plugin_s
{
	// = 0, ok
	int (*init)(tson_t* tson);
	int (*uninit)();

	// = 0, not finish, > 0, finish, < 0 failed.
	int (*parse_packet)(const char* packet, char* sid, char* packet_new);
	// = 0, ok
	int (*proxy_packet)(simpleproxy_t* proxy, proxy_backend_t** backend);
};

struct proxy_pair_s
{
	connection_t* req_con;
	connection_t* pxy_con;

	char* sid;
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

    connection_t* conns;
	int ep_fd;
};

int parse_conf(simpleproxy_t* proxy);

int init_net(simpleproxy_t* proxy);


#endif /* __SIMPLEPROXY_H__ */
