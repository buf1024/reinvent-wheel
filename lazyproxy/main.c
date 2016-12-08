/*
 * main.c
 *
 *  Created on: 2016/5/18
 *      Author: Luo Guochun
 */

#include "lazy.h"

#define DEFAULT_LISTEN_ADDR "0.0.0.0"
#define DEFAULT_LISTEN_PORT 7272
#define DEFAULT_LOG_DIR     "./"

lazy_proxy_t g_proxy;

static void usage()
{
    printf("\nlazyproxy\n"
            "    -- a very simple http/https demo proxy\n"
            "       using noblock io & coroutine\n\n"
    		"-l     log directory(default: %s)\n"
    		"-a     listen address(default: %s)\n"
            "-p     listen port(default: %d)\n"
            "-e     don't start background\n"
            "-h     show this message\n"
            "\n", DEFAULT_LOG_DIR, DEFAULT_LISTEN_ADDR, DEFAULT_LISTEN_PORT);
}

static void sigterm(int signo UNUSED) {
	g_proxy.sig_term = true;
}
static void sigusr1(int signo UNUSED) {
	g_proxy.sig_usr1 = true;
}
static void sigusr2(int signo UNUSED) {
	g_proxy.sig_usr2 = true;
}

int main(int argc, char **argv)
{
    int daemon = 1;
    char addr[MAX_HOST_SIZE] = {0};
    char log_dir[MAX_PATH_SIZE] = {0};

    int  port = DEFAULT_LISTEN_PORT;

    strncpy(addr, DEFAULT_LISTEN_ADDR, sizeof(addr) - 1);
    strncpy(log_dir, DEFAULT_LOG_DIR, sizeof(log_dir) - 1);

    char opts[] = "i:p:l:eh";
    int optch;
    while ((optch = getopt(argc, argv, opts)) != -1) {
        switch (optch) {
        case 'h':
            usage();
            exit(0);
        case 'e':
            daemon = 0;
            break;
        case 'l':
        	strncpy(log_dir, optarg, sizeof(log_dir) - 1);
        	break;
        case 'a':
        	strncpy(addr, optarg, sizeof(addr) - 1);
        	break;
        case 'p':
            port = atoi(optarg);
            break;
        default:
            printf("unknown option %c\n", (char)optch);
            exit(-1);
        }
    }
    if(daemon) {
    	daemonlize();
    }
    g_proxy.port = port;
    strncpy(g_proxy.host, addr, sizeof(addr) - 1);
    strncpy(g_proxy.log_dir, log_dir, sizeof(log_dir) - 1);

    REGISTER_SIGNAL(SIGTERM, sigterm, 0);
    REGISTER_SIGNAL(SIGINT, sigterm, 0);
    REGISTER_SIGNAL(SIGUSR1, sigusr1, 0);
    REGISTER_SIGNAL(SIGUSR2, sigusr2, 0);

    if(log_init(LOG_ALL, LOG_DEBUG,
    		g_proxy.log_dir, "lazyproxy",
			102400, 0, 0) != LOG_SUCCESS) {
    	printf("log_init failed.\n");
    	return -1;
    }

    if(lazy_net_init(&g_proxy) != 0) {
    	LOG_FATAL("lazy_net_init failed.\n");
    	LOG_FINISH();
    	return -1;
    }

    lazy_proxy_task(&g_proxy);


    lazy_net_uninit(&g_proxy);
    log_finish();

    return 0;
}
