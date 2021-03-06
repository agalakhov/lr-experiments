#include "bitset.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>

#define UNITSIZE (8 * sizeof(unsigned long))

struct bitset {
    size_t          max;
    unsigned long   data[];
};
static inline size_t sizeof_struct_bitset(size_t max) {
    return sizeof(struct bitset) + max * sizeof(unsigned long);
}

bitset_t
set_alloc(size_t n)
{
    if (n == 0) {
        errno = EINVAL;
        return NULL;
    }
    size_t max = ((n - 1) / UNITSIZE) + 1;
    struct bitset * set = calloc(1, sizeof_struct_bitset(max));
    if (! set)
        return NULL;
    set->max = max;
    return set;
}

void
set_free(bitset_t set)
{
    free(set);
}

bool
set_has(bitset_t set, unsigned x)
{
    if (x >= set->max * UNITSIZE)
        return false;
    size_t idx = x / UNITSIZE;
    size_t bit = x % UNITSIZE;
    return !! (set->data[idx] & (1 << bit));
}

bool
set_add(bitset_t set, unsigned x)
{
    if (x >= set->max * UNITSIZE)
        return false;
    size_t idx = x / UNITSIZE;
    size_t bit = x % UNITSIZE;
    unsigned long old = set->data[idx];
    set->data[idx] |= (1 << bit);
    return (set->data[idx] != old);
}

bool
set_union(bitset_t set, bitset_t xs)
{
    bool ret = false;
    for (size_t i = 0; i < set->max && i < xs->max; ++i) {
        unsigned long old = set->data[i];
        set->data[i] |= xs->data[i];
        ret = ret || (set->data[i] != old);
    }
    return ret;
}

size_t
set_size(bitset_t set)
{
    static const unsigned long mask[] = {
        (unsigned long) 0x5555555555555555,
        (unsigned long) 0x3333333333333333,
        (unsigned long) 0x0F0F0F0F0F0F0F0F,
        (unsigned long) 0x00FF00FF00FF00FF,
        (unsigned long) 0x0000FFFF0000FFFF,
        (unsigned long) 0x00000000FFFFFFFF,
    };
    size_t ret = 0;
    for (size_t i = 0; i < set->max; ++i) {
        unsigned long cnt = set->data[i];
        for (unsigned j = 0; (1u << j) < UNITSIZE; ++j)
            cnt = (cnt & mask[j]) + ((cnt >> (1u << j)) & mask[j]);
        ret += cnt;
    }
    return ret;
}
