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


#ifdef __cplusplus
}
#endif

#endif /* __TSON_H__ */
