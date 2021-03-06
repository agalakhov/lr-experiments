#pragma once

#include "grammar.h"

#include "strhash.h"
#include "bitset.h"

#include <stddef.h>
#include <stdbool.h>

enum symbol_typex {
    UNKNOWN = SYMBOL_UNKNOWN,
    TERMINAL = SYMBOL_TERMINAL,
    NONTERMINAL = SYMBOL_NONTERMINAL,
    START_NONTERMINAL
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

    const struct symbol * start;

    const char *    machine_name;

    const char *    terminal_host_type;
    const char *    terminal_destructor_code;
    const char *    host_code;
    const char *    extra_argument;
};

struct terminal {
    int             dummy; /* FIXME */
};

struct nonterminal {
    const struct rule *   rules; /* linked list */
};

struct symbol {
    enum symbol_typex   type;       /* terminal or nonterminal */
    const char *        name;       /* left side of production */
    unsigned            id;

    struct symbol *     next;

    bool                nullable;

    const char *        host_type;
    const char *        destructor_code;
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
    struct symbol *         sym;
    const char *            label;
};

struct rule {
    const struct rule *     next; /* linked list */
    unsigned                id;
    const struct symbol *   sym;  /* owner of the rule */
    const char *            ls_label;
    const char *            host_code;

    union {
        struct rule *       que_next;
    }               tmp;

    unsigned                nnl; /* non-nullable length */
    unsigned                length;
    struct right_side       rs[];
};
static inline size_t sizeof_struct_rule(unsigned length) {
    return sizeof(struct rule) + length * sizeof(struct right_side);
}
