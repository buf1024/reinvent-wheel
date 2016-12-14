/*
 * buffer.c
 *
 *  Created on: 2016/12/13
 *      Author: Luo Guochun
 */

#include "buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>



buffer_t* buffer_new(int size)
{
	buffer_t* buf = (buffer_t*)calloc(1, sizeof(*buf));
	if(buf) {
		buf->buf = (char*)malloc(sizeof(char)*size);
		if(!buf->buf) {
			free(buf);
			buf = NULL;
		}
		buf->size = size;
	}
	return buf;
}


int buffer_set_read_size(buffer_t* buf, int size)
{
	if(!buf || size <= 0)  return -1;
	int rd = buf->pos;

	if(size < buf->pos) {
		rd = size;
	}
	if(rd != buf->pos) {
		buf->pos = buf->pos - rd;
		memmove(buf->buf, buf->buf + rd, buf->pos);
	}
	return 0;
}

int buffer_set_write_size(buffer_t* buf, int size)
{
	if(!buf)  return -1;

	if(buf->pos + size > buf->size) {
		buf->pos = buf->size;
	}else{
		buf->pos += size;
	}
	return 0;
}


int buffer_read(buffer_t* buf, char* dest, int size)
{
	if(!buf || !dest) return 0;

	int rd = buf->pos;
	bool rd_all = true;
	if(buf->pos > size){
		rd = size;
		rd_all = false;
	}
	if(rd <= 0) return 0;

	memcpy(dest, buf->buf, rd);
	if(!rd_all) {
		memmove(buf->buf, buf->buf + rd, buf->pos - rd);
	}
	buf->pos -= rd;
	return rd;
}
int buffer_write(buffer_t* buf, const char* src, int size)
{
	if(!buf || !src)  return 0;
	int free_size = buf->size - buf->pos;
	int wr = free_size >= size ? size : free_size;
	if(wr <= 0) return 0;
	memcpy(buf->buf + buf->pos, src, wr);
	buf->pos += wr;
	return wr;
}

int buffer_free(buffer_t* buf)
{
	if(buf) {
		if(buf->buf) {
			free(buf->buf);
			buf->buf = NULL;
		}
		free(buf);
	}
	return 0;
}

