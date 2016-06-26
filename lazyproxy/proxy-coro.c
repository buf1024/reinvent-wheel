/*
 * proxy-coro.c
 *
 *  Created on: 2016/6/20
 *      Author: Luo Guochun
 */

#include "lazy.h"

static int http_req_reset(connection_t* con)
{
    coro_t* coro = con->coro;
	coro_reset(coro);

	con->recv_buf->size = 0;
	con->send_buf->size = 0;

	http_proxy_t* pxy = con->pxy;

	pxy->rsp_coro = NULL;
	pxy->req_con = con;
	pxy->req_type = HTTP_METHORD_NONE;
	dict_empty(pxy->req_head);
	pxy->rsp_con = NULL;

	return 0;
}

static int http_rsp_reset(connection_t* con)
{
    coro_t* coro = con->coro;
    coro_reset(coro);

    con->recv_buf->size = 0;
    con->send_buf->size = 0;

    http_proxy_t* pxy = con->pxy;

    pxy->rsp_coro = coro;
    pxy->rsp_con = con;
    dict_empty(pxy->req_head);

    return 0;
}

static int resolve_reset(connection_t* con)
{
    coro_t* coro = con->coro;
    coro_reset(coro);

    con->recv_buf = NULL;
    con->send_buf = NULL;
    con->pxy = NULL;

    return 0;
}

static int free_http_proxy(http_proxy_t* pxy)
{
    if(pxy->req_con) {
        lazy_del_fd(pxy->req_con->lazy->epfd, pxy->req_con);
        close(pxy->req_con->fd);
    }

    if(pxy->rsp_con) {
        lazy_del_fd(pxy->rsp_con->lazy->epfd, pxy->rsp_con);
        close(pxy->rsp_con->fd);
    }

    return 0;
}


int resume_req_coro(coro_t* coro)
{
	if(coro == NULL) {
		return CORO_FINISH;
	}
	int state = coro_resume(coro);
	if(state == CORO_ABORT || state == CORO_FINISH) {
		connection_t* con = (connection_t*)coro_get_data(coro);
		if(state == CORO_ABORT) {
			LOG_ERROR("coro process failed, connection=%d\n", con->fd);
			lazy_del_fd(con->lazy->epfd, con);
			con->state = CONN_STATE_BROKEN;
			close(con->fd);
		}
		if(state == CORO_FINISH) {
			LOG_DEBUG("coro process success, connection=%d\n", con->fd);
			con->state = CONN_STATE_CLOSING;
		}
	}

	return state;
}

int resume_rsp_coro(coro_t* coro)
{
    if(coro == NULL) {
        return CORO_FINISH;
    }
    int state = coro_resume(coro);
    if(state == CORO_ABORT || state == CORO_FINISH) {
        connection_t* con = (connection_t*)coro_get_data(coro);
        if(state == CORO_ABORT) {
            LOG_ERROR("coro process failed, connection=%d\n", con->fd);
            lazy_del_fd(con->lazy->epfd, con);
            con->state = CONN_STATE_BROKEN;
            close(con->fd);
        }
        if(state == CORO_FINISH) {
            LOG_DEBUG("coro process success, connection=%d\n", con->fd);
            con->state = CONN_STATE_CLOSING;
        }
    }

    return state;
}

int resume_resolve_coro(coro_t* coro)
{
    if(coro == NULL) {
        return CORO_FINISH;
    }
    connection_t* con = (connection_t*)coro_get_data(coro);
    int state = coro_resume(coro);
    if(state == CORO_ABORT) {
        http_proxy_t* pxy = con->pxy;
        if(pxy->req_con) {
            lazy_del_fd(pxy->req_con->lazy->epfd, pxy->req_con);
            close(pxy->req_con->fd);
            pxy->req_con->state = CONN_STATE_CLOSED;
        }
        if(pxy->rsp_con) {
            lazy_del_fd(pxy->rsp_con->lazy->epfd, pxy->req_con);
            close(pxy->rsp_con->fd);
            pxy->rsp_con->state = CONN_STATE_CLOSED;

        }
    }

    resolve_reset(con);

    return CORO_FINISH;
}


