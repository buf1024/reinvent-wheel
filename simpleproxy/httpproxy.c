/*
 * httpproxy.c
 *
 *  Created on: 2016/7/4
 *      Author: Luo Guochun
 */

#include "simpleproxy.h"

static int __http_init(tson_t* tson)
{
	return 0;
}
static int __http_uninit()
{
	return 0;
}
static int __http_parse_packet(const char* packet, char* sid, char* packet_new)
{
	return 0;
}
static int __http_proxy_packet(simpleproxy_t* proxy, proxy_backend_t** backend)
{
	return 0;
}

proxy_plugin_t* proxy_plugin_http()
{
	static proxy_plugin_t plugin = {
			.init = __http_init,
			.uninit = __http_uninit,
			.parse_packet = __http_parse_packet,
			.proxy_packet = __http_proxy_packet
	};

	return &plugin;
}


