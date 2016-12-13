/*
 * thinconf.c
 *
 *  Created on: 2016/5/18
 *      Author: Luo Guochun
 */

#include "simpleproxy.h"

int schedule_fd(simpleproxy_t* proxy, int fd)
{
	static long counter = 0;

	int index = counter++ % proxy->thread_num;
	proxy_thread_t* t = &(proxy->threads[index]);

	LOG_INFO("select thread %d to handle.\n", index);

	if(write(t->fd[1], &fd, sizeof(int)) != sizeof(int)) {
		LOG_ERROR("write fd to client failed.\n");
		return -1;
	}

	return 0;
}

int proxy_main_loop(simpleproxy_t* proxy)
{
	char client_ip[128]; int client_port = 0;
	for(;;) {
		struct sockaddr_storage sa;
		socklen_t salen = sizeof(sa);

		int fd = accept(proxy->fd, (struct sockaddr*) &sa, &salen);

		if(fd < 0) {
			if(proxy->sig_term) {
				LOG_INFO("catch quit signal.\n");
				break;
			}
			if(proxy->sig_usr1) {
				proxy->sig_usr1 = false;
			}
			if(proxy->sig_usr2) {
				LOG_DEBUG("log flush.\n");
				log_flush();
				proxy->sig_usr2 = false;
			}
			continue;
		}


		tcp_sock_name(fd, client_ip, sizeof(client_ip), &client_port);

		LOG_INFO("accept client: ip=%s, port=%d\n", client_ip, client_port);

		if(schedule_fd(proxy, fd) != 0) {
			LOG_ERROR("schedule fd to thread failed.\n");
			close(fd);
		}

		if(proxy->sig_usr1) {
			proxy->sig_usr1 = false;
		}
		if(proxy->sig_usr2) {
			log_flush();
			proxy->sig_usr2 = false;
		}

	}
	return 0;
}

int proxy_init(simpleproxy_t* proxy)
{
	if(log_init(proxy->log_level, proxy->log_level, proxy->log_path,
			proxy->log_head, proxy->log_buf_size, 0, -1) != 0) {
		printf("log_init failed.\n");
		return -1;
	}
	LOG_INFO("logger is ready.\n");

	proxy->fd = proxy->proto_v == PROTO_IPV4 ?
			tcp_server(proxy->listen, proxy->port, LISTEN_BACK_LOG) :
			tcp6_server(proxy->listen, proxy->port, LISTEN_BACK_LOG);

	if(proxy->fd <= 0) {
		LOG_FATAL("listen server failed, host=%s, port=%d\n", proxy->listen, proxy->port);
		return -1;
	}
	LOG_INFO("listening ip=%s, port=%d\n", proxy->listen, proxy->port);

	proxy->nfd = get_max_open_file_count();
	if(proxy->nfd <= 0) {
		LOG_FATAL("get_max_open_file_count failed.\n");
		return -1;
	}
	LOG_INFO("max open file: %u\n", proxy->nfd);
	proxy->conns = calloc(proxy->nfd, sizeof(connection_t));
	if(!proxy->conns) {
		LOG_FATAL("alloc connection failed.\n");
		return -1;
	}

	proxy->threads = calloc(proxy->thread_num, sizeof(proxy_thread_t));
	if(!proxy->threads) {
		LOG_FATAL("alloc thread failed.\n");
		return -1;
	}

	for(int i=0; i<proxy->thread_num; i++) {
		proxy_thread_t* t = &proxy->threads[i];

		t->proxy = proxy;
		t->nfd = proxy->nfd / proxy->thread_num;
		t->evts = calloc(t->nfd, sizeof(struct epoll_event));
		if(!t->evts) {
			LOG_FATAL("alloc thread info failed.\n");
			return -1;
		}
		t->state = THREAD_STATE_DEAD;

		t->epfd = epoll_create1(EPOLL_CLOEXEC);
		if(t->epfd <= 0) {
			LOG_ERROR("epoll_create1 failed, errno=%d\n", errno);
			return -1;
		}

		if(pipe(t->fd) != 0) {
			LOG_ERROR("thread create pipe failed.\n");
			return -1;
		}

		connection_t* con = &(proxy->conns[t->fd[0]]);
		con->fd = t->fd[0];
		con->state = CONN_STATE_CONNECTED;
		con->type = CONN_TYPE_CLIENT;

		if(epoll_add_fd(t->epfd, EPOLLIN, con) != 0) {
			LOG_ERROR("epoll_add_fd failed.\n");
			return -1;
		}

		pthread_create(&t->tid, NULL, proxy_task_routine, t);
		LOG_INFO("create thread suc! tid = %ld\n", t->tid);
	}

	int rfd = tcp_noblock_resolve_fd();
	connection_t* con = &(proxy->conns[rfd]);
	con->fd = rfd;
	con->state = CONN_STATE_CONNECTED;
	con->type = CONN_TYPE_CLIENT;

	if(epoll_add_fd(proxy->threads[0].epfd, EPOLLIN, con) != 0) {
		LOG_ERROR("epoll_add_fd failed.\n");
		return -1;
	}


	if(proxy->plugin->init(proxy) != 0) {
		LOG_ERROR("plugin init failed.\n");
		return -1;
	}

	return 0;

}
int proxy_uninit(simpleproxy_t* proxy)
{
	if (proxy->plugin) {
		if (proxy->plugin->uninit() != 0) {
			LOG_ERROR("plugin init failed.\n");
			//return -1;
		}
	}

	close(proxy->fd);
	log_finish();
	return 0;
}


