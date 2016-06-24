/*
 * proxy-coro.c
 *
 *  Created on: 2016/6/20
 *      Author: Luo Guochun
 */

#include "lazy.h"

static int http_req_reset(http_req_t* req)
{
	req->buf->size = 0;
	req->method = HTTP_METHORD_NONE;
	dict_empty(req->header);

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

	if (!con->req) {
		con->req = malloc(sizeof(http_req_t));
		OOM_CHECK(con->req, "malloc(sizeof(http_req_t))");
		con->req->buf = malloc(sizeof(buffer_t));
		con->req->buf->cache = malloc(sizeof(char) * MAX_REQ_SIZE);
		con->req->buf->cap = MAX_REQ_SIZE;
		OOM_CHECK(con->req, "malloc(sizeof(char) * MAX_REQ_SIZE)");
		con->req->header = dict_create(con);
		OOM_CHECK(con->req, "dict_create(con)");
		con->coro = coro_new(lazy_http_req_coro, con, MAX_CORO_STACK_SIZE);
	}
	coro_reset(con->coro);
	http_req_reset(con->req);

	LOG_INFO("new http request, lazy_spawn_coro coro=0x%x\n", con->coro);

	return resume_coro_demand(con->coro);
}
int lazy_http_req_coro(coro_t* coro)
{
	// proccess read
	if(process_http_req(coro) != CORO_FINISH) {
		return CORO_ABORT;
	}



	return CORO_FINISH;
}

int process_http_req(coro_t* coro)
{
	connection_t* con = (connection_t*)coro_get_data(coro);
	http_req_t* req = con->req;
	buffer_t* buf = req->buf;
	do {
		bool ok = false;
		int rv = tcp_read(con->fd, buf->cache + buf->size, buf->cap, &ok);
		if (rv > 0 && ok) {
			if (buf->size + rv >= buf->cap) {
				int grow = MAX_REQ_GROW_SIZE;
				if(rv > grow) {
					grow = rv;
				}
				buf->cap += grow;
				buf->cache = realloc(buf->cache, buf->cap);
				OOM_CHECK(buf->cache, "realloc(buf->cache, buf->cap)");
			}
			buf->size += rv;

			buf->cache[buf->size] = 0;
			rv = parse_http_req(req);
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

int parse_http_req(http_req_t* req)
{
	if(req->method != HTTP_METHORD_NONE) {
		return 1;
	}

	buffer_t* buf = req->buf;

	if((unsigned)buf->size <= HTTP_END_SIZE) {
		return 0;
	}

	char* pos = strstr(buf->cache, HTTP_END);
	if(!pos) {
		return 0;
	}

	if(strncmp(HTTP_GET, buf->cache, sizeof(HTTP_GET) - 1) == 0) {
		req->method = HTTP_METHORD_GET;
	}else if(strncmp(HTTP_POST, buf->cache, sizeof(HTTP_POST) - 1) == 0) {
		req->method = HTTP_METHORD_POST;
	}else if(strncmp(HTTP_CONNECT, buf->cache, sizeof(HTTP_CONNECT) - 1) == 0) {
		req->method = HTTP_METHORD_CONNECT;
	}else{
		LOG_ERROR("unknown http method, req = \n%s\n", buf->cache);
		return -1;
	}

	char* start = strstr(buf->cache, CRLF);
	if(!start) {
		LOG_ERROR("illegal packet.\n");
		return -1;
	}

	if(parse_req_method(req, buf->cache, start - buf->cache) != 0) {
		LOG_ERROR("parse_req_method failed.\n");
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
			if(parse_req_head(req, head, end - start) != 0) {
				LOG_ERROR("parse_req_head failed.\n");
				return -1;
			}
			break;
		}else{
			char head[pos - start + 1];
			memcpy(head, start, pos - start);
			if(parse_req_head(req, head, pos - start) != 0) {
				LOG_ERROR("parse_req_head failed.\n");
				return -1;
			}
			start = pos + sizeof(CRLF) - 1;
		}
	}

	return 1;
}

int parse_req_method(http_req_t* req, const char* head, int size)
{
#if 0
	const char* pos = strchr(head, ' ');
	if(pos == NULL) {
		return -1;
	}
	pos++;
	while(pos <= head + size) {
		if(isspace(*pos)) {
			pos++;
		}else{
			break;
		}
	}
	if(pos == head + size) {
		return -1;
	}
	const char* end = strchr(pos, ' ');
	if(end == NULL) {
		return -1;
	}
	int* k = malloc(sizeof(*k));
	char* v = malloc(end - pos + 1);

	*k = req->method;
	memcpy(v, pos, end - pos);
	v[end-pos] = 0;

    if(dict_add(req->header, k, sizeof(*k), v, end - pos) != DICT_OK) {
        free(k);free(v);
        LOG_ERROR("dict add failed.\n");
        return -1;
    }

#endif

	char* k = malloc(sizeof(HTTP_METHOD_KEY));
	char* v = malloc(size + 1);

	memcpy(k, HTTP_METHOD_KEY, sizeof(HTTP_METHOD_KEY));
	memcpy(v, head, size);
	v[size] = 0;

	if(dict_add(req->header, k, sizeof(HTTP_METHOD_KEY), v,  size + 1) != DICT_OK) {
		free(k);free(v);
		LOG_ERROR("dict add failed.\n");
		return -1;
	}
	return 0;
}

int parse_req_head(http_req_t* req, const char* head, int size)
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

	if(dict_add(req->header, k, k_s, v, v_s) != DICT_OK) {
		LOG_ERROR("dict add failed.\n");
		free(k);free(v);
		return -1;
	}
	return 0;
}

int check_http_req(http_req_t* req)
{
	dict* d = req->header;
	char* v = dict_fetch_value(d, HTTP_CONTENT_LEN, sizeof(HTTP_CONTENT_LEN));
	if(v != NULL) {
		int rcv = atoi(v);
		if(rcv < req->buf->size) {
			return 0;
		}
	}

	return 1;
}
int foword_http_req(coro_t* coro)
{
	return 0;
}
