/**
 * Copyright (c) 2010 Tom Burdick <thomas.burdick@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef OCTO_HASH_H
#define OCTO_HASH_H

#include <string.h>
#include <assert.h>

#include "list.h"
#include "hash_function.h"

#define min(x, y) ((x)<(y)?(x):(y))

/**
 * intrusive fixed size chained hash table store.
 *
 * choice of hash function is up to the creator of the store.
 * this takes care of retrieving, storing, and removing entries.
 *
 * it uses power of 2 sizes only to avoid using a true modulus
 * and asserts that the hash table really is power of 2.
 * modulus operations are very very slow, making hash bin lookups slow!
 *
 * This hash table is never resized to simplify things. Chaining
 * is used instead of open addressing. Perhaps an open addressing
 * version could be made pretty easily.
 *
 */

typedef struct octo_hash
{
    octo_hash_function hash_function;
    uint32_t hash_seed;
    uint32_t n_hash_bins;
    octo_list *hash_bins;
} octo_hash;

typedef struct octo_hash_entry
{
    octo_list hash_list;
    size_t keylen;
    void *key;
} octo_hash_entry;

static bool power_of_two(uint32_t x)
{
    return (x!=0) && ((x&(x-1)) == 0);
}

/**
 * initialize the hash table, does not allocate any memory
 */
static inline void octo_hash_init(octo_hash *hashtable, octo_hash_function hash_function, uint32_t seed, size_t pow2size)
{
    assert(pow2size < 32);
    hashtable->hash_function = hash_function;
    hashtable->hash_seed = seed;
    hashtable->n_hash_bins = (1<<pow2size);
    hashtable->hash_bins = malloc(sizeof(octo_list)*hashtable->n_hash_bins);

    for(size_t i = 0; i < hashtable->n_hash_bins; ++i)
    {
        octo_list_init(&hashtable->hash_bins[i]);
    }

    assert(hashtable->n_hash_bins != 0);
    assert(power_of_two(hashtable->n_hash_bins));

}

/**
 * clear out the hash table of all entries, does not free any memory
 */
static inline void octo_hash_destroy(octo_hash *hashtable)
{
    hashtable->hash_function = NULL;
    hashtable->hash_seed = 0;
    hashtable->n_hash_bins = 0;
    free(hashtable->hash_bins);
    hashtable->hash_bins = NULL;
}

/**
 * obtain the hash bin number given the number of bins and a hash
 */
static inline uint32_t octo_hash_nbin(uint32_t bins, uint32_t keyhash)
{
    return (keyhash & (bins-1));
}

/**
 * obtain the hash bin given the hash and key
 */
static inline octo_list * octo_hash_bin(octo_hash *hashtable, uint32_t keyhash)
{
    uint32_t nbin = octo_hash_nbin(hashtable->n_hash_bins, keyhash);
    assert(nbin < hashtable->n_hash_bins);
    return &(hashtable->hash_bins[nbin]);
}

static inline size_t octo_hash_size(const octo_hash *hashtable)
{
    size_t size = 0;
    for(uint32_t i = 0; i < hashtable->n_hash_bins; ++i)
    {
        size += octo_list_size(&hashtable->hash_bins[i]);
    }
    return size;
}

/**
 * check if a bin has an entry
 */
static inline bool octo_hash_bin_has(octo_hash *hashtable, uint32_t keyhash, void *key, size_t keylen)
{
    octo_list *list = octo_hash_bin(hashtable, keyhash);
    octo_hash_entry *pos;
    octo_hash_entry *next;
    octo_list_foreach(pos, next, list, hash_list)
    {
        if(strncmp((char*)key, (char*)pos->key, min(pos->keylen, keylen)) == 0)
        {
            return true;
        }
    }

    return false;
}

/**
 * has an entry
 */
static inline bool octo_hash_has(octo_hash *hashtable, void *key, size_t keylen)
{
    uint32_t keyhash = hashtable->hash_function(key, keylen, hashtable->hash_seed);
    return octo_hash_bin_has(hashtable, keyhash, key, keylen);
}

/**
 * set the hash entry key
 */
static inline void octo_hash_set_key(octo_hash_entry *entry, void *key, size_t keylen)
{
    entry->key = key;
    entry->keylen = keylen;
}

/**
 * put an entry in to the hash table, duplicate keys are not put in, this is
 * not a multi hash!
 */
static inline bool octo_hash_put(octo_hash *hashtable, octo_hash_entry *entry)
{
    uint32_t keyhash = hashtable->hash_function(entry->key, entry->keylen, hashtable->hash_seed);
    octo_list *list = octo_hash_bin(hashtable, keyhash);
    if(octo_list_empty(list))
    {
        octo_list_add(list, &entry->hash_list);
    }
    else if(!octo_hash_bin_has(hashtable, keyhash, entry->key, entry->keylen))
    {
        octo_list_add(list, &entry->hash_list);
    }
    else
    {
        return false;
    }
    return true;
}

/**
 * put and set the hash entry key in to the hash table at the same time
 */
static inline bool octo_hash_add(octo_hash *hashtable, octo_hash_entry *entry,
        void *key, size_t keylen)
{
    octo_hash_set_key(entry, key, keylen);
    return octo_hash_put(hashtable, entry);
}

/**
 * get an entry from the hash table
 */
static inline octo_hash_entry * octo_hash_get(octo_hash *hashtable, void *key, size_t keylen)
{
    uint32_t keyhash = hashtable->hash_function(key, keylen, hashtable->hash_seed);
    octo_list *list = octo_hash_bin(hashtable, keyhash);
    octo_hash_entry *pos;
    octo_hash_entry *next;
    octo_list_foreach(pos, next, list, hash_list)
    {
        if(strncmp((char*)key, (char*)pos->key, min(pos->keylen, keylen)) == 0)
        {
            return pos;
        }
    }
    return NULL;
}

/**
 * get and remove an entry from the hash table
 */
static inline octo_hash_entry * octo_hash_pop(octo_hash *hashtable, void *key, size_t keylen)
{
    uint32_t keyhash = hashtable->hash_function(key, keylen, hashtable->hash_seed);
    octo_list *list = octo_hash_bin(hashtable, keyhash);
    octo_hash_entry *pos;
    octo_hash_entry *next;
    octo_list_foreach(pos, next, list, hash_list)
    {
        if(strncmp((char*)key, (char*)pos->key, min(pos->keylen, keylen)) == 0)
        {
            octo_list_remove(&pos->hash_list);
            return pos;
        }
    }

    return NULL;
}

/**
 * obtain a pointer to a struct containing a hash
 */
#define octo_hash_entry(ptr, type, member) \
    ptr_offset(ptr, type, member)

#endif
