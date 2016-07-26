/*
 * mod_test.c
 *
 *  Created on: 2016/7/25
 *      Author: Luo Guochun
 */

#include "httpd.h"

static int mod_test_loadconf(httpd_t* http UNUSED, tson_t* t UNUSED)
{

	return 0;
}

static int mod_test_init(httpd_t* http UNUSED, tson_t* t UNUSED)
{

	return 0;
}
static int mod_test_uninit(httpd_t* http UNUSED)
{
	return 0;
}

static int mod_test_handle(httpd_t* http UNUSED, http_request_t* req UNUSED, http_response_t* rsp UNUSED)
{
	return HTTP_STATUS_OK;
}

http_module_t* http_mod_test()
{
	static http_module_t mod = {
			.load_conf = mod_test_loadconf,
			.init   = mod_test_init,
			.uninit = mod_test_uninit,
			.handle = mod_test_handle
	};

	mod.name = strdup("test");

	return &mod;
}
