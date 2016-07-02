/*
 * evt.c
 *
 *  Created on: 2012-12-14
 *      Author: Luo Guochun
 */

#include "evt.h"
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#if 0

#ifndef HAVE_POLL
#define HAVE_POLL
#endif

#ifdef HAVE_KQUEUE
#    include "kqueue.c"
#else
#    ifdef HAVE_EPOLL
#        include "epoll.c"
#    else
#        ifdef HAVE_POLL
#            include "poll.c"
#        else
#            ifdef HAVE_SELECT
#                include "select.c"
#            else
#                error "unknown event back-end."
#            endif
#        endif
#    endif
#endif

#endif

struct event_s
{
    int exit;
    int defto;                // if poll time out not specific use this as the poll timeout value
    struct evt_timeout* to;   // poll timeout list
    struct list* fds;         //
    struct list* signals;     //
    struct list* timers;      //
    void* data;               // back-end specific data
};
struct evt_poll_s
{
    int fd;               //monitor fd
    int mask;             //interest event
    int evt;              //event fired
    evt_callback cb;
    void* args;
};
struct evt_signal_s
{
    int sig;             //interest signo
    evt_callback cb;     //calback function
    void* args;          //callback args
};
struct evt_timer_s
{
    int intval;     //in ms
    unsigned long lastval;    //last time
    evt_callback cb;
    void* args;
};
struct evt_timeout_s
{
    int flag;
    int timeout;    //is ms
    evt_callback cb;
    void* args;
};



static int __g_sig[SIGUNUSED + 1] = { 0 };

static void __evt_sig(int signo)
{
    __g_sig[signo] = signo;
}

static int __evt_cmp_fd_list(void* v1, void* v2)
{
    struct evt_poll* p1 = (struct evt_poll*) v1;
    int fd = *((int*) (v2));
    if (p1->fd > fd)
        return 1;
    if (p1->fd == fd)
        return 0;
    return -1;
}
static int __evt_cmp_signal_list(void* v1, void* v2)
{
    struct evt_signal* p1 = (struct evt_signal*) v1;
    int signo = *((int*) (v2));
    if (p1->sig > signo)
        return 1;
    if (p1->sig == signo)
        return 0;
    return -1;
}
static int __evt_cmp_timer_list(void* v1, void* v2)
{
    struct evt_timer* p1 = (struct evt_timer*) v1;
    int tm = *((int*) (v2));
    if (p1->intval > tm)
        return 1;
    if (p1->intval == tm)
        return 0;
    return -1;
}

static int __evt_fd_add(struct event* evt, int fd, int mask, evt_callback cb,
        void* args)
{
    struct list_item* itm = NULL;
    struct evt_poll* ep = NULL;
    int ret = 0;
    ret = be_evt_add(evt, fd, mask);
    if (!ret)
        return ret;
    itm = list_pull(evt->fds, &fd);
    if (itm) {
        ep = (struct evt_poll*) itm->data;
    } else {
        ep = (struct evt_poll*) malloc(sizeof(*ep));
        memset(ep, 0, sizeof(*ep));
        if (list_push_tail(evt->fds, ep) != 0) {
            free(ep);
            return -1;
        }
    }
    ep->fd = fd;
    ep->mask |= mask;
    ep->cb = cb;
    ep->args = args;

    return 0;
}
static int __evt_signal_add(struct event* evt, int signo, evt_callback cb,
        void* args)
{
    struct list_item* itm = NULL;
    struct evt_signal* es = NULL;

    struct sigaction act, oact;

    itm = list_pull(evt->signals, &signo);
    if (itm) {
        es = (struct evt_signal*) itm->data;
    } else {
        es = (struct evt_signal*) malloc(sizeof(*es));
        memset(es, 0, sizeof(*es));
        if (list_push_tail(evt->signals, es) != 0) {
            free(es);
            return 0;
        }
    }

    es->sig = signo;
    es->cb = cb;
    es->args = args;

    act.sa_handler = __evt_sig;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    // TODO
    act.sa_flags |= SA_RESTART;

    if (sigaction(signo, &act, &oact) < 0)
        return -1;

    return 0;
}
static int __evt_timer_add(struct event* evt, int intval, evt_callback cb,
        void* args)
{
    struct list_item* itm = NULL;
    struct evt_timer* et = NULL;
    struct timeval tv = { 0 };

    itm = evt->timers->fst_itm;

    while (itm) {
        if (__evt_cmp_timer_list(itm->data, &intval) == 0) {
            et = (struct evt_timer*) itm->data;
            break;
        }
        itm = itm->nxt_itm;
    }
    if (!et) {
        et = (struct evt_timer*) malloc(sizeof(*et));
        if (!et)
            return -1;
        if (list_push_tail(evt->timers, et) != 0)
            return -1;
    }

    gettimeofday(&tv, NULL );

    et->intval = intval;
    et->lastval = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    et->cb = cb;
    et->args = args;

    return 0;
}
static int __evt_timeout_add(struct event* evt, int to, evt_callback cb,
        void* args)
{
    evt->to->timeout = to;
    evt->to->cb = cb;
    evt->to->args = args;

    return 0;
}

