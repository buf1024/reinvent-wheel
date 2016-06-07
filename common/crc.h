/*
 * crc.h
 *
 *  Created on: 2014-1-23
 *      Author: Luo Guochun
 */

#ifndef __CRC_H__
#define __CRC_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t crc16(uint16_t crc, const unsigned char* buf, size_t len);
uint32_t crc32(uint32_t crc, const unsigned char* buf, size_t len);
uint64_t crc64(uint64_t crc, const unsigned char* buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* __CRC_H__ */
