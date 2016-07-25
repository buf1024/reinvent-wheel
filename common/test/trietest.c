/*
 * trietest.c
 *
 *  Created on: 2016/7/25
 *      Author: Luo Guochun
 */

#include <stdio.h>
#include <stdlib.h>
#include "trie.h"

int main(int argc, char **argv)
{
	trie_t* t = trie_new(0);

	const char* p = "/";
	trie_add(t, p, (void*)1);

	p = "/home";
	trie_add(t, p, (void*)2);

	p = "/home/hello";
	trie_add(t, p, (void*)3);

	p = "/home/hello/wa";
	trie_add(t, p, (void*)4);

	p = "/hom";
	trie_add(t, p, (void*)5);

	void* d = NULL;

	d = trie_lookup_prefix(t, "/home/he");
	printf("%d\n", (int)d);

	d = trie_lookup_full(t, "/home/he");
	printf("%d\n", (int)d);

	d = trie_lookup_prefix(t, "/home/hello");
	printf("%d\n", (int)d);

	trie_free(t);

	return 0;
}

