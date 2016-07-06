/*
 * httpproxy.c
 *
 *  Created on: 2016/7/4
 *      Author: Luo Guochun
 */

#include "simpleproxy.h"

static int __http_init(simpleproxy_t* proxy)
{
	return 0;
}
static int __http_uninit()
{
	return 0;
}
static int __http_proxy(proxy_session_t* session)
{
	return CORO_FINISH;
}

proxy_plugin_t* simple_proxy_plugin_http()
{
	static proxy_plugin_t plugin = {
			.init = __http_init,
			.uninit = __http_uninit,
			.proxy = __http_proxy
	};

	return &plugin;
}


