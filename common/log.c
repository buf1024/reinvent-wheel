/*
 * log.c
 *
 *  Created on: 2012-5-9
 *      Author: buf1024@gmail.com
 */

#include "log.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

#define LOG_DEFAULT_HEAD_SIZE   128
#define LOG_DEFAULT_BUFFER_SIZE 4096
#define LOG_DEFAULT_MAX_PATH    256
#define LOG_ONEDAY_SECONDS      86400

#ifdef LOG_THREAD_SAFE

static pthread_mutex_t     _ctx_mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOG_MUTEX_INIT()  memset(&_ctx_mutex, 0, sizeof(_ctx_mutex))
#define LOG_LOCK()   pthread_mutex_lock(&_ctx_mutex)
#define LOG_UNLOCK() pthread_mutex_unlock(&_ctx_mutex)

#else

#define LOG_MUTEX_INIT()
#define LOG_LOCK()
#define LOG_UNLOCK()

#endif //LOG_THREAD_SAFE

struct file_log
{
    char  path[LOG_DEFAULT_MAX_PATH];
    char  prefix[LOG_DEFAULT_MAX_PATH];
    char  name[LOG_DEFAULT_MAX_PATH];

    int   sw_day;

    FILE* fp;
    int   file_count;
    int   switch_time;
    int   switch_size;

    char* buf;
    int   buf_size;
    int   w_size;
    int   w_pos;

    int   level;
};

struct console_log
{
	int level;
};


static int log_init_file(int file_lvl,
        char* file_path, char* prefix, int buf_size,
        int sw_time, int sw_size);
static int log_file_flush();
static int log_file_uninit();
static int log_file_msg(const char* log, int len);
static int log_file_switch();
static int log_file_init();
static int log_console_msg(const char* log, int len);
static int log_init_file(int file_lvl,
        char* file_path, char* prefix, int buf_size,
        int sw_time, int sw_size);
static int log_get_header(int lvl, char* buf, int size);
static void log_message(int lvl, const char* format, va_list ap);

static struct file_log  _file_log_ctx = { {0} };
static struct console_log _console_log_ctx = { 0 };

int log_get_header(int lvl, char* buf, int size)
{
    const char* head = 0;
    switch(lvl)
    {
    case LOG_DEBUG:
        head = "[D]";
        break;
    case LOG_INFO:
        head = "[I]";
        break;
    case LOG_WARN:
        head = "[W]";
        break;
    case LOG_ERROR:
        head = "[E]";
        break;
    case LOG_FATAL:
        head = "[F]";
        break;
    default:
        break;
    }
    struct timeval tv = {0};
    gettimeofday(&tv, NULL);
    struct tm* tm = localtime(&tv.tv_sec);
#if 0
    int ret = snprintf(buf, size, "%s[%04d%02d%02d%02d%02d%02d:%ld]", head,
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec, (long)tv.tv_usec);
#endif

    int ret = snprintf(buf, size, "%s[%02d%02d%02d:%ld]", head,
            tm->tm_hour, tm->tm_min, tm->tm_sec, (long)tv.tv_usec);

    return ret;
}

void log_message(int lvl, const char* format, va_list ap)
{
	int len = 0;
	char msg[LOG_DEFAULT_BUFFER_SIZE] = {0};


	char head[LOG_DEFAULT_HEAD_SIZE] = { 0 };
	log_get_header(lvl, head, LOG_DEFAULT_HEAD_SIZE);

	char log[LOG_DEFAULT_BUFFER_SIZE] = { 0 };
	vsnprintf(log, LOG_DEFAULT_BUFFER_SIZE, format, ap);

	len = snprintf(msg, LOG_DEFAULT_BUFFER_SIZE, "%s%s", head, log);

	if(lvl >= _console_log_ctx.level) {
		log_console_msg(msg, len);
	}

	if(lvl >= _file_log_ctx.level) {
		if (log_file_msg(msg, len) == LOG_FAIL) {
			fprintf(stderr, "fail to log message!\n");
		}
	}
}

int log_get_level(const char* lvl)
{
    if (lvl) {
        if (strcasecmp(lvl, "all") == 0) {
            return LOG_ALL;
        } else if (strcasecmp(lvl, "debug") == 0) {
            return LOG_DEBUG;
        } else if (strcasecmp(lvl, "info") == 0) {
            return LOG_INFO;
        } else if (strcasecmp(lvl, "warn") == 0) {
            return LOG_WARN;
        } else if (strcasecmp(lvl, "error") == 0) {
            return LOG_ERROR;
        } else if (strcasecmp(lvl, "fatal") == 0) {
            return LOG_FATAL;
        } else if (strcasecmp(lvl, "off") == 0) {
            return LOG_OFF;
        }
    }

    return LOG_ALL;
}


