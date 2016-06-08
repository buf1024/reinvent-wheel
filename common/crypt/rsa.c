/*
 * rsa.c
 *
 *  Created on: 2013-7-19
 *      Author: Luo Guochun
 */

#include "rsa.h"

#include <string.h>
#include <stdbool.h>

#include <openssl/ossl_typ.h>
#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/des.h>

typedef struct rsa_ctx_s
{
    RSA* rsa_pub;
    EVP_PKEY* evp_pub;

    RSA* rsa_priv;
    EVP_PKEY* evp_priv;

}rsa_ctx_t;

static int rsa_sign(rsa_ctx_t* ctx, const EVP_MD* ev,
		const char* src_buf, int src_len, char* dest_buf, int* dest_len)
{
	if(!ctx) {
		return -1;
	}
    RSA* rsa = ctx->rsa_priv;
    EVP_PKEY* evp_key = ctx->evp_priv;

    unsigned char sign_value[512] = "";
    unsigned int sign_len = 0;
    EVP_MD_CTX mdctx;

    if (rsa == NULL || evp_key == NULL) {
        printf("rsa = NULL or evp_key = NULL\n");
        return -1;
    }

	EVP_MD_CTX_init(&mdctx);
	if (!EVP_SignInit_ex(&mdctx, ev, NULL)) {
		printf("EVP_SignInit_ex error\n");
		EVP_MD_CTX_cleanup(&mdctx);
		return -1;
	}
	if (!EVP_SignUpdate(&mdctx, src_buf, src_len)) {
		printf("EVP_SignUpdate error\n");
		EVP_MD_CTX_cleanup(&mdctx);
		return -1;
	}
	if (!EVP_SignFinal(&mdctx, sign_value, &sign_len, evp_key)) {
		printf("EVP_SignFinal error\n");
		EVP_MD_CTX_cleanup(&mdctx);
		return -1;
	}
	EVP_MD_CTX_cleanup(&mdctx);
	if (sign_len > (unsigned int) *dest_len) {
		printf("buffer too small.\n");
		return -1;
	}
	memcpy(dest_buf, sign_value, sign_len);
	*dest_len = sign_len;

	return 0;
}

static int rsa_verify(rsa_ctx_t* ctx, const EVP_MD *ev,
		const char* src_buf, int src_len,
        const char* sig_buf, int sig_len)
{
    RSA* rsa = ctx->rsa_pub;
    EVP_PKEY* evp_key = ctx->evp_pub;

    int ret = 0;

    if (rsa == NULL || evp_key == NULL) {
        printf("rsa = NULL or evp_key = NULL\n");
        return -1;
    }

    EVP_MD_CTX mdctx;

    EVP_MD_CTX_init(&mdctx);
    if (!EVP_VerifyInit_ex(&mdctx, ev, NULL))
    {
        printf("EVP_VerifyInit_ex err\n");
        EVP_MD_CTX_cleanup(&mdctx);
        return -1;
    }
    if (!EVP_VerifyUpdate(&mdctx, src_buf, src_len))
    {
        printf("EVP_VerifyUpdate error\n");
        EVP_MD_CTX_cleanup(&mdctx);
        return -1;
    }
    ret = EVP_VerifyFinal(&mdctx, (unsigned char *) sig_buf, sig_len, evp_key);

    if (ret < 0) {
        printf("EVP_VerifyFinal failed.\n");
        EVP_MD_CTX_cleanup(&mdctx);
        return -1;
    } else {
        if (ret > 0) {
            printf("rsa_verify success.\n");
        } else {
            printf("rsa_verify failed.\n");
        }
    }
    EVP_MD_CTX_cleanup(&mdctx);
    return ret;
}