struct event* evt_new()
{
    struct event* evt = (struct event*) malloc(sizeof(*evt));
    if (!evt)
        return NULL ;

    evt->fds = list_new();
    evt->signals = list_new();
    evt->timers = list_new();
    evt->to = (struct evt_timeout*) malloc(sizeof(*(evt->to)));
    if (!evt->fds || !evt->signals || !evt->timers || !evt->to) {
        if (evt->fds) {
            list_del(evt->fds);
        }
        if (evt->signals) {
            list_del(evt->signals);
        }
        if (evt->timers) {
            list_del(evt->timers);
        }
        if (evt->to) {
            free(evt->to);
        }
        free(evt);
        return NULL ;
    }

    memset(__g_sig, 0, sizeof(int) * SIGUNUSED + 1);

    evt->defto = EVT_DEFAULT_POLL_TIMEOUT; //100ms
    evt->data = NULL;

    list_set_attr(evt->fds, NULL, __evt_cmp_fd_list, NULL, NULL);
    list_set_attr(evt->signals, evt->signals->data, __evt_cmp_signal_list, NULL,
            NULL);
    list_set_attr(evt->timers, NULL, __evt_cmp_timer_list, NULL, NULL);

    evt->to->args = NULL;
    evt->to->cb = NULL;
    evt->to->timeout = 0;

    be_evt_init(evt);

    return evt;
}
int evt_del(struct event* evt)
{
    struct list_item* itm = NULL;
    struct evt_poll* ep = NULL;

    be_evt_destroy(evt);

    itm = evt->fds->fst_itm;
    while (itm) {
        ep = (struct evt_poll*) itm->data;
        if (ep->buf)
            evt_buf_del(ep->buf);
        itm = itm->nxt_itm;
    }

    list_del(evt->fds);
    free(evt->signals->data);
    list_del(evt->signals);
    list_del(evt->timers);
    free(evt->to);

    free(evt);

    return 0;
}

struct evt_buf* evt_buf_new()
{
    struct evt_buf* buf = (struct evt_buf*) malloc(sizeof(*buf));
    if (buf) {
        memset(buf, 0, sizeof(*buf));
    }
    return buf;
}
int evt_buf_del(struct evt_buf* buf)
{
    if (buf->rbuf) {
        free(buf->rbuf);
    }
    if (buf->wbuf) {
        free(buf->wbuf);
    }
    free(buf);
    return 0;
}

int evt_buf_set_attr(struct evt_buf* buf, size_t cap, size_t rwater,
        size_t wwater)
{
    buf->cap = cap;
    buf->rwater = rwater;
    buf->wwater = wwater;
    if (buf->rbuf) {
        free(buf->rbuf);
    }
    if (buf->wbuf) {
        free(buf->wbuf);
    }
    buf->rbuf = (char*) malloc(buf->cap);
    buf->wbuf = (char*) malloc(buf->cap);

    return 0;
}
int evt_buf_rget(struct evt_buf* buf, char* req, size_t s)
{
    int r = s >= buf->rpos ? buf->rpos : s;
    memcpy(req, buf->rbuf, r);
    return r;
}
int evt_buf_rdrag(struct evt_buf* buf, char* req, size_t s)
{
    int r = s >= buf->rpos ? buf->rpos : s;
    memcpy(req, buf->rbuf, r);
    memmove(buf->rbuf, buf->rbuf + r, buf->rpos - r);
    buf->rbuf -= r;
    return r;
}
int evt_buf_rput(struct evt_buf* buf, const char* res, size_t s)
{
    if (buf->cap - buf->rpos < s)
        return -1;
    memcpy(buf->rbuf + buf->rpos, res, s);
    return s;
}
int evt_buf_wget(struct evt_buf* buf, char* req, size_t s)
{
    int w = s >= buf->wpos ? buf->wpos : s;
    memcpy(req, buf->wbuf, w);
    return w;
}
int evt_buf_wdrag(struct evt_buf* buf, char* req, size_t s)
{
    int w = s >= buf->wpos ? buf->wpos : s;
    memcpy(req, buf->wbuf, w);
    memmove(buf->wbuf, buf->wbuf + w, buf->wpos - w);
    buf->wbuf -= w;
    return w;
}
int evt_buf_wput(struct evt_buf* buf, const char* res, size_t s)
{
    if (buf->cap - buf->wpos < s)
        return -1;
    memcpy(buf->wbuf + buf->wpos, res, s);
    return s;
}

int evt_assoc_buf(struct event* evt, int fd, struct evt_buf* buf)
{
    struct list_item* itm = NULL;
    struct evt_poll* ep = NULL;
    itm = list_pull(evt->fds, &fd);
    if (itm) {
        ep = (struct evt_poll*) evt->data;
        ep->buf = buf;

        return 0;
    }
    return -1;
}
struct evt_buf* evt_get_buf(struct event* evt, int fd)
{
    struct list_item* itm = NULL;
    struct evt_poll* ep = NULL;
    struct evt_buf* buf = NULL;
    itm = list_pull(evt->fds, &fd);
    if (itm) {
        ep = (struct evt_poll*) itm->data;
        buf = ep->buf;
    }
    return buf;
}

