/*
 * Create on: 2016-04-05
 *    Author: Luo Guochun
 */

#include "mysftp.h"

#include <libssh2.h>
#include <libssh2_sftp.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

sftp_context_t* sftp_connect(const char* host, int port,
        int au_type,
        const char* user, const char* pass,
        const char* pub_key, const char* privt_key, const char* passphrase)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &addr.sin_addr);


    if (connect(sock, (struct sockaddr*)(&addr),
                sizeof(struct sockaddr_in)) != 0) {
        printf("failed to connect: %s!\n", strerror(errno));
        return NULL;
    }
    
    sftp_context_t* ctx = (sftp_context_t*)malloc(sizeof(*ctx));
    if(!ctx) {
        printf("malloc failed.\n");
        close(sock);
        return NULL;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->fd = sock;

    strncpy(ctx->host, host, sizeof(ctx->host) - 1);
    ctx->port = port;
    ctx->mode = au_type;
    if(pub_key) strncpy(ctx->pub_key, pub_key, sizeof(ctx->pub_key) - 1);
    if(privt_key) strncpy(ctx->privt_key, privt_key, sizeof(ctx->privt_key) - 1);
    if(user) strncpy(ctx->user, user, sizeof(ctx->user) - 1);
    if(pass) strncpy(ctx->pass, pass, sizeof(ctx->pass) - 1);
    if(passphrase) strncpy(ctx->passphrase, passphrase, sizeof(ctx->passphrase) - 1);

    ctx->ssh2_session = libssh2_session_init();
    if(!ctx->ssh2_session) {
        printf("libssh2_session_init failed.\n");
        close(sock);
        free(ctx);
        return NULL;
    }

    libssh2_session_set_blocking(ctx->ssh2_session, 1);

    int ret = 0;

    ret = libssh2_session_handshake(ctx->ssh2_session, sock);
    if(ret) {
        printf("libssh2_session_handshake failed.  %d\n", ret);
        libssh2_session_free(ctx->ssh2_session);
        close(sock);
        free(ctx);
        return NULL;
    }
    
    const char* fingerprint = libssh2_hostkey_hash(ctx->ssh2_session, LIBSSH2_HOSTKEY_HASH_SHA1);
 
    printf("fingerprint: ");
    int i = 0;
    for(i = 0; i < 20; i++) {
        if(i == 19) {
            printf("%02X", (unsigned char)fingerprint[i]);
        }else{
            printf("%02X:", (unsigned char)fingerprint[i]);
        }
    }
    printf("\n");
  
    char* userauthlist = libssh2_userauth_list(ctx->ssh2_session, ctx->user, strlen(ctx->user));
 
    printf("Authentication methods: %s\n", userauthlist);
    
    int pass_auth = 0;
    int rsa_auth = 0;
    if (strstr(userauthlist, "password") != NULL) {
        pass_auth = 1;
    }
    if (strstr(userauthlist, "publickey") != NULL) {
        rsa_auth = 1;
    }

    if(ctx->mode == AUTHOR_TYPE_PASSWORD && !pass_auth) {
        printf("sftp server not support password authenticate\n");
        libssh2_session_free(ctx->ssh2_session);
        close(sock);
        free(ctx);
        return NULL;
    }

    if(ctx->mode == AUTHOR_TYPE_RSA_KEY && !rsa_auth) {
        printf("sftp server not support RSA author\n");
        libssh2_session_free(ctx->ssh2_session);
        close(sock);
        free(ctx);
        return NULL;
    }

    if(ctx->mode == AUTHOR_TYPE_PASSWORD) {
        ret = libssh2_userauth_password(ctx->ssh2_session, ctx->user, ctx->pass);
    }
    if(ctx->mode == AUTHOR_TYPE_RSA_KEY) {
        ret = libssh2_userauth_publickey_fromfile(ctx->ssh2_session, ctx->user,
                ctx->pub_key, ctx->privt_key, ctx->passphrase);
    }
    if(ret) {
        printf("authenticate failed.\n");
        libssh2_session_free(ctx->ssh2_session);
        close(sock);
        free(ctx);
        return NULL;
    }

    ctx->sftp_session = libssh2_sftp_init(ctx->ssh2_session);
    if(!ctx->sftp_session) {
        libssh2_session_free(ctx->ssh2_session);
        close(sock);
        free(ctx);
        return NULL;
    }

    return ctx;
}

