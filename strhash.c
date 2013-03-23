#include "strhash.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define MAX_BUCKETS 256
#define HSH_BASIS   2166136261
#define HSH_PRIME   16777619

#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type,member)))

struct bucket {
    struct bucket * next;
    void *          payload;
    char            key[];
};
static inline size_t sizeof_struct_bucket(size_t len) {
    return sizeof(struct bucket) + len + 1;
}

struct strhash {
    struct bucket * buckets[MAX_BUCKETS];
};

static unsigned
hashfunc(const char *str)
{
    uint32_t hsh = HSH_BASIS;
    for (; *str; ++str) {
        hsh ^= *str;
        hsh *= HSH_PRIME;
    }
    hsh ^= (hsh >> 16);
    hsh &= 0xFFFF;
    hsh ^= (hsh >> 8);
    return hsh % MAX_BUCKETS;
}

static bool
equals(const char *s1, const char *s2)
{
    return (s1 == s2) || (strcmp(s1, s2) == 0);
}

strhash_t
strhash_alloc(void)
{
    return (strhash_t)calloc(1, sizeof(struct strhash));
}

void
strhash_free(strhash_t hash, hash_free_func_t freefunc)
{
    for (unsigned i = 0; i < MAX_BUCKETS; ++i) {
        struct bucket *b = hash->buckets[i];
        while (b) {
            struct bucket *n = b->next;
            if (freefunc)
                freefunc(b->payload);
            free(b);
            b = n;
        }
    }
    free(hash);
}

const char *
strhash_key(void **item)
{
    return container_of(item, struct bucket, payload)->key;
}

void **
strhash_find(strhash_t hash, const char *key)
{
    unsigned hsh = hashfunc(key);
    struct bucket * b = hash->buckets[hsh];
    for ( ; b && ! equals(key, b->key); b = b->next)
        ;
    if (! b) {
        b = calloc(1, sizeof_struct_bucket(strlen(key)));
        if (! b)
            return NULL;
        strcpy(b->key, key);
        b->next = hash->buckets[hsh];
        hash->buckets[hsh] = b;
    }
    return &b->payload;
}

void
strhash_iterate(strhash_t hash, hash_iterate_func_t iterfunc, void *param)
{
    struct strhash fh;
    for (unsigned i = 0; i < MAX_BUCKETS; ++i) {
        fh.buckets[i] = hash->buckets[i];
    }
    for (unsigned i = 0; i < MAX_BUCKETS; ++i) {
        for (struct bucket *b = fh.buckets[i]; b; b = b->next) {
            iterfunc(b->key, &b->payload, param);
        }
    }
}
