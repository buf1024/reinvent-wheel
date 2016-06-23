/*
 * thinpxy.h
 *
 *  Created on: 2016/5/17
 *      Author: Luo Guochun
 */

#ifndef THINPXY_SRC_THINPXY_H_
#define THINPXY_SRC_THINPXY_H_

#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include <netdb.h>

#define DEF_IDLE_TO  60*60 /* 1 min */
#define DEF_PXY_ALGO "default"

#define LOG_INFO printf
#define LOG_DEBUG printf
#define LOG_ERROR printf


typedef struct backend_s backend_t;
typedef struct plugin_s plugin_t;
typedef struct connection_s connection_t;
typedef struct thinpxy_s thinpxy_t;
typedef struct config_s config_t;

enum {
	WORK_MODE_CONNECT_NEW,
	WORK_MODE_CONNECT_EXIST,
	WORK_MODE_CONNECT_PACKET,
};
#if 0
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
#endif

struct plugin_s
{
	int (*init)(const char* conf);
	int (*uninit)();

	int (*parse_packet)(const char* packet, char* sid);
	int (*proxy_packet)(thinpxy_t* pxy, backend_t* backend);
};

struct backend_s
{
	thinpxy_t* pxy;

	char* name;
	char* addr_str;
	int   weight;

	struct addrinfo *addrs;

	connection_t* conn;
	int fd;

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

struct config_s
{
	thinpxy_t* pxy;

	char* conf;
	char* log_file;
	char* log_level;
	bool daemon;

	struct addrinfo *addrs;

	struct {
		int line;
		char* error;
		char* conf;
	} parse;
};

struct thinpxy_s
{
	config_t config;

	connection_t* conns;

	int backend_cnt;
	backend_t* backends;

	int mode;
	int idle_to;
	char* pxy_algo;

	char* plugin_name;
	plugin_t* pxy_plugin;

	int listen_fd;
	int epoll_fd;
};

int parse_conf(config_t* conf);

int init_net(thinpxy_t* pxy);

int coro_resume();
int coro_yiled();


#endif /* THINPXY_SRC_THINPXY_H_ */
