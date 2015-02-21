#include "lalr.h"

#include "grammar_i.h"
#include "lr0_i.h"

#include "bitset.h"
#include "common.h"

#include "print.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct lalr_lookback {
    const struct lalr_lookback *    next;
    const struct rule *             rule;
    const struct trans *            trans;
};

struct trans {
    const struct lr0_state *    state1;
    const struct lr0_state *    state2;

    struct {
                                unsigned rank;
    }                       digraph;

    const struct trans_list *   reads;
    const struct trans_list *   includes;

    bitset_t                    set;
};

struct trans_list {
    const struct trans_list *   next;
    struct trans *              trans;
};

static signed
cmp_trans(const void *a, const void *b)
{
    const struct trans * ap = (const struct trans *) a;
    const struct trans * bp = (const struct trans *) b;
    if (ap->state1->id < bp->state1->id)
        return -1;
    if (ap->state1->id > bp->state1->id)
        return +1;
    if (ap->state2->id < bp->state2->id)
        return -1;
    if (ap->state2->id > bp->state2->id)
        return +1;
    return 0;
}

static bitset_t
create_dr(lr0_machine_t lr0_machine, const struct lr0_gototab * gototab)
{
    bitset_t dr = set_alloc(lr0_machine->grammar->n_terminals);
    if (! dr)
        abort();
    for (unsigned i = 0; i < gototab->ngo; ++i) {
        if (gototab->go[i]->access_sym->type == TERMINAL) {
            print("  directly-reads %s\n", gototab->go[i]->access_sym->name);
            set_add(dr, gototab->go[i]->access_sym->id);
        }
    }
    return dr;
}

static struct trans *
lookup_transition(struct trans trans[], unsigned ntrans, const struct lr0_state * state1, const struct lr0_state * state2)
{
    struct trans needle = { .state1 = state1, .state2 = state2 };
    return bsearch(&needle, trans, ntrans, sizeof(trans[0]), cmp_trans);
}

static unsigned
find_transitions(lr0_machine_t lr0_machine, struct trans trans[], unsigned ntrans)
{
    unsigned itrans = 0;
    for (const struct lr0_state * state = lr0_machine->first_state; state; state = state->next) {
        const struct lr0_gototab * gototab = state->gototab;
        for (unsigned igo = 0; igo < gototab->ngo; ++igo) {
            if (gototab->go[igo]->access_sym->type == NONTERMINAL) {
                if (trans != NULL) { /* not dry run? */
                    print("LALR: state %u symbol %s\n", state->id, gototab->go[igo]->access_sym->name);
                    assert(itrans < ntrans);
                    trans[itrans].state1 = state;
                    trans[itrans].state2 = gototab->go[igo];
                    trans[itrans].set = create_dr(lr0_machine, gototab->go[igo]->gototab);
                }
                ++itrans;
            }
        }
    }
    if (trans)
        qsort(trans, itrans, sizeof(trans[0]), cmp_trans);
    return itrans;
}

static void
add_reads(struct trans * trans1, struct trans * trans2)
{
    struct trans_list* rd = calloc(1, sizeof(struct trans_list));
    if (! rd)
        abort();
    rd->trans = trans2;
    rd->next = trans1->reads;
    trans1->reads = rd;
    print("\033[s\n");
    print("*   (%u-%s->%u) \033[33;1mreads\033[0m (%u-%s->%u)\n",
          trans1->state1->id, trans1->state2->access_sym->name, trans1->state2->id,
          trans2->state1->id, trans2->state2->access_sym->name, trans2->state2->id);
    print("\033[u");
}

static void
add_lookback(struct lr0_state * state, const struct rule * rule, const struct trans * trans)
{
    struct lalr_lookback * lb = calloc(1, sizeof(struct lalr_lookback));
    if (! lb)
        abort();
    lb->next = state->tmp.lalr_lookback;
    lb->rule = rule;
    lb->trans = trans;
    state->tmp.lalr_lookback = lb;
    print("*   (%u,rule %s) \033[34;1mlookback\033[0m (%u-%s->%u)\n", state->id, rule->sym->name,
          trans->state1->id, trans->state2->access_sym->name, trans->state2->id);
}

static void
add_includes(struct trans * trans1, struct trans * trans2)
{
    struct trans_list* inc = calloc(1, sizeof(struct trans_list));
    if (! inc)
        abort();
    inc->trans = trans2;
    inc->next = trans1->includes;
    trans1->includes = inc;
    print("\033[s\n");
    print("*   (%u-%s->%u) \033[35;1mincludes\033[0m (%u-%s->%u)\n",
          trans1->state1->id, trans1->state2->access_sym->name, trans1->state2->id,
          trans2->state1->id, trans2->state2->access_sym->name, trans2->state2->id);
    print("\033[u");
}

static void
find_reads(lr0_machine_t lr0_machine, struct trans trans[], unsigned ntrans)
{
    (void) lr0_machine;
    for (unsigned itr = 0; itr < ntrans; ++itr) {
        struct trans * const tr = &trans[itr];
        const struct lr0_gototab * gototab = tr->state2->gototab;
        for (unsigned igo = 0; igo < gototab->ngo; ++igo) {
            if (gototab->go[igo]->access_sym->nullable) {
                struct trans * rtr = lookup_transition(trans, ntrans, tr->state2, gototab->go[igo]);
                add_reads(tr, rtr);
            }
        }
    }
}

