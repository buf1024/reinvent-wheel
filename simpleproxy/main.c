/*
 * main.c
 *
 *  Created on: 2016/5/18
 *      Author: Luo Guochun
 */

#include "simpleproxy.h"
#include "misc.h"
#include <getopt.h>
#include <errno.h>
#include <signal.h>

#define _GNU_SOURCE

static void sign_handler(int signo)
{
}

static void usage()
{
	printf("\nsimple proxy\n"
			"\n  -- a very simple proxy\n\n"
			"--test,    -t <conf file>    test conf file, default simpleproxy.conf\n"
			"--conf,    -c <conf file>    conf file, default simpleproxy.conf\n"
			"--exclude, -e <conf file>    don't start as a daemon process\n"
			"--help,    -h                show this message\n"
			"\n");
}

static int test_conf(const char* conf)
{
	tson_t* t = tson_parse_path(conf);
	if(t) {
	    printf("configure file ok\n\n");

	}
	return 0;
}


int main(int argc, char **argv)
{
	simpleproxy_t proxy;
	memset(&proxy, 0, sizeof(proxy));

	bool daemon = true;

	int opt = 0;
    int opt_idx = 0;

    struct option opts[] = {
    		{.name = "test", .has_arg = optional_argument, .val = 't'},
    		{.name = "conf", .has_arg = optional_argument, .val = 'c'},
    		{.name = "exclude", .has_arg = optional_argument, .val = 'e'},
    		{.name = "help", .has_arg = no_argument, .val = 'h'},
			{}
    };

    while ((opt = getopt_long(argc, argv, "t:c:h", opts, &opt_idx)) != -1) {
        switch (opt) {
          case 't':
              test_conf(optarg);
              exit(0);
              break;
          case 'c':
        	  if (optarg) {
        		  proxy->conf = strdup(optarg);
        	  }
        	  break;
          case 'e':
        	  daemon = false;
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

	if(proxy->conf == NULL) {
		char path[256] = {0};
		getcwd(path, sizeof(path) - 1);

		strcat(path, "/simpleproxy.conf");

		proxy->conf = strdup(path);
	}

	if(access(proxy->conf, F_OK) != 0) {
		printf("conf file %s not exists.\n", proxy->conf);
		exit(-1);
	}

    if(daemon) {
    	daemonlize();
    }

    REGISTER_SIGNAL(SIGTERM, sign_handler, 0);
    REGISTER_SIGNAL(SIGINT, sign_handler, 0);
    REGISTER_SIGNAL(SIGUSR1, sign_handler, 0);
    REGISTER_SIGNAL(SIGUSR2, sign_handler, 0);


	return 0;
}