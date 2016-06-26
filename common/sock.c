/*
 * sock.c
 *
 *  Created on: 2016/6/7
 *      Author: Luo Guochun
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "sock.h"


#ifndef RESOLVE_THREAD_COUNT
static const int __THREAD_COUNT = 1;
#else
static const int __THREAD_COUNT = RESOLVE_THREAD_COUNT;
#endif

#ifndef RESOLVE_SLEEP_TIME
static const int __SLEEP_TIME = 3;
#else
static const int __SLEEP_TIME = RESOLVE_SLEEP_TIME;
#endif

#define RESOLVE_LOCK(l)   pthread_mutex_lock(l)
#define RESOLVE_UNLOCK(l) pthread_mutex_unlock(l)

typedef struct resov_thread_info_s
{
    pthread_mutex_t locker_req;
    pthread_mutex_t locker_rst;
    int fd_req[2];
    int fd_rst[2];

    int count;
    pthread_t* thread;
}resov_thread_info_t;

static resov_thread_info_t* __resov = NULL;

static int __put_resolve_data(resov_thread_info_t* t, resov_data_t* d)
{
    intptr_t ptr = (intptr_t)d;

    int rv = write(t->fd_req[1], &ptr, sizeof(void*));
    if(rv != sizeof(void*)) {
        return -1;
    }

    return 0;
}
static resov_data_t* __get_resolve_data(resov_thread_info_t* t)
{
	resov_data_t* d = NULL;
    intptr_t ptr = 0;

    RESOLVE_LOCK(&t->locker_req);
    int rv = read(t->fd_req[0], &ptr, sizeof(void*));
    if(rv > 0) {
        d = (resov_data_t*)ptr;
    }
    RESOLVE_UNLOCK(&t->locker_req);

    return d;
}
static int __put_result_data(resov_thread_info_t* t, resov_data_t* d)
{
	intptr_t ptr = (intptr_t)d;
	RESOLVE_LOCK(&t->locker_rst);
	int rv = write(t->fd_rst[1], &ptr, sizeof(void*));
	RESOLVE_UNLOCK(&t->locker_rst);

	return rv;
}

static void* __resov_thread(void* d)
{
    resov_thread_info_t* t = (resov_thread_info_t*)d;

    while (true) {
        fd_set rd;
        FD_ZERO(&rd);
        FD_SET(t->fd_req[0], &rd);

        struct timeval tv = { 0 };
        tv.tv_sec = __SLEEP_TIME;
        tv.tv_usec = 0;

        int rv = select(t->fd_req[0] + 1, &rd, NULL, NULL, &tv);
        if (rv > 0) {

        	if(!FD_ISSET(t->fd_req[0], &rd)) {
        		continue;
        	}
        	resov_data_t* reso = __get_resolve_data(t);
            if (!reso)
                continue;

            printf("thread %ld start to resolve %s\n", pthread_self(), reso->host);

            bool state = false;
            if (tcp_resolve(reso->host, reso->addr, sizeof(reso->addr)) == 0) {
            	state = true;
            } else {
            	state = false;
            }
            reso->resov = state;
            __put_result_data(t, reso);
        }else if(rv == 0) {
            //printf("thread %ld select timeout\n", pthread_self());
        }else{
            //printf("thread %ld select error, errno = %d\n", pthread_self(), errno);
            break;
        }
    }

    return d;
}

/* tcp_generic_resolve() is called by tcp_resolve() and tcp_resolve_ip() to
 * do the actual work. It resolves the hostname "host" and set the string
 * representation of the IP address into the buffer pointed by "ipbuf".
 *
 * If flags is set to true the function only resolves hostnames
 * that are actually already IPv4 or IPv6 addresses. This turns the function
 * into a validating / normalizing function. */
static int tcp_generic_resolve(char *host, char *ipbuf, size_t ipbuf_len,
		bool flags) {
	struct addrinfo hints, *info;
	int rv;

	memset(&hints, 0, sizeof(hints));
	if (flags)
		hints.ai_flags = AI_NUMERICHOST;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM; /* specify socktype to avoid dups */

	if ((rv = getaddrinfo(host, NULL, &hints, &info)) != 0) {
		printf("tcp_generic_resolve %s -> %s\n", host, gai_strerror(rv));
		return -1;
	}
	if (info->ai_family == AF_INET) {
		struct sockaddr_in *sa = (struct sockaddr_in *) info->ai_addr;
		inet_ntop(AF_INET, &(sa->sin_addr), ipbuf, ipbuf_len);
	} else {
		struct sockaddr_in6 *sa = (struct sockaddr_in6 *) info->ai_addr;
		inet_ntop(AF_INET6, &(sa->sin6_addr), ipbuf, ipbuf_len);
	}

	freeaddrinfo(info);
	return 0;
}

