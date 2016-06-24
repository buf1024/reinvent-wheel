/*
 * proxy-coro.c
 *
 *  Created on: 2016/6/20
 *      Author: Luo Guochun
 */

#include "lazy.h"

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
			//coro_free(con->coro);
		}
		if(state == CORO_FINISH) {
			LOG_DEBUG("coro process success, connection=%d, coro=0x%x\n", con->fd, con->coro);
			lazy_del_fd(con->pxy->epfd, con);
			con->state = CONN_STATE_CLOSING;
			close(con->fd);
			LOG_DEBUG("connection state = %d, coro=0x%x\n", con->state, con->coro);
			coro_free(con->coro);
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
		con->req->buf->buf = malloc(sizeof(char) * MAX_REQ_SIZE);
		OOM_CHECK(con->req, "malloc(sizeof(buffer_t))");
		con->req->header = dict_create(con);

	}

	con->req->buf->cap = MAX_REQ_SIZE;
	con->req->buf->size = 0;
	dict_empty(con->req->header);
	con->coro = coro_new(lazy_http_req_coro, con, 1024*100);

	LOG_INFO("new http request, coro=0x%x\n", con->coro);

	return resume_coro_demand(con->coro);
}
int lazy_http_req_coro(coro_t* coro)
{
	connection_t* con = (connection_t*)coro_get_data(coro);
	buffer_t* buf = con->req->buf;

	do {
		bool ok = false;
		int rd = tcp_read(con->fd, buf->buf + buf->size, 128, &ok);
		if (rd > 0 && ok) {
			if (buf->size + rd > buf->cap) {
				// TODO
			}
			buf->size += rd;
			char* pos = strstr(buf->buf, "\r\n\r\n");
			if (pos != NULL) {
				buf->buf[buf->size] = 0;
				LOG_DEBUG("RECV(%d) fd(%d): \n%s\n", buf->size, con->fd, buf->buf);
				return CORO_FINISH;
			}
			coro_yield_value(coro, CORO_RESUME);
			continue;
		}
		if (ok && rd == 0) {
			coro_yield_value(coro, CORO_RESUME);
			continue;
		}
		return CORO_ABORT;
	} while (true);

	return CORO_FINISH;
}
