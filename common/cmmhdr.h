/*
 *
 * author: heidonglgc@126.com
 * create: 2015-08-30 15:01:34
 */

#ifndef _CMMHDR_H_
#define _CMMHDR_H_

#ifdef __cplusplus

#define CPP_NS_BEGIN extern "C" {
#define CPP_NS_END }

#else

#define CPP_NS_BEGIN
#define CPP_NS_END

#endif

/* c constructor */
#ifdef _MSC_VER
#define __CCALL __cdecl
#define __func__ __FUNCTION__
#define snprintf _snprintf
#pragma warning(disable:4996) // this is very violent
#pragma section(".CRT$XCU",read)
#define __CONSTRCT(a, b)                                                      \
        void __CCALL __##a##__##b##__ ## 520hjm(void);                       \
    __declspec(allocate(".CRT$XCU"))                                          \
    void (__CCALL * __ctor__##a##__##b##__ ## 520hjm)(void) =                \
    __##a##__##b##__ ## 520hjm;

#else

#define __CCALL
#define __CONSTRCT(a, b) __attribute__((constructor))

#endif /* c constructor _MSC_VER */

#ifndef offset
#define offset(type, field)                                                   \
    ((size_t)&(((type*)(0))->field))
#endif /* offset */

#ifndef containerof
#define containerof(ptr, type, field)                                         \
    ((type *)((char *)(ptr) - offset(type, field)))
#endif /* containerof */


#define LF     ((unsigned char) 10)
#define CR     ((unsigned char) 13)
#define CRLF   "\x0d\x0a"

#ifndef ABS
#define ABS(a)       (((a) >= 0) ? (a) : - (a))
#endif /*abs*/

#ifndef MAX
#define MAX(a, b) ((a)>(b)?(a):(b))
#endif /* MAX */

#ifndef MIN
#define MIN(a, b) ((a)>(b)?(b):(a))
#endif /* MIN */

#ifndef UNUSED
#define UNUSED __attribute__((unused))
#endif

#endif /* _CMMHDR_H_ */

