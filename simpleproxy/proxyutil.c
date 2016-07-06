/*
 * proxyutil.c
 *
 *  Created on: 2016/7/5
 *      Author: Luo Guochun
 */
#include "simpleproxy.h"

void *find_symbol(const char *name)
{
    void *symbol = dlsym(RTLD_NEXT, name);
    if (!symbol)
        symbol = dlsym(RTLD_DEFAULT, name);
    return symbol;
}

int epoll_add_fd(int epfd, int evt, connection_t* con)
{
	struct epoll_event event;
	event.events = evt | EPOLLERR | EPOLLHUP;
	event.data.ptr = con;

	if(epoll_ctl(epfd, EPOLL_CTL_ADD, con->fd, &event) != 0) {
		LOG_ERROR("epoll add failed, errno=%d\n", errno);
		return -1;
	}
	return 0;
}


int epoll_mod_fd(int epfd, int evt, connection_t* con)
{
	struct epoll_event event;
	event.events = evt | EPOLLERR | EPOLLHUP;
	event.data.ptr = con;

	if(epoll_ctl(epfd, EPOLL_CTL_MOD, con->fd, &event) != 0) {
		LOG_ERROR("epoll add failed, errno=%d\n", errno);
		return -1;
	}
	return 0;
}

int epoll_del_fd(int epfd, connection_t* con)
{
	struct epoll_event event;

	if(epoll_ctl(epfd, EPOLL_CTL_DEL, con->fd, &event) != 0) {
		LOG_ERROR("epoll del failed, errno=%d\n", errno);
		return -1;
	}
	return 0;
}


char* trim_str(char* line)
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

int parse_addrinfo(char* v, char** rt_ip, int* rt_port)
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
	*rt_ip = strdup(trim_str(ip));

	return proto_v;
}
int parse_time(char* l, int* sec)
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


