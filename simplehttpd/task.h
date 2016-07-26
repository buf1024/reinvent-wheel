/*
 * task.h
 *
 *  Created on: 2016/7/26
 *      Author: Luo Guochun
 */

#ifndef __TASK_H__
#define __TASK_H__


int schedule_fd(httpd_t* http, int fd);
int httpd_main_loop(httpd_t* http);


#endif /* __TASK_H__ */
