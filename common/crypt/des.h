/*
 * des.h
 *
 *  Created on: 2013-7-19
 *      Author: Luo Guochun
 */

#ifndef __DES_H__
#define __DES_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct des_ctx_s des_ctx_t;

// k1, k2, k3 8字节
des_ctx_t* des_init(const char* k1, const char* k2, const char* k3);

int des_encrypt(des_ctx_t* ctx,
		const char* src_buf, int src_len,
		char* enc_buf, int* enc_len);
int des_decrypt(des_ctx_t* ctx,
		const char* src_buf, int src_len,
		char* dec_buf, int* dec_len);

int desc_uninit(des_ctx_t* ctx);


#ifdef __cplusplus
extern "C" {
#endif

#endif /* __DES_H__ */
