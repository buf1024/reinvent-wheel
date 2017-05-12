#pragma once



int epoll_add_fd(int epfd, int evt, int fd);
int epoll_mod_fd(int epfd, int evt, int fd);
int epoll_del_fd(int epfd, int fd;);

int get_cpu_count(void);
int get_max_open_file_count(void);

const char* http_method_str(int m);