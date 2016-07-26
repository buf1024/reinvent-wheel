/*
 * queue-ext.h
 *
 *  Created on: 2016/7/26
 *      Author: Luo Guochun
 */

#ifndef __QUEUE_EXT_H__
#define __QUEUE_EXT_H__

#include "queue.h"

#define DEF_LIST(listtype, datatype, itemtype, entry) \
typedef struct __##listtype##__##itemtype##entry##__ { \
	datatype data; \
	LIST_ENTRY(__##listtype##__##itemtype##entry##__) entry; \
} itemtype; \
typedef LIST_HEAD(__##entry##itemtype##__##listtype__, \
		__##listtype##__##itemtype##entry##__) listtype


#endif /* __QUEUE_EXT_H__ */
