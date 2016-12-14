/*
 * buffer.h
 *
 *  Created on: 2016/12/13
 *      Author: Luo Guochun
 */

#ifndef __BUFFER_H__
#define __BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct buffer_s buffer_t;

struct buffer_s
{
	int size;
	int pos;
	char* buf;
};


#define buffer_size(b)      ((b)->pos)
#define buffer_capacity(b)  ((b)->size)
#define buffer_free_size(b) (buffer_capacity(buf) - buffer_size(buf))

#define buffer_read_pos(b)  ((b)->buf)
#define buffer_write_pos(b) ((b)->buf + (b)->pos)


buffer_t* buffer_new(int size);

int buffer_set_write_size(buffer_t* buf, int size);
int buffer_set_read_size(buffer_t* buf, int size);

int buffer_read(buffer_t* buf, char* dest, int size);
int buffer_write(buffer_t* buf, const char* src, int size);

int buffer_free(buffer_t* buf);

#ifdef __cplusplus
}
#endif

#endif /* __BUFFER_H__ */
