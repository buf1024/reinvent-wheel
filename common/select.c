/*
 * select.c
 *
 *  Created on: 2012-12-14
 *      Author: Luo Guochun
 */

#include "evt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

typedef struct evt_select_s evt_select_t;

struct evt_select_s
{
    int maxfd;
    fd_set rfd, wfd, efd;
    fd_set erfd, ewfd, eefd;
};

int be_evt_init(event_t* evt)
{
    evt_select_t* ep = (evt_select_t*) malloc(sizeof(*ep));
    if (!ep) return -1;

    memset(ep, 0, sizeof(*ep));

    FD_ZERO(&ep->rfd);
    FD_ZERO(&ep->wfd);

    evt->data = ep;
    return 0;
}
int be_evt_destroy(event_t* evt)
{
    free(evt->data);
    return 0;
}

int be_evt_poll(event_t* evt)
{
    evt_select_t* ep = (evt_select_t*) evt->data;

    struct timeval tv = {0};

    int epr = 0;
    int i = 0, j = 0;

    memcpy(&ep->erfd, &ep->rfd, sizeof(fd_set));
    memcpy(&ep->ewfd, &ep->wfd, sizeof(fd_set));
    memcpy(&ep->eefd, &ep->efd, sizeof(fd_set));

    tv.tv_sec = evt->timeout / 1000;
    tv.tv_usec = evt->timeout % 1000;

    epr = select(ep->maxfd + 1, &ep->erfd, &ep->ewfd, &ep->eefd, &tv);
    if (epr > 0) {
        for (; i <= ep->maxfd; i++) {

            if (FD_ISSET(i, &ep->erfd)){
                evt->events[j].evt |= EVT_READ;
                evt->events[j].fd = i;
            }
            if (FD_ISSET(i, &ep->ewfd)){
                evt->events[j].evt |= EVT_WRITE;
                evt->events[j].fd = i;
            }
            if (FD_ISSET(i, &ep->eefd)) {
                evt->events[j].evt |= EVT_HUP;
                evt->events[j].evt |= EVT_ERROR;
                evt->events[j].fd = i;
            }

        }
    }

    return epr;
}

int be_evt_add(event_t* evt, int fd, int mask)
{
    evt_select_t* ep = (evt_select_t*) evt->data;

    if (mask & EVT_READ)
        FD_SET(fd, &ep->rfd);
    if (mask & EVT_WRITE)
        FD_SET(fd, &ep->wfd);
    if ((mask & EVT_HUP) || (mask & EVT_ERROR))
        FD_SET(fd, &ep->efd);

    if (fd > ep->maxfd)
        ep->maxfd = fd;

    return 0;
}

int be_evt_mod(event_t* evt, int fd, int mask)
{
    evt_select_t* ep = (evt_select_t*) evt->data;

    if (mask & EVT_READ)
        FD_SET(fd, &ep->rfd);
    if (mask & EVT_WRITE)
        FD_SET(fd, &ep->wfd);
    if ((mask & EVT_HUP) || (mask & EVT_ERROR))
        FD_SET(fd, &ep->efd);

    return 0;
}

int be_evt_del(event_t* evt, int fd)
{
    evt_select_t* ep = (evt_select_t*) evt->data;

    FD_CLR(fd, &ep->rfd);
    FD_CLR(fd, &ep->wfd);
    FD_CLR(fd, &ep->efd);

    return 0;
}
