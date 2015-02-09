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
            set_add(dr, gototab->go[i].sym->id);
        }
    }
    return dr;
}

static void
compute_reads(lr0_machine_t lr0_machine)
{
    return;
    create_dr(lr0_machine, NULL);
}


void
lalr_reduce_search(lr0_machine_t lr0_machine)
{
    compute_reads(lr0_machine);
}
