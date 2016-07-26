/*
 * main.c
 *
 *  Created on: 2016/5/13
 *      Author: Luo Guochun
 */

#define _GNU_SOURCE
#include <getopt.h>
#include <signal.h>
#include <dlfcn.h>

#include "httpd.h"
#include "tson.h"
#include "log.h"
#include "misc.h"
#include "task.h"
#include "sock.h"
#include "http_util.h"

static httpd_t* _httpd = NULL;

static void usage()
{
	printf("\nsimple httpd\n"
			"\n  -- a simple scratch web server\n\n"
			"--test, -t <conf file>    test conf file, default simplehttpd.conf\n"
			"--conf, -c <conf file>    conf file, default simplehttpd.conf\n"
			"--help, -h                show this message\n"
			"\n");
}
static void sign_handler(int signo)
{
	if(signo == SIGTERM || signo == SIGINT) {
		_httpd->sig_term = true;
	}
	if(signo == SIGUSR1) {
		_httpd->sig_usr1 = true;
	}
	if(signo == SIGUSR2) {
		_httpd->sig_usr2 = true;
	}
}


static void *find_symbol(const char *name)
{
    void *symbol = dlsym(RTLD_NEXT, name);
    if (!symbol)
        symbol = dlsym(RTLD_DEFAULT, name);
    return symbol;
}

static int parse_listen_conf(httpd_t* http, char* virhost, char* listen, tson_t* t)
{
	int rv = 0;
	struct listener* data = calloc(1, sizeof(*data));
	char* pos = strchr(listen, ':');
	if(!pos) {
		data->port = 8800;
	}else {
		*pos = 0;
		data->port = atoi(pos + 1);
	}
	data->listen = strdup(listen);
	data->host = strdup(virhost);

	tson_get_bool(t, "ssl", &data->ssl);
	if(data->ssl) {
		rv = tson_get_string(t, "ssl-certificate", &data->ssl_certificate);
		if(rv <= 0) {
			printf("missing ssl-certificate conf.\n");
			return -1;
		}
		if(access(data->ssl_certificate, F_OK) != 0) {
			printf("ssl_certificate file %s not exists.\n", data->ssl_certificate);
			return -1;
		}

		rv = tson_get_string(t, "ssl-certificate-key", &data->ssl_certificate_key);
		if(rv <= 0) {
			printf("missing ssl-certificate-key key conf.\n");
			return -1;
		}
		if(access(data->ssl_certificate_key, F_OK) != 0) {
			printf("ssl-certificate-key file %s not exists.\n", data->ssl_certificate_key);
			return -1;
		}

		if(tson_get_string(t, "ssl-session-cache", &data->ssl_session_cache) <= 0) {
			data->ssl_session_cache = strdup("shared:SSL:1m");
		}
		if(tson_get_int(t, "ssl-session-timeout", &data->ssl_session_timeout) <= 0) {
			data->ssl_session_timeout = 300;
		}

		if(tson_get_string(t, "ssl-ciphers", &data->ssl_ciphers) <= 0) {
			data->ssl_ciphers = strdup("HIGH:!aNULL:!MD5");
		}
		if(tson_get_bool(t, "ssl-prefer-server-ciphers", &data->ssl_prefer_server_ciphers) <= 0) {
			data->ssl_prefer_server_ciphers = true;
		}
	}
	listener_t* l = (listener_t*)calloc(1, sizeof(*l));
	l->data = data;
	LIST_INSERT_HEAD(&http->listener, l, entry);

	// path
	tson_arr_t* arr = NULL;

	rv = tson_get_arr(t, "path", &arr);
	if(rv <= 0) {
		printf("missing path conf.\n");
		return -1;
	}
	if(!http->url) {
		http->url = trie_new(NULL);
	}

	if(!http->url) {
		printf("create trie failed.\n");
		return -1;
	}

	for(int i=0; i<rv; i++) {
		//
		if(!arr[i].tson) {
			printf("missing conf.\n");
			return -1;
		}
		tson_t* s = NULL;
		char* v = NULL;
		if(tson_get(arr[i].tson, "module", &v, &s) <= 0) {
			printf("missing module.\n");
			return -1;
		}
		char fname[256] = {0};
		snprintf(fname, sizeof(fname) - 1, "http_mod_%s", v);
		http_mod_create_fun_t fun = (http_mod_create_fun_t)find_symbol(fname);
		if(!fun) {
			printf("load module %s failed.\n", v);
			return -1;
		}

		http_module_t* mod = fun();
		if(!mod) {
			printf("create http module %s failed.\n", v);
			return -1;
		}
		if(mod->load_conf && s) {
			if(mod->load_conf(http, s) != 0) {
				printf("load mod %s conf failed.\n", v);
				return -1;
			}
		}

		free(v);

		http_path_t* path = (http_path_t*)calloc(1, sizeof(*path));
		if(!path) {
			printf("malloc http path failed.\n");
			return -1;
		}
		path->url = strdup(arr[i].value);
		path->mod = mod;

		// todo

		trie_add(http->url, path->url, path->mod);
	}

	return 0;
}

