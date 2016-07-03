/*
 * tsontest.c
 *
 *  Created on: 2016-7-3
 *      Author: Luo Guochun
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tson.h"

int main(int argc, char **argv)
{
    const char* c1 = "./tson1.conf";
    const char* c2 = "./tson2.conf";

    tson_t* t1 = tson_parse_path(c1);
    tson_t* t2 = tson_parse_path(c2);

    char buf1[4096] = {0};
    char buf2[4096] = {0};

    if(t1) tson_dump(t1, buf1, 4096);
    if(t2) tson_dump(t2, buf2, 4096);

    printf("tson1:\n%s\n", buf1);
    printf("tson2:\n%s\n", buf2);

    if(t1) {
        char* v = NULL;
        tson_t* tt = NULL;
        if(tson_get(t1, "listen", &v, &tt) > 0) {
            printf("value = %s\n", v);
        }
    }

    if(t1) tson_free(t1);
    if(t2) tson_free(t2);

    return 0;
}