int lazy_spawn_http_req_coro(connection_t* con)
{

    if(!con->coro) {
        con->coro = coro_new(lazy_http_req_coro, con, MAX_CORO_STACK_SIZE);
        OOM_CHECK(con->coro, "coro_new(lazy_http_req_coro, con, MAX_CORO_STACK_SIZE)");
    }

    if (!con->recv_buf) {
        con->recv_buf = malloc(sizeof(*con->recv_buf));
        OOM_CHECK(con->recv_buf, "malloc(sizeof(*con->recv_buf)");
    }
    if (!con->send_buf) {
        con->send_buf = malloc(sizeof(*con->send_buf));
        OOM_CHECK(con->send_buf, "malloc(sizeof(*con->send_buf)");
    }
    if (!con->pxy) {
        con->pxy = malloc(sizeof(*con->pxy));
        memset(con->pxy, 0, sizeof(*con->pxy));

        con->pxy->req_head = dict_create(con);
        OOM_CHECK(con->pxy->req_head, "dict_create(con)");

    }


    http_req_reset(con);

    return 0;
}

int lazy_spawn_http_rsp_coro(connection_t* con)
{
    if(!con->coro) {
        con->coro = coro_new(lazy_http_rsp_coro, con, MAX_CORO_STACK_SIZE);
        OOM_CHECK(con->coro, "coro_new(lazy_http_req_coro, con, MAX_CORO_STACK_SIZE)");
    }

    if (!con->recv_buf) {
        con->recv_buf = malloc(sizeof(*con->recv_buf));
        OOM_CHECK(con->recv_buf, "malloc(sizeof(*con->recv_buf)");
    }
    if (!con->send_buf) {
        con->send_buf = malloc(sizeof(*con->send_buf));
        OOM_CHECK(con->send_buf, "malloc(sizeof(*con->send_buf)");
    }

    http_rsp_reset(con);

    return 0;
}


