/*
 * simplehttpd.h
 *
 *  Created on: 2016/5/13
 *      Author: Luo Guochun
 */

#ifndef __SIMPLEHTTPD_H__
#define __SIMPLEHTTPD_H__

#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>

typedef struct http_request_s http_request_t;
typedef struct http_response_s http_response_t;
typedef struct httpd_s httpd_t;

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
