/*
 * sock-con.c
 *
 *  Created on: 2016/6/13
 *      Author: Luo Guochun
 */

typedef struct buffer_s        buffer_t;

struct connection_s
{
	int fd;
	int state;

	buffer_t* recv_buf;
	buffer_t* send_buf;

	void* data;
};
struct buffer_s
{
	int size;
	char* buf;
};
