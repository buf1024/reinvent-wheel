/*
 * misc.h
 *
 *  Created on: 2014-1-23
 *      Author: Luo Guochun
 */

#ifndef __MISC_H__
#define __MISC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h> // pid_t

typedef void sigfunc(int);
sigfunc* set_signal(int signo, sigfunc* func, int interupt);

int make_daemon();
int is_prog_runas_root();
int is_prog_running(const char* name);
int runas(const char* user);

int write_pid_file(const char* file);
int read_pid_file(pid_t* pid, const char* file);

int match(const char* text, const char* pattern);

int split(const char* text, char needle, char** dest, int size, int num);

const char* trim_left(const char* text, const char* needle, char* dest, int size);
const char* trim_right(const char* text, const char* needle, char* dest, int size);
const char* trim(const char* text, const char* needle, char* dest, int size);

#ifdef __cplusplus
}
#endif

#endif /* __MISC_H__ */
