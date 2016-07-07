/*
 * httpproxy.c
 *
 *  Created on: 2016/7/4
 *      Author: Luo Guochun
 */

#include "simpleproxy.h"

enum {
	HTTP_METHORD_NONE,
	HTTP_METHORD_GET,
	HTTP_METHORD_POST,
	HTTP_METHORD_CONNECT,
};

enum {
    HTTP_SSL_PORT = 443,
    HTTP_PORT     = 80,
	HTTP_HOST_SIZE = 128,
};

#define LF     ((unsigned char) 10)
#define CR     ((unsigned char) 13)
#define CRLF   "\x0d\x0a"

#define HTTP_END       (CRLF CRLF)
#define HTTP_END_SIZE  sizeof(HTTP_END)

#define HTTP_GET     "GET"
#define HTTP_POST    "POST"
#define HTTP_CONNECT "CONNECT"

#define HTTP_CONTENT_LEN  "Content-Length"
#define HTTP_METHOD_KEY "HTTP_METHOD"

#define HTTP_HOST    "Host"

int parse_http_req_head(const char* req, int size, int* method)
{
	if((unsigned)size <= HTTP_END_SIZE) {
		return 0;
	}

	const char* pos = strstr(req, HTTP_END);
	if(!pos) return 0;

	if(strncmp(HTTP_GET, req, sizeof(HTTP_GET) - 1) == 0) {
		*method = HTTP_METHORD_GET;
	}else if(strncmp(HTTP_POST, req, sizeof(HTTP_POST) - 1) == 0) {
		*method = HTTP_METHORD_POST;
	}else if(strncmp(HTTP_CONNECT, req, sizeof(HTTP_CONNECT) - 1) == 0) {
		*method = HTTP_METHORD_CONNECT;
	}else{
		LOG_ERROR("unknown http method\n");
		return -1;
	}


	return pos - req + HTTP_END_SIZE;
}


static int __http_init(simpleproxy_t* proxy)
{
	int fd = tcp_noblock_resolve(NULL);
	if(fd <= 0) {
        LOG_ERROR("tcp_noblock_resolve failed, errno=%d\n", errno);
        return -1;
	}
    connection_t* con = &proxy->conns[fd];
    con->fd = fd;
    con->type = CONN_TYPE_CLIENT;
    con->state = CONN_STATE_RESOLVING;
    epoll_add_fd(proxy->threads[0].epfd, EPOLLIN, con);

    LOG_INFO("http proxy plugin init. resolve fd=%d\n", fd);

	return 0;
}
static int __http_uninit()
{
	return 0;
}


static proxy_session_t* __http_session(connection_t* con)
{
	if(con->state == CONN_STATE_RESOLVING) {
	    resov_data_t* r = tcp_noblock_resolve_result();

	    return (proxy_session_t*)r->data;
	}
	return con->sess;
}


static int __http_proxy(proxy_session_t* session)
{
	connection_t* con = session->req_con;
	coro_t* coro = session->coro;

	int m = HTTP_METHORD_NONE;
	int rv = 0;
	while(con->rbuf->size) {
		if (m == HTTP_METHORD_NONE) {
			rv = parse_http_req_head(con->rbuf->cache, con->rbuf->size, &m);
			if (rv < 0)
				return CORO_ABORT;
			if (rv == 0) {
				coro_yield_value(coro, CORO_RESUME);
				continue;
			}
		}else{
			break;
		}
	}
	char host[HTTP_HOST_SIZE] = { 0 };
	int port = HTTP_PORT;

	char* host_port = strstr(con->rbuf->cache, HTTP_HOST);
	if (!host_port) {
		LOG_ERROR("failed to fetch host address.\n");
		return CORO_ABORT;
	}
	host_port = strstr(host_port, ":");
	if (!host_port) {
		LOG_ERROR("failed to fetch host address.\n");
		return CORO_ABORT;
	}
	host_port++;
	while(*host_port == ' ') {
		host_port++;
	}
	char* host_port_end = strstr(host_port, CRLF);
	if (!host_port_end) {
		LOG_ERROR("failed to fetch host address CRLF.\n");
		return CORO_ABORT;
	}
	int host_size = host_port_end - host_port;

	strncpy(host, host_port, host_size);
	LOG_DEBUG("host:%s\n", host);

	char* pos = strstr(host, ":");
	if (pos != NULL) {
		*pos = 0;
		port = atoi(pos + 1);
	}

	char* start = strstr(con->rbuf->cache, " ") + 1;
	char* end = strstr(con->rbuf->cache, host);
	end = strstr(end, "/");

	if (m == HTTP_METHORD_GET || m == HTTP_METHORD_POST) {
		int size = con->rbuf->size - (end - start);
		int mv_size = con->rbuf->size - (end - con->rbuf->cache);

		memmove(start, end, mv_size);
		con->rbuf->size = size;

	} else {
		con->rbuf->size = 0;
	}
	LOG_DEBUG("host = %s, port = %d\n", host, port);

	resov_data_t* r = malloc(sizeof(*r));
	memset(r, 0, sizeof(*r));
	strncpy(r->host, host, strlen(host)+1);
	r->data = session;

	tcp_noblock_resolve(r);
	coro_yield_value(coro, CORO_RESUME);

	if (!r->resov) {
		LOG_ERROR("fail to resolve host:%s\n", r->host);
		return CORO_ABORT;
	}

	LOG_INFO("resolve host = %s, addr = %s!\n", host, r->addr);
	LOG_INFO("conneting %s:%d\n", r->addr, port);
	int fd = tcp_noblock_connect(r->addr, port);
	if (fd <= 0) {
		LOG_ERROR("tcp_noblock_connect failed.\n");
		free(r);
		return CORO_ABORT;
	}
	free(r);

	connection_t* rsp_con = &(session->thread->proxy->conns[fd]);

	rsp_con->fd = fd;
	rsp_con->type = CONN_TYPE_CLIENT;
	rsp_con->state = CONN_STATE_CONNECTED;
	rsp_con->sess = session;

	rsp_con->rbuf = malloc(sizeof(buffer_t));
	rsp_con->rbuf->cache = malloc(DEFAULT_BUF_SIZE);
	rsp_con->rbuf->size = 0;
	rsp_con->wbuf = malloc(sizeof(buffer_t));
	rsp_con->wbuf->cache = malloc(DEFAULT_BUF_SIZE);
	rsp_con->wbuf->size = 0;

	LOG_INFO("proxy channel establish....\n");

	epoll_add_fd(session->thread->epfd, EPOLLIN, rsp_con);

	coro_yield_value(coro, CORO_RESUME);
	LOG_INFO("proxy channel establish.\n");

	memcpy(rsp_con->wbuf->cache, con->rbuf->cache, con->rbuf->size);
	rsp_con->wbuf->size = con->rbuf->size;
	con->rbuf->size = 0;
	coro_yield_value(coro, CORO_RESUME);

	while(rsp_con->rbuf->size) {
		memcpy(con->wbuf->cache, rsp_con->rbuf->cache, rsp_con->rbuf->size);
		con->wbuf->size = rsp_con->rbuf->size;
		rsp_con->rbuf->size = 0;
		coro_yield_value(coro, CORO_RESUME);
	}


	return CORO_FINISH;
}

proxy_plugin_t* simple_proxy_plugin_http()
{
	static proxy_plugin_t plugin = {
			.init = __http_init,
			.uninit = __http_uninit,
			.session = __http_session,
			.proxy = __http_proxy
	};

	return &plugin;
}


