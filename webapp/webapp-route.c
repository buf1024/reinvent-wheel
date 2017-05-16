#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "webapp-route.h"
#include "webapp-util.h"

#define ROUTE_MAX_DEPTH 128
#define ROUTE_MAX_PATH  256


route_t* route_new(void* data)
{
    route_t* r = calloc(1, sizeof(*r));
    if(!r) return NULL;

    r->data = data;
    return r;
}
static int route_destroy_node(route_node_t* node)
 {
    if(!node) return 0;
    if(node->nodes_num == 0) {
        if(node->handlers) free(node->handlers);
    }else{
        for(int i=0; i<node->nodes_num; i++) {
            route_destroy_node(&(node->nodes[i]));
        }
        free(node->nodes);
        if(node->handlers) free(node->handlers);
    }
    return 0;
}
int route_destroy(route_t* route)
{
    if(!route) return 0;
    for(int i=0; i<route->nodes_num; i++) {
        route_destroy_node(&(route->nodes[i]));
    }
    if(route->nodes) free(route->nodes);
    if(route->handlers) free(route->handlers);
    free(route);
    return 0;
}
static int route_add_handlers(webapp_handler_t handler, va_list ap, 
    int* handlers_num, webapp_handler_t** handlers)
{
    *handlers_num = 1;
    *handlers = calloc(1, sizeof(webapp_handler_t));
    (*handlers)[*handlers_num - 1] = handler;

    while (1) {
        webapp_handler_t h = va_arg(ap, webapp_handler_t);
        if (h == NULL)
            break;

        (*handlers_num)++;
        (*handlers) = realloc((*handlers), (*handlers_num) * sizeof(webapp_handler_t));
        handlers[(*handlers_num) - 1] = h;
    }
    return 0;
}
int route_add(route_t* route, const char* path, webapp_handler_t handler, ...)
{
        if(!route || !handler || !path || strlen(path) == 0 || path[0] != '/') return -1;

        va_list ap;
        va_start(ap, handler);
        int num = 0;
        webapp_handler_t* handlers = NULL;
        route_add_handlers(handler, ap, &num, &handlers);
        va_end(ap);

        int rv = route_add_group(route, path, num, handlers);

        if(handlers) free(handlers);

        return rv;
}
int route_add_group(route_t* route, const char* path, int num, webapp_handler_t* handler)
{
    if(!route || !handler || !path || strlen(path) == 0 || path[0] != '/') return -1;

    if (path[1] == 0) {
        if (route->handlers) free(route->handlers);       

        route->handlers_num = num;
        if(num > 0) {
            route->handlers = calloc(num, sizeof(webapp_handler_t));
            memcpy(route->handlers, handler, num*sizeof(webapp_handler_t));
        }

        return 0;
    }

    enum route_note_type_t type = NODE_TYPE_NONE;

    bool tsr = false;
    char vec_path[ROUTE_MAX_DEPTH][ROUTE_MAX_PATH] = {0};
    int vec_path_num = split(path+1, '/', vec_path, ROUTE_MAX_PATH, ROUTE_MAX_DEPTH);
    if(vec_path_num > 0 && vec_path[vec_path_num - 1][0] == 0) {
        vec_path_num--;
        tsr = true;
    }

    route_node_t** nodes = &(route->nodes);
    int* nodes_num = &(route->nodes_num);
    route_node_t* node = NULL;

    for (int i = 0; i < vec_path_num; i++) {
        const char* tpath = vec_path[i];
        enum route_note_type_t type = NODE_TYPE_ABS;

        if(tpath[0] == '*' || tpath[0] == ':') {
            if(!tpath[1]) {
                // no name
                return -1; 
            }
            int j = 1;
            while (tpath[j]) {
                if(!isalnum(tpath[j])) {
                    // no alpha or num
                    return -1;
                }
                j++;
            }
            if(tpath[0] == '*') {
                // only allow the last
                if(i != vec_path_num - 1) return -1;
                type = NODE_TYPE_WILDCARD;
            }
            if(tpath[0] == ':') type = NODE_TYPE_PARAM;
        }
        
        if(*nodes == NULL) {
            *nodes  = calloc(1, sizeof(route_node_t));
            (*nodes_num)++;

            node = &((*nodes)[0]);
        }else{
            route_node_t* t = NULL;
            bool cont = false;
            for (int j=0; j < *nodes_num;j++) {
                t = &((*nodes)[j]);
                if (t->type == NODE_TYPE_WILDCARD || type == NODE_TYPE_WILDCARD) {
                    // exists wildcard
                    return -1;
                }
                if (t->type == NODE_TYPE_PARAM || type == NODE_TYPE_PARAM) {
                    // exists wildcard
                    if (i == vec_path_num - 1 && t->nodes_num == 0) {
                        return -1;
                    }
                }
                if (strcmp(t->path, vec_path[i]) == 0) {
                    if (t->type == NODE_TYPE_ABS && i == vec_path_num - 1) {
                        // exists abs path
                        return -1;
                    }
                    if(t->type == NODE_TYPE_NONE && i == vec_path_num - 1) {
                        node = t;
                        node->type = NODE_TYPE_ABS;
                        if(node->handlers) free(node->handlers);
                        node->handlers_num = 0;
                        cont = true;
                        break;
                    }

                    nodes = &(t->nodes);
                    nodes_num = &(t->nodes_num);
                    cont = true;
                    break;
                }
            }
            if(cont) continue;

            (*nodes_num)++;
            if (*nodes == NULL) {
                *nodes = calloc(1, sizeof(route_info_t));
            } else {
                *nodes = realloc(*nodes, *nodes_num * sizeof(route_node_t));
            }
            node = &((*nodes)[*nodes_num - 1]);
            memset(node, 0, sizeof(*node));
        }
        node->type = type;
        if(type == NODE_TYPE_ABS && i != vec_path_num - 1) node->type = NODE_TYPE_NONE;
        node->path = strdup(tpath);

        if(*nodes_num >= 2) {
            route_node_t* last = &((*nodes)[*nodes_num - 2]);
            if(last->type == NODE_TYPE_PARAM && 
                node->type != NODE_TYPE_PARAM) {
                route_node_t tmp = {0};
                memcpy(&tmp, last, sizeof(tmp));
                memmove(last, node, sizeof(route_node_t));
                memcpy(node, &tmp, sizeof(tmp));

                node = last;
            } 
        }

        nodes = &(node->nodes);
        nodes_num = &(node->nodes_num);
    }

    node->tsr = tsr;

    node->handlers_num = num;
    if(num > 0) {
        node->handlers = calloc(num, sizeof(webapp_handler_t));
        memcpy(node->handlers, handler, num*sizeof(webapp_handler_t));
    }

    return 0;
}
route_info_t* route_get(route_t* route, const char* path)
{
    if(!route || !path || strlen(path) == 0 || path[0] != '/') NULL;

    if (path[1] == 0) {
        route_info_t* route_info = calloc(1, sizeof(*route_info));

        route_info->handlers_num = route->handlers_num;
        route_info->handlers = calloc(route->handlers_num, sizeof(webapp_handler_t));
        memcpy(route_info->handlers, route->handlers, route->handlers_num * sizeof(webapp_handler_t));
        return route_info;
    }

    char vec_path[ROUTE_MAX_DEPTH][ROUTE_MAX_PATH] = {0};
    int vec_path_num = split(path+1, '/', vec_path, ROUTE_MAX_PATH, ROUTE_MAX_DEPTH);
    if(vec_path_num > 0 && vec_path[vec_path_num - 1][0] == 0) {
        vec_path_num--;
    }

    route_node_t* nodes = route->nodes;
    int nodes_num = route->nodes_num;
    route_node_t* node = NULL;

    int param_num = 0;
    param_t* param = NULL;

    for (int i = 0; i < vec_path_num; i++) {
        const char* tpath = vec_path[i];
        node = NULL;
        bool found = false;
        bool exit_loop = false;
        for(int j=0; j<nodes_num; j++) {
            found = false;
            node = &(nodes[j]);
            if(node->type == NODE_TYPE_PARAM || node->type == NODE_TYPE_WILDCARD) {
                param_num++;
                if(param == NULL) {
                    param = calloc(1, sizeof(param_t));
                }else{
                    param = realloc(param, param_num * sizeof(param_t));
                }
                param[param_num-1].key = strdup(node->path + 1);
                if(node->type == NODE_TYPE_PARAM) {
                    param[param_num-1].val = strdup(tpath);   
                    if(i ==vec_path_num - 1) {
                        found = true;
                    }
                }else{                    
                    param[param_num-1].val = strdup(strstr(path, tpath));
                    exit_loop = true;
                    break;
                }             
            }else{
                if(strcmp(node->path, tpath) == 0) {
                    found = true;
                }
            }
            if(found) {
                nodes = node->nodes;
                nodes_num = node->nodes_num;
                
                break;
            }
            node = NULL;
        }
        if(!found || exit_loop) break;
        
    }
    if(node == NULL) {
        if(param) free(param);
        return NULL;
    }
    route_info_t* route_info = calloc(1, sizeof(*route_info));
    
    route_info->tsr = node->tsr;
    route_info->handlers_num = node->handlers_num;
    route_info->handlers = calloc(node->handlers_num, sizeof(webapp_handler_t));
    memcpy(route_info->handlers, node->handlers, node->handlers_num*sizeof(webapp_handler_t));

    route_info->params = param;
    route_info->params_num = param_num;

    return route_info;
}
void route_info_free(route_info_t* info)
{
    if(info) {
        if(info->handlers) free(info->handlers);
        if(info->params) free(info->params);
        free(info);
    }
}

#ifndef NDEBUG
static void print_hanlders(int num, webapp_handler_t* handlers)
{
    if(num <= 0) return;
    printf("  --  ");
    for(int i=0; i<num; i++) {
        printf("%d#0x%x\n", i+1, (unsigned)handlers[i]);
    }
}

static void route_walk_node(const char* prefix, route_node_t* node)
{
    if (!node) return;
    if (node->handlers_num) {
        printf("%s%s", prefix, node->path);
        if (node->tsr)
            printf("/");
        print_hanlders(node->handlers_num, node->handlers);
    }

    for (int i = 0; i < node->nodes_num; i++) {
        char buf[2048] = { 0 };
        snprintf(buf, sizeof(buf) - 1, "%s%s/", prefix, node->path);
        route_walk_node(buf, &(node->nodes[i]));
    }
}

void route_walk(route_t* route)
{
    if(!route) return;
    if(route->handlers) {
        printf("/");
        print_hanlders(route->handlers_num, route->handlers);
    }
    for(int i=0; i<route->nodes_num; i++) {
        route_walk_node("/", &(route->nodes[i]));
    }
}
#endif
