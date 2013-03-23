#pragma once

typedef struct strhash *strhash_t;

typedef void (*hash_free_func_t)(void *);
typedef void (*hash_iterate_func_t)(const char *, void **, void *);

strhash_t strhash_alloc(void);

void strhash_free(strhash_t hash, hash_free_func_t freefunc);

void **strhash_find(strhash_t hash, const char *key);

const char *strhash_key(void **item);

void strhash_iterate(strhash_t hash, hash_iterate_func_t iterfunc, void *param);
