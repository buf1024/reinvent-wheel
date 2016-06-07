
#ifndef __MD5_H__
#define __MD5_H__

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	MD5_RESULTLEN (128/8)

struct md5_context {
	uint_fast32_t lo, hi;
	uint_fast32_t a, b, c, d;
	unsigned char buffer[64];
	uint_fast32_t block[MD5_RESULTLEN];
};

void md5_init(struct md5_context *ctx);
void md5_update(struct md5_context *ctx, const void *data, size_t size);
void md5_final(struct md5_context *ctx, unsigned char result[MD5_RESULTLEN]);

void md5_get_digest(const void *data, size_t size,
		    unsigned char result[MD5_RESULTLEN]);

#ifdef __cplusplus
}
#endif

#endif /* __MD5_H__ */
