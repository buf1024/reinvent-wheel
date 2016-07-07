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

	if(con->rbuf->size) {
	    LOG_DEBUG("dummy read(%d): %s\n", con->rbuf->size, con->rbuf->cache);

	    session->rsp_con = con;

	    memcpy(con->wbuf->cache, con->rbuf->cache, con->rbuf->size);
	    con->wbuf->size = con->rbuf->size;

	    buffer_set_read_size(con->rbuf, con->rbuf->size);

	    LOG_DEBUG("dummy write(%d): %s\n", con->wbuf->size, con->wbuf->cache);
	}

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