void log_clearup()
{
   LOG_MUTEX_INIT();
}


void log_flush()
{

	LOG_LOCK();
    log_file_flush();
    LOG_UNLOCK();
}

void log_debug(const char* format, ...)
{
    va_list ap;

    LOG_LOCK();
    va_start(ap, format);
    log_message(LOG_DEBUG, format, ap);
    va_end(ap);
    LOG_UNLOCK();
}
void log_info (const char* format, ...)
{
    va_list ap;

    LOG_LOCK();
    va_start(ap, format);
    log_message(LOG_INFO, format, ap);
    va_end(ap);
    LOG_UNLOCK();
}
void log_warn (const char* format, ...)
{
    va_list ap;

    LOG_LOCK();
    va_start(ap, format);
    log_message(LOG_WARN, format, ap);
    va_end(ap);
    LOG_UNLOCK();
}
void log_error(const char* format, ...)
{
    va_list ap;

    LOG_LOCK();
    va_start(ap, format);
    log_message(LOG_ERROR, format, ap);
    va_end(ap);
    LOG_UNLOCK();
}
void log_fatal(const char* format, ...)
{
    va_list ap;

    LOG_LOCK();
    va_start(ap, format);
    log_message(LOG_FATAL, format, ap);
    va_end(ap);
    LOG_UNLOCK();
}

int log_finish()
{
    log_flush();

    LOG_LOCK();

    if(log_file_uninit() != LOG_SUCCESS) {
    	LOG_UNLOCK();
    	return LOG_FAIL;
    }
    LOG_UNLOCK();

    return LOG_SUCCESS;
}
int log_init(int con_lvl, int file_lvl,
        char* file_path, char* prefix, int buf_size,
        int sw_time, int sw_size)
{
    log_clearup();

    _console_log_ctx.level = con_lvl;

    if (log_init_file(file_lvl, file_path,
            prefix, buf_size, sw_time, sw_size) == LOG_FAIL) {
        return LOG_FAIL;
    }

    return LOG_SUCCESS;
}


int log_console_msg(const char* log, int len)
{
    printf("%s", log);
    return LOG_SUCCESS;
}


int log_init_file(int file_lvl,
        char* file_path, char* prefix, int buf_size,
        int sw_time, int sw_size)
{
    if(!file_path || !prefix || buf_size <= 0) {
        return LOG_FAIL;
    }

    memset(&_file_log_ctx, 0, sizeof(_file_log_ctx));

    strncpy(_file_log_ctx.path, file_path, sizeof(_file_log_ctx.path) - 1);
    if(_file_log_ctx.path[strlen(_file_log_ctx.path) - 1] == '/') {
        _file_log_ctx.path[strlen(_file_log_ctx.path) - 1] = 0;
    }

    strncpy(_file_log_ctx.prefix, prefix, sizeof(_file_log_ctx.prefix) - 1);
    _file_log_ctx.buf_size = buf_size;
    _file_log_ctx.switch_size = sw_size;
    _file_log_ctx.switch_time = sw_time % LOG_ONEDAY_SECONDS;
    _file_log_ctx.buf = malloc(sizeof(char)*buf_size);
    _file_log_ctx.level = file_lvl;

    if(!_file_log_ctx.buf) {
        return LOG_FAIL;
    }
    memset(_file_log_ctx.buf, 0, sizeof(char)*buf_size);

    return log_file_init();
}


int log_file_init()
{
    FILE* fp = _file_log_ctx.fp;
    char* rst = NULL;
    int rst_size = 0;
    if(fp != NULL){
        if(_file_log_ctx.buf && _file_log_ctx.w_pos > 0) {
            rst_size = _file_log_ctx.w_pos;
            rst = (char*)malloc(_file_log_ctx.w_pos);
            if(!rst) {
                return LOG_FAIL;
            }
            memcpy(rst, _file_log_ctx.buf, _file_log_ctx.w_pos);
        }

        fclose(fp);

        char old_path[LOG_DEFAULT_MAX_PATH] = {0};
        char new_path[LOG_DEFAULT_MAX_PATH] = {0};

        snprintf(old_path, LOG_DEFAULT_MAX_PATH, "%s/%s.tmp",
                _file_log_ctx.path, _file_log_ctx.name);

        snprintf(new_path, LOG_DEFAULT_MAX_PATH, "%s/%s.log",
                        _file_log_ctx.path, _file_log_ctx.name);

        if(rename(old_path, new_path) != 0) {
            fprintf(stderr, "rename %s - > %s failed: %s\n", old_path, new_path, strerror(errno));
        }

        _file_log_ctx.fp = NULL;
        memset(_file_log_ctx.name, 0, LOG_DEFAULT_MAX_PATH);
        _file_log_ctx.file_count += 1;
        _file_log_ctx.w_size = 0;
        _file_log_ctx.w_pos = 0;
    }

    char file[LOG_DEFAULT_MAX_PATH] = {0};
    pid_t pid = getpid();

    struct timeval tv = {0};
    gettimeofday(&tv, NULL);
    struct tm* tm = localtime(&tv.tv_sec);

    snprintf(_file_log_ctx.name, LOG_DEFAULT_MAX_PATH, "%s_%d_%04d%02d%02d_%d",
            _file_log_ctx.prefix, pid,
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            _file_log_ctx.file_count);

    snprintf(file, LOG_DEFAULT_MAX_PATH, "%s/%s.tmp",
            _file_log_ctx.path, _file_log_ctx.name);

    fp = fopen(file, "a+");
    if(fp == NULL){
        return LOG_FAIL;
    }
    _file_log_ctx.fp = fp;

    if(rst) {
        fwrite(rst, 1, rst_size, _file_log_ctx.fp);
        free(rst);
    }

    return LOG_SUCCESS;
}

