
#ifndef __SHA1_H__
#define __SHA1_H__

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHA1_RESULTLEN  (160/8)

struct sha1_ctxt {
	union {
		uint8_t	b8[20];
		uint32_t b32[5];
	} h;
	union {
		uint8_t	b8[8];
		uint64_t b64[1];
	} c;
	union {
		uint8_t	b8[64];
		uint32_t b32[16];
	} m;
	uint8_t	count;

	int endian : 1;
};

void sha1_init(struct sha1_ctxt *);
void sha1_pad(struct sha1_ctxt *);
void sha1_loop(struct sha1_ctxt *, const void *, size_t);
void sha1_result(struct sha1_ctxt *, void *);

void sha1_get_digest(const void *, size_t,
	unsigned char [SHA1_RESULTLEN]);

#ifdef __cplusplus
}
#endif

#endif  /* __SHA1_H__ */