static int parse_conf(httpd_t* http)
{
	tson_t* t = tson_parse_path(http->conf);
	if(!t) {
		printf("parse tson failed, file=%s\n", http->conf);
		return -1;
	}

	char*  v = NULL;
	int rv = 0;

	rv = tson_get_string(t, "log-path", &http->log_path);
	if(!rv) {
		printf("missing log-path.\n");
		return -1;
	}

	rv = tson_get_string(t, "log-level", &v);
	if(!rv) {
		printf("missing log-level.\n");
		return -1;
	}
	http->log_level = log_get_level(v);

	rv = tson_get_int(t, "log-buf-size", &http->log_buf_size);
	if(!rv) {
		printf("missing log-buf-size.\n");
		return -1;
	}

	tson_get_string(t, "user", &http->user);
	tson_get_bool(t, "daemon", &http->daemon);


	tson_get_int(t, "thread", &http->thread_num);
	if(http->thread_num <= 0) {
		http->thread_num = get_cpu_count();
		printf("set to cpu number(%d)\n", http->thread_num);
	}

	tson_arr_t* arr = NULL;
	rv = tson_get_arr(t, "listen", &arr);
	for(int i=0; i<rv; i++) {
		if(parse_listen_conf(http, "*", arr[i].value, arr[i].tson) != 0) {
			printf("parse listen section failed: listen = %s\n", arr[i].value);
			return -1;
		}
	}
	free(arr);

	rv = tson_get_arr(t, "server", &arr);
	for(int i=0; i<rv; i++) {
		if(!arr[i].tson) {
			printf("missing listen section.\n");
			return -1;
		}
		tson_arr_t* arr_listen = NULL;
		int arr_count = tson_get_arr(arr[i].tson, "listen", &arr_listen);
		for (int j = 0; j < arr_count; j++) {
			if (parse_listen_conf(http, arr[i].value, arr_listen[i].value, arr_listen[i].tson) != 0) {
				printf("parse listen section failed: listen = %s\n", arr[i].value);
				return -1;
			}
		}
	}

	if(LIST_EMPTY(&http->listener)) {
		printf("listener is empty.\n");
		return -1;
	}

	tson_free(t);


	return 0;
}

static int test_conf(const char* conf)
{
	httpd_t http;
	memset(&http, 0, sizeof(http));

	http.conf = strdup(conf);
	if (parse_conf(&http) != 0) {
		printf("test configure file failed.\n\n");
	} else {
		printf("test configure file success.\n\n");
	}
	return 0;
}

