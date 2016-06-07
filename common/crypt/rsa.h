/*
 * crypt.h
 *
 *  Created on: 2013-7-19
 *      Author: Luo Guochun
 */

#ifndef __RSA_H__
#define __RSA_H__


#ifdef __cplusplus
extern "C" {
#endif

typedef struct rsa_ctx_s rsa_ctx_t;

rsa_ctx_t* rsa_init(const char* rsa_pub_key_file, const char* rsa_pri_key_file);

int rsa_md5_sign(rsa_ctx_t* ctx,
		const char* src_buf, int src_len, char* dest_buf, int* dest_len);
int rsa_md5_verify(rsa_ctx_t* ctx,
		const char* src_buf, int src_len,
        const char* sig_buf, int sig_len);

int rsa_sha1_sign(rsa_ctx_t* ctx,
		const char* src_buf, int src_len, char* dest_buf, int* dest_len);
int rsa_sha1_verify(rsa_ctx_t* ctx,
		const char* src_buf, int src_len,
        const char* sig_buf, int sig_len);

int rsa_uninit(rsa_ctx_t* ctx);

#ifdef __cplusplus
}
#endif


#endif /* __RSA_H__ */
