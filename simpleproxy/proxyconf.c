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
	tson_get_string(t, "log-path", &proxy->log_path);
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
	plugin_create_fun_t fun = find_symbol(proxy->plugin_name);

	if(!fun) {
		printf("load plugin %s failed.\n", proxy->plugin_name);
		return -1;
	}
	proxy->plugin = fun();
	if(!proxy->plugin) {
		printf("create plugin %s failed.\n", proxy->plugin_name);
		return -1;
	}

	tson_free(t);


	return 0;
}

