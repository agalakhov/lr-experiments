#include "lalr.h"

#include "grammar_i.h"
#include "lr0_i.h"

#include "bitset.h"

#include "print.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct trans {
    const struct lr0_state *    state1;
    const struct lr0_state *    state2;
    const struct includes *     includes;
    bitset_t                    set;
};

struct includes {
    const struct includes *     next;
    const struct trans *        trans;
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
lookup_transition(struct trans trans[], unsigned ntrans, const struct trans * needle)
{
    return bsearch(needle, trans, ntrans, sizeof(trans[0]), cmp_trans);
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
add_lookback(const struct lr0_state * state, const struct rule * rule, const struct trans * trans)
{
    print("*   (%u,rule %s) \033[34;1mlookback\033[0m (%u-%s->%u)\n", state->id, rule->sym->name,
          trans->state1->id, trans->state2->access_sym->name, trans->state2->id);
}

static void
add_includes(struct trans * trans1, const struct trans * trans2)
{
    struct includes * inc = calloc(1, sizeof(struct includes));
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
find_lookback_includes(lr0_machine_t lr0_machine, struct trans trans[], unsigned ntrans)
{
    (void) lr0_machine;
    for (unsigned itr = 0; itr < ntrans; ++itr) {
        const struct trans * const tr = &trans[itr];
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
                    struct trans it = { .state1 = oldst, .state2 = st };
                    struct trans * ltr = lookup_transition(trans, ntrans, &it);
                    add_includes(ltr, tr);
                }
            }
            print("\n");
            add_lookback(st, rule, tr);
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
    find_lookback_includes(lr0_machine, trans, ntrans);

    for (unsigned i = 0; i < ntrans; ++i) {
        while (trans[i].includes) {
            const struct includes * inc = trans[i].includes;
            trans[i].includes = trans[i].includes->next;
            free((void *)inc);
        }
        set_free(trans[i].set);
    }
}
