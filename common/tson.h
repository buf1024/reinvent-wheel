/*
 * tson.h
 *
 *  Created on: 2013-7-3
 *      Author: Luo Guochun
 */

#ifndef __TSON_H__
#define __TSON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct tson_s     tson_t;
typedef struct tson_arr_s tson_arr_t;

struct tson_arr_s
{
	char*  value;
	tson_t* tson;
};


tson_t* tson_parse(const char* str);
tson_t* tson_parse_path(const char* path);

tson_t* tson_new();
int tson_free(tson_t* tson);

int tson_dump(tson_t* tson, char* buf, int size);

int tson_get(tson_t* tson, const char* key, char** value, tson_t** sub_tson);
int tson_set(tson_t* tson, const char* key, const char* value, tson_t* sub_tson);
int tson_get_arr(tson_t* tson, const char* key, tson_arr_t** arr);

int tson_get_bool(tson_t* tson, const char* key, bool* value);
int tson_get_double(tson_t* tson, const char* key, double* value);
int tson_get_int(tson_t* tson, const char* key, int* value);
int tson_get_long(tson_t* tson, const char* key, long* value);
int tson_get_string(tson_t* tson, const char* key, char** value);


#ifdef __cplusplus
}
#endif

#endif /* __TSON_H__ */
