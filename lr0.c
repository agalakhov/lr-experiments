#include "lr0.h"

#include "grammar_i.h"

#include "print.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_BUCKETS 256

struct lr0_point {
    struct rule *               rule;
    unsigned                    pos;
};

struct lr0_state {
    unsigned                    id;
    /* Search and processing handling */
    struct lr0_state *          list_next;
    struct lr0_state *          hash_next;
    /* Data */
    struct lr0_gototab *        gototab;
    unsigned                    npoints;
    struct lr0_point            points[];
};
static inline size_t sizeof_struct_lr0_state(unsigned npoints) {
    return sizeof(struct lr0_state) + npoints * sizeof(struct lr0_point);
}

struct lr0_go {
    struct symbol *             sym;
    union {
        struct lr0_state *      state;
        unsigned                ssize;
    }                       s;
};

struct lr0_gototab {
    unsigned                    ngo;
    struct lr0_go               go[];
};
static inline size_t sizeof_struct_lr0_gototab(unsigned ngoto) {
    return sizeof(struct lr0_gototab) + ngoto * sizeof(struct lr0_go);
}

struct lr0_data {
    unsigned                    nstates;
    struct lr0_state *          process_first;
    struct lr0_state *          process_last;
    struct lr0_state *          buckets[MAX_BUCKETS];
};

static struct lr0_data build_data = { 0 }; /* FIXME */

static bool
lr0_compare_state(const struct lr0_state *a, const struct lr0_state *b)
{
    if (a->npoints != b->npoints)
        return false;
    for (unsigned i = 0; i < a->npoints; ++i) {
        if (a->points[i].rule != b->points[i].rule)
            return false;
        if (a->points[i].pos != b->points[i].pos)
            return false;
    }
    return true;
}

static uint32_t /* FIXME bad */
lr0_hash_state(const struct lr0_state *state)
{
    uint32_t hsh = 0;
    for (unsigned i = 0; i < state->npoints; ++i) {
        hsh += (uint32_t) state->points[i].rule;
        hsh += (uint32_t) state->points[i].pos;
    }
    hsh *= 2654435761; /* Knuth's algorithm */
    return hsh;
}

static struct lr0_state *
commit_state(struct lr0_state * state)
{
    struct lr0_data * data = &build_data;
    unsigned hsh = lr0_hash_state(state) % MAX_BUCKETS;
    struct lr0_state * s = data->buckets[hsh];
    for ( ; s && ! lr0_compare_state(s, state); s = s->hash_next)
        ;
    if (s) {
        free(state);
        return s;
    }
    state->id = data->nstates;
    state->list_next = NULL;
    state->hash_next = data->buckets[hsh];
    data->buckets[hsh] = state;
    data->process_last->list_next = state;
    data->process_last = state;
    ++(data->nstates);
    return state;
}

static int
cmp_goto(const void *a, const void *b)
{
    const struct lr0_go *ap = (const struct lr0_go *) a;
    const struct lr0_go *bp = (const struct lr0_go *) b;
    if (ap->sym->id < bp->sym->id)
        return -1;
    if (ap->sym->id > bp->sym->id)
        return 1;
    return 0;
}

static int
cmp_point(const void *a, const void *b)
{
    const struct lr0_point *ap = (const struct lr0_point *) a;
    const struct lr0_point *bp = (const struct lr0_point *) b;
    if (ap->rule->id < bp->rule->id)
        return -1;
    if (ap->rule->id > bp->rule->id)
        return +1;
    if (ap->pos < bp->pos)
        return -1;
    if (ap->pos > bp->pos)
        return +1;
    return 0;
}

static void
print_point(const struct lr0_point * pt, const char *prefix)
{
    print("%s%s ::=", prefix, pt->rule->sym->name);
    for (unsigned i = 0; i < pt->pos; ++i)
        print(" %s", pt->rule->rs[i].sym->name);
    print(" _");
    for (unsigned i = pt->pos; i < pt->rule->length; ++i)
        print(" %s", pt->rule->rs[i].sym->name);
    print("\n");
}


