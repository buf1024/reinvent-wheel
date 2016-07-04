/*
 * tson.c
 *
 *  Created on: 2013-7-3
 *      Author: Luo Guochun
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "tson.h"
#include "misc.h"

#define TSON_HASH_ARRAY_SIZE 16
#define TSON_LINE_SIZE       1024

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


static char* __trim(char* line)
{

    char* end = line + strlen(line) - 1;
    char* start = line;

    while(start <= end) {
            if (*start == ' ' || *start == '\r' || *start == '\n') {
                start++;
            } else {
                break;
            }
    }
    while(start <= end) {
        if(*end == ' ' || *end == '\r' || *end == '\n') {
            end--;
        }else{
            break;
        }
    }
    *(end + 1) = 0;

    return start;
}

static const char* __get_line(const char* buf, char* l, int size)
{
    char* pos = NULL;

    pos = strchr(buf, '\n');
    if(!pos) {
        strncpy(l, buf, size);
        return NULL;
    }

    memcpy(l, buf, pos - buf);
    l[pos - buf] = 0;

    return pos + 1;
}

static tson_node_t* __parse_line(const char* l)
{
    char* kv = strchr(l, ' ');
    if(kv == NULL) {
        return NULL;
    }

    int s = kv - l;

#if 0
    while(s >= 0) {
        if(!isalpha(kv[s])) {
            return NULL;
        }
        s--;
    }
#endif

    tson_node_t* n = (tson_node_t*)malloc(sizeof(*n));
    memset(n, 0, sizeof(*n));

    tson_t* tson = NULL;

    n->k = strndup(l, s);
    kv = __trim(kv);

    if(kv[strlen(kv) - 1] == '{') {
        kv[strlen(kv) - 1] = 0;
        kv = __trim(kv);

        tson = (tson_t*)malloc(sizeof(*tson));
        memset(tson, 0, sizeof(*tson));
    }

    if(*kv == 0) {
        free(n->k);
        free(n);
        if(tson) {
            free(tson);
        }
        return NULL;
    }
    n->v = strdup(kv);
    n->t = tson;

    return n;
}

static tson_t* __parse_tson(tson_t* tson, const char* tstr)
{
    const char* next = tstr;
    char line[TSON_LINE_SIZE] = {0};
    char* l = NULL;

    int index = 0;

    tson_node_t* n = NULL;
    tson_node_t* p = NULL;

    while (next && *next) {
        next = __get_line(next, line, TSON_LINE_SIZE);
        l = __trim(line);

        if (*l == 0) {
            continue;
        }

        if (*l == '#') {
            continue;
        }

        n = __parse_line(l);
        if(!n) {
            return NULL;
        }
        index = murmur((unsigned char*)n->k, strlen(n->k)) % TSON_HASH_ARRAY_SIZE;
        p = tson->n[index];
        if(!p) tson->n[index] = n;
        else{
          while(p->n) {
              p = p->n;
          }
          p->n = n;
        }
        if(n->t) {
            int num = 1;
            const char* start = next;
            const char* end = NULL;
            while(*start) {
                if(*start == '{') {
                    num++;
                }
                if(*start == '}') {
                    num--;
                }
                start++;
                if(num == 0) {
                    while(*start) {
                        if(*start == ' ') start++;
                        if(*start == '\n' || *start != ' ') break;
                    }
                    if(*start != '\n' && *(start+1) != 0) {
                        tson_free(n->t);
                        n->t = NULL;
                        return NULL;
                    }
                    end = start - 1;
                    start = next;

                    next = end + 2;
                    break;
                }
            }
            if(num != 0) {
                tson_free(n->t);
                n->t = NULL;
                return NULL;
            }
            char ststr[end - start + 1];
            strncpy(ststr, start, end - start);
            ststr[end - start] = 0;

            if(__parse_tson(n->t, ststr) == NULL) {
                tson_free(n->t);
                n->t = NULL;
                return NULL;
            }
        }
    }
    return tson;
}



static int __tson_dump_node(tson_node_t* n, int space, char* buf, int size)
{
    int s = 0;

    while(space > 0) {
        snprintf(buf, size, " ");

        buf++;
        s++;
        size--;
        space--;
    }

    if(n->t) {
        s += snprintf(buf, size, "%s %s {\n", n->k, n->v);
    }else{
        s += snprintf(buf, size, "%s %s\n", n->k, n->v);
    }
    return s;
}

static int __tson_dump_help(tson_t* tson, int space, char* buf, int size)
{
    int i = 0;
    tson_node_t* n = NULL;
    int s = 0;
    int r = size;

    for(;i<TSON_HASH_ARRAY_SIZE; i++) {
        n = tson->n[i];

        while(n) {
            if(size <= 0) {
                return -1;
            }
            s = __tson_dump_node(n, space, buf, size);
            if(s <= 0) {
                return -1;
            }
            size -= s;
            buf += s;

            if(n->t) {
                if(size <= 0) {
                    return -1;
                }
                s = __tson_dump_help(n->t, space + 2, buf, size);
                if(s <= 0) {
                    return -1;
                }
                size-=s;
                buf+=s;

                int ept = space;

                if(ept == 0) {
                    s = snprintf(buf, size, "}\n");
                    size-=s;
                    buf += s;
                } else {
                    while (ept > 0) {
                        snprintf(buf, size, " ");

                        buf++;
                        s++;
                        size--;
                        ept--;
                    }

                    s = snprintf(buf, size, "}\n");
                    size -= s;
                    buf += s;
                }
            }
            n = n->n;
        }
    }


    return r - size;
}

tson_t* tson_parse(const char* str)
{
    tson_t* tson = (tson_t*)malloc(sizeof(*tson));
    memset(tson, 0, sizeof(*tson));

    if(__parse_tson(tson, str) == NULL) {
        tson_free(tson);
        return NULL;
    }

    return tson;
}

tson_t* tson_parse_path(const char* path)
{
    FILE* fp = fopen(path, "rb");
    if(!fp) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    int s = ftell(fp);
    char* buf = (char*)malloc(s + 1);
    rewind(fp);

    buf[s] = 0;
    int idx = 0;
    while(!feof(fp)) {
        idx = fread(buf + idx, 1, 1024, fp);
    }
    fclose(fp);

    tson_t* tson = tson_parse(buf);

    free(buf);

    return tson;
}

int tson_free(tson_t* tson)
{
    int i = 0;
    tson_node_t* n = NULL;
    tson_node_t* p = NULL;
    for(;i<TSON_HASH_ARRAY_SIZE; i++) {
        n = tson->n[i];
        while(n) {
            if(n->t) {
                tson_free(n->t);
                n->t = NULL;
            }
            p = n;
            n = n->n;

            if(p->k) free(p->k);
            if(p->v) free(p->v);
            if(p) free(p);

        }
    }
    free(tson);
    return 0;
}

int tson_dump(tson_t* tson, char* buf, int size)
{
    return __tson_dump_help(tson, 0, buf, size);
}

int tson_get(tson_t* tson, const char* key, char** value, tson_t** sub_tson)
{
    *value = NULL; *sub_tson = NULL;

    int index = murmur((unsigned char*)key, strlen(key)) % TSON_HASH_ARRAY_SIZE;
    tson_node_t* n = tson->n[index];

    if(!n) return 0;

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
    char* v = NULL;
    tson_t* t = NULL;

    if(tson_get(tson, key, &v, &t) > 0) {
        return -1;
    }

    int index = murmur((unsigned char*)key, strlen(key)) % TSON_HASH_ARRAY_SIZE;
    tson_node_t* n = tson->n[index];

    if(!n) {
        n = (tson_node_t*)malloc(sizeof(tson_node_t));
        memset(n, 0, sizeof(tson_node_t));

        tson->n[index] = n;
    }else{
        tson_node_t* p = n;
        while(n) {
            p = n;
            n = n->n;
        }
        n = (tson_node_t*)malloc(sizeof(tson_node_t));
        memset(n, 0, sizeof(tson_node_t));

        p->n = n;
    }

    n->k = strdup(key);
    n->v = strdup(value);
    n->t = sub_tson;

    return 0;
}
