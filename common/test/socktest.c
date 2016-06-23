/*
 * socktest.c
 *
 *  Created on: 2016/6/8
 *      Author: Luo Guochun
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../sock.h"

static int server_test()
{

	char ip[128] = {0};
	int port = 8787;

	if(tcp_resolve("localhost", ip, sizeof(ip)) != 0) {
		printf("tcp_resolve failed.\n");
		return -1;
	}

	printf("tcp_resolve: ip = %s\n", ip);

	int sfd = tcp_server(ip, port, 128);
	if(sfd <= 0) {
		printf("tcp_server failed.\n");
		return sfd;
	}
	printf("tcp_server listen on: ip = %s, port = %d\n", ip, port);

	while(1) {
		char peer_ip[128] = {0};
		int peer_port = 0;
		int cfd = tcp_accept(sfd, peer_ip, sizeof(peer_ip), &peer_port);
		if(cfd <= 0) {
			printf("tcp_accept failed.\n");
			return -1;
		}

		printf("tcp_accept: ip = %s, port = %d\n", peer_ip, peer_port);

		memset(peer_ip, 0, sizeof(peer_ip)); peer_port = 0;
		tcp_sock_name(cfd, peer_ip, sizeof(peer_ip), &peer_port);
		printf("tcp_sock_name: ip = %s, port = %d\n", peer_ip, peer_port);

		memset(peer_ip, 0, sizeof(peer_ip)); peer_port = 0;
		tcp_peer_name(cfd, peer_ip, sizeof(peer_ip), &peer_port);
		printf("tcp_peer_name: ip = %s, port = %d\n", peer_ip, peer_port);

		tcp_noblock(cfd, true);

		char buf[1024] = {0};
		while(1) {
			bool ok;
			int rd = tcp_read(cfd, buf, sizeof(buf) - 1, &ok);
			if(rd > 0) {
				printf("READ: \n%s\n", buf);
				memset(buf, 0, rd);
			}
			if(!ok) {
				close(cfd);
				printf("connection close.\n");
				break;
			}
		}
	}

	return 0;
}

static int client_test()
{
	char* host = "www.baidu.com";
	int port = 80;

	char ip[128] = {0};


	int ret = tcp_resolve(host, ip, sizeof(ip));
	if(ret != 0) {
		return ret;
	}
	printf("tcp_resolve www.baidu.com: ip = %s\n", ip);

	int s = tcp_connect(ip, port);
	if(s <= 0) {
		printf("tcp_connect failed.\n");
		return -1;
	}

	char packe[] = "GET / HTTP/1.1\r\n"
			"Host: www.baidu.com\n\n"
			"Cache-Control: max-age=0\r\n"
			"User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/48.0.2564.116 Safari/537.36\r\n"
			"Accept: */*\r\n"
			"Accept-Encoding: gzip, deflate, sdch\r\n"
			"Accept-Language: zh-CN,zh;q=0.8\r\n"
			"\r\n\r\n";

	bool broken;
	ret = tcp_write(s, packe, sizeof(packe), &broken);
	if(ret != sizeof(packe)) {
		close(s);
		printf("tcp_write failed.\n");
		return -1;
	}

	char buf[1024] = {0};

	while(1) {
	    ret = tcp_read(s, buf, sizeof(buf) - 1, &broken);
	    if(ret > 0) {
	    	printf("READ: \n%s\n", buf);
	    }
	    if(broken) {
	    	close(s);
	    	printf("connection close\n");
	    	break;
	    }
	}

	return 0;

}

int main(int argc, char **argv)
{
	if(argc > 1) {
		if(strcmp(argv[1], "client") == 0) {
			return client_test();
		}
	}

	return server_test();
}