static void
find_lookback_includes(lr0_machine_t lr0_machine, struct trans trans[], unsigned ntrans)
{
    (void) lr0_machine;
    for (unsigned itr = 0; itr < ntrans; ++itr) {
        struct trans * const tr = &trans[itr];
        const struct symbol * const sym = tr->state2->access_sym;
        print("LALR: transition from %u via %s\n", tr->state1->id, tr->state2->access_sym->name);
        assert(sym->type == NONTERMINAL);
        for (const struct rule * rule = sym->nt.rules; rule; rule = rule->next) {
            const struct lr0_state * oldst = NULL;
            const struct lr0_state * st = tr->state1;
            print("  rule: %s ::= (%u)", sym->name, st->id);
            for (unsigned i = 0; i < rule->length; ++i) {
                print(" %s", rule->rs[i].sym.sym->name);
                oldst = st;
                st = lr0_goto_find(st->gototab, rule->rs[i].sym.sym);
                assert(st != NULL);
                print(" (%u)", st->id);
                if ((i + 1 >= rule->nnl) && (rule->rs[i].sym.sym->type == NONTERMINAL)) {
                    struct trans * ltr = lookup_transition(trans, ntrans, oldst, st);
                    add_includes(ltr, tr);
                }
            }
            print("\n");
            add_lookback((struct lr0_state *)st, rule, tr);
        }
    }
}

static void
traverse(struct trans * stack[], unsigned * sp, struct trans * trans, ptrdiff_t list_offset)
{
    print("  traverse (%u-%s->%u) %u\n", trans->state1->id, trans->state2->access_sym->name, trans->state2->id, *sp);
    stack[(*sp)++] = trans;
    const unsigned d = *sp;
    trans->digraph.rank = d;
    print("  / %u\n", d);
    for (const struct trans_list * it = field_of(trans, struct trans_list *, list_offset); it; it = it->next) {
        if (it->trans->digraph.rank == 0)
            traverse(stack, sp, it->trans, list_offset);
        if (it->trans->digraph.rank < trans->digraph.rank)
            trans->digraph.rank = it->trans->digraph.rank;
        set_union(trans->set, it->trans->set);
    }
    print("  \\ %u\n", trans->digraph.rank);
    if (d == trans->digraph.rank) {
        while (*sp >= d) {
            stack[--(*sp)]->digraph.rank = UINT_MAX;
        }
    }
}

static void
digraph(struct trans trans[], unsigned ntrans, ptrdiff_t list_offset)
{
    print("\033[31;1mstart digraph\033[0m\n");
    for (unsigned i = 0; i < ntrans; ++i)
        trans[i].digraph.rank = 0;
    struct trans * stack[ntrans];
    unsigned sp = 0;
    for (unsigned i = 0; i < ntrans; ++i) {
        if (trans[i].digraph.rank == 0)
            traverse(stack, &sp, &trans[i], list_offset);
    }
    print("\033[31;1mend digraph, %u at stack\033[0m\n", sp);
    assert(sp == 0);
}


static void
lalr_state(lr0_machine_t lr0_machine, const struct lr0_state * state)
{
    struct lr0_point closure[state->nclosure];
    unsigned n = lr0_closure(lr0_machine, closure, state);
    assert(n == state->nclosure);
    for (unsigned i = 0; i < n; ++i) {
        if (closure[i].pos >= closure[i].rule->length) {
            const struct rule * rule = closure[i].rule;
            print("May reduce %s in state %u\n", rule->sym->name, state->id);
        }
    }
}

void
lalr_reduce_search(lr0_machine_t lr0_machine)
{
    const unsigned ntrans = find_transitions(lr0_machine, NULL, 0);
    print("LALR: total %u transitions.\n", ntrans);
    struct trans trans[ntrans];
    memset(trans, 0, sizeof(trans));
    find_transitions(lr0_machine, trans, ntrans);
    find_reads(lr0_machine, trans, ntrans);
    digraph(trans, ntrans, offsetof(struct trans, reads));
    find_lookback_includes(lr0_machine, trans, ntrans);
    digraph(trans, ntrans, offsetof(struct trans, includes));

    for (const struct lr0_state * state = lr0_machine->first_state; state; state = state->next)
        lalr_state(lr0_machine, state);

    for (unsigned i = 0; i < ntrans; ++i) {
        while (trans[i].reads) {
            const struct trans_list * inc = trans[i].reads;
            trans[i].reads = trans[i].reads->next;
            free((void *)inc);
        }
        while (trans[i].includes) {
            const struct trans_list * inc = trans[i].includes;
            trans[i].includes = trans[i].includes->next;
            free((void *)inc);
        }
        set_free(trans[i].set);
    }

    for (const struct lr0_state * state = lr0_machine->first_state; state; state = state->next) {
        while (state->tmp.lalr_lookback) {
            struct lalr_lookback * lb = (struct lalr_lookback *) state->tmp.lalr_lookback;
            ((struct lr0_state *)state)->tmp.lalr_lookback = lb->next;
            free(lb);
        }
    }
}
