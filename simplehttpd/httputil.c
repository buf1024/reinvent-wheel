/*
 * httputil.c
 *
 *  Created on: 2016/7/26
 *      Author: Luo Guochun
 */

#define _GNU_SOURCE
#include "httpd.h"
#include "httputil.h"
#include "log.h"
#include <dlfcn.h>
#include <errno.h>

void *find_symbol(const char *name)
{
    void *symbol = dlsym(RTLD_NEXT, name);
    if (!symbol)
        symbol = dlsym(RTLD_DEFAULT, name);
    return symbol;
}

int epoll_add_fd(int epfd, int evt, connection_t* con)
{
	struct epoll_event event;
	event.events = evt | EPOLLERR | EPOLLHUP;
	event.data.ptr = con;

	if(epoll_ctl(epfd, EPOLL_CTL_ADD, con->fd, &event) != 0) {
		LOG_ERROR("epoll add failed, errno=%d\n", errno);
		return -1;
	}
	return 0;
}


int epoll_mod_fd(int epfd, int evt, connection_t* con)
{
	struct epoll_event event;
	event.events = evt | EPOLLERR | EPOLLHUP;
	event.data.ptr = con;

	if(epoll_ctl(epfd, EPOLL_CTL_MOD, con->fd, &event) != 0) {
		LOG_ERROR("epoll add failed, errno=%d\n", errno);
		return -1;
	}
	return 0;
}

int epoll_del_fd(int epfd, connection_t* con)
{
	struct epoll_event event;

	if(epoll_ctl(epfd, EPOLL_CTL_DEL, con->fd, &event) != 0) {
		LOG_ERROR("epoll del failed, errno=%d\n", errno);
		return -1;
	}
	return 0;
}

