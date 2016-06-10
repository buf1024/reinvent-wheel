/*
 * main.c
 *
 *  Created on: 2016/5/18
 *      Author: Luo Guochun
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#define DEFAULT_LISTEN_PORT 7272

static void usage()
{
    printf("\nlazyproxy\n"
            "    -- a very simple http/https demo proxy\n"
            "       using noblock io & coroutine\n\n"
            "-p     listen port(default: %d)\n"
            "-e     don't start background\n"
            "-h     show this message\n"
            "\n", DEFAULT_LISTEN_PORT);
}

int main(int argc, char **argv)
{
    int daemon = 1;
    int port = DEFAULT_LISTEN_PORT;

    char opts[] = "peh";
    int optch;
    while ((optch = getopt(argc, argv, opts)) != -1) {
        switch (optch) {
        case 'h':
            usage();
            exit(0);
        case 'e':
            daemon = 0;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        default:
            printf("unknown option %c\n", (char)optch);
            exit(-1);
        }
    }

    return 0;
}