rsa_ctx_t* rsa_init(const char* rsa_pub_key_file, const char* rsa_pri_key_file)
{
	static bool algo = false;
	if(!algo) {
        OpenSSL_add_all_algorithms();
        algo = true;
	}

    rsa_ctx_t* ctx = (rsa_ctx_t*)malloc(sizeof(*ctx));
    if(!ctx) {
    	return NULL;
    }

    FILE* fp = NULL;

    // RSA PUBLIC
    if ((fp = fopen(rsa_pub_key_file, "r")) == NULL) {
        printf("open public key file %s failed.\n", rsa_pub_key_file);
        return NULL;
    }

    ctx->rsa_pub = PEM_read_RSA_PUBKEY(fp, NULL, NULL, NULL);
    if(ctx->rsa_pub == NULL) {
        printf("PEM_read_RSA_PUB_KEY error\n");
        fclose(fp);
        return NULL;
    }
    fclose(fp);

    ctx->evp_pub = EVP_PKEY_new();
    if (ctx->evp_pub == NULL) {
        printf("EVP_PKEY_new error\n");
        RSA_free(ctx->rsa_pub);
        return NULL;
    }

    if (EVP_PKEY_set1_RSA(ctx->evp_pub, ctx->rsa_pub) != 1){
        printf("EVP_PKEY_set1_RSA error\n");
        RSA_free(ctx->rsa_pub);
        EVP_PKEY_free(ctx->evp_pub);
        return NULL;
    }

    // RSA PRIV
    if ((fp = fopen(rsa_pri_key_file, "r")) == NULL) {
        printf("open rsa private key file %s failed..\n",
                rsa_pri_key_file);
        RSA_free(ctx->rsa_pub);
        EVP_PKEY_free(ctx->evp_pub);
        return NULL;
    }

    ctx->rsa_priv = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL);
    if(ctx->rsa_priv == NULL) {
        printf("PEM_read_RSA_PRIVATE_KEY error\n");
        fclose(fp);

        RSA_free(ctx->rsa_pub);
        EVP_PKEY_free(ctx->evp_pub);
        return NULL;
    }
    fclose(fp);

    ctx->evp_priv = EVP_PKEY_new();
    if (ctx->evp_priv == NULL) {
        printf("EVP_PKEY_new error\n");
        RSA_free(ctx->rsa_priv);

        RSA_free(ctx->rsa_pub);
        EVP_PKEY_free(ctx->evp_pub);
        return NULL;
    }

    if (EVP_PKEY_set1_RSA(ctx->evp_priv, ctx->rsa_priv) != 1){
        printf("EVP_PKEY_set1_RSA error\n");
        RSA_free(ctx->rsa_priv);
        EVP_PKEY_free(ctx->evp_priv);

        RSA_free(ctx->rsa_pub);
        EVP_PKEY_free(ctx->evp_pub);
        return NULL;
    }
    return ctx;
}

int rsa_md5_sign(rsa_ctx_t* ctx,
		const char* src_buf, int src_len, char* dest_buf, int* dest_len)
{
	return rsa_sign(ctx, EVP_md5(), src_buf, src_len, dest_buf, dest_len);
}
int rsa_md5_verify(rsa_ctx_t* ctx,
		const char* src_buf, int src_len,
        const char* sig_buf, int sig_len)
{
	return rsa_verify(ctx, EVP_md5(), src_buf, src_len, sig_buf, sig_len);
}

int rsa_sha1_sign(rsa_ctx_t* ctx,
		const char* src_buf, int src_len, char* dest_buf, int* dest_len)
{
	return rsa_sign(ctx, EVP_sha1(), src_buf, src_len, dest_buf, dest_len);
}
int rsa_sha1_verify(rsa_ctx_t* ctx,
		const char* src_buf, int src_len,
        const char* sig_buf, int sig_len)
{
	return rsa_verify(ctx, EVP_sha1(), src_buf, src_len, sig_buf, sig_len);
}

int rsa_uninit(rsa_ctx_t* ctx)
{
    if (ctx->rsa_priv) {
        RSA_free(ctx->rsa_priv);
    }
    if (ctx->evp_priv) {
        EVP_PKEY_free(ctx->evp_priv);
    }

    if (ctx->rsa_pub) {
        RSA_free(ctx->rsa_pub);
    }
    if (ctx->evp_pub) {
        EVP_PKEY_free(ctx->evp_pub);
    }

	static bool algo = false;
	if(!algo) {
		EVP_cleanup();
        algo = true;
	}

    return 0;
}