static int tcp_listen(int s, struct sockaddr *sa, socklen_t len, int backlog) {
	if (bind(s, sa, len) == -1) {
		printf("bind: %s", strerror(errno));
		close(s);
		return -1;
	}

	if (listen(s, backlog) == -1) {
		printf("listen: %s", strerror(errno));
		close(s);
		return -1;
	}
	return 0;
}

static int tcp_generic_server(char *bindaddr, int port, int af, int backlog) {
	int s, rv;
	char _port[6]; /* strlen("65535") */
	struct addrinfo hints, *servinfo, *p;

	snprintf(_port, 6, "%d", port);
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = af;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; /* No effect if bindaddr != NULL */

	if ((rv = getaddrinfo(bindaddr, _port, &hints, &servinfo)) != 0) {
		printf("%s", gai_strerror(rv));
		return -1;
	}
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((s = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			continue;

		if (af == AF_INET6 && tcp_ipv6only(s, true) == -1) {
			close(s);
			s = -1;
			break;
		}
		if (tcp_reuse_addr(s, true) == -1) {
			close(s);
			s = -1;
			break;
		}
		if (tcp_listen(s, p->ai_addr, p->ai_addrlen, backlog) == -1) {
			close(s);
			s = -1;
			break;
		}
		break;
	}
	if (p == NULL) {
		printf("unable to bind socket");
		s = -1;
	}

	freeaddrinfo(servinfo);
	return s;
}

static int tcp_generic_accept(int s, struct sockaddr *sa, socklen_t *len) {
	int fd;
	while (1) {
		fd = accept(s, sa, len);
		if (fd == -1) {
			if (errno == EINTR)
				continue;
			else {
				printf("accept: %s", strerror(errno));
				return -1;
			}
		}
		break;
	}
	return fd;
}

static int tcp_generic_connect(char *addr, int port, bool noblock) {
	int s = -1, rv;
	char portstr[6]; /* strlen("65535") + 1; */
	struct addrinfo hints, *servinfo, *p;

	snprintf(portstr, sizeof(portstr), "%d", port);
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(addr, portstr, &hints, &servinfo)) != 0) {
		printf("%s", gai_strerror(rv));
		return -1;
	}
	for (p = servinfo; p != NULL; p = p->ai_next) {
		/* Try to create the socket and to connect it.
		 * If we fail in the socket() call, or on connect(), we retry with
		 * the next entry in servinfo. */
		if ((s = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			continue;
		if (tcp_reuse_addr(s, true) == -1) {
			close(s);
			s = -1;
			break;
		}
		if ((noblock) &&tcp_noblock(s, true) != 0) {
			close(s);
			s = -1;
			break;
		}
		if (connect(s, p->ai_addr, p->ai_addrlen) == -1) {
			/* If the socket is non-blocking, it is ok for connect() to
			 * return an EINPROGRESS error here. */
			if (errno == EINPROGRESS && (noblock))
				break;
			close(s);
			s = -1;
			continue;
		}

		/* If we ended an iteration of the for loop without errors, we
		 * have a connected socket. Let's return to the caller. */
		break;
	}
	if (p == NULL)
		printf("creating socket: %s", strerror(errno));

	freeaddrinfo(servinfo);
	return s;
}

static int tcp_create_socket(int domain) {
	int s;
	if ((s = socket(domain, SOCK_STREAM, 0)) == -1) {
		printf("creating socket: %s", strerror(errno));
		return -1;
	}

	/* Make sure connection-intensive things like the redis benchmark
	 * will be able to close/open sockets a zillion of times */
	if (tcp_reuse_addr(s, true) == -1) {
		close(s);
		return -1;
	}
	return s;
}
static int unix_doman_generic_connect(char *path, bool noblock) {
	int s;
	struct sockaddr_un sa;

	if ((s = tcp_create_socket(AF_LOCAL)) == -1)
		return -1;

	sa.sun_family = AF_LOCAL;
	strncpy(sa.sun_path, path, sizeof(sa.sun_path) - 1);
	if (noblock) {
		if (tcp_noblock(s, true) != 0)
			return -1;
	}
	if (connect(s, (struct sockaddr*) &sa, sizeof(sa)) == -1) {
		if (errno == EINPROGRESS && noblock)
			return s;

		printf("connect: %s", strerror(errno));
		close(s);
		return -1;
	}
	return s;
}

int tcp_resolve(char *host, char *ipbuf, size_t ipbuf_len) {
	return tcp_generic_resolve(host, ipbuf, ipbuf_len, false);
}

int tcp_noblock_resolve(resov_data_t* resov)
{
	if (!__resov) {
		__resov = (resov_thread_info_t*)malloc(sizeof(*__resov));
		memset(__resov, 0, sizeof(*__resov));
		pipe(__resov->fd_req);
		pipe(__resov->fd_rst);
		tcp_noblock(__resov->fd_rst[0], true);

		__resov->count = __THREAD_COUNT;
        if (resov) {
            __resov->count = resov->thread;
            if (resov->thread < __THREAD_COUNT) {
                __resov->count = __THREAD_COUNT;
            }
        }

		__resov->thread = (pthread_t*)malloc(__resov->count * sizeof(pthread_t));


		int i = 0;
		for (i = 0; i < __resov->count; i++) {
			pthread_t tid;
			pthread_create(&tid, NULL, __resov_thread, __resov);
			printf("thread %d started, thread id = %ld\n", i, tid);
			__resov->thread[i] = tid;
		}
	}

    if (resov) {
        if (resov->thread > __resov->count) {
            int diff = resov->thread - __resov->count;
            __resov->thread = realloc(__resov->thread,
                    resov->thread * sizeof(pthread_t));
            int i = 0;
            for (i = 0; i < diff; i++) {
                pthread_t tid;
                pthread_create(&tid, NULL, __resov_thread, __resov);
                printf("new thread %d started, thread id = %ld\n", i, tid);
                __resov->thread[i + __resov->count] = tid;
            }
            __resov->count = resov->thread;
        }

        if (__put_resolve_data(__resov, resov) != 0) {
            return -1;
        }
    }

    return __resov->fd_rst[0];
}
int tcp_noblock_resolve_pollfd()
{
	int fd = -1;
	if(__resov) {
		fd = __resov->fd_rst[0];;
	}
	return fd;
}
resov_data_t* tcp_noblock_resolve_result()
{
	resov_data_t* reso = NULL;
	int fd = tcp_noblock_resolve_pollfd();
	if(fd < 0) return reso;

	intptr_t ptr = 0;
	int rv = read(fd, &ptr, sizeof(void*));
	if(rv != sizeof(void*)) return reso;
	reso = (resov_data_t*)ptr;

	return reso;
}

int tcp_resolve_ip(char *host, char *ipbuf, size_t ipbuf_len) {
	return tcp_generic_resolve(host, ipbuf, ipbuf_len, true);
}

int tcp_server(char *bindaddr, int port, int backlog) {
	return tcp_generic_server(bindaddr, port, AF_INET, backlog);
}

int tcp6_server(char *bindaddr, int port, int backlog) {
	return tcp_generic_server(bindaddr, port, AF_INET6, backlog);
}

int tcp_accept(int s, char *ip, size_t ip_len, int *port) {
	int fd;
	struct sockaddr_storage sa;
	socklen_t salen = sizeof(sa);
	if ((fd = tcp_generic_accept(s, (struct sockaddr*) &sa, &salen)) == -1)
		return -1;

	if (sa.ss_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *) &sa;
		if (ip)
			inet_ntop(AF_INET, (void*) &(s->sin_addr), ip, ip_len);
		if (port)
			*port = ntohs(s->sin_port);
	} else {
		struct sockaddr_in6 *s = (struct sockaddr_in6 *) &sa;
		if (ip)
			inet_ntop(AF_INET6, (void*) &(s->sin6_addr), ip, ip_len);
		if (port)
			*port = ntohs(s->sin6_port);
	}
	return fd;
}
int tcp_connect(char *addr, int port) {
	return tcp_generic_connect(addr, port, false);
}

