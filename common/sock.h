/*
 * sock.h
 *
 *  Created on: 2016年6月7日
 *      Author: Luo Guochun
 */

#ifndef __SOCK_H__
#define __SOCK_H__

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__sun)
#define AF_LOCAL AF_UNIX
#endif

// tcp
int tcp_resolve(char *host, char *ipbuf, size_t ipbuf_len);
int tcp_resolve_ip(char *host, char *ipbuf, size_t ipbuf_len);
int tcp_server(char *bindaddr, int port, int backlog);
int tcp6_server(char *bindaddr, int port, int backlog);
int tcp_accept(int serversock, char *ip, size_t ip_len, int *port);
int tcp_connect(char *addr, int port);
int tcp_noblock_connect(char *addr, int port);
int tcp_read(int fd, char *buf, int count, bool* fd_broken);
int tcp_read_needle(int fd, const char* needle, char *buf, int count, bool* fd_broken);
int tcp_write(int fd, char *buf, int count, bool* fd_broken);
int tcp_peer_name(int fd, char *ip, size_t ip_len, int *port);
int tcp_sock_name(int fd, char *ip, size_t ip_len, int *port);

// tcp option
int tcp_noblock(int fd, bool on);
int tcp_nodelay(int fd, bool on);
int tcp_keepalive(int fd, bool on, int interval);
int tcp_reuse_addr(int fd, bool on);
int tcp_ipv6only(int fd, bool on);
int tcp_set_send_buffer(int fd, int buffsize);
int tcp_set_recv_buffer(int fd, int buffsize);

// unix domain
int unix_domain_server(char *path, mode_t perm, int backlog);
int unix_domain_accept(int serversock);
int unix_domain_connect(char *path);
int unix_domain_noblock_connect(char *path);
/*fd generated from socketpair function call*/
ssize_t unix_domain_send_fd(int fd,/* void* ptr, size_t nbytes, */int sendfd);
ssize_t unix_domain_recv_fd(int fd,/* void* ptr, size_t nbytes, */int* recvfd);

#ifdef __cplusplus
}
#endif


#endif /* __SOCK_H__ */
