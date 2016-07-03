/*
 * evt.h
 *
 *  Created on: 2012-12-14
 *      Author: Luo Guochun
 */


#ifndef __EVT_H__
#define __EVT_H__


#ifdef __cplusplus
extern "C" {
#endif

typedef struct event_s event_t;
typedef struct poll_event_s poll_event_t;

struct event_s
{
    int evt_size;
    int timeout;

    poll_event_t* events;

    void* data;
};

struct poll_event_s
{
    int evt;
    int fd;
};

enum
{
    EVT_NONE  = 0x00,
    EVT_READ  = 0x01,
    EVT_WRITE = 0x02,
    EVT_RDWR  = 0x03, // 0x02 & 0x01 = 0x03
    EVT_HUP   = 0x04,
    EVT_ERROR = 0x08
};

event_t* evt_new(int evt_size, int to);
int evt_free(event_t* evt);

int evt_add(event_t* evt, int fd, int mask);
int evt_mod(event_t* evt, int fd, int mask);
int evt_del(event_t* evt, int fd);

int evt_poll(event_t* evt, poll_event_t** poll_evt);


#ifdef __cplusplus
}
#endif


#endif /* __EVT_H__ */
