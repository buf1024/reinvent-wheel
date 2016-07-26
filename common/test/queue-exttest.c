/*
 * queue-exttest.c
 *
 *  Created on: 2016/7/26
 *      Author: Luo Guochun
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue-ext.h"

DEF_LIST(mylist_t, int, item_t, entry);

int main(int argc, char **argv)
{
	mylist_t head;
	LIST_INIT(&head);

	item_t i;
	i.data = 1;
	LIST_INSERT_HEAD(&head, &i, entry);

	item_t i2;
	i2.data = 2;
	LIST_INSERT_HEAD(&head, &i2, entry);

	item_t* var;
	LIST_FOREACH(var, &head, entry) {
		printf("%d\n", var->data);
	}

	return 0;
}