void* proxy_task_routine(void* args)
{
	proxy_thread_t* t = (proxy_thread_t*)args;
	t->state = THREAD_STATE_ACTIVE;

	connection_t* con = NULL;

	for (;;) {
		int rv = epoll_wait(t->epfd, t->evts, t->nfd, EPOLL_TIMEOUT);
		if(rv == 0) {
			//if(proxy_timer_task(t) != 0) {
			//	LOG_ERROR("proxy_timer_task failed.\n");
			//}
			continue;
		}else if(rv < 0) {
			if(errno == EINTR) {
				if(t->proxy->sig_term) {
					break;
				}
				continue;
			}
			LOG_ERROR("epoll_wait error, errno=%d\n", errno);
			break;
		}else{
			for(int i=0;i<rv; i++) {
				struct epoll_event* ev = &(t->evts[i]);
				con = (connection_t*)ev->data.ptr;

				if(con->fd == t->fd[0]) {
					int fd = 0;
					if(read(con->fd, &fd, sizeof(int)) != sizeof(int)) {
						LOG_ERROR("pipe read failed.\n");
						continue;
					}

					LOG_INFO("thread accept: fd=%d\n", fd);

					tcp_nodelay(fd, true);
					tcp_noblock(fd, true);


					con =  &(t->proxy->conns[fd]);
					con->fd = fd;
					con->type = CONN_TYPE_CLIENT;
					con->state = CONN_STATE_CONNECTED;
					con->sess = NULL;
					if(!con->rbuf) {
						con->rbuf = (buffer_t*)malloc(sizeof(buffer_t));
						con->rbuf->cache = (char*)malloc(DEFAULT_BUF_SIZE);
					}
					con->rbuf->size = 0;
					if(!con->wbuf) {
						con->wbuf = (buffer_t*)malloc(sizeof(buffer_t));
						con->wbuf->cache = (char*)malloc(DEFAULT_BUF_SIZE);
					}
					con->wbuf->size = 0;

					if(epoll_add_fd(t->epfd, EPOLLIN, con) < 0) {
						LOG_ERROR("epoll_add_fd failed.\n");
						close(fd);
						continue;
					}
				}
				if(proxy_biz_task(t, con) != 0) {
					LOG_ERROR("proxy_biz_task failed.\n");
				}
				if(con->state == CONN_STATE_BROKEN) {
					LOG_INFO("connection is broken, delete it.\n");
					epoll_del_fd(t->epfd, con);
					close(con->fd);
				}

			}
		}
	}
	t->state = THREAD_STATE_DEAD;
	return NULL;
}

