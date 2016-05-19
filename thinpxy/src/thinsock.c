/*
 * thinsock.c
 *
 *  Created on: 2016/5/18
 *      Author: Luo Guochun
 */

#include "thinpxy.h"

#include <sys/epoll.h>
#include <sys/resource.h>

static bool set_sock_opt(int fd, int lv, int name)
{
    int enable = 1;
    return 0 == setsockopt(fd, lv, name, (void*)&enable, sizeof(enable));

}

static int listen_addr(struct addrinfo *addrs)
{
    const struct addrinfo *addr = NULL;

    for (addr = addrs; addr; addr = addr->ai_next) {
        int fd = socket(addr->ai_family,
            addr->ai_socktype | SOCK_CLOEXEC, addr->ai_protocol);
        if (fd < 0)
            continue;

        set_sock_opt(fd, SOL_SOCKET, SO_REUSEADDR);
        set_sock_opt(fd, SOL_SOCKET, SO_REUSEPORT);

        if (!bind(fd, addr->ai_addr, addr->ai_addrlen)) {
        	int backlog = 128;
            FILE* fp  = fopen("/proc/sys/net/core/somaxconn", "rb");
            if (fp) {
                int tmp;
                if (fscanf(fp, "%d", &tmp) == 1){
                    backlog = tmp;
                }
                fclose(fp);
            }

            if(listen(fd, backlog) < 0) {
            	LOG_ERROR("listen failed.\n");
            }
            return fd;
        }

        close(fd);
    }

    return -1;
}

static int epoll_add(thinpxy_t* pxy, int fd)
{
	struct epoll_event ee;
	ee.data.ptr = &(pxy->conns[fd]);
	ee.events |= EPOLLERR; ee.events |= EPOLLIN; ee.events |= EPOLLOUT;

	return epoll_ctl(pxy->epoll_fd, fd, &ee);
}

int init_net(thinpxy_t* pxy)
{
    struct rlimit r;

    if (getrlimit(RLIMIT_NOFILE, &r) < 0) {
    	return -1;
    }
    pxy->conns = calloc(r.rlim_cur, sizeof(connection_t));
    if(!pxy->conns) {
    	return -1;
    }

    pxy->listen_fd = listen_addr(pxy->config.addrs);
    if(pxy->listen_fd < 0) {
    	LOG_ERROR("listen_addr failed.\n");
    }

    connection_t* conn = &(pxy->conns[pxy->listen_fd]);
    conn->fd = pxy->listen_fd;
    conn->type = CONN_TYPE_SERVER;
    conn->state = CONN_STATE_LISTENING;


	pxy->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
	if(pxy->epoll_fd < 0) {
		return -1;
	}

	if(epoll_add(pxy, pxy->listen_fd) != 0) {
		LOG_ERROR("epoll_add failed.\n");
		return -1;
	}


}

