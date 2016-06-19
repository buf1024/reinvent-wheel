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

int tcp_noblock_resolve_coro(coro_t* coro);


#ifdef __cplusplus
}
#endif

#endif /* __SOCK_EXT_H__ */
