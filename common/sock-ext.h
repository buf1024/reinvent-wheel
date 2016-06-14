/*
 * sock-ext.h
 *
 *  Created on: 2016/6/9
 *      Author: Luo Guochun
 */

#ifndef __SOCK_EXT_H__
#define __SOCK_EXT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "coro.h"
#include <stdbool.h>

typedef struct resov_data_s resov_data_t;

int tcp_noblock_resolve(coro_t* coro);
resov_data_t* tcp_resolve_set_data(const char* host, bool rst_poll);
void tcp_resolve_data_free(resov_data_t* data);

int tcp_resolve_data_host(resov_data_t* data, char* host);
int tcp_resolve_data_addr(resov_data_t* data, char* addr);
int tcp_resolve_data_pollfd(resov_data_t* data);


#ifdef __cplusplus
}
#endif

#endif /* __SOCK_EXT_H__ */
