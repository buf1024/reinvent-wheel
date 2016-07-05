/*
 * dummyproxy.c
 *
 *  Created on: 2016-7-5
 *      Author: Luo Guochun
 */

#include "simpleproxy.h"

static int __dummy_init(simpleproxy_t* proxy, tson_t* tson)
{
    return 0;
}
static int __dummy_uninit()
{
    return 0;
}
static int __dummy_parse(const char* packet)
{
    return 0;
}
static proxy_session_t* __dummy_proxy(simpleproxy_t* proxy, connection_t* con)
{
    return 0;
}

proxy_plugin_t* proxy_plugin_dummy()
{
    static proxy_plugin_t plugin = {
            .init = __dummy_init,
            .uninit = __dummy_uninit,
            .parse = __dummy_parse,
            .proxy = __dummy_proxy
    };

    return &plugin;
}




