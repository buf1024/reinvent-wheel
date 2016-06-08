/*
 * iniconf.h
 *
 *  Created on: 2014-1-22
 *      Author: Luo Guochun
 */

#ifndef __INICONF_H__
#define __INICONF_H__

#include "dict.h"

/*this implementation don't care about the performance*/

#ifdef __cplusplus
extern "C" {
#endif

typedef dict ini_conf_t;
typedef dict ini_sec_t;

int ini_uninit(ini_conf_t* ini);
ini_conf_t* ini_init();

int ini_load(ini_conf_t* ini, const char* file);
int ini_save(ini_conf_t* ini, const char* file);

int ini_dump(ini_conf_t* ini, char* buf, size_t len);

ini_sec_t* ini_insert_sec(ini_conf_t* ini, const char* key);
int ini_delete_sec(ini_conf_t* ini, const char* key);
ini_sec_t* ini_get_sec(ini_conf_t* ini, const char* key);

int ini_insert_val(ini_sec_t* sec, const char* key, const char* val);
int ini_delete_val(ini_sec_t* sec, const char* key);
const char* ini_get_val(ini_sec_t* sec, const char* key);

#ifdef __cplusplus
}
#endif

#endif /* __INICONF_H__ */
