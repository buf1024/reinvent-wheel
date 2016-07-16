/*
 * buffer.h
 *
 *  Created on: 2016-7-16
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
    char* ptr;

    int size;
    int cap;
};

buffer_t* buffer_new(int size);
buffer_t* buffer_enlarge(buffer_t* buf, int size);
int buffer_free(buffer_t* buf);


#ifdef __cplusplus
}
#endif

#endif /* __BUFFER_H__ */
