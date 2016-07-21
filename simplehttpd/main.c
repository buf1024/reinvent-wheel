/*
 * main.c
 *
 *  Created on: 2016/5/13
 *      Author: Luo Guochun
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>

#include "simplehttpd.h"

static void usage()
{
	printf("\nthin httpd\n"
			"\n  -- a simple scratch web server\n\n"
			"--test, -t <conf file>    test conf file, default thinhttpd.conf\n"
			"--conf, -c <conf file>    conf file, default thinhttpd.conf\n"
			"--help, -h                show this message\n"
			"\n");
}

static int parse_opt(httpd_t* httpd, int argc, char** argv)
{
	int run_type = THIN_HTTPD_RUN_EXIT;

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
                  httpd->conf = strdup(optarg);
              }
              run_type = THIN_HTTPD_RUN_TEST;
              break;
          case 'c':
        	  if (optarg) {
        		  httpd->conf = strdup(optarg);
        	  }
        	  run_type = THIN_HTTPD_RUN_NORMAL;
        	  break;
          case 'h':
        	  usage();
        	  run_type = THIN_HTTPD_RUN_EXIT;
        	  return run_type;
          default:
        	  printf("unknown options\n");
        	  run_type = THIN_HTTPD_RUN_EXIT;
        	  return run_type;

        }
    }
    if(run_type != THIN_HTTPD_RUN_EXIT) {
    	if(httpd->conf == NULL) {
    		char path[256] = {0};
    		getcwd(path, sizeof(path) - 1);

    		strcat(path, "/thinhttpd.conf");

    		httpd->conf = strdup(path);
    	}

    	if(access(httpd->conf, F_OK) != 0) {
    		printf("conf file %s not exists.\n", httpd->conf);
    		run_type = THIN_HTTPD_RUN_EXIT;
    	}
    }
    return run_type;
}

int main(int argc, char **argv)
{
	httpd_t httpd;
	memset(&httpd, 0, sizeof(httpd));

	int run_type = parse_opt(&httpd, argc, argv);
	if(run_type == THIN_HTTPD_RUN_EXIT) {
		exit(0);
	}

	return 0;
}


