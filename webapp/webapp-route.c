
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "webapp-route.h"

static route_node_t* find_node(route_node_t** nodes, int nodes_num, const char* path,
    int s_index, int e_index)
{
    for(int i=0; i<nodes_num; i++) {
        route_node_t* node = nodes[i];
        if(node == NULL) {
            return NULL;
        }
        if (node->type == NODE_TYPE_PARAM || node->type == NODE_TYPE_WILDCARD){
            return node;
        }
        if (strncmp(node->path, path+s_index, e_index - s_index) == 0){
            return node;
        }
    }
    return NULL;
}

route_t* route_new(void* data)
{
    route_t* r = calloc(1, sizeof(*r));
    if(!r) return NULL;

    r->data = data;
    return r;
}
int route_destroy(route_t* route)
{
    return 0;
}

int add_route(route_t* route, const char* path, void* handler, ...)
{
    if(!route || !handler || !path || strlen(path) == 0 || path[0] != '/') return -1;
    
    enum route_note_type_t type = NODE_TYPE_NONE;

    int* nodes_num = NULL;
    route_node_t*** nodes = NULL;
    
    route_node_t* node = NULL;
    if(!route->nodes) {
        route->nodes_num++;
        
        // todo oom check
        route->nodes = (route_node_t**)calloc(route->nodes_num, sizeof(route_node_t*));
    }
    nodes = &(route->nodes);
    nodes_num = &(route->nodes_num);

    int offset = 0;

    int index = 1;
    const char* p = path;
    while(p[index]) {
        char ch = p[index];
        if (ch != ':' && ch != '*' && ch != '/') {
            index++;
            continue;
        }
        if(ch == '/') {
            node = find_node(*nodes, nodes_num, path, offset, index);
            if (node && node->type != NODE_TYPE_NONE ){
                // duplicate
                return -1;
            }
            if (!node){
                (*nodes_num)++;
                // todo oom check
                *nodes = realloc(*nodes, sizeof(route_node_t*));
                *nodes[(*nodes_num) - 1] = calloc(1, sizeof(route_node_t));
                node = *nodes[(*nodes_num) - 1];
            }
            int sz = index - offset + 1;
            char* sub_path = (char*)calloc(1, sz);
            memcpy(sub_path, path + offset, sz - 1);
            offset = index;

            type = NODE_TYPE_ABS;

            index++;
            continue;
        }
        if(ch == '*') {
            node = find_node(*nodes, nodes_num, path, offset, index);
            if (node != NULL) {
                return -1;
            }
            if(p[index - 1] != '/') {
                // no start witch /
                return -1;
            }
            if(!p[index + 1]) {
                // no name
                return -1; 
            }
            int j = index+1;
            while (p[j]) {
                if(!isalnum(p[j])) {
                    // no alpha or num
                    return -1;
                }
                j++;
            }
            int sz = strlen(path) - index + 1;
            char* sub_path = (char*)calloc(1, sz);
            memcpy(sub_path, path + offset, sz - 1);
            offset = index;

            type = NODE_TYPE_WILDCARD;
            break;
        }

        if(ch == ':') {
            node = find_node(*nodes, nodes_num, path, offset, index);
            if (node && node->type != NODE_TYPE_NONE ){
                // duplicate
                return -1;
            }
            if (!node){
                (*nodes_num)++;
                // todo oom check
                *nodes = realloc(*nodes, sizeof(route_node_t*));
                *nodes[(*nodes_num) - 1] = calloc(1, sizeof(route_node_t));
                node = *nodes[(*nodes_num) - 1];
            }
        }
    }
    node->type = type;
    node->handlers_num++;
    if(node->handlers != NULL) {
        node->handlers = realloc(node->handlers, node->handlers_num*sizeof(void*));
        node->handlers[node->handlers_num - 1] = handler;
    }else{
        node->handlers = calloc(1, sizeof(void*));
        node->handlers[node->handlers_num - 1] = handler;
    }
    va_list ap;
    va_start(ap, handler);
    while (1) {
        void* h = va_arg(ap, void*);
        if (h == NULL)
            break;

        node->handlers_num++;
        node->handlers = realloc(node->handlers, node->handlers_num * sizeof(void*));
        node->handlers[node->handlers_num - 1] = h;
    }
    va_end(ap);

    return 0;
}
route_info_t* get_route(route_t* route, const char* path)
{
    return NULL;
}

int route_info_free(route_info_t* info)
{
    return 0;
}