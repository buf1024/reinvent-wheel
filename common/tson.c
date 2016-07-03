/*
 * tson.c
 *
 *  Created on: 2013-7-3
 *      Author: Luo Guochun
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tson.h"
#include "murmur.h"

#define TSON_HASH_ARRAY_SIZE 16

typedef struct tson_node_s tson_node_t;

struct tson_node_s
{
    char* k;
    char* v;
    tson_t* t;

    tson_node_t* n;
};

struct tson_s
{
    tson_node_t* n[TSON_HASH_ARRAY_SIZE];
};

tson_t* tson_parse(const char* str)
{
    return 0;
}
tson_t* tson_parse_fd(int fd)
{
    return 0;
}
tson_t* tson_parse_path(const char* path)
{
    return 0;
}

int     tson_free(tson_t* tson)
{
    return 0;
}

void tson_dump(tson_t* tson, char* buf, int size)
{

}
int tson_save(tson_t* tson, const char* path)
{
    return 0;
}

int tson_get(tson_t* tson, const char* key, char** value, tson_t** sub_tson)
{
    *value = NULL; *sub_tson = NULL;

    int index = murmur(key, strlen(key)) % TSON_HASH_ARRAY_SIZE;
    tson_node_t* n = tson->n[index];

    if(!n) return -1;

    while(n && strcmp(n->k, key) != 0) {
        n = n->n;
    }

    if(n) {
        *value = n->v;
        *sub_tson = n->t;

        return 1;
    }

    return 0;
}
int tson_set(tson_t* tson, const char* key, const char* value, tson_t* sub_tson)
{
    return 0;
}
