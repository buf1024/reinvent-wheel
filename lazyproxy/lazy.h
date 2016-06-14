/*
 * lazy.h
 *
 *  Created on: 2016/6/14
 *      Author: Luo Guochun
 */

#ifndef __LAZY_H__
#define __LAZY_H__

#include "coro.h"

typedef struct lazy_mine_s
{
	int epfd;
}lazy_mine_t;


typedef struct http_req_s
{
	coro_t* coro_resv;
}http_req_t;

typedef struct http_rsp_s
{

}http_rsp_t;


int lazy_net_init(lazy_mine_t* l);
int lazy_net_uninit(lazy_mine_t* l);

int lazy_spawn_coro(lazy_mine_t* l);

int lazy_http_req_coro(coro_t* coro);


#endif /* __LAZY_H__ */