int lazy_http_req_coro(coro_t* coro)
{
    connection_t* con = (connection_t*)coro_get_data(coro);
    http_proxy_t* p = con->pxy;

    buffer_t* recv_buf = p->req_con->recv_buf;
    buffer_t* send_buf = p->req_con->send_buf;

    do {
        bool ok = false;
        bool rdblock = false;
        bool wrblock = false;
        int rv = tcp_read(p->req_con->fd, recv_buf->cache + recv_buf->size, sizeof(recv_buf->cache) - recv_buf->size, &ok);
        if(ok) {
            if (rv > 0) {
                recv_buf->size += rv;

                if (p->rsp_con == NULL ) {
                    rv = parse_http_req(p, recv_buf->cache, recv_buf->size);
                    if (rv < 0) {
                        LOG_ERROR("parse_http_req failed.\n");
                        return CORO_ABORT;
                    }
                    if (rv == 0) {
                        coro_yield_value(coro, CORO_RESUME);
                        continue;
                    }

                    LOG_DEBUG("req header done\n\n");

                    dict_iterator* it = dict_get_safe_iterator(p->req_head);
                    dict_entry* de = NULL;
                    while ((de = dict_next(it)) != NULL ) {
                        char* k = (char*) de->key;
                        char* v = (char*) de->val;

                        LOG_DEBUG("%s: %s\n", k, v);
                    }
                    dict_release_iterator(it);

                    if (resolve_proxy_tunnel(p) == CORO_ABORT) {
                        LOG_ERROR("resolve_proxy_tunnel failed.\n");
                        return CORO_ABORT;
                    }

                    if (p->req_type == HTTP_METHORD_GET) {
                        char* host_port = (char*) dict_fetch_value(p->req_head, HTTP_HOST, sizeof(HTTP_HOST));
                        char* start = strstr(recv_buf->cache, " ") + 1;
                        char* end = strstr(recv_buf->cache, host_port) + strlen(host_port);

                        int size = recv_buf->size - (end - start);
                        int mv_size = recv_buf->size - (end - recv_buf->cache);

                        memmove(start, end, mv_size);
                        recv_buf->size = size;
                    }else{
                        recv_buf->size = 0;
                    }

                } else {
                    buffer_t* rsp_send_buf = p->rsp_con->send_buf;
                    int free_size = sizeof(rsp_send_buf->cache) - rsp_send_buf->size;
                    if(recv_buf->size <= free_size) {
                        free_size = recv_buf->size;
                    }
                    memcpy(rsp_send_buf->cache + rsp_send_buf->size, recv_buf->cache, free_size);
                    rsp_send_buf->size += free_size;
                    recv_buf->size -= free_size;
                    if(recv_buf->size > 0) {
                        memmove(recv_buf->cache, recv_buf->cache + free_size, recv_buf->size);
                    }
                }

            }else{
                rdblock = true;
            }
        } else {
            free_http_proxy(p);
            return CORO_ABORT;
        }

        if(send_buf->size > 0) {
            rv = tcp_write(p->req_con->fd, send_buf->cache, send_buf->size, &ok);
            if(ok) {
                if(rv > 0) {
                    send_buf->size -= rv;

                    if(send_buf->size > 0) {
                        memmove(send_buf->cache, send_buf->cache + rv, send_buf->size);
                    }
                }else{
                    wrblock = true;
                    return CORO_FINISH;
                }
            }else{
                free_http_proxy(p);
                return CORO_ABORT;
            }
        }

        if(rdblock || wrblock) {
            coro_yield_value(coro, CORO_RESUME);
        }
    } while (true);

    return CORO_FINISH;
}
int lazy_http_rsp_coro(coro_t* coro)
{
    connection_t* rsp_con = (connection_t*)coro_get_data(coro);
    connection_t* req_con = rsp_con->pxy->req_con;

    buffer_t* rsp_send_buf = rsp_con->send_buf;
    buffer_t* rsp_recv_buf = rsp_con->recv_buf;

    buffer_t* req_send_buf = req_con->send_buf;
    buffer_t* req_recv_buf = req_con->recv_buf;

    int rv = 0;

    do {
        bool ok = false;
        bool rdblock = false;
        bool wrblock = false;
        if(rsp_send_buf->size > 0) {
            rv = tcp_write(rsp_con->fd, rsp_send_buf->cache, rsp_send_buf->size, &ok);
            if(ok) {
                if(rv > 0) {
                    rsp_send_buf->size -= rv;

                    if(rsp_send_buf->size > 0) {
                        memmove(rsp_send_buf->cache, rsp_send_buf->cache + rv, rsp_send_buf->size);
                    }
                }else{
                    rdblock = true;
                }
            }else{
                free_http_proxy(rsp_con->pxy);
                return CORO_ABORT;
            }
        }
        rv = tcp_read(rsp_con->fd, rsp_recv_buf->cache + rsp_recv_buf->size, sizeof(rsp_recv_buf->cache) - rsp_recv_buf->size, &ok);
        if (ok) {
            if (rv > 0) {

                rsp_recv_buf->size += rv;
                int free_size = sizeof(req_send_buf->cache) - req_send_buf->size;
                if (rsp_recv_buf->size <= free_size) {
                    free_size = req_send_buf->size;
                }

                memcpy(req_send_buf->cache + req_send_buf->size, rsp_recv_buf->cache, free_size);
                req_send_buf->size += free_size;
                rsp_recv_buf->size -= free_size;

                if(rsp_recv_buf->size > 0) {
                    memmove(rsp_recv_buf->cache, rsp_recv_buf->cache + free_size, rsp_send_buf->size);
                }
            }else{

                wrblock = true;
            }
        }

        if(rdblock || wrblock) {
            coro_yield_value(coro, CORO_RESUME);
        }

    }while(true);

    return CORO_FINISH;
}
int lazy_http_resolve_coro(coro_t* coro)
{
    resov_data_t* d = tcp_noblock_resolve_result();
    if(!d) {
        return CORO_ABORT;
    }
    if(!d->resov) {
        LOG_ERROR("fail to resolve host:%s\n", d->host);
        return CORO_ABORT;
    }

    http_proxy_t* pxy = (http_proxy_t*)d->data;

    int fd = tcp_noblock_connect(d->addr, pxy->req_port);
    if(fd <=0) {
        LOG_ERROR("tcp_noblock_connect failed.\n");
        free(d);
        return CORO_ABORT;
    }
    free(d);


    lazy_proxy_t* lazy = pxy->req_con->lazy;
    if(fd >= lazy->conn_size) {
        int grow = MAX_CONCURRENT_CONN_GROW;

        lazy->conn = realloc(lazy->conn, sizeof(connection_t) * (lazy->conn_size + grow));
        OOM_CHECK(lazy->conn, "realloc(lazy->conn, sizeof(connection_t) * (lazy->conn_size + grow))");

        lazy->events = realloc(lazy->events, sizeof(struct epoll_event) * (lazy->conn_size + grow));
        OOM_CHECK(lazy->events, "realloc(lazy->events, sizeof(struct epoll_event) * (lazy->conn_size + grow))");

        memset(lazy->conn + lazy->conn_size, 0, sizeof(connection_t) * grow);
        memset(lazy->events + lazy->conn_size, 0, sizeof(struct epoll_event) * grow);

        lazy->conn_size += grow;
    }

    connection_t* rsp_con = &lazy->conn[fd];

    rsp_con->lazy = lazy;
    rsp_con->fd = fd;
    rsp_con->type = CONN_TYPE_CLIENT;
    if(errno == EINPROGRESS) {
        rsp_con->state = CONN_STATE_CONNECTING;
    }else{
        rsp_con->state = CONN_STATE_CONNECTED;
    }
    rsp_con->pxy = pxy;

    if(rsp_con->state == CONN_STATE_CONNECTED) {
        lazy_spawn_http_rsp_coro(rsp_con);
    }

    lazy_add_fd(lazy->epfd, EPOLLIN|EPOLLOUT, rsp_con);


    lazy_spawn_http_rsp_coro(rsp_con);

    return CORO_FINISH;

}