int tcp_noblock_connect(char *addr, int port) {
	return tcp_generic_connect(addr, port, true);
}


int tcp_read(int fd, char *buf, int count, bool* ok) {
	int nread, totlen = 0;
	*ok = true;
	while (totlen != count) {
		nread = read(fd, buf, count - totlen);
		if (nread == 0) {
			printf("recv data = 0, connection close, fd=%d\n", fd);
			*ok = false;
			break;
		}
		if (nread == -1) {
			if (errno == EINTR) {
				continue;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				break;
			}
			*ok = false;
			break;
		}
		totlen += nread;
		buf += nread;
	}
	return totlen;
}

int tcp_read_needle(int fd, const char* needle, char *buf, int count, bool* ok)
{
	int nread, ndlen, totlen = 0;
	*ok = true;
	ndlen = strlen(needle);
	while (totlen != count) {
		nread = read(fd, buf, count - totlen);
		if (nread == 0) {
			printf("recv data = 0, connection close, fd=%d\n", fd);
			*ok = false;
			break;
		}
		if (nread == -1) {
			if (errno == EINTR) {
				continue;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				break;
			}
			*ok = false;
			break;
		}
		totlen += nread;
		buf += nread;

		if(totlen >= ndlen) {
			if(strncmp(buf + totlen - ndlen, needle, ndlen) == 0) {
				break;
			}
		}
	}
	return totlen;
}

