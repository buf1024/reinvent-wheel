/*
 * main.c
 *
 *  Created on: 2016/5/18
 *      Author: Luo Guochun
 */

#include "thinpxy.h"
#include <getopt.h>
#include <errno.h>

#define _GNU_SOURCE

enum {
	THINPXY_RUN_TEST,
	THINPXY_RUN_NORMAL,
	THINPXY_RUN_EXIT,
};


static void usage()
{
	printf("\nthin proxy\n"
			"\n  -- a very simple handed proxy/loadbalance\n\n"
			"--test, -t <conf file>    test conf file, default thinpxy.conf\n"
			"--conf, -c <conf file>    conf file, default thinpxy.conf\n"
			"--help, -h                show this message\n"
			"\n");
}

static int parse_opt(config_t* pxy_conf, int argc, char** argv)
{
	int run_type = THINPXY_RUN_EXIT;

	int opt = 0;
    int opt_idx = 0;

    struct option opts[] = {
    		{.name = "test", .has_arg = optional_argument, .val = 't'},
    		{.name = "conf", .has_arg = optional_argument, .val = 'c'},
    		{.name = "help", .has_arg = no_argument, .val = 'h'},
			{}
    };

    while ((opt = getopt_long(argc, argv, "t:c:h", opts, &opt_idx)) != -1) {
        switch (opt) {
          case 't':
              if (optarg) {
            	  pxy_conf->conf = strdup(optarg);
              }
              run_type = THINPXY_RUN_TEST;
              break;
          case 'c':
        	  if (optarg) {
        		  pxy_conf->conf = strdup(optarg);
        	  }
        	  run_type = THINPXY_RUN_NORMAL;
        	  break;
          case 'h':
        	  usage();
        	  run_type = THINPXY_RUN_EXIT;
        	  return run_type;
          default:
        	  printf("unknown options\n");
        	  run_type = THINPXY_RUN_EXIT;
        	  return run_type;

        }
    }
    if(run_type != THINPXY_RUN_EXIT) {
    	if(pxy_conf->conf == NULL) {
    		char path[256] = {0};
    		getcwd(path, sizeof(path) - 1);

    		strcat(path, "/thinpxy.conf");

    		pxy_conf->conf = strdup(path);
    	}

    	if(access(pxy_conf->conf, F_OK) != 0) {
    		printf("conf file %s not exists.\n", pxy_conf->conf);
    		run_type = THINPXY_RUN_EXIT;
    	}
    }
    return run_type;
}

static int test_conf(config_t* pxy_conf)
{
	if(parse_conf(pxy_conf) != 0) {
		printf("configuration file(%s) error:\nline: %d\nconf: %s\nemsg: %s\n\n",
				pxy_conf->conf, pxy_conf->parse.line,
				pxy_conf->parse.conf, pxy_conf->parse.error);
		return -1;
	}
	printf("configure file ok\n\n");
	return 0;
}


int main(int argc, char **argv)
{
	thinpxy_t pxy;
	memset(&pxy, 0, sizeof(pxy));

	config_t* conf = &(pxy.config);
	conf->pxy = &pxy;

	int run_type = parse_opt(conf, argc, argv);
	if(run_type == THINPXY_RUN_EXIT) {
		exit(0);
	}
	if(run_type == THINPXY_RUN_TEST) {
		test_conf(conf);
		exit(0);
	}

	return 0;
}
