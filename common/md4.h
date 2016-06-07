
#ifndef __MD4_H__
#define __MD4_H__

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	MD4_RESULTLEN (128/8)

struct md4_context {
	uint_fast32_t lo, hi;
	uint_fast32_t a, b, c, d;
	unsigned char buffer[64];
	uint_fast32_t block[MD4_RESULTLEN];
};

void md4_init(struct md4_context *ctx);
void md4_update(struct md4_context *ctx, const void *data, size_t size);
void md4_final(struct md4_context *ctx, unsigned char result[MD4_RESULTLEN]);

void md4_get_digest(const void *data, size_t size,
		    unsigned char result[MD4_RESULTLEN]);

#ifdef __cplusplus
}
#endif

#endif /* __MD4_H__ */
