/*
 * thinconf.c
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
		col = strstr(v, ']');
		if(col == NULL) return -1;

		strncpy(ip, v + 1, col - v - 1);

		col = strstr(col, ':');
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
	rt_ip = strdup(__trim(ip));

	return proto_v;
}

static int parse_backend(config_t* conf, backend_t* backend, char* v) {
	//backend newwork 127.0.0.1:11001 50;
	char* col = NULL, *col_nxt = NULL;
	char v_t[1024] = {0};

	col = strchr(v, ' '); if(col == NULL) return -1;

	col = trim(col); if(*col == 0) return -1;
	col_nxt = strchr(col, ' '); if(col_nxt == NULL) return -1;
	strncpy(v_t, col, col_nxt - col);
	backend->name = strdup(v_t);

	col = trim(col_nxt); if(*col == 0) return -1;
	col_nxt = strchr(col, ' '); if(col_nxt == NULL) return -1;
	strncpy(v_t, col, col_nxt - col);
	backend->addr_str = strdup(v_t);
	sa_family_t af;
	char node[1024] = {0};
	char port[1024] = {0};
	if(parse_addrinfo(v_t, &af, node, port) != 0) {
		conf->parse.conf = strdup(v);
		conf->parse.error = strdup("work-mode, ip address missing port or port is not valid)");
		return -1;
	}
    struct addrinfo hints = {
        .ai_family = af,
        .ai_socktype = SOCK_STREAM,
        .ai_flags = AI_PASSIVE
    };

    if(getaddrinfo(node, port, &hints, &backend->addrs) != 0) {
		conf->parse.error = strdup("work-mode, ip address is not valid");
		return -1;
    }

    col = trim(col_nxt); if(*col == 0) return -1;
	backend->weight = atoi(col);

	if(backend->weight <= 0) {
		conf->parse.conf = strdup(col);
		conf->parse.error = strdup("work-mode, weight must greater than 0");
		return -1;
	}
	backend->pxy = conf->pxy;
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

static int parse_mode(config_t* conf, FILE* fp, char* mode)
{
	char line[1024] = {0};
	char* l = NULL;

	if(strcasecmp(mode, "find-connect-new") == 0) {
		conf->pxy->mode = WORK_MODE_CONNECT_NEW;
	}

	if(strcasecmp(mode, "find-connect-exist") == 0) {
		conf->pxy->mode = WORK_MODE_CONNECT_EXIST;
	}

	if(strcasecmp(mode, "find-connect-packet") == 0) {
		conf->pxy->mode = WORK_MODE_CONNECT_PACKET;
	}

	int backend_cnt = 0;
	backend_t backend[64];


	bool end = false;
	while(!feof(fp)) {
		l = fgets(line, sizeof(line) - 1, fp);
		conf->parse.line++;

		l = trim(l);
		if(*l == 0) {
			continue;
		}
		if(*l == '}') {
			if(backend_cnt == 0 && conf->pxy->backends == NULL) {
				conf->parse.conf = strdup(mode);
				conf->parse.error = strdup("work-mode missing backend");
				return -1;
			}
			end = true;
			break;
		}
		if(*l == '#') {
			if(skip_comment(conf, l, fp) != 0) {
				return -1;
			}
			continue;
		}
		if(strncasecmp(l, "backend", strlen("backend")) == 0) {
			if(backend_cnt >= sizeof(backend)/sizeof(backend[0])) {
				if(conf->pxy->backends == NULL) {
				    conf->pxy->backends = malloc(backend_cnt * sizeof(backend_t));
				}else{
					conf->pxy->backends = realloc(conf->pxy->backends, (backend_cnt + conf->pxy->backend_cnt)* sizeof(backend_t));
				}
				memcpy(conf->pxy->backends + conf->pxy->backend_cnt, backend, backend_cnt * sizeof(backend_t));

				conf->pxy->backend_cnt += backend_cnt;
			}
			if(parse_backend(conf, &backend[backend_cnt], l) != 0) {
				conf->parse.conf = strdup(l);
				conf->parse.error = strdup("work-mode backend invalid");
				return -1;
			}
			backend_cnt++;
		}else {
			char k[1024] = { 0 }, v[1024] = { 0 };
			if (parse_kv(l, k, v) != 0) {
				conf->parse.conf = strdup(l);
				conf->parse.error = strdup("missing value");
				return -1;
			}

			l = trim(v);

			if (strcasecmp(k, "proxy-algorithm") == 0) {
				if (conf->pxy->pxy_algo) {
					free(conf->pxy->pxy_algo);
					conf->pxy->pxy_algo = strdup(l);
				}

			} else if (strcasecmp(k, "packet-parser-plugin") == 0) {
				conf->pxy->plugin_name = strdup(l);
			}else{
				conf->parse.conf = strdup(k);
				conf->parse.error = strdup("unknown conf option");
				return -1;
			}
		}
	}
	if(!end) {

		conf->parse.conf = strdup(mode);
		conf->parse.error = strdup("mode missing '}'");
		return -1;
	}
	if(backend_cnt > 0) {
		if(conf->pxy->backends == NULL) {
		    conf->pxy->backends = malloc(backend_cnt * sizeof(backend_t));
		}else{
			conf->pxy->backends = realloc(conf->pxy->backends, (conf->pxy->backend_cnt + backend_cnt) * sizeof(backend_t));
		}
		memcpy(conf->pxy->backends + conf->pxy->backend_cnt, backend, backend_cnt * sizeof(backend_t));

		conf->pxy->backend_cnt += backend_cnt;
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

	}
	TSON_READ_STR_MUST(t, "idle-close-time", v, s);
	if(__parse_time(v, &proxy->idle_to) < 0) {
		printf("parse time failed.\n");
		return -1;
	}

	tson_free(t);


	conf->pxy->idle_to = DEF_IDLE_TO;
	conf->pxy->pxy_algo = strdup(DEF_PXY_ALGO);

	while(!feof(fp)) {
		l = fgets(line, sizeof(line) - 1, fp);
		conf->parse.line++;

		l = trim(l);

		if(*l == 0) {
			continue;
		}

		if(*l == '#') {
			if(skip_comment(conf, l, fp) != 0) {
				fclose(fp);
				return -1;
			}
			continue;
		}else{
			char k[1024] = {0}, v[1024] = {0};
			if(parse_kv(l, k, v) != 0) {
				conf->parse.conf = strdup(l);
				conf->parse.error = strdup("missing value");
				fclose(fp);
				return -1;
			}
			if(strcasecmp(k, "log-path-file") == 0) {
				conf->log_file = strdup(v);
			}else if(strcasecmp(k, "log-level") == 0) {
				char* v_lower = to_lower(v);
				if(strstr("debug info warn error off", v_lower) != NULL) {
					conf->log_level = strdup(v_lower);
				}else{
					conf->parse.conf = strdup(v);
					conf->parse.error = strdup("log-level is not in (debug info warn error off)");
					fclose(fp);
					return -1;
				}
			}else if(strcasecmp(k, "daemon") == 0) {
				conf->daemon = parse_bool(v);
			}else if(strcasecmp(k, "listen") == 0) {
				sa_family_t af;
				char node[1024] = {0};
				char port[1024] = {0};
				if(parse_addrinfo(v, &af, node, port) != 0) {
					conf->parse.conf = strdup(v);
					conf->parse.error = strdup("ip address missing port or port is not valid)");
					fclose(fp);
					return -1;
				}
			    struct addrinfo hints = {
			        .ai_family = af,
			        .ai_socktype = SOCK_STREAM,
			        .ai_flags = AI_PASSIVE
			    };

			    if(getaddrinfo(node, port, &hints, &conf->addrs) != 0) {
					conf->parse.conf = strdup(v);
					conf->parse.error = strdup("ip address is not valid/or unreachable");
					fclose(fp);
					return -1;
			    }
			}else if (strcasecmp(k, "idle-close-time") == 0) {
				if(parse_time(v, &conf->pxy->idle_to) != 0 || conf->pxy->idle_to <= 0) {
					conf->parse.conf = strdup(l);
					conf->parse.error = strdup("time format in valid, h, m, s");
					fclose(fp);
					return -1;
				}

			} else if(strcasecmp(k, "work-mode") == 0) {
				if(*(v + strlen(v) - 1) != '{') {
					conf->parse.conf = strdup(k);
					conf->parse.error = strdup("work-mode missing '{'");
					fclose(fp);
					return -1;
				}
				l = trim(v);

				char k_m[1024] = {0}, v_m[1024] = {0};
				if(parse_kv(v, k_m, v_m) != 0) {
					conf->parse.conf = strdup(v);
					conf->parse.error = strdup("work-mode missing 'mode', 'mode' in 'find-connect-new/find-connect-exist/find-connect-packet'");
					fclose(fp);
					return -1;
				}
				char* v_lower = to_lower(k_m);
				if(strstr("find-connect-new find-connect-exist find-connect-packet", v_lower) == NULL) {
					conf->parse.conf = strdup(k_m);
					conf->parse.error = strdup("work-mode invalid, 'mode' in 'find-connect-new/find-connect-exist/find-connect-packet'");
					fclose(fp);
					return -1;
				}

				if(parse_mode(conf, fp, v_lower) != 0) {
					fclose(fp);
					return -1;
				}
			}else{

				conf->parse.conf = strdup(l);
				conf->parse.error = strdup("unknown conf option");
				return -1;
			}
		}
	}

	fclose(fp);
	return 0;
}

