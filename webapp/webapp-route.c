
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
static int route_destroy_node(route_node_t* nodes) {
    if(!nodes) return 0;
    if(nodes->nodes_num == 0) {
        if(nodes->handlers) free(nodes->handlers);
        free(nodes);
    }else{
        for(int i=0; i<nodes->nodes_num; i++) {
            route_destroy_node(nodes->nodes);
        }
        free(nodes->nodes);
        if(nodes->handlers) free(nodes->handlers);
        free(nodes);
    }
    return 0;
}
int route_destroy(route_t* route)
{
    if(!route) return 0;

    return route_destroy_node(route->nodes);    
}

int route_add(route_t* route, const char* path, webapp_handler_t handler, ...)
{
    if(!route || !handler || !path || strlen(path) == 0 || path[0] != '/') return -1;
    
    enum route_note_type_t type = NODE_TYPE_NONE;

    char vec_path[ROUTE_MAX_DEPTH][ROUTE_MAX_PATH] = {0};
    int vec_path_num = split(path+1, '/', vec_path, ROUTE_MAX_PATH, ROUTE_MAX_DEPTH);

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
            if(tpath[0] == '*') type = NODE_TYPE_WILDCARD;
            if(tpath[0] == ':') type = NODE_TYPE_PARAM;
        }
        
        if(*nodes == NULL) {
            *nodes  = calloc(1, sizeof(route_node_t));
            (*nodes_num)++;

            node = &((*nodes)[0]);
        }else{
            route_node_t* t = NULL;
            for (int j = 0; j < *nodes_num; j++) {
                t = &((*nodes)[j]);

                if (t->type == NODE_TYPE_PARAM || t->type == NODE_TYPE_WILDCARD) {
                    // exists wildcard
                    return -1;
                }
                if (strcmp(t->path, vec_path[i]) == 0) {
                    if (t->type != NODE_TYPE_NONE && i == vec_path_num - 1) {
                        // exists abs path
                        return -1;
                    }
                    nodes = &(t->nodes);
                    nodes_num = &(t->nodes_num);
                    j = 0;
                }
            }
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

        nodes = &(node->nodes);
        nodes_num = &(node->nodes_num);
    }

    node->handlers_num++;
    node->handlers = calloc(1, sizeof(webapp_handler_t));
    node->handlers[node->handlers_num - 1] = handler;
    
    va_list ap;
    va_start(ap, handler);
    while (1) {
        webapp_handler_t h = va_arg(ap, webapp_handler_t);
        if (h == NULL)
            break;

        node->handlers_num++;
        node->handlers = realloc(node->handlers, node->handlers_num * sizeof(webapp_handler_t));
        node->handlers[node->handlers_num - 1] = h;
    }
    va_end(ap);

    return 0;
}
route_info_t* route_get(route_t* route, const char* path)
{
        if(!route || !path || strlen(path) == 0 || path[0] != '/') NULL;
    
    enum route_note_type_t type = NODE_TYPE_NONE;

    char vec_path[ROUTE_MAX_DEPTH][ROUTE_MAX_PATH] = {0};
    int vec_path_num = split(path+1, '/', vec_path, ROUTE_MAX_PATH, ROUTE_MAX_DEPTH);

    route_node_t* nodes = &(route->nodes);
    route_node_t* node = NULL;

    int param_num = 0;
    param_t* param = NULL;

    for (int i = 0; i < vec_path_num; i++) {
        const char* tpath = vec_path[i];
        for(int j=0; j<nodes->nodes_num; j++) {
            bool found = false;
            node = &(nodes[j]);
            if(node->type == NODE_TYPE_PARAM || node->type == NODE_TYPE_WILDCARD) {
                param_num++;
                if(param == NULL) {
                    param = calloc(1, sizeof(param_t));
                }else{
                    param = realloc(param, param_num * sizeof(param_t));
                }
                param[param_num-1].key = strdup(node->path + 1);
                param[param_num-1].val = strdup(tpath);

                nodes = &(node->nodes);
                found = true;;
            }
            if(strcmp(node->path, tpath) == 0) {
                nodes = &(node->nodes);
                found = true;;
            }
            if(i == vec_path_num - 1 && !found) {
                node = NULL;
            }
        }
    }
    if(node == NULL) {
        if(param) free(param);
        return NULL;
    }
    //todo tsr
    route_info_t* route_info = calloc(1, sizeof(*route_info));
    
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
