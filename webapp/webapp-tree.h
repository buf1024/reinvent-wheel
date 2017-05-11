/*
 * trie.h
 *
 *  Created on: 2016/7/25
 *      Author: Luo Guochun
 */

#ifndef __TRIE_H__
#define __TRIE_H__

#ifdef __cplusplus
extern "C" {
#endif



typedef struct trie_s trie_t;
typedef struct webapp_tree_s webapp_tree_t;

struct webapp_tree_s {
    trie_t* trie;
};

trie_t* trie_new(void* data);
int trie_free(trie_t* trie);
void* trie_data(trie_t* trie);

int trie_add(trie_t* trie, const char* key, void* data);
void* trie_lookup_full(trie_t* trie, const char* key);
void* trie_lookup_prefix(trie_t* trie, const char* key);
int trie_entry_count(trie_t *trie);

#ifdef __cplusplus
}
#endif



#endif /* __TRIE_H__ */
