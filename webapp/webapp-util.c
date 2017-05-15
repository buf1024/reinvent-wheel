#include <sys/resource.h>

#include "webapp-privt.h"
#include "webapp-util.h"
#include "webapp.h"

#ifndef MAX_OPEN_FILE
#define MAX_OPEN_FILE 1024
#endif

int get_cpu_count(void)
{
    long n_online_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (n_online_cpus < 0) {
        return 1;
    }
    return (int)n_online_cpus;
}
int get_max_open_file_count(void)
{
    struct rlimit r;

    if (getrlimit(RLIMIT_NOFILE, &r) < 0) {
        printf("getrlimit");
        return 0;
    }

    if (r.rlim_max != r.rlim_cur) {
        if (r.rlim_max == RLIM_INFINITY)
            r.rlim_cur = MAX_OPEN_FILE;
        else if (r.rlim_cur < r.rlim_max)
            r.rlim_cur = r.rlim_max;
        if (setrlimit(RLIMIT_NOFILE, &r) < 0) {
            printf("setrlimit");
            return 0;
        }
    }

    return (int)r.rlim_cur;
}


int epoll_add_fd(int epfd, int evt, int fd)
{
	struct epoll_event event;
	event.events = evt | EPOLLERR | EPOLLHUP;
	event.data.ptr = fd;

	if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) != 0) {
		debug("epoll add failed, errno=%d\n", errno);
		return -1;
	}
	return 0;
}


int epoll_mod_fd(int epfd, int evt, int fd)
{
	struct epoll_event event;
	event.events = evt | EPOLLERR | EPOLLHUP;
	event.data.ptr = fd;

	if(epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event) != 0) {
		debug("epoll add failed, errno=%d\n", errno);
		return -1;
	}
	return 0;
}

int epoll_del_fd(int epfd, int fd)
{
	struct epoll_event event;

	if(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &event) != 0) {
		debug("epoll del failed, errno=%d\n", errno);
		return -1;
	}
	return 0;
}

const char* http_method_str(int m)
{
    static const char* http_methods[] = {
        [HTTP_GET] = "GET",
        [HTTP_POST] = "POST",
        [HTTP_PUT] = "PUT",
        [HTTP_DELETE] = "DELETE",
        [HTTP_HEAD] = "HEAD",
        [HTTP_OPTIONS] = "OPTIONS",
    };

    assert(m >= 0 && m <= HTTP_OPTIONS);

    return http_methods[m];
}
int split(const char* text, char needle, char** dest, int size, int num)
{
    if (!text || text[0] == 0 || !dest || size <= 0 || num <= 0) {
        return -1;
    }
    const char* orig = text;
    const char* tmp = text;

    int cur_num = 0;
    while ((tmp = strchr(tmp, needle)) != NULL) {
        int len = tmp - orig;

        if (len >= size) {
            return -1;
        }
        if (cur_num >= num) {
            return -1;
        }

        memcpy((char*)dest + size*cur_num, orig, len);
        *((char*)dest + size*cur_num + len) = 0;

        cur_num++;
        tmp++;
        orig = tmp;
    }
    if(orig != NULL){
        strcpy((char*)dest + size*cur_num, orig);
        cur_num ++ ;
    }
    if(cur_num == 0){
        strcpy((char*)dest, text);
        cur_num++;
    }
    return cur_num;
}
