#pragma once

#include "grammar.h"

#include <stddef.h>

struct lr0_machine {
    const struct grammar *      grammar;
    const struct lr0_state *    first_state;
};

struct lr0_point {
    const struct rule *         rule;
    unsigned                    pos;
};

struct lr0_state {
    unsigned                    id;
    const struct symbol *       access_sym;

    struct lr0_state *          next;

    union {
        const struct lr0_state *        hash_next;
        const struct lalr_lookback *    lalr_lookback;
    }                           tmp;

    /* Data */
    const struct lr0_gototab *  gototab;
    const struct lr_reducetab * reducetab;
    unsigned                    nclosure;
    unsigned                    npoints;
    struct lr0_point            points[];
};
static inline size_t sizeof_struct_lr0_state(unsigned npoints) {
    return sizeof(struct lr0_state) + npoints * sizeof(struct lr0_point);
}

struct lr0_gototab {
    unsigned                    ngo;
    const struct lr0_state *    go[];
};
static inline size_t sizeof_struct_lr0_gototab(unsigned ngoto) {
    return sizeof(struct lr0_gototab) + ngoto * sizeof(const struct lr0_state *);
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

unsigned lr0_closure(lr0_machine_t mach, struct lr0_point points[], const struct lr0_state * kernel);

const struct lr0_state * lr0_goto_find(const struct lr0_gototab * gototab, const struct symbol * sym);