static int httpd_init(httpd_t* http)
{

	if(log_init(http->log_level, http->log_level, http->log_path,
			"simplehttpd", http->log_buf_size, 0, -1) != 0) {
		printf("log_init failed.\n");
		return -1;
	}
	LOG_INFO("logger is ready.\n");

	http->epfd = epoll_create1(EPOLL_CLOEXEC);
	if(http->epfd <= 0) {
		LOG_ERROR("epoll_create1 failed, errno=%d\n", errno);
		return -1;
	}

	listener_t* var = NULL;
	LIST_FOREACH(var, &http->listener, entry) {
		struct listener* d = var->data;

		int fd = tcp_server(d->listen, d->port, LISTEN_BACK_LOG);

		if(fd <= 0) {
			LOG_FATAL("listen server failed, host=%s, port=%d\n", d->listen, d->port);
			return -1;
		}
		LOG_INFO("listening ip=%s, port=%d\n", d->listen, d->port);
	}


#if 0
	http->nfd = get_max_open_file_count();
	if(http->nfd <= 0) {
		LOG_FATAL("get_max_open_file_count failed.\n");
		return -1;
	}
	LOG_INFO("max open file: %u\n", http->nfd);
	http->conns = calloc(http->nfd, sizeof(connection_t));
	if(!http->conns) {
		LOG_FATAL("alloc connection failed.\n");
		return -1;
	}

	http->threads = calloc(http->thread_num, sizeof(http_thread_t));
	if(!http->threads) {
		LOG_FATAL("alloc thread failed.\n");
		return -1;
	}

	for(int i=0; i<http->thread_num; i++) {
		http_thread_t* t = &http->threads[i];

		t->http = http;
		t->nfd = http->nfd / http->thread_num;
		t->evts = calloc(t->nfd, sizeof(struct epoll_event));
		if(!t->evts) {
			LOG_FATAL("alloc thread info failed.\n");
			return -1;
		}
		t->state = THREAD_STATE_DEAD;

		t->epfd = epoll_create1(EPOLL_CLOEXEC);
		if(t->epfd <= 0) {
			LOG_ERROR("epoll_create1 failed, errno=%d\n", errno);
			return -1;
		}

		if(pipe(t->fd) != 0) {
			LOG_ERROR("thread create pipe failed.\n");
			return -1;
		}

		connection_t* con = &(http->conns[t->fd[0]]);
		con->fd = t->fd[0];
		con->state = CONN_STATE_CONNECTED;
		con->type = CONN_TYPE_CLIENT;

		if(epoll_add_fd(t->epfd, EPOLLIN, con) != 0) {
			LOG_ERROR("epoll_add_fd failed.\n");
			return -1;
		}

		pthread_create(&t->tid, NULL, proxy_task_routine, t);
		LOG_INFO("create thread suc! tid = %ld\n", t->tid);
	}

	int rfd = tcp_noblock_resolve_fd();
	connection_t* con = &(http->conns[rfd]);
	con->fd = rfd;
	con->state = CONN_STATE_CONNECTED;
	con->type = CONN_TYPE_CLIENT;

	if(epoll_add_fd(http->threads[0].epfd, EPOLLIN, con) != 0) {
		LOG_ERROR("epoll_add_fd failed.\n");
		return -1;
	}


	if(http->plugin->init(http) != 0) {
		LOG_ERROR("plugin init failed.\n");
		return -1;
	}
#endif
	return 0;

}

static int httpd_uninit(httpd_t* http UNUSED)
{
#if 0
	if (http->plugin) {
		if (http->plugin->uninit() != 0) {
			LOG_ERROR("plugin init failed.\n");
			//return -1;
		}
	}

	close(http->fd);
#endif
	log_finish();
	return 0;
}


int main(int argc, char **argv)
{

	httpd_t http;
	memset(&http, 0, sizeof(http));

	_httpd = &http;

	int opt = 0;
    int opt_idx = 0;

    struct option opts[] = {
    		{.name = "test", .has_arg = optional_argument, .val = 't'},
    		{.name = "conf", .has_arg = optional_argument, .val = 'c'},
    		{.name = "exclude", .has_arg = no_argument, .val = 'e'},
    		{.name = "help", .has_arg = no_argument, .val = 'h'},
			{0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "t:c:h", opts, &opt_idx)) != -1) {
        switch (opt) {
          case 't':
              test_conf(optarg);
              exit(0);
              break;
          case 'c':
        	  if (optarg) {
        		  http.conf = strdup(optarg);
        	  }
        	  break;
          case 'h':
        	  usage();
        	  exit(0);
        	  break;
          default:
        	  printf("unknown options\n");
        	  exit(-1);

        }
    }

	if(http.conf == NULL) {
		char path[256] = {0};
		getcwd(path, sizeof(path) - 1);

		strcat(path, "/simplehttpd.conf");

		http.conf = strdup(path);
	}

	if(access(http.conf, F_OK) != 0) {
		printf("conf file %s not exists.\n", http.conf);
		exit(-1);
	}

    REGISTER_SIGNAL(SIGTERM, sign_handler, 1);
    REGISTER_SIGNAL(SIGINT, sign_handler, 1);
    REGISTER_SIGNAL(SIGUSR1, sign_handler, 1);
    REGISTER_SIGNAL(SIGUSR2, sign_handler, 1);

	if (parse_conf(&http) != 0) {
		printf("configure file error.\n");
		exit(-1);
	}

    if(httpd_init(&http) != 0) {
    	printf("httpd_init failed.\n");
    	exit(-1);
    }
    LOG_INFO("http init!\n");
    LOG_INFO("http start main loop...\n");

    httpd_main_loop(&http);

    LOG_INFO("http end main loop...\n");
    LOG_INFO("http uninit\n");

    httpd_uninit(&http);

	return 0;
}
