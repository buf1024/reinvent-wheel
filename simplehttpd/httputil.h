/*
 * httputil.h
 *
 *  Created on: 2016/7/26
 *      Author: Luo Guochun
 */

#ifndef __HTTPUTIL_H__
#define __HTTPUTIL_H__

#include <sys/epoll.h>

typedef struct connection_s    connection_t;

void* find_symbol(const char* name);

int epoll_add_fd(int epfd, int evt, connection_t* con);
int epoll_mod_fd(int epfd, int evt, connection_t* con);
int epoll_del_fd(int epfd, connection_t* con);


#endif /* __HTTPUTIL_H__ */
