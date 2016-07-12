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

#include "sock.h"

int tcp_noblock_resolve(const char* host, void* data, int thread);
bool tcp_noblock_resolve_result(char** host, char** addr, void** data);
int tcp_noblock_resolve_fd();


#ifdef __cplusplus
}
#endif

#endif /* __SOCK_EXT_H__ */
