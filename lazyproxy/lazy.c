/*
 * lazy.c
 *
 *  Created on: 2016/6/20
 *      Author: Luo Guochun
 */


#include "lazy.h"

int lazy_net_init(lazy_proxy_t* pxy)
{
	pxy->conn_size = MAX_CONCURRENT_CONN;
	pxy->timout = MAX_EPOLL_TIMEOUT;

	pxy->conn = malloc(sizeof(connection_t) * pxy->conn_size);
	OOM_CHECK(pxy->conn, "malloc(sizeof(connection_t) * pxy->conn_size)");

	pxy->events = malloc(sizeof(struct epoll_event) * pxy->conn_size);
    OOM_CHECK(pxy->events, "malloc(sizeof(struct epoll_event) * pxy->conn_size)");

    memset(pxy->conn, 0, sizeof(connection_t) * pxy->conn_size);
    memset(pxy->events, 0, sizeof(struct epoll_event) * pxy->conn_size);


	char ipbuf[MAX_ADDR_SIZE] = {0};
	if(tcp_resolve(pxy->host, ipbuf, sizeof(ipbuf)) != 0) {
		LOG_ERROR("tcp_resolve failed, errno=%d\n", errno);
		return -1;
	}
	pxy->listen = tcp_server(ipbuf, pxy->port, 128);
	if(pxy->listen <= 0) {
		LOG_ERROR("tcp_server failed, errno=%d\n", errno);
		return -1;
	}
	connection_t* con = &pxy->conn[pxy->listen];
	con->fd = pxy->listen;
	con->type = CONN_TYPE_SERVER;
	con->state = CONN_STATE_LISTENING;

	pxy->epfd = epoll_create1(EPOLL_CLOEXEC);
	if(pxy->epfd <= 0) {
		LOG_ERROR("epoll_create1 failed, errno=%d\n", errno);
		return -1;
	}

	lazy_add_fd(pxy->epfd, EPOLLIN|EPOLLOUT, con);

	LOG_INFO("listening: host=%s, ip=%s, port=%d\n", pxy->host, ipbuf, pxy->port);

	int fd = tcp_noblock_resolve(NULL);
	if(fd <= 0) {
        LOG_ERROR("tcp_noblock_resolve failed, errno=%d\n", errno);
        return -1;
	}
    con = &pxy->conn[fd];
    con->fd = fd;
    con->type = CONN_TYPE_CLIENT;
    con->state = CONN_STATE_RESOLVING;
    lazy_add_fd(pxy->epfd, EPOLLIN, con);

	return 0;
}
int lazy_net_uninit(lazy_proxy_t* pxy)
{
	LOG_INFO("lazy_net_uninit, leave os to free resouce. 0x%x\n", pxy);
	return 0;
}

int lazy_add_fd(int epfd, int evt, connection_t* con)
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


int lazy_mod_fd(int epfd, int evt, connection_t* con)
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

int lazy_del_fd(int epfd, connection_t* con)
{
	struct epoll_event event;

	if(epoll_ctl(epfd, EPOLL_CTL_DEL, con->fd, &event) != 0) {
		LOG_ERROR("epoll del failed, errno=%d\n", errno);
		return -1;
	}
	return 0;
}
