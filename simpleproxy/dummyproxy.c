/*
 * dummyproxy.c
 *
 *  Created on: 2016-7-5
 *      Author: Luo Guochun
 */

#include "simpleproxy.h"

static int __dummy_init(simpleproxy_t* proxy)
{
	(void)proxy;
    return 0;
}
static int __dummy_uninit()
{
    return 0;
}

static int __dummy_proxy(proxy_session_t* session)
{
	connection_t* con = session->req_con;

	char buf[1024] = {0};
	bool ok;
	tcp_read(con->fd, buf, 1024, &ok);
	LOG_DEBUG("dummy read: %s\n", buf);

    return CORO_FINISH;
}

proxy_plugin_t* simple_proxy_plugin_dummy()
{
    static proxy_plugin_t plugin = {
            .init = __dummy_init,
            .uninit = __dummy_uninit,
            .proxy = __dummy_proxy
    };

    return &plugin;
}




