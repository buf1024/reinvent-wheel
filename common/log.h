/*
 * log.h
 *
 *  Created on: 2012-5-9
 *      Author: buf1024@gmail.com
 */

#ifndef __LOG_H__
#define __LOG_H__

#include <stdarg.h>

#define LOG_DEBUG(...)                                               \
do{                                                                  \
    log_debug(__VA_ARGS__);                                          \
}while(0)                                                            \

#define LOG_INFO(...)                                                \
do{                                                                  \
    log_info(__VA_ARGS__);                                           \
}while(0)                                                            \

#define LOG_WARN(...)                                                \
do{                                                                  \
    log_warn(__VA_ARGS__);                                           \
}while(0)                                                            \

#define LOG_ERROR(...)                                               \
do{                                                                  \
    log_error(__VA_ARGS__);                                          \
}while(0)                                                            \

#define LOG_FATAL(...)                                               \
do{                                                                  \
    log_fatal(__VA_ARGS__);                                          \
}while(0)                                                            \

#define LOG_FINISH() log_finish()
#define LOG_FLUSH()  log_flush()

#define LOG_FAIL                             -1
#define LOG_SUCCESS                           0

#ifdef __cplusplus
extern "C" {
#endif

enum LogLevel
{
    LOG_ALL     = 0,
    LOG_DEBUG   = 100,
    LOG_INFO    = 200,
    LOG_WARN    = 300,
    LOG_ERROR   = 400,
    LOG_FATAL   = 500,
    LOG_OFF     = 600
};


int  log_init(int con_lvl, int file_lvl,
        const char* file_path, const char* prefix, int buf_size,
        int sw_time, int sw_size);
int  log_finish();

void log_flush();

void log_debug(const char* format, ...);
void log_info (const char* format, ...);
void log_warn (const char* format, ...);
void log_error(const char* format, ...);
void log_fatal(const char* format, ...);


int  log_get_level(const char* lvl);

#ifdef __cplusplus
}
#endif

#endif /* __LOG_H__ */
