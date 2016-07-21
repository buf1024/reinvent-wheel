/*
 * simplehttpd.h
 *
 *  Created on: 2016/5/13
 *      Author: Luo Guochun
 */

#ifndef __SIMPLEHTTPD_H__
#define __SIMPLEHTTPD_H__

#define _GNU_SOURCE

typedef struct http_request_s http_request_t;
typedef struct http_response_s http_response_t;
typedef struct httpd_s httpd_t;


enum {
	THIN_HTTPD_RUN_TEST,
	THIN_HTTPD_RUN_NORMAL,
	THIN_HTTPD_RUN_EXIT,
};

struct http_request_s
{

};

struct http_response_s
{

};

struct httpd_s
{
	char* conf;
};




#endif /* __SIMPLEHTTPD_H__ */
