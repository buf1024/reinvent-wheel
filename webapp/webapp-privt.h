#pragma once

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>

#include "webapp.h"
#include "webapp-route.h"
#include "webapp-util.h"
#include "webapp-sock.h"
#include "list.h"

#ifdef DEBUG
static void _debug(const char* fmt, ...)
{
    struct timeval tv = { 0 };
    gettimeofday(&tv, NULL);
    struct tm* tm = localtime(&tv.tv_sec);
    printf("[D][%02d%02d%02d:%ld] ",
        tm->tm_hour, tm->tm_min, tm->tm_sec, (long)tv.tv_usec);
	va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}
#else
static void _debug(){}
#endif

#define debug(...) _debug(__VA_ARGS__)

#define DEFAULT_LISTEN_BACK_LOG  128

typedef struct list_head webapp_list_t;
typedef struct webapp_list_node_s webapp_list_node_t;

typedef struct webapp_thread_s webapp_thread_t;
typedef struct connection_s connection_t;
typedef struct buffer_s buffer_t;

enum {
    HTTP_GET    = 0,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_HEAD,
    HTTP_OPTIONS,
	HTTP_METHODS,
};

struct connection_s
{
	int fd;
	//int type;
	int state;

	time_t up_time;

	buffer_t* rbuf;
	buffer_t* wbuf;
};
struct buffer_s
{
	int size;
	char* cache;
};

struct webapp_thread_s
{
	webapp_t* web;

	int fd[2];
	int epfd;

	int nfd;	
    struct epoll_event* evts;
	pthread_t tid;

    webapp_list_t read_queue;
	webapp_list_t write_queue;
};

struct webapp_s {
	int   thread_num;
    webapp_thread_t* threads;

	size_t nfd;
    connection_t* conns;

	int epfd;
	struct epoll_event* evts;
    int main_fd;

    route_t* route[HTTP_METHODS];

	pthread_barrier_t* barrier;
	pthread_t job_tid;

    webapp_list_t job_queue;

	int midware_num;
	webapp_handler_t* midware;

	bool group;
	char* group_pattern;
	int group_midware_num;
	webapp_handler_t* group_midware;
};

struct webapp_list_node_s {
    struct list_node node;
    void* data;
};
