/*
 * myftp.c
 *
 *  Created on: 2013-10-21
 *      Author: Luo Guochun
 */

#include <stdio.h>
#include <stdlib.h>
#include "ftplib.h"
#include "myftp.h"



int ftp_put_file(const char* local_file, const char* dest_path, const char* dest_file, const char* host, const char* user, const char* pass)
{
    netbuf * conn = NULL;
    //int int_mode = FTPLIB_PORT;
    int int_mode = FTPLIB_PASSIVE;
    static char mode = 'I';

    FtpInit();

    if (!FtpConnect(host, &conn)) {
        printf("connet host = %s failed...\n", host);
        return -1;
    }
    printf("ftp conneted...\n");

    FtpOptions( FTPLIB_CONNMODE, int_mode, conn);

    if (!FtpLogin(user, pass, conn)) {
        printf("login fail, user = %s, pass = %s ....\n", user, pass);
        FtpQuit(conn);
        return -1;
    }
    printf("ftp logined...\n");

    if (!FtpChdir(dest_path, conn)) {
        printf("ftp chdir = %s Fail..\n", dest_path);
        FtpQuit(conn);
        return -1;
    }
    printf("ftp chdir ok!\n");

    if (!FtpPut(local_file, dest_file, mode, conn)) {
        printf("ftpput fail!\n");
        FtpQuit(conn);
        return -1;
    }
    printf("ftp put ok!\n");

    FtpQuit(conn);
    printf("ftp quit ok!\n");

    return 0;
}

int ftp_get_file(const char* local_file, const char* dest_path, const char* dest_file, const char* host, const char* user, const char* pass)
{
    netbuf * conn = NULL;
    //int int_mode = FTPLIB_PORT;
    int int_mode = FTPLIB_PASSIVE;
    static char mode = 'I';

    FtpInit();

    if (!FtpConnect(host, &conn)) {
        printf("connet host = %s failed...\n", host);
        return -1;
    }
    printf("ftp conneted...\n");

    FtpOptions( FTPLIB_CONNMODE, int_mode, conn);

    if (!FtpLogin(user, pass, conn)) {
        printf("login fail, user = %s, pass = %s ....\n", user, pass);
        FtpQuit(conn);
        return -1;
    }
    printf("ftp logined...\n");

    if (!FtpChdir(dest_path, conn)) {
        printf("ftp chdir = %s Fail..\n", dest_path);
        FtpQuit(conn);
        return -1;
    }
    printf("ftp chdir ok!\n");

    if (!FtpGet(local_file, dest_file, mode, conn)) {
        printf("ftp get fail!\n");
        FtpQuit(conn);
        return -1;
    }
    printf("ftp get ok!\n");

    FtpQuit(conn);
    printf("ftp quit ok!\n");

    return 0;

}
