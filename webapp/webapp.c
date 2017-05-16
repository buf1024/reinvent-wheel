#include <stdarg.h>
#include "webapp-privt.h"
#include "webapp.h"
#include "webapp-task.h"

#define WEBAPP_DO_ADD_HANDLER(w, m, p)                    \
    do {                                                  \
        if (!w->route[m]) {                               \
            web->route[m] = route_new(w);                 \
        }                                                 \
        int n = 0;                                        \
        webapp_handler_t* h = NULL;                       \
        va_list ap;                                       \
        va_start(ap, p);                                  \
        webapp_va_handlers(ap, &n, &h);                   \
        va_end(ap);                                       \
        w = webapp_http_handler(w, w->route[m], p, n, h); \
        if (h) {                                          \
            free(h);                                      \
        }                                                 \
    } while (0)

static bool running = false;
static pthread_mutex_t job_wait_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t job_wait_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t job_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

static webapp_t* the_webapp = NULL;

static void* job_thread(void *data)
{
	webapp_t* web = (webapp_t*)data;
	debug("job thread(%ld)", web->job_tid);
    pthread_mutex_lock(&job_wait_mutex);
    int job_wait_sec = 1;
    
    while (running) {
        bool had_job = false;

        if (!pthread_mutex_lock(&job_queue_mutex)) {
            //struct job *job;

            //list_for_each(&jobs, job, jobs)
            //    had_job |= job->cb(job->data);

            pthread_mutex_unlock(&job_queue_mutex);
        }


        if (had_job)
            job_wait_sec = 1;
        else if (job_wait_sec < 15)
            job_wait_sec++;
        
        struct timeval now;
        gettimeofday(&now, NULL);
        struct timespec rgtp = { now.tv_sec + job_wait_sec, now.tv_usec * 1000 };

        pthread_cond_timedwait(&job_wait_cond, &job_wait_mutex, &rgtp);

    }
     pthread_mutex_unlock(&job_wait_mutex);

    return NULL;
}

static void* work_thread(void* data)
{
	webapp_thread_t* t = (webapp_thread_t*)data;
	webapp_t* web = t->web;
	// todo things to wait
	pthread_barrier_wait(web->barrier);
    debug("work thread(%ld) loop start.\n", t->tid);
	
	for (;;) {
		if(webapp_task_read(t) != 0) {
			debug("webapp_task_read failed.\n");
			break;
		}
		if(webapp_task_process(t) != 0) {
			debug("webapp_task_process failed.\n");
			break;
		}
		if(webapp_task_write(t) != 0) {
			debug("webapp_task_write failed.\n");
			break;
		}
	}
	return NULL;
}

int schedule_fd(webapp_t* web, int fd)
{

	static unsigned long counter = 0;

	int index = counter++ % web->thread_num;
	webapp_thread_t* t = &(web->threads[index]);

	debug("select thread %d to handle.\n", index);

	intptr_t ptr = (intptr_t)fd;
	if(write(t->fd[1], (void*)&ptr, sizeof(intptr_t)) != sizeof(intptr_t)) {
		debug("write fd to client failed.\n");
		return -1;
	}
	return 0;
}

webapp_t* webapp_new()
{
	webapp_t* web = the_webapp;
	if(web) return web;

    web = (webapp_t*)calloc(1, sizeof(*web));
    if(!web) return NULL;
	
	web->nfd = get_max_open_file_count();
	if(web->nfd <= 0) goto ERR;

	web->conns = (connection_t*)calloc(web->nfd, sizeof(connection_t));
	if(!web->conns) goto ERR;

    web->epfd = epoll_create1(EPOLL_CLOEXEC);
    if (web->epfd <= 0) goto ERR;

	web->thread_num = get_cpu_count();
	web->threads = calloc(web->thread_num, sizeof(webapp_thread_t));
	if(!web->threads) goto ERR;

    web->barrier = (pthread_barrier_t*)calloc(1, sizeof(*web->barrier));
    if (pthread_barrier_init(web->barrier, NULL,
		(unsigned int)web->thread_num + 1) != 0) {
		goto ERR;
    }

    for (int i = 0; i < web->thread_num; i++) {
        webapp_thread_t* t = &web->threads[i];

        t->web = web;
        t->nfd = web->nfd / web->thread_num;
        t->evts = (struct epoll_event*)calloc(t->nfd, sizeof(struct epoll_event));
        if (!t->evts) goto ERR;

        t->epfd = epoll_create1(EPOLL_CLOEXEC);
        if (t->epfd <= 0) goto ERR;

        if (pipe2(t->fd, O_NONBLOCK | O_CLOEXEC) != 0) goto ERR;

        connection_t* con = &(web->conns[t->fd[0]]);
        con->fd = t->fd[0];

        if (epoll_add_fd(t->epfd, EPOLLIN, con) != 0) {
            debug("epoll_add_fd failed.\n");
            goto ERR;
        }
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        if (pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM) != 0) {
            debug("pthread_attr_setscope failed.\n");
        }

        if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) != 0) {
            debug("pthread_attr_setdetachstate failed.\n");
        }

        pthread_create(&t->tid, &attr, work_thread, t);
    }

    running = true;
    if (pthread_create(&web->job_tid, NULL, job_thread, NULL) < 0){
		debug("low level job create failed.\n");
		running = false;
		goto ERR;
	}

