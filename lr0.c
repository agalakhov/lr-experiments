#include "lr0.h"

#include "lr0_i.h"
#include "grammar_i.h"

#include "print.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_BUCKETS 256

struct lr0_machine_builder {
    struct lr0_machine          machine;
    unsigned                    nstates;
    struct lr0_state *          last_state;
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
commit_state(struct lr0_machine_builder * builder, struct lr0_state * state)
{
    unsigned hsh = lr0_hash_state(state) % MAX_BUCKETS;
    const struct lr0_state * s = builder->buckets[hsh];
    for ( ; s && ! lr0_compare_state(s, state); s = s->tmp.hash_next)
        ;
    if (s) {
        free(state);
        return s;
    }
    state->id = builder->nstates;
    state->next = NULL;
    state->tmp.hash_next = builder->buckets[hsh];
    builder->buckets[hsh] = state;
    builder->last_state->next = state;
    builder->last_state = state;
    ++(builder->nstates);
    return state;
}

static signed
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

unsigned
lr0_closure(lr0_machine_t mach, struct lr0_point points[], const struct lr0_state * kernel)
{
    unsigned ir = 0;
    unsigned iw = 0;
    bool seen[mach->grammar->n_nonterminals];
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
            const unsigned seen_id = fs->id - mach->grammar->n_terminals;
            if (seen[seen_id]) /* already there */
                continue;
            seen[seen_id] = true;
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
    struct lr0_state *          state;
    struct {
        const struct symbol *   sym;
        unsigned                ssize;
    }               tmp;
};

static signed
cmp_goto_sym(const void *s, const void *t)
{
    const struct symbol * sym = (const struct symbol *) s;
    const struct lr0_state * state = *(const struct lr0_state * const *)t;
    if (sym->id < state->access_sym->id)
        return -1;
    if (sym->id > state->access_sym->id)
        return +1;
    return 0;
}

static signed
cmp_goto(const void *a, const void *b)
{
    const union lr0_goto_scratch *ap = (const union lr0_goto_scratch *) a;
    const union lr0_goto_scratch *bp = (const union lr0_goto_scratch *) b;
    return cmp_goto_sym(ap->state->access_sym, &bp->state);
}

static unsigned
lr0_goto(struct lr0_machine_builder * builder, struct lr0_state * state, const struct lr0_point closure[], unsigned nclosure)
{
    unsigned nsym = 0;
    unsigned const grammar_size = builder->machine.grammar->n_terminals + builder->machine.grammar->n_nonterminals;
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
        if (! symlookup[fs->id]) {
            symlookup[fs->id] = &scratch[nsym++];
            symlookup[fs->id]->tmp.sym = fs;
        }
        ++(symlookup[fs->id]->tmp.ssize);
    }
    for (unsigned i = 0; i < nsym; ++i) {
        struct lr0_state * state = calloc(1, sizeof_struct_lr0_state(scratch[i].tmp.ssize));
        if (! state)
            abort();
        state->access_sym = scratch[i].tmp.sym;
        scratch[i].state = state;
    }
    /* Second pass - actually build the kernels */
    for (unsigned i = 0; i < nclosure; ++i) {
        const struct rule * r = closure[i].rule;
        if (r->length <= closure[i].pos) /* nothing to add */
            continue;
        const struct symbol * fs = r->rs[closure[i].pos].sym;
        struct lr0_state * newstate = (struct lr0_state *) symlookup[fs->id]->state;
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
        struct lr0_state * newstate = scratch[i].state;
        qsort(newstate->points, newstate->npoints, sizeof(struct lr0_point), cmp_point);
        gototab->go[i] = commit_state(builder, newstate);
    }
    state->gototab = gototab;
    return nsym;
}

const struct lr0_state *
lr0_goto_find(const struct lr0_gototab * gototab, const struct symbol * sym)
{
    return *(struct lr0_state * const *)
        bsearch(sym, gototab->go, gototab->ngo, sizeof(gototab->go[0]), cmp_goto_sym);
}

lr0_machine_t
lr0_build(const struct grammar *grammar)
{
    struct lr0_machine_builder * builder = calloc(1, sizeof(struct lr0_machine_builder));
    if (! builder)
        abort();
    builder->machine.grammar = grammar;

    struct lr0_state * state0 = calloc(1, sizeof_struct_lr0_state(1));
    if (! state0)
        abort();
    state0->npoints = 1;
    state0->points[0].rule = grammar->start->nt.rules; /* first rule */
    assert(state0->points[0].rule->next == NULL);
    state0->points[0].pos = 0;

    builder->nstates = 1;
    builder->machine.first_state = state0;
    builder->last_state = state0;

    {
        struct lr0_state * state_end = calloc(1, sizeof_struct_lr0_state(1));
        if (! state_end)
            abort();
        state_end->npoints = 1;
        state_end->points[0].rule = grammar->start->nt.rules; /* first rule */
        state_end->points[0].pos = 2;
        state_end->access_sym = grammar->symlist.first;
        assert(state_end->access_sym->id == 0);
        commit_state(builder, state_end);
    }

    for (struct lr0_state * s = state0; s; s = s->next) {
        struct lr0_point points[s->npoints + grammar->n_rules];
        unsigned n = lr0_closure(&builder->machine, points, s);
        s->nclosure = n;
        lr0_goto(builder, s, points, n);
    }
    lr0_machine_t mach = calloc(1, sizeof(struct lr0_machine));
    if (! mach)
        abort();
    *mach = builder->machine;
    free(builder);
    for (const struct lr0_state * s = mach->first_state; s; s = s->next)
        ((struct lr0_state*)s)->tmp.hash_next = NULL;
    return mach;
}


void
lr0_free(lr0_machine_t mach)
{
    for (const struct lr0_state * s = mach->first_state; s; ) {
        const struct lr0_state * f = s;
        s = s->next;
        free((void *)f->gototab);
        if (f->reducetab)
            free((void *)f->reducetab);
        free((void *)f);
    }
    free(mach);
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

void
lr0_print(lr0_machine_t mach)
{
    for (const struct lr0_state * s = mach->first_state; s; s = s->next) {
        printo(P_LR0_KERNELS, "\nState %u:\n", s->id); // FIXME
        if (print_opt(P_LR0_KERNELS)) {
            struct lr0_point points[s->nclosure];
            unsigned n = lr0_closure(mach, points, s);
            assert(n == s->nclosure);
            for (unsigned i = 0; i < n; ++i) {
                if (! print_opt(P_LR0_CLOSURES) && (i >= s->npoints))
                    break;
                print_point(&points[i], "  ");
            }
        }
        if (print_opt(P_LR0_GOTO) && s->gototab) {
            for (unsigned i = 0; i < s->gototab->ngo; ++i) {
                const struct lr0_state *go = s->gototab->go[i];
                print("    [%s] -> %u\n", go->access_sym->name, go->id);
            }
        }
        if (print_opt(P_LR_REDUCE) && s->reducetab) {
            for (unsigned i = 0; i < s->reducetab->nreduce; ++i) {
                const struct lr_reduce *rdc = &s->reducetab->reduce[i];
                print("    [%s] :< %s ~%u\n", rdc->sym->name, rdc->rule->sym->name, rdc->rule->id);
            }
        }
    }
}
