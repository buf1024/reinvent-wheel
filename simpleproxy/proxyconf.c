/*
 * proxyconf.c
 *
 *  Created on: 2016/5/18
 *      Author: Luo Guochun
 */

#include "simpleproxy.h"

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
	proxy->proto_v = parse_addrinfo(v, &proxy->listen, &proxy->port);
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
	if(parse_time(v, &proxy->idle_to) < 0) {
		printf("parse time failed.\n");
		return -1;
	}
	TSON_READ_STR_OPT(t, "work-mode", v, s);

	if(!v) {
		printf("not configure work-mode, use the default plugin.\n");
		v = "dummy";
	}

	const char* plugin_prefix = "simple_proxy_plugin_";
	proxy->plugin_name = calloc(strlen(plugin_prefix) + strlen(v) + 1, 1);
	strcat(proxy->plugin_name, plugin_prefix);
	strcat(proxy->plugin_name, v);

	printf("load plugin %s\n", proxy->plugin_name);
	proxy->plugin = find_symbol(proxy->plugin_name);

	if(!proxy->plugin) {
		printf("load plugin %s failed.\n", proxy->plugin_name);
		return -1;
	}

#if 0
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
#endif
	tson_free(t);


	return 0;
}