#ifdef SCHED_IDLE
    struct sched_param sched_param = {
        .sched_priority = 0
    };
    if (pthread_setschedparam(web->job_tid, SCHED_IDLE, &sched_param) < 0){
        debug("pthread_setschedparam");
	}
#endif  /* SCHED_IDLE */

	the_webapp = web;

    return web;
ERR:
	if(running) {
		running = false;
		pthread_cond_signal(&job_wait_cond);

		pthread_join(web->job_tid, NULL);
	}
	for (int i = 0; i < web->thread_num; i++) {
        webapp_thread_t* t = &web->threads[i];
		if(t->evts) free(t->evts);
		if(t->fd[0] > 0 && t->fd[1] > 0) {
			close(t->fd[0]);
			close(t->fd[1]);
		}
		if(t->epfd > 0) {
			close(t->epfd);
		}
	}
	if(web->threads) free(web->threads);
	if(web->conns) free(web->conns);
	if(web->epfd > 0) close(web->epfd);
	if(web->evts) free(web->evts);
	free(web);

	return NULL;
}
void webapp_destroy(webapp_t* web)
{

}

int webapp_run(webapp_t* web, const char* host)
{
    if (!web) return -1;
    if (host == NULL || strlen(host) == 0) {
        host = DEFAULT_LISTEN_HOST;
    }
    // host:port
    char* listen = NULL;
    int port = 0;

    int fd = tcp_server(listen, port, DEFAULT_LISTEN_BACK_LOG);

    if (fd <= 0) {
        debug("listen server failed, host=%s, port=%d\n", listen, port);
        return -1;
    }
    debug("listening ip=%s, port=%d\n", listen, port);

    tcp_reuse_addr(fd, true);
    tcp_noblock(fd, true);

    struct linger linger = { 1, 1 };
    setsockopt(fd, SOL_SOCKET, SO_LINGER, (void*)&linger, (socklen_t)sizeof(struct linger));
    int fastopen_opt = 5;
    setsockopt(fd, SOL_TCP, TCP_FASTOPEN, (void*)&fastopen_opt, (socklen_t)sizeof(int));
    int quick_ack = 0;
    setsockopt(fd, SOL_TCP, TCP_QUICKACK, (void*)&quick_ack, (socklen_t)sizeof(int));

    connection_t* con = &web->conns[fd];
    con->fd = fd;
    if (epoll_add_fd(web->epfd, EPOLLIN, con) != 0) {
        debug("epoll add failed.\n");
        return -1;
    }
    web->nfd++;
	if(web->evts) {
		web->evts = (struct epoll_event*)realloc(web->evts, web->nfd*sizeof(struct epoll_event));
		if(!web->evts) {
			debug("realloc epoll events failed.\n");
			return -1;
		}
	}

	pthread_barrier_wait(web->barrier);
	pthread_barrier_destroy(web->barrier);
	web->barrier = NULL;
	debug("main loop start.\n");

	for(;;) {

		int rv = epoll_wait(web->epfd, web->evts, web->nfd, 100);
		if(rv == 0) {
			debug("main_timer_task failed.\n");
			continue;
		}else if(rv < 0) {
			// todo signal			
			debug("epoll_wait error, errno=%d\n", errno);
			break;
		} else {
			for (int i = 0; i < rv; i++) {
				struct epoll_event* ev = &(web->evts[i]);
				connection_t* con = (connection_t*) ev->data.ptr;

				struct sockaddr_storage sa;
				socklen_t salen = sizeof(sa);

				int fd = accept4(con->fd, (struct sockaddr*) &sa, &salen,
						SOCK_NONBLOCK);

				if (fd < 0) {
					debug("accept failed.\n");
					continue;
				}
				char client_ip[64] = {0};
				int client_port = 0;
				tcp_sock_name(fd, client_ip, sizeof(client_ip), &client_port);

				debug("accept client: ip=%s, port=%d\n", client_ip, client_port);

				if (schedule_fd(web, fd) != 0) {
					debug("schedule fd to thread failed.\n");
					close(fd);
				}
			}
		}
	}

    return 0;
}