int log_file_switch()
{
    int sw_log = 0;

    if (_file_log_ctx.switch_time >= 0) {
        struct timeval tv = { 0 };
        gettimeofday(&tv, NULL );
        struct tm* tm = localtime(&tv.tv_sec);

        if(tm->tm_mday != _file_log_ctx.sw_day && _file_log_ctx.sw_day != 0) {
            int seconds = tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec;
            if (seconds >= _file_log_ctx.switch_time) {
                sw_log = 1;
                _file_log_ctx.sw_day = tm->tm_mday;
            }
        }
    }
    if (!sw_log) {
        if (_file_log_ctx.switch_size > 0
                && _file_log_ctx.w_size >= _file_log_ctx.switch_size) {
            sw_log = 1;
        }
    }
    if (sw_log) {
        if (log_file_init() != LOG_SUCCESS) {
            return LOG_FAIL;
        }
    }

    return LOG_SUCCESS;
}

int log_file_msg(const char* log, int len)
{
    FILE* fp = _file_log_ctx.fp;
    if (fp == NULL ) {
        return LOG_FAIL;
    }

    if(log_file_switch() != LOG_SUCCESS) {
        return LOG_FAIL;
    }

    int m_size = len;

    if (m_size > (_file_log_ctx.buf_size - _file_log_ctx.w_pos)) {
        if (_file_log_ctx.w_pos > 0) {
            _file_log_ctx.w_size += fwrite(_file_log_ctx.buf, 1,
                    _file_log_ctx.w_pos, _file_log_ctx.fp);
            _file_log_ctx.w_pos = 0;
        }
        _file_log_ctx.w_size += fwrite(log, 1, m_size, _file_log_ctx.fp);
    } else {
        memcpy(_file_log_ctx.buf + _file_log_ctx.w_pos, log, m_size);
        _file_log_ctx.w_pos += m_size;
        _file_log_ctx.w_size += m_size;
    }
    return LOG_SUCCESS;
}
int log_file_uninit()
{
    FILE* fp = _file_log_ctx.fp;
    char* buf = _file_log_ctx.buf;
    if(fp) {
        if(log_file_switch() != LOG_SUCCESS) {
            return LOG_FAIL;
        }

        if(_file_log_ctx.buf && _file_log_ctx.w_pos > 0) {
            fwrite(_file_log_ctx.buf, 1,
                    _file_log_ctx.w_pos, _file_log_ctx.fp);
        }

        fclose(fp);

        char old_path[LOG_DEFAULT_MAX_PATH] = {0};
        char new_path[LOG_DEFAULT_MAX_PATH] = {0};

        snprintf(old_path, LOG_DEFAULT_MAX_PATH, "%s/%s.tmp",
                _file_log_ctx.path, _file_log_ctx.name);

        snprintf(new_path, LOG_DEFAULT_MAX_PATH, "%s/%s.log",
                        _file_log_ctx.path, _file_log_ctx.name);

        if(rename(old_path, new_path) != 0) {
            fprintf(stderr, "rename %s - > %s failed: %s\n", old_path, new_path, strerror(errno));
        }
    }
    if(buf) {
        free(buf);
    }
    memset(&_file_log_ctx, 0, sizeof(_file_log_ctx.buf));

    return LOG_SUCCESS;
}

int log_file_flush()
{
    FILE* fp = _file_log_ctx.fp;
    if(fp) {
        if(log_file_switch() != LOG_SUCCESS) {
            return LOG_FAIL;
        }
        if(_file_log_ctx.buf && _file_log_ctx.w_pos > 0) {
            _file_log_ctx.w_size += fwrite(_file_log_ctx.buf, 1,
                    _file_log_ctx.w_pos, _file_log_ctx.fp);
            _file_log_ctx.w_pos = 0;

            fflush(fp);
        }
    }
    return LOG_SUCCESS;
}
