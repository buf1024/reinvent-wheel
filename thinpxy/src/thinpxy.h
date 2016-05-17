/*
 * thinpxy.h
 *
 *  Created on: 2016/5/17
 *      Author: Luo Guochun
 */

#ifndef THINPXY_SRC_THINPXY_H_
#define THINPXY_SRC_THINPXY_H_

#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


enum {
	WORK_MODE_CONNECT_NEW,
	WORK_MODE_CONNECT_FD,
	WORK_MODE_CONNECT_PACKET,
};

struct thinpxy_s {
	char* conf;
};


#endif /* THINPXY_SRC_THINPXY_H_ */
