/*
 * hmac.c
 *
 *  Created on: 2016/6/8
 *      Author: Luo Guochun
 */

#include "hmac.h"

#include <stdbool.h>

#include <openssl/hmac.h>
#include <openssl/evp.h>

int hmac_init()
{
	static bool algo = false;
	if(!algo) {
        OpenSSL_add_all_algorithms();
        algo = true;
	}
	return 0;
}

unsigned char* hmac_md5(const void *key, int key_len,
		const unsigned char *src_buf, int src_len,
		unsigned char *md, unsigned int *md_len)
{
	return HMAC(EVP_md5(), key, key_len,
			src_buf, src_len,
			md, md_len);
}

unsigned char* hmac_sha1(const void *key, int key_len,
		const unsigned char *src_buf, int src_len,
		unsigned char *md, unsigned int *md_len)
{
	return HMAC(EVP_sha1(), key, key_len,
			src_buf, src_len,
			md, md_len);
}

int hmac_uninit()
{
	static bool algo = false;
	if(!algo) {
		EVP_cleanup();
        algo = true;
	}
	return 0;
}
