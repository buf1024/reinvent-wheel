/*
 * task.c
 *
 *  Created on: 2016/6/20
 *      Author: Luo Guochun
 */

#include "lazy.h"

int lazy_proxy_task(lazy_proxy_t* pxy)
{
	coro_switcher_t switcher;
	while (true) {
		int rv = epoll_wait(pxy->epfd, pxy->events, pxy->conn_size, 1000);
		if(rv == 0) {
			lazy_timer_task(pxy);
		}else if(rv < 0) {
			if(errno == EINTR) {
				if(pxy->sig_term) {
					LOG_INFO("catch signal TERM\n");
					break;
				}
				if(pxy->sig_usr2) {
					LOG_INFO("catch signal USER2\n");
					log_flush();
					pxy->sig_usr2 = false;
				}
				continue;
			}
			LOG_ERROR("epoll_wait error, errno=%d\n", errno);
			break;
		}else{
			int i=0;
			for(;i<rv; i++) {
				struct epoll_event* ev = &(pxy->events[i]);
				connection_t* con = (connection_t*)ev->data.ptr;

				if((ev->events & EPOLLIN) !=  EPOLLIN) continue;

				if(con->state == CONN_STATE_LISTENING) {
					char addr[MAX_ADDR_SIZE] = {0};
					int port = 0;
					int fd = tcp_accept(pxy->listen, addr, MAX_ADDR_SIZE, &port);
					if(fd <= 0) {
						LOG_ERROR("tcp_accept failed\n");
						continue;
					}
					LOG_INFO("tcp_accept: fd=%d, ip=%s, port=%d\n", fd, addr, port);

					tcp_nodelay(fd, true);
					tcp_noblock(fd, true);

					if(fd >= pxy->conn_size) {
						int grow = MAX_CONCURRENT_CONN/4;
						pxy->conn = realloc(pxy->conn, sizeof(connection_t) * (pxy->conn_size + grow));
						pxy->events = realloc(pxy->events, sizeof(struct epoll_event) * (pxy->conn_size + grow));
						if(!pxy->conn || !pxy->events) {
							LOG_ERROR("realloc oom! size = %d\n", pxy->conn_size);
							log_finish();
							abort();
						}
						memset(pxy->conn + pxy->conn_size, 0, sizeof(connection_t) * grow);
						memset(pxy->events + pxy->conn_size, 0, sizeof(struct epoll_event) * grow);

						pxy->conn_size += grow;
					}

					connection_t* con = &pxy->conn[fd];
					con->fd = fd;
					con->type = CONN_TYPE_CLIENT;
					con->state = CONN_STATE_CONNECTED;
					con->pxy = pxy;

					if(lazy_add_fd(pxy->epfd, con) < 0) {
						LOG_ERROR("lazy_add_fd failed.\n");
						close(fd);
					}
					lazy_spawn_coro(&switcher, con);
					// TODO biz
				}

				if(con->state == CONN_STATE_CONNECTED) {
					resume_coro_demand(con->coro);
				}
				if(con->state == CONN_STATE_CONNECTING) {

				}

				if(con->state == CONN_STATE_CLOSING) {
				}

			}
		}
	}
	return 0;
}
int lazy_timer_task(lazy_proxy_t* pxy)
{
	LOG_DEBUG("lazy_timer_task: timeout = %dms\n", pxy->timout);
	return 0;
}