int tcp_write(int fd, char *buf, int count, bool* ok) {
	int nwritten, totlen = 0;
	*ok = true;
	while (totlen != count) {
		nwritten = write(fd, buf, count - totlen);
		if (nwritten == 0) {
			printf("send data = 0, connection close, fd=%d\n", fd);
			*ok = false;
			break;
		}
		if (nwritten == -1) {
			if (errno == EINTR) {
				continue;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				break;
			}
			*ok = false;
			break;
		}
		totlen += nwritten;
		buf += nwritten;
	}
	return totlen;
}

int tcp_peer_name(int fd, char *ip, size_t ip_len, int *port) {
	struct sockaddr_storage sa;
	socklen_t salen = sizeof(sa);

	if (getpeername(fd, (struct sockaddr*) &sa, &salen) == -1) {
		if (port)
			*port = 0;
		ip[0] = '?';
		ip[1] = '\0';
		return -1;
	}
	if (sa.ss_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *) &sa;
		if (ip)
			inet_ntop(AF_INET, (void*) &(s->sin_addr), ip, ip_len);
		if (port)
			*port = ntohs(s->sin_port);
	} else {
		struct sockaddr_in6 *s = (struct sockaddr_in6 *) &sa;
		if (ip)
			inet_ntop(AF_INET6, (void*) &(s->sin6_addr), ip, ip_len);
		if (port)
			*port = ntohs(s->sin6_port);
	}
	return 0;
}

int tcp_sock_name(int fd, char *ip, size_t ip_len, int *port) {
	struct sockaddr_storage sa;
	socklen_t salen = sizeof(sa);

	if (getsockname(fd, (struct sockaddr*) &sa, &salen) == -1) {
		if (port)
			*port = 0;
		ip[0] = '?';
		ip[1] = '\0';
		return -1;
	}
	if (sa.ss_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *) &sa;
		if (ip)
			inet_ntop(AF_INET, (void*) &(s->sin_addr), ip, ip_len);
		if (port)
			*port = ntohs(s->sin_port);
	} else {
		struct sockaddr_in6 *s = (struct sockaddr_in6 *) &sa;
		if (ip)
			inet_ntop(AF_INET6, (void*) &(s->sin6_addr), ip, ip_len);
		if (port)
			*port = ntohs(s->sin6_port);
	}
	return 0;
}

int tcp_noblock(int fd, bool on) {
	int flags;

	/* Set the socket non-blocking.
	 * Note that fcntl(2) for F_GETFL and F_SETFL can't be
	 * interrupted by a signal. */
	if ((flags = fcntl(fd, F_GETFL)) == -1) {
		printf("fcntl(F_GETFL): %s", strerror(errno));
		return -1;
	}
	flags = on ? (flags | O_NONBLOCK) : (flags & (~O_NONBLOCK));
	if (fcntl(fd, F_SETFL, flags) == -1) {
		printf("fcntl(F_SETFL,O_NONBLOCK): %s", strerror(errno));
		return -1;
	}
	return 0;
}
int tcp_nodelay(int fd, bool on) {
	int val = on ? 1 : 0;
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1) {
		printf("setsockopt TCP_NODELAY: %s", strerror(errno));
		return -1;
	}
	return 0;
}

/* Set TCP keep alive option to detect dead peers. The interval option
 * is only used for Linux as we are using Linux-specific APIs to set
 * the probe send time, interval, and count. */
int tcp_keepalive(int fd, bool on, int interval) {
	int val = on ? 1 : 0;

	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == -1) {
		printf("setsockopt SO_KEEPALIVE: %s", strerror(errno));
		return -1;
	}

	if (!on)
		return 0;

