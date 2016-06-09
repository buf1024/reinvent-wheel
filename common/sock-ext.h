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

typedef struct resov_data_s resov_data_t;

int tcp_noblock_resolve(coro_t* coro);
resov_data_t* tcp_reslove_data(const char* host);
int tcp_reslove_data_host(resov_data_t* data, char* host);
int tcp_reslove_data_addr(resov_data_t* data, char* addr);


#ifdef __cplusplus
}
#endif

#endif /* __SOCK_EXT_H__ */
