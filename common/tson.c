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

static char* to_lower(char* v) {
    char* t = v;
    while(*t++) {
        if(isupper(*t)) {
            *t = tolower(*t);
        }
    }
    return v;
}

static char* trim(char* line)
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

static int parse_kv(const char*line, char*k, char* v)
{
    char* kv = strchr(line, ' ');
    if(kv == NULL) {
        return -1;
    }
    strncpy(k, line, kv - line);
    kv = trim(kv);

    strncpy(v, kv, line + strlen(line) - kv);

    return 0;
}

static bool parse_bool(char* v)
{
    if(strcasecmp(v, "yes") == 0) {
        return true;
    }
    if(strcasecmp(v, "true") == 0) {
        return true;
    }
    if(strcasecmp(v, "1") == 0) {
        return true;
    }

    return false;
}

static int parse_addrinfo(char* v, sa_family_t* af, char* ip, char* port)
{

    char* col = strchr(v, ':');
    if(col == NULL) {
        return -1;
    }
    if(*(col + 1) == 0) {
        return -1;
    }
    if(*v == '[') {
        *af = AF_INET6;
        strncpy(ip, v, col - v);
    } else if(*v == '*') {
        *af = AF_INET;
        strncpy(ip, "0.0.0.0", sizeof("0.0.0.0"));
    }else{
        *af = AF_INET;
        strncpy(ip, v, col - v);
    }

    int iport = atoi(col + 1);
    if(iport <= 0) {
        return -1;
    }
    strcpy(port, col + 1);

    char* t_ip = trim(ip);
    char* t_port = trim(port);
    if(t_ip != ip) {
        memmove(ip, t_ip, strlen(t_ip));
        ip[strlen(t_ip) - (t_ip - ip)] = 0;
    }
    if(t_port != port) {
        memmove(port, t_port, strlen(t_port));
        port[strlen(port) - (t_port - port)] = 0;
    }

    return 0;
}

static int parse_backend(config_t* conf, backend_t* backend, char* v) {
    //backend newwork 127.0.0.1:11001 50;
    char* col = NULL, *col_nxt = NULL;
    char v_t[1024] = {0};

    col = strchr(v, ' '); if(col == NULL) return -1;

    col = trim(col); if(*col == 0) return -1;
    col_nxt = strchr(col, ' '); if(col_nxt == NULL) return -1;
    strncpy(v_t, col, col_nxt - col);
    backend->name = strdup(v_t);

    col = trim(col_nxt); if(*col == 0) return -1;
    col_nxt = strchr(col, ' '); if(col_nxt == NULL) return -1;
    strncpy(v_t, col, col_nxt - col);
    backend->addr_str = strdup(v_t);
    sa_family_t af;
    char node[1024] = {0};
    char port[1024] = {0};
    if(parse_addrinfo(v_t, &af, node, port) != 0) {
        conf->parse.conf = strdup(v);
        conf->parse.error = strdup("work-mode, ip address missing port or port is not valid)");
        return -1;
    }
    struct addrinfo hints = {
        .ai_family = af,
        .ai_socktype = SOCK_STREAM,
        .ai_flags = AI_PASSIVE
    };

    if(getaddrinfo(node, port, &hints, &backend->addrs) != 0) {
        conf->parse.error = strdup("work-mode, ip address is not valid");
        return -1;
    }

    col = trim(col_nxt); if(*col == 0) return -1;
    backend->weight = atoi(col);

    if(backend->weight <= 0) {
        conf->parse.conf = strdup(col);
        conf->parse.error = strdup("work-mode, weight must greater than 0");
        return -1;
    }
    backend->pxy = conf->pxy;
    return 0;
}

static int parse_time(char* l, int* sec)
{
    char* s = l, *s_nxt = NULL;
    char v[1024] = { 0 };
    *sec = 0;

    s_nxt = strchr(s, 'h');
    if (s_nxt != NULL) {
        if (s_nxt != s) {
            strncpy(v, s, s_nxt - s);
            s_nxt = strrchr(v, ' ');
            if (s_nxt != NULL) {
                s_nxt = v;
            }
        } else {
            return -1;
        }
        *sec += 3600 * atoi(s_nxt);
    }
    s_nxt = strchr(s, 'm');
    if (s_nxt != NULL) {
        if (s_nxt != s) {
            strncpy(v, s, s_nxt - s);
            s_nxt = strrchr(v, ' ');
            if (s_nxt != NULL) {
                s_nxt = v;
            }
            *sec += 60 * atoi(v);
        }
    }
    s_nxt = strchr(s, 's');
    if (s_nxt != NULL) {
        strncpy(v, s, s_nxt - s);
        s_nxt = strrchr(v, ' ');
        if (s_nxt != NULL) {
            s_nxt = v;
        }
        *sec += atoi(v);
    } else {
        return -1;
    }

    return 0;
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

    return pos + 1;
}


static int __tson_dump_node(tson_node_t* n, int space, char* buf, int size)
{
    int s = 0;

    while(space > 0) {
        s = sprintf(buf, size, " ");
        size -= s;
        buf += s;
        space--;
    }

    if(n->t) {
        s = snprintf(buf, size, "%s %s {\n", n->k, n->v);
    }else{
        s = snprintf(buf, size, "%s %s\n", n->k, n->v);
    }
    return s;
}

