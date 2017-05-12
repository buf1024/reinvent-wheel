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

#include "webapp.h"
#include "webapp-tree.h"
#include "webapp-util.h"
#include "webapp-sock.h"
#include "list.h"

#ifdef DEBUG
#  define debug printf
#else
#  define debug (void)
#endif

#define DEFAULT_LISTEN_HOST      "127.0.0.1:8080"
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
	int type;
	int state;

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
};

struct webapp_s {
	int   thread_num;
    webapp_thread_t* threads;
	size_t nfd;
    connection_t* conns;

	int epfd;
	struct epoll_event* evts;
    int main_fd;
    webapp_tree_t* tree[HTTP_METHODS];

	pthread_barrier_t* barrier;
	pthread_t job_tid;
};

struct webapp_list_node_s {
    struct list_node node;
    void* data;
};
