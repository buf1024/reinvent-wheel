/*
 * task.c
 *
 *  Created on: 2016/7/26
 *      Author: Luo Guochun
 */

#include "httpd.h"
#include "task.h"

int schedule_fd(httpd_t* http, int fd)
{
	(void)(http);
	(void)(fd);
#if 0
	static long counter = 0;

	int index = counter++ % http->thread_num;
	proxy_thread_t* t = &(http->threads[index]);

	LOG_INFO("select thread %d to handle.\n", index);

	if(write(t->fd[1], &fd, sizeof(int)) != sizeof(int)) {
		LOG_ERROR("write fd to client failed.\n");
		return -1;
	}
#endif
	return 0;
}

int httpd_main_loop(httpd_t* http)
{
	(void)(http);
#if 0
	char client_ip[128]; int client_port = 0;
	for(;;) {
		struct sockaddr_storage sa;
		socklen_t salen = sizeof(sa);

		int fd = accept(http->fd, (struct sockaddr*) &sa, &salen);

		if(fd < 0) {
			if(http->sig_term) {
				LOG_INFO("catch quit signal.\n");
				break;
			}
			if(http->sig_usr1) {
				http->sig_usr1 = false;
			}
			if(http->sig_usr2) {
				LOG_DEBUG("log flush.\n");
				log_flush();
				http->sig_usr2 = false;
			}
			continue;
		}


		tcp_sock_name(fd, client_ip, sizeof(client_ip), &client_port);

		LOG_INFO("accept client: ip=%s, port=%d\n", client_ip, client_port);

		if(schedule_fd(http, fd) != 0) {
			LOG_ERROR("schedule fd to thread failed.\n");
			close(fd);
		}

		if(http->sig_usr1) {
			http->sig_usr1 = false;
		}
		if(http->sig_usr2) {
			log_flush();
			http->sig_usr2 = false;
		}

	}
#endif
	return 0;
}