int proxy_timer_task(proxy_thread_t* t)
{
	LOG_DEBUG("timer task tid=%ld, timeout=%d\n", t->tid, EPOLL_TIMEOUT);
	return 0;
}
int proxy_biz_task(proxy_thread_t* t, connection_t* con)
{
	bool ok = false;
	if (con->state == CONN_STATE_CONNECTED) {
		con->rbuf->size += tcp_read(con->fd, con->rbuf->cache, DEFAULT_BUF_SIZE - con->rbuf->size, &ok);
		if (!ok) {
			con->state = CONN_STATE_BROKEN;
			if (con->sess) {
				coro_free(con->sess->coro);
				free(con->sess);
				con->sess = NULL;
			}
			con->rbuf->size = 0;
			con->wbuf->size = 0;
			return -1;
		}
		if (con->rbuf->size <= 0) {
			LOG_DEBUG("conn size <= 0");
			return 0;
		}
	}

	proxy_session_t* session = t->proxy->plugin->session(con);
	if(!session) {
		session = (proxy_session_t*)malloc(sizeof(*session));
		if(!session) {
			LOG_ERROR("allocate session failed.\n");
			return -1;
		}
		session->coro = coro_new2(&t->proxy->switcher, proxy_coro_fun, session, DEFAULT_CORO_STACK);
		if(!session->coro) {
			LOG_ERROR("coro create failed.\n");
			return -1;
		}

		session->req_con = con;
		session->state = CORO_RESUME;
		session->thread = t;
		con->sess = session;
	}
	if(session->state == CORO_RESUME) {
		session->state = coro_resume(session->coro);
	}


	if(con->state != CONN_STATE_CONNECTED) return 0;

	connection_t* rsp_con = session->rsp_con;


	if(session->state == CORO_ABORT) {
		LOG_DEBUG("coro abort!\n");
		con->sess = NULL;
		coro_free(session->coro);
		free(session);
		session = NULL;
	}else {
		if(rsp_con && rsp_con->wbuf->size) {
			buffer_t* w = session->rsp_con->wbuf;
			tcp_block_write(rsp_con->fd, w->cache, w->size, &ok);
			if(!ok) {
				rsp_con->state = CONN_STATE_BROKEN;
			}
		}
		LOG_DEBUG("coro state = %d\n", session->state);
		if(session->state == CORO_FINISH) {
			con->sess = NULL;
			coro_free(session->coro);
			free(session);
			session = NULL;
		}
	}

	if(con->state == CONN_STATE_BROKEN) {
		LOG_INFO("connetion state broken, delete it.\n");
		epoll_del_fd(t->epfd, con);
		close(con->fd);
		if(session) {
			con->sess = NULL;
			coro_free(session->coro);
			free(session);
			session = NULL;
		}
		con->rbuf->size = 0;
		con->wbuf->size = 0;
	}
	if(rsp_con && rsp_con != con && con->state == CONN_STATE_BROKEN) {
		LOG_INFO("connetion state broken, delete it.\n");
		epoll_del_fd(t->epfd, rsp_con);
		close(rsp_con->fd);
		if(session) {
			rsp_con->sess = NULL;
			coro_free(session->coro);
			free(session);
			session = NULL;
		}
		rsp_con->rbuf->size = 0;
		rsp_con->wbuf->size = 0;
	}


	return 0;
}
int proxy_coro_fun(coro_t* coro)
{
	LOG_INFO("invoke coro function!\n");
	proxy_session_t* session = (proxy_session_t*)coro_get_data(coro);
	proxy_plugin_t* plugin = session->thread->proxy->plugin;

	int state = plugin->proxy(session);
	if(state == CORO_RESUME) {
		coro_yield_value(session->coro, CORO_RESUME);
	}
	return state;
}
