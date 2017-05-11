
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/epoll.h>
#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>

#include "webapp-util.h"
#include "webapp.h"

#ifndef MAX_OPEN_FILE
#define MAX_OPEN_FILE 1024
#endif

int get_cpu_count(void)
{
    long n_online_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (n_online_cpus < 0) {
        return 1;
    }
    return (int)n_online_cpus;
}
int get_max_open_file_count(void)
{
    struct rlimit r;

    if (getrlimit(RLIMIT_NOFILE, &r) < 0) {
        printf("getrlimit");
        return 0;
    }

    if (r.rlim_max != r.rlim_cur) {
        if (r.rlim_max == RLIM_INFINITY)
            r.rlim_cur = MAX_OPEN_FILE;
        else if (r.rlim_cur < r.rlim_max)
            r.rlim_cur = r.rlim_max;
        if (setrlimit(RLIMIT_NOFILE, &r) < 0) {
            printf("setrlimit");
            return 0;
        }
    }

    return (int)r.rlim_cur;
}



void *find_symbol(const char *name)
{
    void *symbol = dlsym(RTLD_NEXT, name);
    if (!symbol)
        symbol = dlsym(RTLD_DEFAULT, name);
    return symbol;
}

int epoll_add_fd(int epfd, int evt, int fd)
{
	struct epoll_event event;
	event.events = evt | EPOLLERR | EPOLLHUP;
	event.data.ptr = fd;

	if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) != 0) {
		debug("epoll add failed, errno=%d\n", errno);
		return -1;
	}
	return 0;
}


int epoll_mod_fd(int epfd, int evt, int fd)
{
	struct epoll_event event;
	event.events = evt | EPOLLERR | EPOLLHUP;
	event.data.ptr = fd;

	if(epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event) != 0) {
		debug("epoll add failed, errno=%d\n", errno);
		return -1;
	}
	return 0;
}

int epoll_del_fd(int epfd, int fd)
{
	struct epoll_event event;

	if(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &event) != 0) {
		debug("epoll del failed, errno=%d\n", errno);
		return -1;
	}
	return 0;
}

