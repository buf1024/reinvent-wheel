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

#include <netdb.h>

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

	int mode;

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
	thinpxy_t* pxy;
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
	} parse;
};

struct thinpxy_s
{
	config_t config;

	connection_t* conns;
	backend_t* backends;

	int mode;
	int idle_to;
	char* pxy_algo;

	plugin_t* pxy_plugin;

	int fd;
};

int parse_conf(config_t* conf);

int coro_resume();
int coro_yiled();


#endif /* THINPXY_SRC_THINPXY_H_ */
