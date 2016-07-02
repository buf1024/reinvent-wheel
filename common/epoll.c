/*
 * epoll.c
 *
 *  Created on: 2012-12-14
 *      Author: Luo Guochun
 */

#include "evt.h"
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct evt_epoll_s evt_epoll_t;

struct evt_epoll_s
{
    int epfd;
    int maxfd;
    struct epoll_event* evts;
};

int be_evt_init(event_t* evt)
{
    evt_epoll_t* ep = (evt_epoll_t*) malloc(sizeof(*ep));
    if (!ep) return -1;
    memset(ep, 0, sizeof(*ep));

    ep->epfd = epoll_create1(EPOLL_CLOEXEC);
    if (ep->epfd <= 0) {
        free(ep);
        return -1;
    }
    ep->maxfd = evt->evt_size;

    ep->evts = (struct epoll_event*)malloc(sizeof(struct epoll_event) * evt->evt_size);
    if(!ep->evts) {
        close(ep->epfd);
        free(ep);
        return -1;
    }
    evt->data = ep;

    return 0;
}
int be_evt_destroy(event_t* evt)
{
    evt_epoll_t* ep = (evt_epoll_t*) evt->data;

    close(ep->epfd);
    free(ep->evts);
    free(ep);
    evt->data = NULL;

    return 0;
}

int be_evt_poll(event_t* evt)
{
    evt_epoll_t* ep = (evt_epoll_t*) evt->data;

    int epr = 0;
    int i = 0;

    epr = epoll_wait(ep->epfd, ep->evts, evt->evt_size, evt->timeout);
    if (epr > 0) {
        for (; i < epr; i++) {
            if (ep->evts[i].events & EPOLLIN)
                evt->events[i].evt |= EVT_READ;
            if (ep->evts[i].events & EPOLLOUT)
                evt->events[i].evt |= EVT_WRITE;
            if (ep->evts[i].events & EPOLLHUP)
                evt->events[i].evt |= EVT_HUP;
            if (ep->evts[i].events & EPOLLERR)
                evt->events[i].evt |= EVT_ERROR;

            evt->events[i].fd = ep->evts[i].data.fd;

        }
    }

    return epr;
}

int be_evt_add(event_t* evt, int fd, int mask)
{
    evt_epoll_t* ep = (evt_epoll_t*) evt->data;

    if(fd > ep->maxfd) {
        ep->evts = realloc(ep->evts, sizeof(struct epoll_event) * evt->evt_size);
        if(!ep->evts) {
            return -1;
        }

        ep->maxfd = evt->evt_size;
    }


    struct epoll_event epevt;

    if (mask & EVT_READ)
        epevt.events |= EPOLLIN;
    if (mask & EVT_WRITE)
        epevt.events |= EPOLLOUT;
    if (mask & EVT_HUP)
        epevt.events |= EPOLLHUP;
    if (mask & EVT_ERROR)
        epevt.events |= EPOLLERR;

    epevt.data.fd = fd;

    if (epoll_ctl(ep->epfd, EPOLL_CTL_ADD, fd, &epevt) < 0){
        return -1;
    }

    return 0;
}

int be_evt_mod(event_t* evt, int fd, int mask)
{
    evt_epoll_t* ep = (evt_epoll_t*) evt->data;

    struct epoll_event* epevt = NULL;

    if (mask & EVT_READ)
        epevt->events |= EPOLLIN;
    if (mask & EVT_WRITE)
        epevt->events |= EPOLLOUT;
    if (mask & EVT_HUP)
        epevt->events |= EPOLLHUP;
    if (mask & EVT_ERROR)
        epevt->events |= EPOLLERR;

    epevt->data.fd = fd;

    if (epoll_ctl(ep->epfd, EPOLL_CTL_MOD, fd, epevt) < 0){
        return -1;
    }

    return 0;
}

int be_evt_del(event_t* evt, int fd)
{
    evt_epoll_t* ep = (evt_epoll_t*) evt->data;
    struct epoll_event* epevt = NULL;
    if (epoll_ctl(ep->epfd, EPOLL_CTL_DEL, fd, epevt) < 0) return -1;
    return 0;
}
