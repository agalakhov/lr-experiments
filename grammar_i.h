#pragma once

#include "strhash.h"
#include "bitset.h"

#include <stddef.h>
#include <stdbool.h>

union rsptr {
    const char *    raw;
    struct symbol * sym;
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

    unsigned        recursion_cookie;
};

enum symbol_type {
    TERMINAL,
    NONTERMINAL
};

struct terminal {
    int             dummy; /* FIXME */
};

struct nonterminal {
    struct rule *   rules; /* linked list */
};

struct symbol {
    enum symbol_type    type;       /* terminal or nonterminal */
    const char *        name;       /* left side of production */
    unsigned            id;

    struct symbol *     next;

    bool                nullable;
    bitset_t            first;      /* FIRST(1) set of terminals */
    bitset_t            follow;     /* FOLLOW(1) set of terminals */

    unsigned            use_count;
    unsigned            recursion_stop;

    union {
        struct symbol * que_next;
    }               tmp;

    union {
        struct terminal     t;
        struct nonterminal  nt;
    }                   s;          /* contents */
};

struct rule {
    struct rule *           next; /* linked list */
    unsigned                id;
    struct symbol *         sym;  /* owner of the rule */
    unsigned                length;
    union rsptr             rs[];
};
static inline size_t sizeof_struct_rule(unsigned length) {
    return sizeof(struct rule) + length * sizeof(union rsptr);
}
