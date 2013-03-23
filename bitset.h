#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct bitset * bitset_t;

bitset_t set_alloc(size_t n);
void set_free(bitset_t set);
bool set_has(bitset_t set, unsigned x);
bool set_add(bitset_t set, unsigned x);
bool set_union(bitset_t set, bitset_t xs);
