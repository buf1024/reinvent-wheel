/*
 * sftp.h
 *   Created on: 2016-04-05
 *       Author: Luo Guochun
 */

#ifndef __MYSFTP_H__
#define __MYSFTP_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _LIBSSH2_SESSION             LIBSSH2_SESSION;
typedef struct _LIBSSH2_SFTP                LIBSSH2_SFTP;

typedef struct {
    char host[128];
    int  port;
    int  mode;
    char user[128];
    char pass[128];
    char pub_key[256];
    char privt_key[256];
    char passphrase[256];

    int fd;
    LIBSSH2_SESSION* ssh2_session;
    LIBSSH2_SFTP* sftp_session;

}sftp_context_t;

enum {
    AUTHOR_TYPE_PASSWORD,
    AUTHOR_TYPE_RSA_KEY
};

sftp_context_t* sftp_connect(const char* host, int port,
        int au_type,
        const char* user, const char* pass,
        const char* pub_key, const char* privt, const char* passphrase);

int sftp_get(sftp_context_t* ctx, const char* remote_path, const char* local_path);
int sftp_put(sftp_context_t* ctx, const char* local_path, const char* remote_path);
int sftp_path_exists(sftp_context_t* ctx, const char* remote_path);

int sftp_close(sftp_context_t* ctx);

#ifdef __cplusplus
extern }
#endif

#endif /* __MYSFTP_H__ */



