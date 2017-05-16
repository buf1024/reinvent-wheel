#pragma once

typedef struct connection_s connection_t;

int epoll_add_fd(int epfd, int evt, connection_t* con);
int epoll_mod_fd(int epfd, int evt, connection_t* con);
int epoll_del_fd(int epfd, connection_t* con);

int get_cpu_count(void);
int get_max_open_file_count(void);

const char* http_method_str(int m);

int split(const char* text, char needle, char** dest, int size, int num);
