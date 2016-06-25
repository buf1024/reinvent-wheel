/*
 * proxy-coro.c
 *
 *  Created on: 2016/6/20
 *      Author: Luo Guochun
 */

#include "lazy.h"

static int http_req_reset(http_proxy_t* pxy)
{
	coro_reset(pxy->coro);
	pxy->req_con = NULL;
	pxy->req_type = HTTP_METHORD_NONE;
	dict_empty(pxy->req_head);
	pxy->rsp_con = NULL;

	return 0;
}


int resume_coro_demand(coro_t* coro)
{
	if(coro == NULL) {
		return CORO_FINISH;
	}
	int state = coro_resume(coro);
	if(state == CORO_ABORT || state == CORO_FINISH) {
		connection_t* con = (connection_t*)coro_get_data(coro);
		if(state == CORO_ABORT) {
			LOG_ERROR("coro process failed, connection=%d\n", con->fd);
			lazy_del_fd(con->pxy->epfd, con);
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



int lazy_spawn_coro(connection_t* con)
{
	if (!con->pxy) {
	    con->pxy = malloc(sizeof(*con->pxy));
	    OOM_CHECK(con->pxy, "malloc(sizeof(*con->pxy)");
	    memset(con->pxy, 0, sizeof(*con->pxy));

	    con->pxy->req_head = dict_create(con);
		OOM_CHECK(con->pxy->req_head, "dict_create(con)");

		con->pxy->coro = coro_new(lazy_http_req_coro, con, MAX_CORO_STACK_SIZE);
		OOM_CHECK(con->pxy->coro, "coro_new(lazy_http_req_coro, coro, MAX_CORO_STACK_SIZE)");

	}


	http_req_reset(con->pxy);

	con->pxy->req_con = con;

	LOG_INFO("new http request, lazy_spawn_coro coro=0x%x\n", con->pxy->coro);

	return resume_coro_demand(con->pxy->coro);
}
int lazy_http_req_coro(coro_t* coro)
{
	// proccess read
	if(process_http_req(coro) != CORO_FINISH) {
		return CORO_ABORT;
	}

    if(foword_http_req(coro) != CORO_FINISH) {
        return CORO_ABORT;
    }


	return CORO_FINISH;
}

int process_http_req(coro_t* coro)
{
    connection_t* con = (connection_t*)coro_get_data(coro);
    http_proxy_t* p = con->pxy;

	buffer_t buf;
	buf.size = 0;

	do {
		bool ok = false;
		int rv = tcp_read(p->req_con->fd, buf->cache + buf->size, sizeof(buf.cache) - buf.size, &ok);
		if (rv > 0 && ok) {
			buf->size += rv;
			rv = parse_http_req(p, buf.cache, buf.size);
			if(rv < 0) {
				LOG_ERROR("parse_http_req failed.\n");
				return CORO_ABORT;
			}
			if(rv == 0) {
				coro_yield_value(coro, CORO_RESUME);
				continue;
			}


			LOG_DEBUG("req done\n%s\n", buf->cache);
			dict_iterator* it = dict_get_safe_iterator(p->req_head);
			dict_entry* de = NULL;
			while((de = dict_next(it)) != NULL) {
			    char* k = (char*)de->key;
			    char* v = (char*)de->val;

			    LOG_DEBUG("%s: %s\n", k, v);
			}
			dict_release_iterator(it);


			break; // READ DONE

		}
		if (ok && rv == 0) {
			coro_yield_value(coro, CORO_RESUME);
			continue;
		}
		return CORO_ABORT;
	} while (true);

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

int foword_http_req(coro_t* coro)
{
    connection_t* con = (connection_t*)coro_get_data(coro);
    http_proxy_t* p = con->http_pxy;

    http_req_t* req = p->req;
    http_req_t* rsp = p->rsp;

    char* host_port = (char*)dict_fetch_value(req->header, HTTP_HOST, sizeof(HTTP_HOST));
    if(!host_port) {
        LOG_ERROR("host header not found.\n");
        return CORO_ABORT;
    }

    char host[strlen(host_port) + 1];
    int port = 0;

    const char* pos = strstr(host_port, ":");
    if(pos == NULL) {
        strcpy(host, host_port);
        if(req->method == HTTP_METHORD_CONNECT) {
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

    resov_data_t* d = malloc(sizeof(*d));
    memset(d, 0, sizeof(*d));
    strncpy(d->host, host, MIN(sizeof(d->host)-1, strlen(host)));
    d->data = coro;

    int fd = tcp_noblock_resolve(d);
    if(fd < 0) {
        LOG_ERROR("tcp_noblock_resolve failed.\n");
        return CORO_ABORT;
    }

    lazy_proxy_t* pxy = con->pxy;
    connection_t* rsvcon = &pxy->conn[fd];
    rsvcon->fd = fd;
    rsvcon->type = CONN_TYPE_CLIENT;
    rsvcon->state = CONN_STATE_RESOLVING;
    rsvcon->pxy = pxy;
    rsvcon->http_pxy = p;

    lazy_add_fd(con->pxy->epfd, EPOLLIN, rsvcon);

    LOG_DEBUG("resolv fd = %d\n", fd);

    coro_yield_value(p->coro, CORO_RESUME);

    if(d->resov) {
        LOG_INFO("RESOLVE SUCESS: host(%s) -> %s\n", host, d->addr);
    }else{
        LOG_INFO("RESOLVE FAIL: host(%s)\n", host);

    }


    fd = tcp_noblock_connect(d->addr, port);
    free(d);
    if(fd < 0) {
        LOG_ERROR("tcp_noblock_connect failed.\n");
        return CORO_ABORT;
    }

    connection_t* dstcon = &pxy->conn[fd];
    dstcon->fd = fd;
    dstcon->type = CONN_TYPE_CLIENT;
    if(errno == EINPROGRESS) {
        dstcon->state = CONN_STATE_CONNECTING;
    }else{
        dstcon->state = CONN_STATE_CONNECTED;
    }
    dstcon->pxy = pxy;
    dstcon->http_pxy = p;
    rsp->con = dstcon;

    lazy_add_fd(con->pxy->epfd, EPOLLIN, rsvcon);

    LOG_DEBUG("connect fd = %d\n", fd);

    if(dstcon->state == CONN_STATE_CONNECTING) {
        coro_yield_value(p->coro, CORO_RESUME);
    }

    buffer_t* rsp_buf = rsp->buf;

    do {
        bool ok = false;
        int rv = tcp_read(dstcon->fd, rsp_buf->cache, rsp_buf->cap, &ok);
        if (rv > 0 && ok) {
            char* pos = strstr(rsp_buf->cache, HTTP_END);
            if(!pos) {
                return 0;
            }
            if(rv < 0) {
                LOG_ERROR("parse_http_req failed.\n");
                return CORO_ABORT;
            }
            if(rv == 0) {
                coro_yield_value(coro, CORO_RESUME);
                continue;
            }

            rv = check_http_req(req);
            if(rv < 0) {
                LOG_ERROR("check_http_req failed.\n");
                return CORO_ABORT;
            }
            if(rv == 0) {
                coro_yield_value(coro, CORO_RESUME);
                continue;
            }

            LOG_DEBUG("req done\n%s\n", buf->cache);
            dict_iterator* it = dict_get_safe_iterator(req->header);
            dict_entry* de = NULL;
            while((de = dict_next(it)) != NULL) {
                char* k = (char*)de->key;
                char* v = (char*)de->val;

                LOG_DEBUG("%s: %s\n", k, v);
            }
            dict_release_iterator(it);


            break; // READ DONE

        }
        if (ok && rv == 0) {
            coro_yield_value(coro, CORO_RESUME);
            continue;
        }
        return CORO_ABORT;
    } while (true);



	return CORO_FINISH;
}
