/*
 * des.c
 *
 *  Created on: 2013-7-19
 *      Author: Luo Guochun
 */

#include "des.h"

#include <string.h>
#include <stdbool.h>

#include <openssl/des.h>

typedef struct des_ctx_s
{
    DES_key_schedule des_key_1;
    DES_key_schedule des_key_2;
    DES_key_schedule des_key_3;

}des_ctx_t;
des_ctx_t* des_init(const char* k1, const char* k2, const char* k3)
{

	des_ctx_t* ctx = (des_ctx_t*)malloc(sizeof(*ctx));
	if(!ctx) {
		return NULL;
	}

	char key1[8+1] = {0}, key2[8+1] = {0}, key3[8+1] = {0};
	strncpy(key1, k1, 8);strncpy(key2, k2, 8);strncpy(key3, k3, 8);

    DES_set_key((DES_cblock *)key1, &ctx->des_key_1);
    DES_set_key((DES_cblock *)key2, &ctx->des_key_2);
    DES_set_key((DES_cblock *)key3, &ctx->des_key_3);

    return ctx;
}

int des_encrypt(des_ctx_t* ctx,
		const char* src_buf, int src_len, char* enc_buf, int* enc_len)
{
	if(!ctx) {
		return -1;
	}
    int data_len = src_len;
    int data_rest = data_len % 8;
    int group_cnt = src_len / 8;

    if(*enc_len < src_len + 8) {
    	return -1;
    }

    *enc_len = 0;
    
    char orgin_buf[src_len + 8];
    memset(orgin_buf, 0, src_len + 8);
    memcpy(orgin_buf, src_buf, src_len);

    unsigned char ch = 8 - data_rest;
    memset((char *) (orgin_buf + data_len), ch, 8 - data_rest);
    memset((char *) (orgin_buf + data_len + (8 - data_rest)), '\0', 1);
    group_cnt += 1;


    int i = 0;
    for (i = 0; i< group_cnt; i ++)
    {
        DES_ecb3_encrypt((DES_cblock *)(orgin_buf + (i*8)),
            (DES_cblock *)(enc_buf + (i*8) ),
            &ctx->des_key_1, &ctx->des_key_2,
            &ctx->des_key_3, DES_ENCRYPT);

        *enc_len += 8;
    }

    return 0;
}
int des_decrypt(des_ctx_t* ctx,
		const char* src_buf, int src_len, char* dec_buf, int* dec_len)
{
	if(!ctx) {
		return -1;
	}

	if(*dec_len < src_len) {
		return -1;
	}

    int group_cnt = src_len/8;
    *dec_len = 0;
    int i = 0;
    for (i = 0; i < group_cnt; i ++)
    {
        DES_ecb3_encrypt((DES_cblock *)(src_buf + (i*8)),
            (DES_cblock *)(dec_buf + (i*8) ),
            &ctx->des_key_1, &ctx->des_key_2,
            &ctx->des_key_2, DES_DECRYPT);
        *dec_len += 8;
    }

    unsigned char ch = dec_buf[*dec_len -1];
    int idx = *dec_len - ch;
    for(; idx < *dec_len; idx++){
        if((unsigned char)dec_buf[idx] != ch){
            break;
        }
    }
    if (idx == *dec_len) {
    	*dec_len -= ch;
    }
    
    dec_buf[*dec_len] = 0;

    return 0;
}

int desc_uninit(des_ctx_t* ctx)
{
	if(ctx) {
		free(ctx);
	}

	return 0;
}
