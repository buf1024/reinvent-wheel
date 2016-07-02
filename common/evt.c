/*
 * evt.c
 *
 *  Created on: 2012-12-14
 *      Author: Luo Guochun
 */

#include "evt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>


#define EVT_DEFAULT_POLL_TIMEOUT    1000
#define EVT_DEFAULT_EVENT_SIZE      512
#define EVT_DEFAULT_EVENT_GROW_SIZE 128

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

event_t* evt_new(int evt_size, int to)
{
    event_t* evt = (event_t*) malloc(sizeof(*evt));
    if (!evt) return NULL ;

    memset(evt, 0, sizeof(*evt));

    evt->evt_size = evt_size;
    evt->timeout = to;

    if(evt_size <= 0) evt->evt_size = EVT_DEFAULT_EVENT_SIZE;
    if(to <= 0) evt->timeout = EVT_DEFAULT_POLL_TIMEOUT;

    evt->events = (poll_event_t*)malloc(sizeof(poll_event_t) * evt->evt_size);
    if(!evt->events){
        free(evt);
        return NULL;
    }

    if(be_evt_init(evt) != 0) {
        free(evt->events);
        free(evt);
        return NULL;
    }

    return evt;
}
int evt_free(event_t* evt)
{
    be_evt_destroy(evt);

    free(evt);

    return 0;
}

int evt_poll(event_t* evt, poll_event_t** poll_evt)
{
    int rv = be_evt_poll(evt);
    if(rv > 0) {
        *poll_evt = evt->events;
    }
    return rv;
}

int evt_add(event_t* evt, int fd, int mask)
{
    if(fd > evt->evt_size) {
        int size = evt->evt_size + EVT_DEFAULT_EVENT_GROW_SIZE;
        evt->events = realloc(evt->events, sizeof(poll_event_t) * size);
        if(!evt->events) return -1;
        evt->evt_size = size;
    }
    if(be_evt_add(evt, fd, mask) != 0) return -1;

    return 0;
}
int evt_mod(event_t* evt, int fd, int mask)
{
    return be_evt_mod(evt, fd, mask);
}
int evt_del(event_t* evt, int fd)
{
    return be_evt_del(evt, fd);
}

