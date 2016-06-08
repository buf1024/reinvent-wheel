/*
 * hmac.h
 *
 *  Created on: 2016/6/8
 *      Author: Luo Guochun
 */

#ifndef __HMAC_H__
#define __HMAC_H__

#ifdef __cplusplus
extern "C" {
#endif

int hmac_init();

unsigned char* hmac_md5(const void *key, int key_len,
		const unsigned char *src_buf, int src_len,
		unsigned char *md, unsigned int *md_len);

unsigned char* hmac_sha1(const void *key, int key_len,
		const unsigned char *src_buf, int src_len,
		unsigned char *md, unsigned int *md_len);

int hmac_uninit();

#ifdef __cplusplus
}
#endif

#endif /* __HMAC_H__ */
