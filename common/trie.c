/*
 * trie.c
 *
 *  Created on: 2016/7/25
 *      Author: Luo Guochun
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "trie.h"

typedef struct trie_node_s trie_node_t;
typedef struct trie_leaf_s trie_leaf_t;
typedef struct trie_s trie_t;

struct trie_node_s {
    trie_node_t *next[8];
    trie_leaf_t *leaf;
    int ref_count;
};

struct trie_leaf_s {
    char *key;
    void *data;
    trie_leaf_t *next;
};

struct trie_s {
	void* data;
    trie_node_t *root;
};


static trie_leaf_t* find_leaf_with_key(trie_node_t *node, const char *key, size_t len)
{
    trie_leaf_t *leaf = node->leaf;

    if (!leaf)
        return NULL;

    if (!leaf->next) /* No collisions -- no need to strncmp() */
        return leaf;

    for (; leaf; leaf = leaf->next) {
        if (!strncmp(leaf->key, key, len - 1))
            return leaf;
    }

    return NULL;
}

static trie_node_t* lookup_node(trie_node_t *root, const char *key, bool prefix, size_t *prefix_len)
{
    trie_node_t *node, *previous_node = NULL;
    const char *orig_key = key;

    for (node = root; node && *key; node = node->next[(int)(*key++ & 7)]) {
        if (node->leaf)
            previous_node = node;
    }

    *prefix_len = (size_t)(key - orig_key);
    if (node && node->leaf)
        return node;
    if (prefix && previous_node)
        return previous_node;
    return NULL;
}
static void trie_node_destroy(trie_t *trie, trie_node_t *node)
{
    if (!node) return;

    int32_t nodes_destroyed = node->ref_count;

    for (trie_leaf_t *leaf = node->leaf; leaf;) {
        trie_leaf_t *tmp = leaf->next;

        free(leaf->key);
        free(leaf);
        leaf = tmp;
    }

    for (int32_t i = 0; nodes_destroyed > 0 && i < 8; i++) {
        if (node->next[i]) {
            trie_node_destroy(trie, node->next[i]);
            --nodes_destroyed;
        }
    }

    free(node);
}



trie_t* trie_new(void* data)
{
	trie_t* trie = calloc(1, sizeof(*trie));
    if (trie) {
    	trie->data = data;
    }
    return trie;
}
int trie_free(trie_t *trie)
{
    if (!trie || !trie->root) return 0;
    trie_node_destroy(trie, trie->root);

    return 0;
}

void* trie_data(trie_t* trie)
{
	if(trie) {
		return trie->data;
	}
	return NULL;
}

#define GET_NODE() \
    do { \
        if (!(node = *knode)) { \
            *knode = node = calloc(1, sizeof(*node)); \
            if (!node) \
                return -1; \
        } \
        ++node->ref_count; \
    } while(0)

int trie_add(trie_t *trie, const char *key, void *data)
{
    if (!trie || !key) return -1;

    trie_node_t **knode, *node;
    const char *orig_key = key;

    /* Traverse the trie, allocating nodes if necessary */
    for (knode = &trie->root; *key; knode = &node->next[(int)(*key++ & 7)])
        GET_NODE();

    /* Get the leaf node (allocate it if necessary) */
    GET_NODE();

    trie_leaf_t *leaf = find_leaf_with_key(node, orig_key, (size_t)(key - orig_key));
    bool had_key = leaf;
    if (!leaf) {
        leaf = malloc(sizeof(*leaf));
        if (!leaf) return -1;
    }

    leaf->data = data;
    if (!had_key) {
        leaf->key = strdup(orig_key);
        leaf->next = node->leaf;
        node->leaf = leaf;
    }
    return 0;
}

#undef GET_NODE



void* trie_lookup_full(trie_t *trie, const char *key)
{
    if (!trie) return NULL;

    size_t prefix_len;
    trie_node_t *node = lookup_node(trie->root, key, false, &prefix_len);
    if (!node)
        return NULL;
    trie_leaf_t *leaf = find_leaf_with_key(node, key, prefix_len);
    return leaf ? leaf->data : NULL;
}

void* trie_lookup_prefix(trie_t *trie, const char *key)
{
    if (!trie) return NULL;

    size_t prefix_len;
    trie_node_t *node = lookup_node(trie->root, key, true, &prefix_len);
    if (!node)
        return NULL;
    trie_leaf_t *leaf = find_leaf_with_key(node, key, prefix_len);
    return leaf ? leaf->data : NULL;
}

int trie_entry_count(trie_t *trie)
{
    return (trie && trie->root) ? trie->root->ref_count : 0;
}


