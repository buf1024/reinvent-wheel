#pragma once

#include <stdbool.h>
#include "webapp.h"

typedef struct route_s route_t;
typedef struct route_node_s route_node_t;

typedef struct param_s param_t;
typedef struct route_info_s route_info_t;

struct param_s {
    char* key;
    char* val;
};

struct route_info_s {
    int handlers_num;
    webapp_handler_t* handlers;

    int params_num;
    param_t* params;

    bool tsr;
};

struct route_s {
    int nodes_num;
    route_node_t* nodes;
    void* data;
};

enum route_note_type_t {
    NODE_TYPE_NONE,
    NODE_TYPE_ABS,
    NODE_TYPE_PARAM,
    NODE_TYPE_WILDCARD,
};

struct route_node_s {
    char* path;
    enum route_note_type_t type;
    
    int nodes_num;
    route_node_t* nodes;

    int handlers_num;
    webapp_handler_t* handlers;
};

route_t* route_new(void* data);
int route_destroy(route_t* route);

int route_add(route_t* route, const char* path, webapp_handler_t handler, ...);
route_info_t* route_get(route_t* route, const char* path);
void route_info_free(route_info_t* info);



