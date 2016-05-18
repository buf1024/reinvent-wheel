/*
 * thinconf.c
 *
 *  Created on: 2016/5/18
 *      Author: Luo Guochun
 */

#include "thinpxy.h"
#include <errno.h>
#include <bits/sockaddr.h>

static char* to_lower(char* v) {
	char* t = v;
	while(*t) {
		if(isupper(*t)) {
			*t = tolower(*t);
		}
	}
	return v;
}

static char* trim(char* line)
{

	char* end = line + strlen(line) - 1;
	char* start = line;

	bool s_find = false, e_find = false;
	while(start < end) {
			if (*start == ' ' || *start == '\r' || *start == '\n') {
				start++;
			} else {
				break;
			}
	}
	while(start < end) {
		if(*end == ' ' || *end == '\r' || *end == '\n') {
			end--;
		}else{
			break;
		}
	}
	*(end + 1) = 0;

	return start;
}

static int parse_kv(const char*line, char*k, char* v)
{
	char* kv = strchr(line, ' ');
	if(kv == NULL) {
		return -1;
	}
	strncpy(k, line, kv - line - 1);
	kv = trim(kv);

	strncpy(v, kv, line + strlen(line) - kv);

	return 0;
}

static bool parse_bool(char* v)
{
	if(strcasecmp(v, "yes") == 0) {
		return true;
	}
	if(strcasecmp(v, "true") == 0) {
		return true;
	}
	if(strcasecmp(v, "1") == 0) {
		return true;
	}

	return false;
}

static int parse_addrinfo(char* v, sa_family_t* af, char* ip, int* port)
{
	char* col = strchr(v, ':');
	if(col == NULL) {
		return -1;
	}
	if(*(col + 1) == 0) {
		return -1;
	}
	if(*v == '[') {
		*af = AF_INET6;
		strncpy(ip, v, col - v - 1);
	} else if(*v == "*") {
		*af = AF_INET;
		strncpy(ip, "0.0.0.0", sizeof("0.0.0.0"));
	}else{
		*af = AF_INET;
		strncpy(ip, v, col - v - 1);
	}

	*port = atoi(col + 1);
	if(*port <= 0) {
		return -1;
	}

	return 0;
}

static int parse_backend(config_t* conf, backend_t* backend, char* v) {
	//backend newwork 127.0.0.1:11001 50;
	char* col = NULL, col_nxt = NULL;
	char v_t[1024] = {0};

	col = strchr(v, ' '); if(col == NULL) return -1;

	col = trim(col); if(*col == 0) return -1;
	col_nxt = strchr(col, ' '); if(col_nxt == NULL) return -1;
	strncpy(v_t, col, col_nxt - col - 1);
	backend->name = strdup(v_t);

	col = trim(col); if(*col == 0) return -1;
	col_nxt = strchr(col, ' '); if(col_nxt == NULL) return -1;
	strncpy(v_t, col, col_nxt - col - 1);
	backend->addr_str = strdup(v_t);
	sa_family_t af;
	char node[1024] = {0};
	int port = 0;
	if(parse_addrinfo(v, &af, node, &port) != 0) {
		conf->parse.error = strdup("work-mode, ip address missing port or port is not valid)");
		return -1;
	}
    struct addrinfo hints = {
        .ai_family = af,
        .ai_socktype = SOCK_STREAM,
        .ai_flags = AI_PASSIVE
    };

    if(getaddrinfo(node, port, &hints, backend->addrs) != 0) {
		conf->parse.error = strdup("work-mode, ip address is not valid");
		return -1;
    }

	col = trim(col); if(*col == 0) return -1;
	backend->weight = atoi(col);

	if(backend->weight <= 0) {
		conf->parse.error = strdup("work-mode, weight must greater than 0");
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
	backend_t backend[64] = {0};

	while(!feof(fp)) {
		l = fgets(line, sizeof(line) - 1, fp);
		conf->parse.line++;

		l = trim(l);
		if(*l == 0) {
			continue;
		}
		if(*l == '}') {
			if(backend_cnt == 0) {
				conf->parse.error = strdup("work-mode missing backend");
				return -1;
			}
			break;
		}
		if(strncasecmp(l, "backend", strlen("backend")) == 0) {
			backend_cnt++;
			if(parse_backend(conf, &backend[backend_cnt], l) != 0) {
				conf->parse.error = strdup("work-mode backend invalid");
				return -1;
			}
		}else if(strcasecmp(l, "idle-close-time") == 0) {

		}else if(strcasecmp(l, "proxy-algorithm") == 0) {

		}else if(strcasecmp(l, "packet-parser-plugin") == 0) {

		}
	}
}

int parse_conf(config_t* conf)
{
	FILE* fp = fopen(conf->conf, "rb");
	if(!fp) {
		printf("open file %s failed, errno %d\n", conf->conf, errno);
		return -1;
	}

	char line[1024] = {0};
	char* l = NULL;

	while(!feof(fp)) {
		l = fgets(line, sizeof(line) - 1, fp);
		conf->parse.line++;

		l = trim(l);

		if(*l == 0) {
			continue;
		}

		if(*l == '#') {
			if(strncmp(l, "#if 0", strlen("#if 0")) == 0) {
				while(!feof(fp)) {
					l = fgets(line, sizeof(line) - 1, fp);
					conf->parse.line++;

					l = trim(l);
					if(strncmp(l, "#endif", strlen("#endif")) == 0) {
						break;
					}
				}
			}
			continue;
		}else{
			char k[1024] = {0}, v[1024] = {0};
			if(parse_kv(l, k, v) != 0) {
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
					conf->parse.error = strdup("log-level is not in (debug info warn error off)");
					fclose(fp);
					return -1;
				}
			}else if(strcasecmp(k, "daemon") == 0) {
				conf->daemon = parse_bool(v);
			}else if(strcasecmp(k, "listen") == 0) {
				sa_family_t af;
				char node[1024] = {0};
				int port = 0;
				if(parse_addrinfo(v, &af, node, &port) != 0) {
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
					conf->parse.error = strdup("ip address is not valid");
					fclose(fp);
					return -1;
			    }
			}else if(strcasecmp(k, "work-mode") == 0) {
				if(*(v + strlen(v)) != '{') {
					conf->parse.error = strdup("work-mode missing '{'");
					fclose(fp);
					return -1;
				}
				l = trim(v);

				char k_m[1024] = {0}, v_m[1024] = {0};
				if(parse_kv(v, k_m, v_m) != 0) {
					conf->parse.error = strdup("work-mode missing 'mode', 'mode' in 'find-connect-new/find-connect-exist/find-connect-packet'");
					fclose(fp);
					return -1;
				}
				char* v_lower = to_lower(k_m);
				if(strstr("find-connect-new find-connect-exist find-connect-packet", v_lower) == NULL) {
					conf->parse.error = strdup("work-mode invalid, 'mode' in 'find-connect-new/find-connect-exist/find-connect-packet'");
					fclose(fp);
					return -1;
				}

				if(parse_mode(conf, fp, v_lower) != 0) {
					conf->parse.error = strdup("work-mode invalid");
					fclose(fp);
					return -1;
				}
			}
		}
	}

	fclose(fp);
	return 0;
}

