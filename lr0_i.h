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
    const struct lr_reducetab * reducetab;
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

struct lr_reduce {
    const struct symbol *       sym;
    const struct rule *         rule;
};

struct lr_reducetab {
    unsigned                    nreduce;
    struct lr_reduce            reduce[];
};
static inline size_t sizeof_struct_lr_reducetab(unsigned nreduce) {
    return sizeof(struct lr_reducetab) + nreduce * sizeof(struct lr_reduce);
}