static unsigned
lr0_closure(struct lr0_point points[], struct lr0_state * kernel, unsigned cookie)
{
    unsigned ir = 0;
    unsigned iw = 0;
    /* Copy kernel */
    for (; iw < kernel->npoints; ++iw)
        points[iw] = kernel->points[iw];
    /* Add nonkernel points */
    for (; ir < iw; ++ir) {
        struct rule * r = points[ir].rule;
        if (r->length <= points[ir].pos) /* nothing to add */
            continue;
        struct symbol * fs = r->rs[points[ir].pos].sym;
        if (fs->type == NONTERMINAL) {
            if (fs->recursion_stop == cookie) /* already there */
                continue;
            fs->recursion_stop = cookie;
            for (struct rule * nr = fs->s.nt.rules; nr; nr = nr->next) {
                points[iw].rule = nr;
                points[iw].pos = 0;
                ++iw;
            }
        }
    }
    return iw;
}

static unsigned
lr0_goto(grammar_t grammar, struct lr0_point closure[], unsigned nclosure)
{
    const unsigned nsymmax = grammar->n_terminals + grammar->n_nonterminals;
    unsigned nsym = 0;
    struct lr0_go scratch[nsymmax];
    struct lr0_go *symlookup[nsymmax];
    memset(scratch, 0, sizeof(scratch));
    memset(symlookup, 0, sizeof(symlookup));
    /* First pass - determine used symbols and kernel sizes */
    for (unsigned i = 0; i < nclosure; ++i) {
        struct rule * r = closure[i].rule;
        if (r->length <= closure[i].pos) /* nothing to add */
            continue;
        struct symbol * fs = r->rs[closure[i].pos].sym;
        if (! symlookup[fs->id - 1]) {
            symlookup[fs->id - 1] = &scratch[nsym++];
            symlookup[fs->id - 1]->sym = fs;
        }
        ++(symlookup[fs->id - 1]->s.ssize);
    }
    for (unsigned i = 0; i < nsym; ++i) {
        scratch[i].s.state = calloc(1, sizeof_struct_lr0_state(scratch[i].s.ssize));
    }
    /* Second pass - actually build the kernels */
    for (unsigned i = 0; i < nclosure; ++i) {
        struct rule * r = closure[i].rule;
        if (r->length <= closure[i].pos) /* nothing to add */
            continue;
        struct symbol * fs = r->rs[closure[i].pos].sym;
        struct lr0_state * k = symlookup[fs->id - 1]->s.state;
        k->points[k->npoints].rule = r;
        k->points[k->npoints].pos = closure[i].pos + 1;
        ++(k->npoints);
    }
    /* Done - get the result */
    qsort(scratch, nsym, sizeof(struct lr0_go), cmp_goto);
    for (unsigned i = 0; i < nsym; ++i) {
        qsort(scratch[i].s.state->points, scratch[i].s.state->npoints, sizeof(struct lr0_point), cmp_point);
        scratch[i].s.state = commit_state(scratch[i].s.state);
        printo(P_LR0_CLOSURES, "    [%s] -> %u\n", scratch[i].sym->name, scratch[i].s.state->id);
    }
    return nsym;
}


void
build_lr0(grammar_t grammar)
{
    struct lr0_data * data = &build_data;
    struct lr0_state * state0 = calloc(1, sizeof_struct_lr0_state(1));
    if (! state0)
        abort();
    state0->npoints = 1;
    state0->points[0].rule = grammar->start.sym->s.nt.rules; /* first rule */
    state0->points[0].pos = 0;
    data->nstates = 1;
    data->process_first = state0;
    data->process_last = state0;

    unsigned cookie = 42;
    for (struct lr0_state * s = data->process_first; s; s = s->list_next) {
        printo(P_LR0_KERNELS, "\nState %u:\n", s->id);
        struct lr0_point points[s->npoints + grammar->n_rules];
        unsigned n = lr0_closure(points, s, ++cookie);
        if (print_opt(P_LR0_KERNELS)) {
            for (unsigned i = 0; i < n; ++i) {
                if (! print_opt(P_LR0_CLOSURES) && (i >= s->npoints))
                    break;
                print_point(&points[i], "  ");
            }
        }
        lr0_goto(grammar, points, n);
    }

}