webapp_t* webapp_use(webapp_t* web, webapp_handler_t handler)
{
    if (!web->group) {
        web->midware_num++;
        if (!web->midware) {
            web->midware = calloc(1, sizeof(webapp_handler_t));
            web->midware[0] = handler;
            return web;
        }

        web->midware = realloc(web->midware, web->midware_num * sizeof(webapp_handler_t));
        web->midware[web->midware_num - 1] = handler;
    } else {
		web->group_midware_num++;
        if (!web->group_midware) {
            web->group_midware = calloc(1, sizeof(webapp_handler_t));
            web->group_midware[0] = handler;
            return web;
        }

        web->group_midware = realloc(web->group_midware, web->group_midware_num * sizeof(webapp_handler_t));
        web->midware[web->group_midware_num - 1] = handler;
    }

    return web;
}
webapp_t* webapp_group_begin(webapp_t* web, const char* pattern)
{
	if(pattern == NULL || pattern[0] == 0) return NULL;
	
	webapp_group_end(web);
	web->group = true;
	web->group_pattern = strdup(pattern);

    return web;
}
webapp_t* webapp_group_end(webapp_t* web)
{
	web->group = false;
	if(web->group_pattern) {
		free(web->group_pattern);
		web->group_pattern = NULL;
	}
	if(web->group_midware) {
		free(web->group_midware);
		web->group_midware = NULL;
		web->group_midware_num = 0;
	}
    return web;
}

static int webapp_merge_handlers(int src_num1, webapp_handler_t* src_handlers1,
	int src_num2, webapp_handler_t* src_handlers2, 
	int src_num3, webapp_handler_t* src_handlers3,
	int* dst_num, webapp_handler_t** dst_handlers)
{
	*dst_num = src_num1 + src_num2;
	*dst_handlers = calloc(*dst_num, sizeof(webapp_handler_t));
	
	for(int i=0; i<src_num1; i++){
		(*dst_handlers)[i] = src_handlers1[i];
	}
	for(int i=0; i<src_num2; i++){
		(*dst_handlers)[src_num1 - i] = src_handlers2[i];
	}
	for(int i=0; i<src_num3; i++){
		(*dst_handlers)[src_num1 + src_num2- i] = src_handlers3[i];
	}
	return 0;
}

static webapp_t* webapp_http_handler(webapp_t* web, route_t* route, const char* pattern, 
	int num, webapp_handler_t* handlers)
{
	int n = 0;
	webapp_handler_t* h = NULL;

	webapp_merge_handlers(web->midware_num, web->midware, 
		web->group_midware_num, web->group_midware,
		num, handlers, &n, &h);
	
	if(route_add_group(route, pattern, n, h) != 0) {
		if(h) free(h);
		return NULL;
	}
	return web;	
}
static int webapp_va_handlers(va_list ap, int* num, webapp_handler_t** handlers)
{
	while (1){
        webapp_handler_t h = va_arg(ap, webapp_handler_t);
        if (h == NULL)
            break;

        (*num)++;
		if(!(*handlers)) {
			*handlers = calloc(1, sizeof(webapp_handler_t));
			(*handlers)[0] = h;
			continue;
		}
        (*handlers) = realloc((*handlers), (*num) * sizeof(webapp_handler_t));
        (*handlers)[(*num) - 1] = h;
	}
	return 0;
}
webapp_t* webapp_get(webapp_t* web, const char* pattern, ...)
{
	WEBAPP_DO_ADD_HANDLER(web, HTTP_GET, pattern);
	return web;
}
webapp_t* webapp_post(webapp_t* web, const char* pattern, ...)
{
	WEBAPP_DO_ADD_HANDLER(web, HTTP_POST, pattern);
	return web;
}
webapp_t* webapp_put(webapp_t* web, const char* pattern, ...)
{
	WEBAPP_DO_ADD_HANDLER(web, HTTP_PUT, pattern);
	return web;
}
webapp_t* webapp_delete(webapp_t* web, const char* pattern, ...)
{
	WEBAPP_DO_ADD_HANDLER(web, HTTP_DELETE, pattern);
	return web;
}
webapp_t* webapp_head(webapp_t* web, const char* pattern, ...)
{
	WEBAPP_DO_ADD_HANDLER(web, HTTP_HEAD, pattern);
	return web;
}
webapp_t* webapp_options(webapp_t* web, const char* pattern, ...)
{
	WEBAPP_DO_ADD_HANDLER(web, HTTP_OPTIONS, pattern);
	return web;
}
