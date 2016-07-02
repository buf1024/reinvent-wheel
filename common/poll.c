/*
 * poll.c
 *
 *  Created on: 2012-12-14
 *      Author: Luo Guochun
 */

#include "evt.h"
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct evt_poll_s evt_poll_t;

struct evt_poll_s
{

    int maxfd;
    int poll_size;
    struct pollfd* evts;
};

int be_evt_init(event_t* evt)
{
    evt_poll_t* ep = (evt_poll_t*) malloc(sizeof(*ep));
    if (!ep) return -1;

    memset(ep, 0, sizeof(*ep));

    ep->evts = (struct pollfd*)malloc(sizeof(struct pollfd) * evt->evt_size);
    if(!ep->evts) {
        free(ep);
        return -1;
    }

    ep->maxfd = evt->evt_size;
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
    evt_poll_t* ep = (evt_poll_t*) evt->data;

    int epr = 0;
    int i = 0, j = 0;;

    epr = poll(ep->evts, ep->poll_size, evt->timeout);
    if (epr > 0) {
        for (; i < ep->poll_size; i++) {
            if (ep->evts[i].revents & POLLIN)
                evt->events[j].evt |= EVT_READ;
            if (ep->evts[i].revents & POLLOUT)
                evt->events[j].evt |= EVT_WRITE;
            if (ep->evts[i].revents & POLLHUP)
                evt->events[j].evt |= EVT_HUP;
            if (ep->evts[i].revents & POLLERR)
                evt->events[j].evt |= EVT_ERROR;
            if(ep->evts[i].revents) {
                evt->events[j].fd = ep->evts[i].fd;
                j++;
            }

        }
    }

    return epr;
}

int be_evt_add(event_t* evt, int fd, int mask)
{
    evt_poll_t* ep = (evt_poll_t*) evt->data;

    if(fd > ep->maxfd) {

        ep->evts = realloc(ep->evts, sizeof(struct pollfd) * evt->evt_size);
        if(!ep->evts) {
            return -1;
        }
        ep->maxfd = evt->evt_size;
    }

    int i = 0;
    for (; i < ep->poll_size; i++) {
        if (fd == ep->evts[i].fd) {
            break;
        }
    }
    if (i >= ep->poll_size) {
        ep->evts[i].fd = fd;
        ep->evts[i].revents = 0;
        ep->poll_size++;
    }

    if (mask & EVT_READ)
        ep->evts[i].events |= POLLIN;
    if (mask & EVT_WRITE)
        ep->evts[i].events |= POLLOUT;
    if (mask & EVT_HUP)
        ep->evts[i].events |= POLLHUP;
    if (mask & EVT_ERROR)
        ep->evts[i].events |= POLLERR;

    return 0;
}

int be_evt_mod(event_t* evt, int fd, int mask)
{
    evt_poll_t* ep = (evt_poll_t*) evt->data;


    int i = 0;
    for (; i < ep->poll_size; i++) {
        if (fd == ep->evts[i].fd) {
            break;
        }
    }
    if (i >= ep->poll_size) {
        return -1;
    }

    if (mask & EVT_READ)
        ep->evts[i].events |= POLLIN;
    if (mask & EVT_WRITE)
        ep->evts[i].events |= POLLOUT;
    if (mask & EVT_HUP)
        ep->evts[i].events |= POLLHUP;
    if (mask & EVT_ERROR)
        ep->evts[i].events |= POLLERR;

    return 0;
}

int be_evt_del(event_t* evt, int fd)
{
    evt_poll_t* ep = (evt_poll_t*) evt->data;
    if(ep->poll_size <= 0) return 0;

    int i = 0;
    for (; i < ep->poll_size; i++) {
        if (fd == ep->evts[i].fd) {
            break;
        }
    }
    if (i < ep->poll_size) {
        if(i != 0) {
            memmove(&(ep->evts[i]), &(ep->evts[i + 1]), (ep->poll_size - i - 1)*sizeof(struct pollfd));
        }
        ep->poll_size--;
    }

    return 0;
}
