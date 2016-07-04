/*
 * misc.h
 *
 *  Created on: 2014-1-23
 *      Author: Luo Guochun
 */

#ifndef __MISC_H__
#define __MISC_H__

#include <unistd.h> // pid_t
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define REGISTER_SIGNAL(signo, func, interupt)            \
    do {                                                  \
        if(set_signal(signo, func, interupt) == SIG_ERR){ \
            printf("register signal %d failed:%s\n",      \
                    signo, strerror(errno));              \
            return -1;                                    \
        }                                                 \
    }while(0)


typedef void sigfunc(int);
sigfunc* set_signal(int signo, sigfunc* func, int interupt);

int daemonlize();
int is_runas_root();
int is_running(const char* name);
int runas(const char* user);

int write_pid(const char* file);
int read_pid(pid_t* pid, const char* file);

// hash
uint32_t murmur(unsigned char *data, size_t len);
unsigned short get_cpu_count(void);

// string
int match(const char* text, const char* pattern);
int split(const char* text, char needle, char** dest, int size, int num);
const char* trim_left(const char* text, const char* needle, char* dest, int size);
const char* trim_right(const char* text, const char* needle, char* dest, int size);
const char* trim(const char* text, const char* needle, char* dest, int size);
int get_hex(char ch);
int to_hex(const char* src, size_t src_len, char* dst, size_t dst_len);
int to_upper(const char* src, size_t src_len, char* dst, size_t dst_len);
int to_lower(const char* src, size_t src_len, char* dst, size_t dst_len);

#ifdef __cplusplus
}
#endif

#endif /* __MISC_H__ */