int evt_add(struct event* evt, int type, int fd, int mask, evt_callback cb,
        void* args)
{
    if (type == EVT_SOCK) {
        return __evt_fd_add(evt, fd, mask, cb, args);
    } else if (type == EVT_SIGNAL) {
        return __evt_signal_add(evt, fd, cb, args);
    } else if (type == EVT_TIMER) {
        return __evt_timer_add(evt, fd, cb, args);
    } else if (type == EVT_TIMEOUT) {
        return __evt_timeout_add(evt, fd, cb, args);
    }
    return -1;
}
int evt_drop(struct event* evt, int fd, int type)
{
    struct list* lst = NULL;
    if (type == EVT_SOCK) {
        lst = evt->fds;
    } else if (type == EVT_SIGNAL) {
        lst = evt->signals;
    } else if (type == EVT_TIMER) {
        lst = evt->timers;
    }
    if (type == EVT_SOCK || type == EVT_SIGNAL || type == EVT_TIMER) {
        return list_drop(lst, &fd);
    } else if (type == EVT_TIMEOUT) {
        memset(evt->to, 0, sizeof(*(evt->to)));
        return 0;
    }
    return -1;
}

/*
 #define __evt_fd_poll(evt) \
    do { \
    }while(0)
 #define __evt_signal_poll(evt) \
    do { \
    }while(0)
 #define __evt_timer_poll(evt) \
    do { \
    }while(0)
 #define __evt_timeout_poll(evt) \
    do { \
    }while(0)
 #define __evt_bf_loop(evt) \
    do { \
    }while(0)
 #define __evt_af_loop(evt) \
    do { \
    }while(0)
 */

static void __evt_fd_poll(struct event* evt)
{
    struct list_item* itm = evt->fds->fst_itm;
    struct evt_poll* ep = NULL;
    struct evt_buf* buf = NULL;
    int ret = 0;

    if (be_evt_poll(evt) > 0) {
        while (itm) {
            ep = (struct evt_poll*) itm->data;
            if (ep->mask & ep->evt) {
                buf = ep->buf;
                if (buf) {
                    if (ep->evt & EVT_READ) {
                        while (ret < buf->rwater - buf->rpos) {
                            ret = read(ep->fd, buf->rbuf + buf->rpos,
                                    buf->rwater - buf->rpos);
                            if (ret == 0) {
                                ep->evt |= EVT_HUP;
                            } else if (ret < 0) {
                                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                    break;
                                }
                                if (errno == EINTR) {
                                    continue;
                                }
                                ep->evt |= EVT_ERROR;
                            } else {
                                buf->rpos += ret;
                            }
                        }
                    }
                    if (ep->evt & EVT_WRITE) {
                        while (ret < buf->wwater - buf->wpos) {
                            ret = write(ep->fd, buf->wbuf + buf->wpos,
                                    buf->wwater - buf->wpos);
                            if (ret == 0) {
                                ep->evt |= EVT_HUP;
                            } else if (ret < 0) {
                                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                    break;
                                }
                                if (errno == EINTR) {
                                    continue;
                                }
                                ep->evt |= EVT_ERROR;
                            } else {
                                buf->wpos += ret;
                            }
                        }
                    }
                }
                ep->cb(evt, ep->fd, ep->evt, ep->args);
            }
        }
    }
}

static void __evt_signal_poll(struct event* evt)
{
    struct evt_signal* es = NULL;
    struct list_item* itm = NULL;
    int i = 0;

    for (; i < SIGUNUSED + 1; i++) {
        if (__g_sig[i] != 0) {
            itm = list_pull(evt->signals, __g_sig + i);

            if (itm) {
                es = (struct evt_signal*) itm->data;
                es->cb(evt, es->sig, 0, es->args);
            }
            __g_sig[i] = 0; //mark solve
        }
    }
}

static void __evt_timer_poll(struct event* evt)
{
    struct timeval tv = { 0 };
    struct list_item* itm = NULL;
    struct evt_timer* tmr = NULL;
    int intval = 0;
    unsigned long ms = 0L;

    itm = evt->timers->fst_itm;
    if (itm) {
        gettimeofday(&tv, NULL );
        ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        while (itm) {
            tmr = (struct evt_timer*) itm->data;
            intval = ms - tmr->lastval;

            if (intval >= tmr->intval) {
                tmr->cb(evt, tmr->intval, 0, tmr->args);
                tmr->lastval = ms;
            }

            itm = itm->nxt_itm;
        }
    }
}

static void __evt_timeout_poll(struct event* evt)
{
    if (evt->to->flag) {
        evt->to->cb(evt, evt->to->timeout, 0, evt->to->args);
        evt->to->flag = 0;
    }
}

int evt_loop(struct event* evt)
{
    while (!evt->exit) {
        __evt_fd_poll(evt);
        __evt_signal_poll(evt);
        __evt_timer_poll(evt);
        __evt_timeout_poll(evt);
    }
    return 0;
}

int evt_loop_exit(struct event* evt)
{
    evt->exit = 1;
    return 0;
}
