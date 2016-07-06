/*
 * httpproxy.c
 *
 *  Created on: 2016/7/4
 *      Author: Luo Guochun
 */

#include "simpleproxy.h"

static int __http_init(simpleproxy_t* proxy, tson_t* tson)
{
	return 0;
}
static int __http_uninit()
{
	return 0;
}
static int __http_parse(const char* packet)
{
	return 0;
}
static proxy_session_t* __http_proxy(simpleproxy_t* proxy, connection_t* con)
{
	return 0;
}

proxy_plugin_t* proxy_plugin_http()
{
	static proxy_plugin_t plugin = {
			.init = __http_init,
			.uninit = __http_uninit,
			.parse = __http_parse,
			.proxy = __http_proxy
	};

	return &plugin;
}