int sftp_get(sftp_context_t* ctx,
        const char* remote_path, const char* local_path)
{
    LIBSSH2_SFTP_HANDLE* sftp_handle =
        libssh2_sftp_open(ctx->sftp_session, remote_path, LIBSSH2_FXF_READ, 0);
 
  
    if (!sftp_handle) {
        printf("Unable to open file with SFTP: %ld\n",
                libssh2_sftp_last_error(ctx->sftp_session));
 
        return -1;
    }
 
    FILE* fp = fopen(local_path, "w+");
    if(fp == NULL){
        printf("fopen failed.\n");
        libssh2_sftp_close(sftp_handle);
        return -1;
    }
    do {
		char mem[1024] = { 0 };
		size_t rc = libssh2_sftp_read(sftp_handle, mem, sizeof(mem));
		if (rc > 0) {
			size_t len = fwrite(mem, 1, rc, fp);
			if (len != rc) {
				printf("fwrite error.\n");
				fclose(fp);
		        libssh2_sftp_close(sftp_handle);
				return -1;
			}
		}else{
			break;
		}
    }while(1);

    fclose(fp);
    libssh2_sftp_close(sftp_handle);

    return 0;
}
int sftp_put(sftp_context_t* ctx,
        const char* local_path, const char* remote_path)
{
    LIBSSH2_SFTP_HANDLE* sftp_handle =
        libssh2_sftp_open(ctx->sftp_session, remote_path,
        		                      LIBSSH2_FXF_WRITE|LIBSSH2_FXF_CREAT|LIBSSH2_FXF_TRUNC,
        		                      LIBSSH2_SFTP_S_IRUSR|LIBSSH2_SFTP_S_IWUSR|
        		                      LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IROTH);


    if (!sftp_handle) {
        printf("Unable to open file with SFTP: %ld\n",
                libssh2_sftp_last_error(ctx->sftp_session));

        return -1;
    }

    FILE* fp = fopen(local_path, "rb");
    if(fp == NULL){
        printf("fopen failed.\n");
        libssh2_sftp_close(sftp_handle);
        return -1;
    }
    do {
		char mem[1024] = { 0 };
		size_t rc = fread(mem, 1, sizeof(mem), fp);
		if (rc > 0) {
			size_t len = libssh2_sftp_write(sftp_handle, mem, rc);
			if (len != rc) {
				printf("libssh2_sftp_write error.\n");
				fclose(fp);
		        libssh2_sftp_close(sftp_handle);
				return -1;
			}
		}else{
			break;
		}
    }while(1);

    fclose(fp);
    libssh2_sftp_close(sftp_handle);

    return 0;
}
int sftp_path_exists(sftp_context_t* ctx,
        const char* remote_path)
{
    LIBSSH2_SFTP_HANDLE* sftp_handle =
        libssh2_sftp_open(ctx->sftp_session, remote_path, LIBSSH2_FXF_READ, 0);


    if (!sftp_handle) {

        return 0;
    }
    libssh2_sftp_close(sftp_handle);

    return 1;

}

int sftp_close(sftp_context_t* ctx)
{
	if(! ctx) {
		return 0;
	}
    libssh2_sftp_shutdown(ctx->sftp_session);
    libssh2_session_disconnect(ctx->ssh2_session, "Normal Shutdown, Thank you for playing");
    libssh2_session_free(ctx->ssh2_session);
    close(ctx->fd);

    return 0;
}

