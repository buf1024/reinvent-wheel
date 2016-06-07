/*
 * Create on: 2016-04-05
 *    Author: Luo Guochun
 */


#include "../mysftp.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv)
{

    const char* host = NULL;
    int port = 22;
    const char* user = NULL;
    const char* pass = NULL;
    const char* pub_key = NULL;
    const char* privt_key = NULL;


    if(argc != 7) {
        printf("%s <addr> <port> <user name> <password> <pubkey> <privkey>\n", argv[0]);
        exit(0);
    }

    host = argv[1];
    port = atoi(argv[2]);

    user = argv[3];
    pass = argv[4];
    if(strcasecmp(pass, "null") == 0) {
    	pass = NULL;
    }
    pub_key = argv[5];
    privt_key = argv[6];

    int mode = AUTHOR_TYPE_PASSWORD;

    if(access(pub_key, F_OK) == 0 &&
            access(privt_key, F_OK == 0)) {
        mode = AUTHOR_TYPE_RSA_KEY;
    }
    
    sftp_context_t* ctx = NULL;

    ctx = sftp_connect(host, port,
    		mode,
			user, pass, pub_key, privt_key, pass);
    if(!ctx) {
        printf("sftp_connect error\n");
        exit(-1);
    }

    int rc = 0;
    
    char local_path[256] = {0};
    strncpy(local_path, argv[0], sizeof(local_path));

    char remote_path[256] = {0};
    int len = strlen(argv[0]);
    int i = len - 1;
    for(; i>=0; i--) {
    	if(argv[0][i] == '/') {
    		break;
    	}
    }
//strncpy(remote_path, "/home/heidong/mysftp", sizeof(remote_path));
    strncpy(remote_path, argv[0] + i + 1, sizeof(remote_path));

    printf("check sftp path %s\n", remote_path);
    rc = sftp_path_exists(ctx, remote_path);
    if(rc) {
        printf("sftp path %s exist.\n", remote_path);
    }else {
        printf("sftp path %s not exist.\n", remote_path);
    }

    printf("put file: %s to remote sftp server: %s\n", local_path, remote_path);
    rc = sftp_put(ctx, local_path, remote_path);
    if(rc) {
        printf("sftp_put failed\n");
        exit(-1);
    }

    printf("check sftp path %s exist.\n", remote_path);
    rc = sftp_path_exists(ctx, remote_path);
    if(rc) {
        printf("sftp path %s exist.\n", remote_path);
    }else {
        printf("sftp path %s not exist.\n", remote_path);
        printf("test failed.\n");
        exit(-1);
    }

    strcat(local_path, ".remote");

    printf("sftp get %s to local %s.\n", remote_path, local_path);
    rc = sftp_get(ctx, remote_path, local_path);
    if(rc) {
        printf("sftp_get failed.\n");
        exit(-1);
    }

    rc = sftp_close(ctx);
    if(rc) {
        printf("sftp_close failed.\n");
        exit(-1);
    }


    return 0;
}