static int __tson_dump_help(tson_t* tson, int space, char* buf, int size)
{
    int i = 0;
    tson_node_t* n = NULL;
    int s = 0;
    for(;i<TSON_HASH_ARRAY_SIZE; i++) {
        n = tson->n[i];

        while(n) {
            s = __tson_dump_node(n, space, buf, size);
            if(s <= 0) {
                return -1;
            }
            size -= s;
            buf += s;

            if(n->t) {
                if(__tson_dump_help(n->t, space + 2, buf, size) < 0) {
                    return -1;
                }
            }else{
                n = n->n;
            }
        }
    }
    return 0;
}

tson_t* tson_parse(const char* str)
{
    const char* next = str;
    char line[TSON_LINE_SIZE] = {0};
    char* l = NULL;

    while (next && *next) {
        next = __get_line(next, line);
        l = trim(line);

        if (*l == 0) {
            continue;
        }

        if (*l == '#') {
            continue;
        }


            char k[1024] = {0}, v[1024] = {0};
            if(parse_kv(l, k, v) != 0) {
                conf->parse.conf = strdup(l);
                conf->parse.error = strdup("missing value");
                fclose(fp);
                return -1;
            }
            if(strcasecmp(k, "log-path-file") == 0) {
                conf->log_file = strdup(v);
            }else if(strcasecmp(k, "log-level") == 0) {
                char* v_lower = to_lower(v);
                if(strstr("debug info warn error off", v_lower) != NULL) {
                    conf->log_level = strdup(v_lower);
                }else{
                    conf->parse.conf = strdup(v);
                    conf->parse.error = strdup("log-level is not in (debug info warn error off)");
                    fclose(fp);
                    return -1;
                }
            }else if(strcasecmp(k, "daemon") == 0) {
                conf->daemon = parse_bool(v);
            }else if(strcasecmp(k, "listen") == 0) {
                sa_family_t af;
                char node[1024] = {0};
                char port[1024] = {0};
                if(parse_addrinfo(v, &af, node, port) != 0) {
                    conf->parse.conf = strdup(v);
                    conf->parse.error = strdup("ip address missing port or port is not valid)");
                    fclose(fp);
                    return -1;
                }
                struct addrinfo hints = {
                    .ai_family = af,
                    .ai_socktype = SOCK_STREAM,
                    .ai_flags = AI_PASSIVE
                };

                if(getaddrinfo(node, port, &hints, &conf->addrs) != 0) {
                    conf->parse.conf = strdup(v);
                    conf->parse.error = strdup("ip address is not valid/or unreachable");
                    fclose(fp);
                    return -1;
                }
            }else if (strcasecmp(k, "idle-close-time") == 0) {
                if(parse_time(v, &conf->pxy->idle_to) != 0 || conf->pxy->idle_to <= 0) {
                    conf->parse.conf = strdup(l);
                    conf->parse.error = strdup("time format in valid, h, m, s");
                    fclose(fp);
                    return -1;
                }

            } else if(strcasecmp(k, "work-mode") == 0) {
                if(*(v + strlen(v) - 1) != '{') {
                    conf->parse.conf = strdup(k);
                    conf->parse.error = strdup("work-mode missing '{'");
                    fclose(fp);
                    return -1;
                }
                l = trim(v);

                char k_m[1024] = {0}, v_m[1024] = {0};
                if(parse_kv(v, k_m, v_m) != 0) {
                    conf->parse.conf = strdup(v);
                    conf->parse.error = strdup("work-mode missing 'mode', 'mode' in 'find-connect-new/find-connect-exist/find-connect-packet'");
                    fclose(fp);
                    return -1;
                }
                char* v_lower = to_lower(k_m);
                if(strstr("find-connect-new find-connect-exist find-connect-packet", v_lower) == NULL) {
                    conf->parse.conf = strdup(k_m);
                    conf->parse.error = strdup("work-mode invalid, 'mode' in 'find-connect-new/find-connect-exist/find-connect-packet'");
                    fclose(fp);
                    return -1;
                }

                if(parse_mode(conf, fp, v_lower) != 0) {
                    fclose(fp);
                    return -1;
                }
            }else{

                conf->parse.conf = strdup(l);
                conf->parse.error = strdup("unknown conf option");
                return -1;
            }
        }
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

int tson_free(tson_t* tson)
{
    int i = 0;
    tson_node_t* n = NULL;
    tson_node_t* p = NULL;
    for(;i<TSON_HASH_ARRAY_SIZE; i++) {
        n = tson->n[i];
        while(n) {
            p = n;
            if(n->t) {
                tson_free(n->t);
            }else{
                n = n->n;
            }
            if(p->k) free(p->k);
            if(p->v) free(p->v);
            if(p) free(p);
        }
    }
    return 0;
}

int tson_dump(tson_t* tson, char* buf, int size)
{
    return __tson_dump_help(tson, 0, buf, size);
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

    int index = murmur(key, strlen(key)) % TSON_HASH_ARRAY_SIZE;
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
