#include "lr0.h"

#include "lr0_i.h"
#include "grammar_i.h"

#include "print.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_BUCKETS 256

struct lr0_machine {
    grammar_t                   grammar;
    unsigned                    nstates;
    struct lr0_state *          process_first;
    struct lr0_state *          process_last;
    const struct lr0_state *    buckets[MAX_BUCKETS];
};


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
        hsh += (uint32_t) (intptr_t) state->points[i].rule;
        hsh += (uint32_t) state->points[i].pos;
    }
    hsh *= 2654435761; /* Knuth's algorithm */
    return hsh;
}

static const struct lr0_state *
commit_state(lr0_machine_t mach, struct lr0_state * state)
{
    unsigned hsh = lr0_hash_state(state) % MAX_BUCKETS;
    const struct lr0_state * s = mach->buckets[hsh];
    for ( ; s && ! lr0_compare_state(s, state); s = s->hash_search.next)
        ;
    if (s) {
        free(state);
        return s;
    }
    state->id = mach->nstates;
    state->next = NULL;
    state->hash_search.next = mach->buckets[hsh];
    mach->buckets[hsh] = state;
    mach->process_last->next = state;
    mach->process_last = state;
    ++(mach->nstates);
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
lr0_closure(lr0_machine_t mach, struct lr0_point points[], const struct lr0_state * kernel)
{
    unsigned ir = 0;
    unsigned iw = 0;
    bool seen[mach->grammar->n_terminals + mach->grammar->n_nonterminals];
    memset(seen, 0, sizeof(seen));
    /* Copy kernel */
    for (; iw < kernel->npoints; ++iw)
        points[iw] = kernel->points[iw];
    /* Add nonkernel points */
    for (; ir < iw; ++ir) {
        const struct rule * r = points[ir].rule;
        if (r->length <= points[ir].pos) /* nothing to add */
            continue;
        const struct symbol * fs = r->rs[points[ir].pos].sym;
        if (fs->type == NONTERMINAL) {
            if (seen[fs->id - 1]) /* already there */
                continue;
            seen[fs->id - 1] = true;
            for (const struct rule * nr = fs->nt.rules; nr; nr = nr->next) {
                points[iw].rule = nr;
                points[iw].pos = 0;
                ++iw;
            }
        }
    }
    return iw;
}


union lr0_goto_scratch {
    struct lr0_go   go;
    struct {
        const struct symbol *   sym;
        unsigned                ssize;
    }               tmp;
};

static unsigned
lr0_goto(lr0_machine_t mach, struct lr0_state * state, const struct lr0_point closure[], unsigned nclosure)
{
    unsigned nsym = 0;
    unsigned const grammar_size = mach->grammar->n_terminals + mach->grammar->n_nonterminals;
    union lr0_goto_scratch scratch[grammar_size];
    union lr0_goto_scratch *symlookup[grammar_size];
    memset(scratch, 0, sizeof(scratch));
    memset(symlookup, 0, sizeof(symlookup));
    /* First pass - determine used symbols and kernel sizes */
    for (unsigned i = 0; i < nclosure; ++i) {
        const struct rule * r = closure[i].rule;
        if (r->length <= closure[i].pos) /* nothing to add */
            continue;
        const struct symbol * fs = r->rs[closure[i].pos].sym;
        if (! symlookup[fs->id - 1]) {
            symlookup[fs->id - 1] = &scratch[nsym++];
            symlookup[fs->id - 1]->tmp.sym = fs;
        }
        ++(symlookup[fs->id - 1]->tmp.ssize);
    }
    for (unsigned i = 0; i < nsym; ++i) {
        const struct symbol * sym = scratch[i].tmp.sym;
        scratch[i].go.state = calloc(1, sizeof_struct_lr0_state(scratch[i].tmp.ssize));
        if (! scratch[i].go.state)
            abort();
        scratch[i].go.sym = sym;
    }
    /* Second pass - actually build the kernels */
    for (unsigned i = 0; i < nclosure; ++i) {
        const struct rule * r = closure[i].rule;
        if (r->length <= closure[i].pos) /* nothing to add */
            continue;
        const struct symbol * fs = r->rs[closure[i].pos].sym;
        struct lr0_state * newstate = (struct lr0_state *) symlookup[fs->id - 1]->go.state;
        newstate->points[newstate->npoints].rule = r;
        newstate->points[newstate->npoints].pos = closure[i].pos + 1;
        ++(newstate->npoints);
    }
    /* Done - get the result */
    qsort(scratch, nsym, sizeof(union lr0_goto_scratch), cmp_goto);
    struct lr0_gototab * gototab  = calloc(1, sizeof_struct_lr0_gototab(nsym));
    if (! gototab)
        abort();
    gototab->ngo = nsym;
    for (unsigned i = 0; i < nsym; ++i) {
        struct lr0_state * newstate = (struct lr0_state *) scratch[i].go.state;
        qsort(newstate->points, newstate->npoints, sizeof(struct lr0_point), cmp_point);
        scratch[i].go.state = commit_state(mach, newstate);
        memcpy(&(gototab->go[i]), &(scratch[i].go), sizeof(struct lr0_go));
        printo(P_LR0_GOTO, "    [%s] -> %u\n", scratch[i].go.sym->name, scratch[i].go.state->id);
    }
    state->gototab = gototab;
    return nsym;
}

lr0_machine_t
lr0_build(grammar_t grammar)
{
    lr0_machine_t mach = calloc(1, sizeof(struct lr0_machine));
    if (! mach)
        abort();
    mach->grammar = grammar;
    struct lr0_state * state0 = calloc(1, sizeof_struct_lr0_state(1));
    if (! state0)
        abort();
    state0->npoints = 1;
    state0->points[0].rule = grammar->start.sym->nt.rules; /* first rule */
    state0->points[0].pos = 0;
    mach->nstates = 1;
    mach->process_first = state0;
    mach->process_last = state0;

    for (struct lr0_state * s = mach->process_first; s; s = s->next) {
        printo(P_LR0_KERNELS, "\nState %u:\n", s->id);
        struct lr0_point points[s->npoints + grammar->n_rules];
        unsigned n = lr0_closure(mach, points, s);
        if (print_opt(P_LR0_KERNELS)) {
            for (unsigned i = 0; i < n; ++i) {
                if (! print_opt(P_LR0_CLOSURES) && (i >= s->npoints))
                    break;
                print_point(&points[i], "  ");
            }
        }
        lr0_goto(mach, s, points, n);
    }

    return mach;
}


void
lr0_free(lr0_machine_t mach)
{
    for (struct lr0_state * s = mach->process_first; s; ) {
        struct lr0_state * f = s;
        s = s->next;
        free((void *)f->gototab);
        if (f->reducetab)
            free((void *)f->reducetab);
        free(f);
    }
    free(mach);
}
