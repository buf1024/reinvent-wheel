/*
 * task.h
 *
 *  Created on: 2016/7/26
 *      Author: Luo Guochun
 */

#ifndef __TASK_H__
#define __TASK_H__

#include "httpd.h"

void* http_task_thread(void* data);
int thread_timer_task(http_thread_t* t);

int http_req_task(http_thread_t* t, connection_t* con);

int schedule_fd(httpd_t* http, int fd);
int httpd_main_loop(httpd_t* http);
int main_timer_task(httpd_t* http);

int write_cmd_msg(int fd, http_msg_t* msg);
http_msg_t* read_cmd_msg(int fd);


#endif /* __TASK_H__ */
