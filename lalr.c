#include "lalr.h"

#include "grammar_i.h"
#include "lr0_i.h"

#include "bitset.h"

#include "print.h"

#include <assert.h>
#include <stdlib.h>

struct trans {
    const struct lr0_state *    state;
    unsigned                    go;
    bitset_t                    set;
};

static bitset_t
create_dr(lr0_machine_t lr0_machine, const struct lr0_gototab * gototab)
{
    bitset_t dr = set_alloc(lr0_machine->grammar->n_terminals + 1);
    if (! dr)
        abort();
    for (unsigned i = 0; i < gototab->ngo; ++i) {
        if (gototab->go[i].sym->type == TERMINAL) {
            print("  directly-reads %s\n", gototab->go[i].sym->name);
            set_add(dr, gototab->go[i].sym->id);
        }
    }
    return dr;
}

static unsigned
find_transitions(lr0_machine_t lr0_machine, struct trans trans[], unsigned ntrans)
{
    unsigned itrans = 0;
    for (const struct lr0_state * state = lr0_machine->first_state; state; state = state->next) {
        const struct lr0_gototab * gototab = state->gototab;
        for (unsigned igo = 0; igo < gototab->ngo; ++igo) {
            if (gototab->go[igo].sym->type == NONTERMINAL) {
                if (trans != NULL) { /* not dry run? */
                    print("LALR: state %u symbol %s\n", state->id, gototab->go[igo].sym->name);
                    assert(itrans < ntrans);
                    trans[itrans].state = state;
                    trans[itrans].go = igo;
                    trans[itrans].set = create_dr(lr0_machine, gototab->go[igo].state->gototab);
                }
                ++itrans;
            }
        }
    }
    return itrans;
}

void
lalr_reduce_search(lr0_machine_t lr0_machine)
{
    const unsigned ntrans = find_transitions(lr0_machine, NULL, 0);
    struct trans trans[ntrans];
    find_transitions(lr0_machine, trans, ntrans);

    for (unsigned i = 0; i < ntrans; ++i)
        set_free(trans[i].set);
}