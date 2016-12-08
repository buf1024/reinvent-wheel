/*
 * task.c
 *
 *  Created on: 2016/6/20
 *      Author: Luo Guochun
 */

#include "lazy.h"

int lazy_proxy_task(lazy_proxy_t* lazy)
{
	while (true) {
		int rv = epoll_wait(lazy->epfd, lazy->events, lazy->conn_size, lazy->timout);
		if(rv == 0) {
			lazy_timer_task(lazy);
		}else if(rv < 0) {
			if(errno == EINTR) {
				if(lazy->sig_term) {
					LOG_INFO("catch signal TERM\n");
					break;
				}
				if(lazy->sig_usr2) {
					LOG_INFO("catch signal USER2\n");
					log_flush();
					lazy->sig_usr2 = false;
				}
				continue;
			}
			LOG_ERROR("epoll_wait error, errno=%d\n", errno);
			break;
		}else{
			for(int i=0; i<rv; i++) {
				struct epoll_event* ev = &(lazy->events[i]);
				connection_t* con = (connection_t*)ev->data.ptr;

				LOG_DEBUG("epoll fd = %d, state = %d\n", con->fd, con->state);
				if(con->state == CONN_STATE_LISTENING) {
					char addr[MAX_ADDR_SIZE] = {0};
					int port = 0;
					int fd = tcp_accept(lazy->listen, addr, MAX_ADDR_SIZE, &port);
					if(fd <= 0) {
						LOG_ERROR("tcp_accept failed\n");
						continue;
					}
					LOG_INFO("tcp_accept: fd=%d, ip=%s, port=%d\n", fd, addr, port);

					tcp_nodelay(fd, true);
					tcp_noblock(fd, true);

					if(fd >= lazy->conn_size) {
						int grow = MAX_CONCURRENT_CONN_GROW;

						lazy->conn = realloc(lazy->conn, sizeof(connection_t) * (lazy->conn_size + grow));
						OOM_CHECK(lazy->conn, "realloc(lazy->conn, sizeof(connection_t) * (lazy->conn_size + grow))");

						lazy->events = realloc(lazy->events, sizeof(struct epoll_event) * (lazy->conn_size + grow));
						OOM_CHECK(lazy->events, "realloc(lazy->events, sizeof(struct epoll_event) * (lazy->conn_size + grow))");

						memset(lazy->conn + lazy->conn_size, 0, sizeof(connection_t) * grow);
						memset(lazy->events + lazy->conn_size, 0, sizeof(struct epoll_event) * grow);

						lazy->conn_size += grow;
					}

					connection_t* con = &lazy->conn[fd];
					con->lazy = lazy;
					con->fd = fd;
					con->type = CONN_TYPE_CLIENT;
					con->state = CONN_STATE_CONNECTED;

					if(lazy_add_fd(lazy->epfd, EPOLLIN, con) < 0) {
						LOG_ERROR("lazy_add_fd failed.\n");
						close(fd);
					}
					lazy_spawn_http_req_coro(con);
					resume_http_req_coro(con->pxy->coro);
				}

				if(con->state == CONN_STATE_CONNECTED) {

					resume_http_req_coro(con->pxy->coro);
				}
				if(con->state == CONN_STATE_CONNECTING) {
					LOG_DEBUG("connected fd = %d.\n", con->fd);
				    con->state = CONN_STATE_CONNECTED;
				    resume_http_req_coro(con->pxy->coro);
				}
				if(con->state == CONN_STATE_RESOLVING) {
					char* addr = NULL;
					http_proxy_t* p = NULL;
					bool resov = tcp_noblock_resolve_result(NULL, &addr, (void**)&p);
					if(resov) {
				        LOG_DEBUG("resove host \n");
					}
					if(p->req_con) {
						strcpy(p->dst_host, addr);
					    resume_http_req_coro(p->req_con->pxy->coro);
					}
					free(addr);
				}
				if(con->state == CONN_STATE_WAIT) {
					LOG_DEBUG("unexpected event, free resouce\n");
					free_http_proxy(con->pxy);
				}
			}
		}
	}
	return 0;
}
int lazy_timer_task(lazy_proxy_t* lazy)
{
	LOG_DEBUG("lazy_timer_task: timeout = %dms\n", lazy->timout);
	return 0;
}
