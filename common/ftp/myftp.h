/*
 * myftp.h
 *
 *  Created on: 2013-10-21
 *      Author: Luo Guochun
 */

#ifndef __MYFTP_H__
#define __MYFTP_H__

#ifdef __cplusplus
extern "C" {
#endif

int ftp_put_file(const char* local_file,
        const char* dest_path, const char* dest_file,
        const char* host, const char* user, const char* pass);

int ftp_get_file(const char* local_file,
        const char* dest_path, const char* dest_file,
        const char* host, const char* user, const char* pass);

#ifdef __cplusplus
extern }
#endif


#endif /* __MYFTP_H__ */