int parse_http_req(http_proxy_t* pxy, const char* req, int size)
{
	if(pxy->req_type != HTTP_METHORD_NONE) {
		return 1;
	}

	if((unsigned)size <= HTTP_END_SIZE) {
		return 0;
	}

	char* pos = strstr(req, HTTP_END);
	if(!pos) {
		return 0;
	}

	if(strncmp(HTTP_GET, req, sizeof(HTTP_GET) - 1) == 0) {
		pxy->req_type = HTTP_METHORD_GET;
	}else if(strncmp(HTTP_POST, req, sizeof(HTTP_POST) - 1) == 0) {
	    pxy->req_type = HTTP_METHORD_POST;
	}else if(strncmp(HTTP_CONNECT, req, sizeof(HTTP_CONNECT) - 1) == 0) {
	    pxy->req_type = HTTP_METHORD_CONNECT;
	}else{
		LOG_ERROR("unknown http method\n");
		return -1;
	}

	char* start = strstr(req, CRLF);
	if(!start) {
		LOG_ERROR("illegal packet.\n");
		return -1;
	}

	start += (sizeof(CRLF) - 1);
	char* end = pos - 1;

	while(start < end) {
		pos = strstr(start, CRLF);
		if(!pos) {
			char head[end - start + 1];
			memcpy(head, start, end - start);
			head[end - start] = 0;
			if(parse_req_head(pxy, head, end - start) != 0) {
				LOG_ERROR("parse_req_head failed.\n");
				return -1;
			}
			break;
		}else{
			char head[pos - start + 1];
			memcpy(head, start, pos - start);
			if(parse_req_head(pxy, head, pos - start) != 0) {
				LOG_ERROR("parse_req_head failed.\n");
				return -1;
			}
			start = pos + sizeof(CRLF) - 1;
		}
	}

	return 1;
}

int parse_req_head(http_proxy_t* pxy, const char* head, int size)
{
	char* k = NULL;
	int k_s = 0;
	char* v = NULL;
	int v_s = 0;

	const char* pos = strchr(head, ':');
	if(!pos) {
		return -1;
	}
	const char* end = pos - 1;
	while(end >= head) {
		if(isspace(*end)) {
			end--;
		}else{
			k_s = end - head + 2;
			k = malloc(k_s);
			memcpy(k, head, k_s);
			k[k_s - 1] = 0;
			break;
		}
	}
	if(!k) {
		LOG_ERROR("invalid head\n", head);
		return -1;
	}

	end = pos + 1;
	while(end <= head + size) {
		if(isspace(*end)) {
			end++;
		}else{
			v_s = head + size - end + 1;
			v = malloc(v_s);
			memcpy(v, end, v_s);
			v[v_s - 1] = 0;
			break;
		}
	}

	if(!v) {
		LOG_ERROR("invalid value\n", head);
		return -1;
	}

	if(dict_add(pxy->req_head, k, k_s, v, v_s) != DICT_OK) {
		LOG_ERROR("dict add failed.\n");
		free(k);free(v);
		return -1;
	}
	return 0;
}

int resolve_proxy_tunnel(http_proxy_t* pxy)
{

    if(pxy->rsp_con != NULL) {
        return CORO_FINISH;
    }

    LOG_INFO("try establish http proxy tunnel\n");

    dict* d = pxy->req_head;
    char* host_port = (char*)dict_fetch_value(d, HTTP_HOST, sizeof(HTTP_HOST));
    if(!host_port) {
        LOG_ERROR("host header not found.\n");
        return CORO_ABORT;
    }

    char host[strlen(host_port) + 1];
    int port = 0;

    const char* pos = strstr(host_port, ":");
    if(pos == NULL) {
        strcpy(host, host_port);
        if(pxy->req_type == HTTP_METHORD_CONNECT) {
            port = HTTP_SSL_PORT;
        }else{
            port = HTTP_PORT;
        }
    }else {
        strncpy(host, host_port, pos - host_port);
        host[pos - host_port] = 0;

        port = atoi(pos + 1);
    }

    LOG_DEBUG("host = %s, port = %d\n", host, port);

    resov_data_t* r = malloc(sizeof(*r));
    memset(r, 0, sizeof(*r));
    strncpy(r->host, host, MIN(sizeof(r->host)-1, strlen(host)));
    r->data = pxy;

    pxy->req_port = port;

    tcp_noblock_resolve(r);

	return CORO_FINISH;
}
