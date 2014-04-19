#pragma once

#include "grammar.h"

#include <stddef.h>


struct lr0_point {
    const struct rule *         rule;
    unsigned                    pos;
};

struct lr0_state {
    unsigned                    id;

    struct lr0_state *          next;

    struct {
        const struct lr0_state *    next;
    }                   hash_search;

    /* Data */
    const struct lr0_gototab *  gototab;
    unsigned                    npoints;
    struct lr0_point            points[];
};
static inline size_t sizeof_struct_lr0_state(unsigned npoints) {
    return sizeof(struct lr0_state) + npoints * sizeof(struct lr0_point);
}

struct lr0_go {
    const struct symbol *       sym;
    const struct lr0_state *    state;
};

struct lr0_gototab {
    unsigned                    ngo;
    struct lr0_go               go[];
};
static inline size_t sizeof_struct_lr0_gototab(unsigned ngoto) {
    return sizeof(struct lr0_gototab) + ngoto * sizeof(struct lr0_go);
}
