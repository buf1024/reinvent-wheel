/*
 * socktest.c
 *
 *  Created on: 2016/6/8
 *      Author: Luo Guochun
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../sock.h"

int main(int argc, char **argv) {

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


		char buf[1024] = {0};
		while(1) {
			int rd = tcp_read(cfd, buf, sizeof(buf) - 1);
			if(rd > 0) {
				printf("READ: \n%s\n", buf);
				memset(buf, 0, rd);
			}else{
				printf("connection close\n");
				break;
			}
		}
	}

	return 0;
}

