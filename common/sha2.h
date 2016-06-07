
#ifndef __SHA2_H__
#define __SHA2_H__

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHA256_RESULTLEN (256 / 8)
#define SHA256_BLOCK_SIZE (512 / 8)

#define SHA512_RESULTLEN (512 / 8)
#define SHA512_BLOCK_SIZE (1024 / 8)

struct sha256_ctx {
	size_t tot_len;
	size_t len;
	unsigned char block[2 * SHA256_BLOCK_SIZE];
	uint32_t h[8];
};

struct sha512_ctx {
	size_t tot_len;
	size_t len;
	unsigned char block[2 * SHA512_BLOCK_SIZE];
	uint64_t h[8];
};

void sha256_init(struct sha256_ctx *ctx);
void sha256_loop(struct sha256_ctx *ctx, const void *data, size_t len);
void sha256_result(struct sha256_ctx *ctx,
		   unsigned char digest[SHA256_RESULTLEN]);

void sha256_get_digest(const void *data, size_t size,
		       unsigned char digest[SHA256_RESULTLEN]);

void sha512_init(struct sha512_ctx *ctx);
void sha512_loop(struct sha512_ctx *ctx, const void *data, size_t len);
void sha512_result(struct sha512_ctx *ctx,
		   unsigned char digest[SHA512_RESULTLEN]);

void sha512_get_digest(const void *data, size_t size,
		       unsigned char digest[SHA512_RESULTLEN]);

extern const struct hash_method hash_method_sha256;
extern const struct hash_method hash_method_sha512;

#ifdef __cplusplus
}
#endif

#endif /* __SHA2_H__ */
