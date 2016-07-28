/*
 * httpd.h
 *
 *  Created on: 2016/5/13
 *      Author: Luo Guochun
 */

#ifndef __HTTPD_H__
#define __HTTPD_H__

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>

#include "cmmhdr.h"
#include "queue-ext.h"
#include "trie.h"
#include "tson.h"
#include "log.h"

typedef struct http_request_s http_request_t;
typedef struct http_response_s http_response_t;
typedef struct http_thread_s http_thread_t;
typedef struct http_module_s http_module_t;
typedef struct http_path_s http_path_t;
typedef struct connection_s connection_t;
typedef struct http_msg_s http_msg_t;
typedef struct httpd_s httpd_t;


typedef http_module_t* (*http_mod_create_fun_t)();


enum {
	HTTP_STATUS_NONE                = 0,
    HTTP_STATUS_OK                  = 200,
    HTTP_STATUS_PARTIAL_CONTENT     = 206,
    HTTP_STATUS_MOVED_PERMANENTLY   = 301,
    HTTP_STATUS_NOT_MODIFIED        = 304,
    HTTP_STATUS_BAD_REQUEST         = 400,
    HTTP_STATUS_NOT_AUTHORIZED      = 401,
    HTTP_STATUS_FORBIDDEN           = 403,
    HTTP_STATUS_NOT_FOUND           = 404,
    HTTP_STATUS_NOT_ALLOWED         = 405,
    HTTP_STATUS_TIMEOUT             = 408,
    HTTP_STATUS_TOO_LARGE           = 413,
    HTTP_STATUS_RANGE_UNSATISFIABLE = 416,
    HTTP_STATUS_I_AM_A_TEAPOT       = 418,
    HTTP_STATUS_INTERNAL_ERROR      = 500,
    HTTP_STATUS_NOT_IMPLEMENTED     = 501,
    HTTP_STATUS_UNAVAILABLE         = 503,
};

enum {
	HTTP_METHORD_NONE,
	HTTP_METHORD_GET,
	HTTP_METHROD_POST,
	HTTP_METHROD_HEAD,
};

enum {
	CONN_TYPE_SERVER,
	CONN_TYPE_CLIENT,
	CONN_TYPE_PIPE,
};

enum {
	CONN_STATE_LISTENING,
	CONN_STATE_CONNECTING,
	CONN_STATE_CONNECTED,
	CONN_STATE_BROKEN,
};

enum {
	LISTEN_BACK_LOG      = 128,
	EPOLL_TIMEOUT        = 1000,
	DEFAULT_IDEL_TIMEOUT = 3600,
	DEFAULT_BUF_SIZE     = 4096,
};

struct http_module_s
{
	char* name;
	int (*load_conf)(httpd_t* http, tson_t* tson);

	int (*init)(httpd_t* http);
	int (*uninit)(httpd_t* http);

	int (*handle)(httpd_t* http, http_request_t* req, http_response_t* rsp);
};


struct http_thread_s
{
	httpd_t* http;
	pthread_barrier_t* barrier;

	int fd[2];
	int epfd;


	int nfd;
	struct epoll_event* evts;

	pthread_t tid;
};

struct http_request_s
{

};

struct http_response_s
{

};

struct http_path_s
{
	char* url;
	http_module_t* mod;
};

struct connection_s
{
	int fd;
	int type;
	int state;
};

struct listener
{
	bool ssl;
	char* ssl_certificate;
	char* ssl_certificate_key;
	char* ssl_session_cache;
	int ssl_session_timeout;
	char* ssl_ciphers;
	bool ssl_prefer_server_ciphers;

	char* host;

	char* listen;
	int   port;
};

struct http_msg_s
{
	unsigned cmd;

	union {
		int fd;
	} data;
};

DEF_LIST(list_listener_t, struct listener*, listener_t, entry);
DEF_LIST(list_mod_t, http_module_t*, mod_map_t, entry);

struct httpd_s
{
	char* conf;

	char* log_path;
	int   log_level;
	int   log_buf_size;

	char* user;
	bool daemon;

	list_listener_t listener;

	int   thread_num;

	int epfd;

	size_t maxfd;
    connection_t* conns;
    http_thread_t* threads;

    int nfd;
    struct epoll_event* evts;

    trie_t* urls;
    list_mod_t mods;

    bool sig_term;
    bool sig_usr1;
    bool sig_usr2;
};




#endif /* __HTTPD_H__ */
