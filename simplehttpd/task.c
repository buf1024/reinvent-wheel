/*
 * task.c
 *
 *  Created on: 2016/7/26
 *      Author: Luo Guochun
 */

#include "httpd.h"
#include "httputil.h"
#include "task.h"
#include "sock.h"
#include <sys/epoll.h>
#include <sys/socket.h>


int write_cmd_msg(int fd, http_msg_t* msg)
{
	if(write(fd, msg, sizeof(http_msg_t*)) != sizeof(http_msg_t*)) {
		LOG_ERROR("write fd to client failed.\n");
		return -1;
	}
	return 0;
}

http_msg_t* read_cmd_msg(int fd)
{
	http_msg_t* msg = NULL;
	if(read(fd, msg, sizeof(http_msg_t*)) != sizeof(http_msg_t*)) {
		LOG_ERROR("pipe read failed.\n");
	}
	return msg;
}

int schedule_fd(httpd_t* http, int fd)
{

	static unsigned long counter = 0;

	int index = counter++ % http->thread_num;
	http_thread_t* t = &(http->threads[index]);

	LOG_INFO("select thread %d to handle.\n", index);

	http_msg_t* msg = (http_msg_t*)calloc(1, sizeof(*msg));
	msg->data.fd = fd;
	if(msg != NULL) {
	    return write_cmd_msg(t->fd[1], msg);
	}

	return -1;
}

int httpd_main_loop(httpd_t* http)
{

	char client_ip[128]; int client_port = 0;
	for(;;) {

		int rv = epoll_wait(http->epfd, http->evts, http->nfd, EPOLL_TIMEOUT);
		if(rv == 0) {
			if(main_timer_task(http) != 0) {
				LOG_ERROR("main_timer_task failed.\n");
			}
			continue;
		}else if(rv < 0) {
			if (http->sig_term) {
				LOG_INFO("catch quit signal.\n");
				break;
			}
			if (http->sig_usr1) {
				http->sig_usr1 = false;
				continue;
			}
			if (http->sig_usr2) {
				LOG_DEBUG("log flush.\n");
				log_flush();
				http->sig_usr2 = false;
				continue;
			}
			LOG_ERROR("epoll_wait error, errno=%d\n", errno);
			break;
		} else {
			for (int i = 0; i < rv; i++) {
				struct epoll_event* ev = &(http->evts[i]);
				connection_t* con = (connection_t*) ev->data.ptr;

				struct sockaddr_storage sa;
				socklen_t salen = sizeof(sa);

				int fd = accept4(con->fd, (struct sockaddr*) &sa, &salen,
						SOCK_NONBLOCK);

				if (fd < 0) {
					LOG_ERROR("accept failed.\n");
					continue;
				}

				tcp_sock_name(fd, client_ip, sizeof(client_ip), &client_port);

				LOG_INFO("accept client: ip=%s, port=%d\n", client_ip, client_port);

				if (schedule_fd(http, fd) != 0) {
					LOG_ERROR("schedule fd to thread failed.\n");
					close(fd);
				}
			}
		}
	}
	return 0;
}

void* http_task_thread(void* data)
{
	http_thread_t* t = (http_thread_t*)data;

	// todo things to wait
	pthread_barrier_wait(t->barrier);
	t->barrier = NULL;

	connection_t* con = NULL;

	for (;;) {
		int rv = epoll_wait(t->epfd, t->evts, t->nfd, EPOLL_TIMEOUT);
		if(rv == 0) {
			if(thread_timer_task(t) != 0) {
				LOG_ERROR("thread_timer_task failed.\n");
				continue;
			}
		} else if (rv < 0) {
			if (t->http->sig_term || t->http->sig_usr1 || t->http->sig_usr2) {
				continue;
			}
			LOG_ERROR("epoll_wait error, errno=%d\n", errno);
			break;
		}else{
			for(int i=0;i<rv; i++) {
				struct epoll_event* ev = &(t->evts[i]);
				con = (connection_t*)ev->data.ptr;

				if(con->fd == t->fd[0]) {
					http_msg_t* msg = read_cmd_msg(con->fd);
					if(!msg) {
						LOG_ERROR("msg is null.\n");
						continue;
					}
					int fd = msg->data.fd;
					LOG_INFO("thread accept: fd=%d\n", fd);
					free(msg);

					tcp_nodelay(fd, true);
					tcp_noblock(fd, true);


					con =  &(t->http->conns[fd]);
					con->fd = fd;
					con->type = CONN_TYPE_CLIENT;

					if(epoll_add_fd(t->epfd, EPOLLIN, con) < 0) {
						LOG_ERROR("epoll_add_fd failed.\n");
						close(fd);
						continue;
					}
				}
				if(http_req_task(t, con) != 0) {
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
	return NULL;
}


int thread_timer_task(http_thread_t* t)
{
	LOG_DEBUG("thread timer task call.tid=%ld\n", t->tid);

	return 0;
}


int main_timer_task(httpd_t* http)
{
	LOG_DEBUG("main timer task call.tnum: %d\n", http->thread_num);

	return 0;
}

int http_req_task(http_thread_t* t, connection_t* con)
{
	LOG_DEBUG("thread timer task call.tid=%ld, fd=%d\n", t->tid, con->fd);

	return 0;
}
