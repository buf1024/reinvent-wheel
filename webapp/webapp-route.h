#pragma once

#include <stdbool.h>

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
    void** handlers;

    int params_num;
    param_t** params;

    bool tsr;
};

struct route_s {
    int nodes_num;
    route_info_t** nodes;
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
    route_node_t** nodes;

    int handlers_num;
    void** handlers;
};

route_t* route_new(void* data);
int route_destroy(route_t* route);

int add_route(route_t* route, const char* path, void* handler, ...);
route_info_t* get_route(route_t* route, const char* path);

int route_info_free(route_info_t* info);