#ifdef __linux__
	/* Default settings are more or less garbage, with the keepalive time
	 * set to 7200 by default on Linux. Modify settings to make the feature
	 * actually useful. */

	/* Send first probe after interval. */
	val = interval;
	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0) {
		printf("setsockopt TCP_KEEPIDLE: %s\n", strerror(errno));
		return -1;
	}

	/* Send next probes after the specified interval. Note that we set the
	 * delay as interval / 3, as we send three probes before detecting
	 * an error (see the next setsockopt call). */
	val = interval / 3;
	if (val == 0)
		val = 1;
	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) < 0) {
		printf("setsockopt TCP_KEEPINTVL: %s\n", strerror(errno));
		return -1;
	}

	/* Consider the socket in error state after three we send three ACK
	 * probes without getting a reply. */
	val = 3;
	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) < 0) {
		printf("setsockopt TCP_KEEPCNT: %s\n", strerror(errno));
		return -1;
	}
#endif

	return 0;
}

int tcp_reuse_addr(int fd, bool on) {
	int yes = on ? 1 : 0;
	/* Make sure connection-intensive things like the redis benckmark
	 * will be able to close/open sockets a zillion of times */
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
		printf("setsockopt SO_REUSEADDR: %s", strerror(errno));
		return -1;
	}
	return 0;
}
int tcp_ipv6only(int fd, bool on) {
	int yes = on ? 1 : 0;
	if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof(yes)) == -1) {
		printf("setsockopt: %s", strerror(errno));
		return -1;
	}
	return 0;
}

int tcp_set_send_buffer(int fd, int buffsize) {
	if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffsize, sizeof(buffsize))
			== -1) {
		printf("setsockopt SO_SNDBUF: %s", strerror(errno));
		return -1;
	}
	return 0;
}
int tcp_set_recv_buffer(int fd, int buffsize) {
	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &buffsize, sizeof(buffsize))
			== -1) {
		printf("setsockopt SO_RCVBUF: %s", strerror(errno));
		return -1;
	}
	return 0;
}

int unix_domain_server(char *path, mode_t perm, int backlog) {
	int s;
	struct sockaddr_un sa;

	if ((s = tcp_create_socket(AF_LOCAL)) == -1)
		return -1;

	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_LOCAL;
	strncpy(sa.sun_path, path, sizeof(sa.sun_path) - 1);
	if (tcp_listen(s, (struct sockaddr*) &sa, sizeof(sa), backlog) == -1) {
		close(s);
		return -1;
	}
	if (perm)
		chmod(sa.sun_path, perm);
	return s;
}

int unix_domain_accept(int s) {
	int fd;
	struct sockaddr_un sa;
	socklen_t salen = sizeof(sa);
	if ((fd = tcp_generic_accept(s, (struct sockaddr*) &sa, &salen)) == -1)
		return -1;

	return fd;
}

int unix_domain_connect(char *path) {
	return unix_doman_generic_connect(path, false);
}

int unix_domain_noblock_connect(char *path) {
	return unix_doman_generic_connect(path, true);
}
ssize_t unix_domain_send_fd(int fd,/* void* ptr, size_t nbytes, */int sendfd)
{
    struct msghdr msg;
    struct iovec iov[1];

    union {
        struct cmsghdr cm;
        char control[CMSG_SPACE(sizeof(int))];
    }control_un;
    struct cmsghdr *cmptr;

    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    cmptr = CMSG_FIRSTHDR(&msg);
    cmptr->cmsg_len = CMSG_LEN(sizeof(int));
    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;
    *((int*)CMSG_DATA(cmptr)) = sendfd;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

/*    iov[0].iov_base = ptr;
    iov[0].iov_len = nbytes;*/

    iov[0].iov_base = "";
    iov[0].iov_len = 1;

    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    return sendmsg(fd, &msg, 0);

}
ssize_t unix_domain_recv_fd(int fd,/* void* ptr, size_t nbytes, */int* recvfd)
{
    struct msghdr msg;
    struct iovec iov[1];
    ssize_t n;

    *recvfd = -1;

    union {
        struct cmsghdr cm;
        char control[CMSG_SPACE(sizeof(int))];
    }control_un;
    struct cmsghdr* cmptr;

    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    char c;
    iov[0].iov_base = &c;
    iov[0].iov_len = 1;

    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    if((n = recvmsg(fd, &msg, 0)) <= 0){
        return n;
    }

    if((cmptr = CMSG_FIRSTHDR(&msg)) != NULL &&
            cmptr->cmsg_len == CMSG_LEN(sizeof(int))){
        if(cmptr->cmsg_level != SOL_SOCKET){
            return -1;
        }
        if(cmptr->cmsg_type != SCM_RIGHTS){
            return -1;
        }
        *recvfd = *((int*)CMSG_DATA(cmptr));
    }else{
        *recvfd = -1;
    }
    return n;
}
