#ifndef __BASE64_H__
#define __BASE64_H__

#include <stddef.h>

#define base64_encode_max_size(size) ((size) / 3 * 4 + 2 + 2)
#define base64_decode_max_size(size) ((size) / 4 * 3 + 3)

#ifdef __cplusplus
extern "C" {
#endif

int base64_encode(const unsigned char* src, size_t src_len, unsigned char* dst, size_t dst_len);
int base64_decode(const unsigned char *src, size_t src_len, unsigned char* dst, size_t dst_len);

#ifdef __cplusplus
}
#endif

#endif /* __BASE64_H__ */
