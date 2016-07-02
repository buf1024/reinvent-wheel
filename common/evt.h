/*
 * evt.h
 *
 *  Created on: 2012-12-14
 *      Author: Luo Guochun
 */


#ifndef COMMON_EVT_H_
#define COMMON_EVT_H_


#ifdef __cplusplus
extern "C" {
#endif

#define EVT_DEFAULT_POLL_TIMEOUT  1000

typedef struct event_s event_t;

typedef int (*evt_callback_t)(event_t* evt, int fd, int mask, void* args);

enum evt_mask
{
    EVT_READ  = 0x01,
    EVT_WRITE = 0x02,
    EVT_RDWR  = 0x03, // 0x02 & 0x01 = 0x03
    EVT_HUP   = 0x04,
    EVT_ERROR = 0x08
};

enum evt_type
{
    EVT_SOCK      = 0x01,
    EVT_SIGNAL    = 0x02,
    EVT_TIMER     = 0x04,
    EVT_TIMEOUT   = 0x08
};

event_t* evt_new(int evt_size, int to);
int evt_free(event_t* evt);

int evt_add(event_t* evt, int type, int fd, int mask, evt_callback_t cb, void* args);
int evt_mod(event_t* evt);
int evt_del(event_t* evt, int fd, int type);

int evt_do_poll(event_t* evt);


int evt_loop(event_t* evt);
int evt_loop_exit(event_t* evt);

#ifdef __cplusplus
}
#endif


#endif /* COMMON_EVT_H_ */
