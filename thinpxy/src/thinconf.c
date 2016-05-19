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
	while(*t++) {
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

static int skip_comment(config_t* conf, char* l, FILE* fp)
{
	char line[1024] = {0};
	if(*l == '#') {
		if(strncmp(l, "#if 0", strlen("#if 0")) == 0) {
			bool end = false;
			while(!feof(fp)) {
				l = fgets(line, sizeof(line) - 1, fp);
				conf->parse.line++;

				l = trim(l);
				if(strncmp(l, "#endif", strlen("#endif")) == 0) {
					end = true;
					break;
				}
			}
			if(!end) {
				conf->parse.conf = strdup("#if 0");
				conf->parse.error = strdup("missing #endif");
				return -1;
			}
		}
	}
	return 0;
}
static int parse_kv(const char*line, char*k, char* v)
{
	char* kv = strchr(line, ' ');
	if(kv == NULL) {
		return -1;
	}
	strncpy(k, line, kv - line);
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

static int parse_addrinfo(char* v, sa_family_t* af, char* ip, char* port)
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
		strncpy(ip, v, col - v);
	} else if(*v == '*') {
		*af = AF_INET;
		strncpy(ip, "0.0.0.0", sizeof("0.0.0.0"));
	}else{
		*af = AF_INET;
		strncpy(ip, v, col - v);
	}

	int iport = atoi(col + 1);
	if(iport <= 0) {
		return -1;
	}
	strcpy(port, col + 1);

	char* t_ip = trim(ip);
	char* t_port = trim(port);
	if(t_ip != ip) {
		memmove(ip, t_ip, strlen(t_ip));
		ip[strlen(t_ip) - (t_ip - ip)] = 0;
	}
	if(t_port != port) {
		memmove(port, t_port, strlen(t_port));
		port[strlen(port) - (t_port - port)] = 0;
	}

	return 0;
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

static int parse_time(char* l, int* sec)
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

int parse_conf(config_t* conf)
{
	FILE* fp = fopen(conf->conf, "rb");
	if(!fp) {
		printf("open file %s failed, errno %d\n", conf->conf, errno);
		return -1;
	}

	char line[1024] = {0};
	char* l = NULL;


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

