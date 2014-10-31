#pragma once

#include "strhash.h"
#include "bitset.h"

#include <stddef.h>
#include <stdbool.h>

union rsptr {
    struct {
        const char *        raw;
    }                   tmp;
    const struct symbol *   sym;
};

struct grammar {
    strhash_t       hash;  /* of symbols */
    struct {
        struct symbol * first;
        struct symbol * last;
    }               symlist;

    unsigned        n_terminals;
    unsigned        n_nonterminals;
    unsigned        n_rules;

    union rsptr     start;
};

enum symbol_type {
    UNKNOWN = 0,
    TERMINAL,
    NONTERMINAL
};

struct terminal {
    int             dummy; /* FIXME */
};

struct nonterminal {
    const struct rule *   rules; /* linked list */
};

struct symbol {
    enum symbol_type    type;       /* terminal or nonterminal */
    const char *        name;       /* left side of production */
    unsigned            id;

    struct symbol *     next;

    bool                nullable;
    bitset_t            first;      /* FIRST(1) set of terminals */
    bitset_t            follow;     /* FOLLOW(1) set of terminals */

    const char *        host_type;
    unsigned            use_count;

    union {
        struct symbol * que_next;
    }               tmp;

    union {
        struct terminal     t;
        struct nonterminal  nt;
    };                              /* contents */
};

struct right_side {
    union rsptr             sym;
    const char *            label;
};

struct rule {
    const struct rule *     next; /* linked list */
    unsigned                id;
    const struct symbol *   sym;  /* owner of the rule */
    const char *            ls_label;
    const char *            host_code;
    unsigned                length;
    struct right_side       rs[];
};
static inline size_t sizeof_struct_rule(unsigned length) {
    return sizeof(struct rule) + length * sizeof(struct right_side);
}
