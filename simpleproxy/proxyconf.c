/*
 * proxyconf.c
 *
 *  Created on: 2016/5/18
 *      Author: Luo Guochun
 */

#include "simpleproxy.h"

static char* __trim(char* line)
{

	char* end = line + strlen(line) - 1;
	char* start = line;

	while(start <= end) {
			if (*start == ' ' || *start == '\r' || *start == '\n') {
				start++;
			} else {
				break;
			}
	}
	while(start <= end) {
		if(*end == ' ' || *end == '\r' || *end == '\n') {
			end--;
		}else{
			break;
		}
	}
	*(end + 1) = 0;

	return start;
}

static int __parse_addrinfo(char* v, char** rt_ip, int* rt_port)
{
	char ip[128] = {0};
	int proto_v = PROTO_IPV4;

	char* col = strchr(v, ':');
	if(col == NULL) return -1;
	if(*(col + 1) == 0) return -1;

	if(*v == '[') {
		if(*(v+1) == ']') return -1;
		col = strchr(v, ']');
		if(col == NULL) return -1;

		strncpy(ip, v + 1, col - v - 1);

		col = strchr(col, ':');
		if(col == NULL) return -1;

		if(*(col + 1) == 0) return -1;

		proto_v = PROTO_IPV6;

	} else if(*v == '*') {
		strncpy(ip, "0.0.0.0", sizeof("0.0.0.0"));

		proto_v = PROTO_IPV4;
	}else{
		strncpy(ip, v, col - v);
		proto_v = PROTO_IPV4;
	}

	*rt_port = atoi(col + 1);
	if(*rt_port <= 0) {
		return -1;
	}
	*rt_ip = strdup(__trim(ip));

	return proto_v;
}

static int __parse_backend(char* v, proxy_backend_t* backend) {
	//backend newwork 127.0.0.1:11001 50;
	char* col = NULL, *col_nxt = NULL;
	char v_t[1024] = {0};

	col = v;
	col_nxt = strchr(col, ' '); if(col_nxt == NULL) return -1;
	strncpy(v_t, col, col_nxt - col);
	backend->name = strdup(v_t);

	col = __trim(col_nxt); if(*col == 0) return -1;
	col_nxt = strchr(col, ' '); if(col_nxt == NULL) return -1;
	strncpy(v_t, col, col_nxt - col);
	backend->proto_v = __parse_addrinfo(v, &backend->addr, &backend->port);
	if(backend->proto_v < 0) {
		printf("parse ip address failed.\n");
		return -1;
	}

    col = __trim(col_nxt); if(*col == 0) return -1;
	backend->weight = atoi(col);

	if(backend->weight <= 0) {
		return -1;
	}
	return 0;
}

static int __parse_time(char* l, int* sec)
{
	char* s = l, *s_nxt = NULL;
	char v[1024] = { 0 };
	*sec = 0;

	s_nxt = strchr(s, 'h');
	if (s_nxt != NULL) {
		if (s_nxt != s) {
			strncpy(v, s, s_nxt - s);
			s_nxt = strrchr(v, ' ');
			if (s_nxt != NULL) {
				s_nxt = v;
			}
		} else {
			return -1;
		}
		*sec += 3600 * atoi(s_nxt);
	}
	s_nxt = strchr(s, 'm');
	if (s_nxt != NULL) {
		if (s_nxt != s) {
			strncpy(v, s, s_nxt - s);
			s_nxt = strrchr(v, ' ');
			if (s_nxt != NULL) {
				s_nxt = v;
			}
			*sec += 60 * atoi(v);
		}
	}
	s_nxt = strchr(s, 's');
	if (s_nxt != NULL) {
		strncpy(v, s, s_nxt - s);
		s_nxt = strrchr(v, ' ');
		if (s_nxt != NULL) {
			s_nxt = v;
		}
		*sec += atoi(v);
	} else {
		return -1;
	}

	return 0;
}

int parse_conf(simpleproxy_t* proxy)
{
	tson_t* t = tson_parse_path(proxy->conf);
	if(!t) {
		printf("parse tson failed, file=%s\n", proxy->conf);
		return -1;
	}

	tson_t* s = NULL;
	char* v = NULL;

	TSON_READ_STR_MUST(t, "log-path", v, s);
	proxy->log_path = strdup(v);
	TSON_READ_STR_MUST(t, "log-level", v, s);
	proxy->log_level = log_get_level(v);
	TSON_READ_INT_MUST(t, "log-buf-size", proxy->log_buf_size, s);

	TSON_READ_STR_MUST(t, "listen", v, s);
	proxy->proto_v = __parse_addrinfo(v, &proxy->listen, &proxy->port);
	if(proxy->proto_v < 0) {
		printf("parse ip address failed.\n");
		return -1;
	}
	TSON_READ_INT_MUST(t, "thread", proxy->thread_num, s);
	if(proxy->thread_num <= 0) {
		proxy->thread_num = get_cpu_count();
		printf("conf thread number not valid, set to cpu number(%d)\n", proxy->thread_num);
	}
	TSON_READ_STR_MUST(t, "idle-close-time", v, s);
	if(__parse_time(v, &proxy->idle_to) < 0) {
		printf("parse time failed.\n");
		return -1;
	}
	TSON_READ_STR_MUST(t, "work-mode", v, s);
	if(strcmp(v, "find-connect-new") == 0) {
		proxy->mode = WORK_MODE_CONNECT_NEW;
	}else if(strcmp(v, "find-connect-packet")) {
		proxy->mode = WORK_MODE_CONNECT_PACKET;
	}else {
		printf("work mode not correct.\n");
		return -1;
	}
	if(s == NULL) {
		printf("sub tson is null, configure syntax not right.\n");
		return -1;
	}

	tson_arr_t* arr = NULL;
	proxy->backend_cnt = tson_get_arr(s, "backend", &arr);
	if(proxy->backend_cnt <= 0) {
		printf("backend == 0\n");
		return -1;
	}
	proxy->backends = (proxy_backend_t*)malloc(sizeof(proxy_backend_t) * proxy->backend_cnt);
	int i=0;
	for(;i<proxy->backend_cnt; i++) {
		if(__parse_backend(arr[i].value, &proxy->backends[i]) < 0) {
			printf("parse backend failed.\n");
			return -1;
		}
	}

	tson_t* ss = NULL;
	TSON_READ_STR_MUST(s, "proxy-algorithm", proxy->algo, ss);
	TSON_READ_STR_OPT(s, "proxy-plugin", proxy->plugin_name, ss);

	tson_free(t);


	return 0;
}

