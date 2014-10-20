#include "slr.h"

#include "grammar_i.h"
#include "lr0_i.h"

#include "print.h"

#include <assert.h>
#include <stdlib.h>

static inline void
add_reduce(struct lr_reduce * rdc, const struct symbol * sym, const struct rule * rule)
{
    printo(P_LR_REDUCE, "    [%s] :< %s ~%u\n", (sym ? sym->name : "$"), rule->sym->name, rule->id);
    rdc->sym = sym;
    rdc->rule = rule;
}

void
slr_reduce_search(lr0_machine_t lr0_machine)
{
    for (const struct lr0_state * state = lr0_machine->first_state; state; state = state->next) {
        printo(P_LR_REDUCE, "\nState %u:\n", state->id);
        /* Determine the reducetab size */
        unsigned rtab_size = 0;
        for (unsigned i = 0; i < state->npoints; ++i) {
            const struct lr0_point * p = &state->points[i];
            if (p->pos == p->rule->length)
                rtab_size += set_size(p->rule->sym->follow);
        }
        /* Calculate the reducdetab */
        struct lr_reducetab * rtab = calloc(1, sizeof_struct_lr_reducetab(rtab_size));
        if (! rtab)
            abort();
        unsigned nrtab = 0;;
        for (unsigned i = 0; i < state->npoints; ++i) {
            const struct lr0_point * p = &state->points[i];
            if (p->pos == p->rule->length) {
                if (set_has(p->rule->sym->follow, 0)) {
                    add_reduce(&rtab->reduce[nrtab], NULL, p->rule);
                    ++nrtab;
                }
            }
        }
        for (const struct symbol * sym = lr0_machine->grammar->symlist.first; sym; sym = sym->next) {
            for (unsigned i = 0; i < state->npoints; ++i) {
                const struct lr0_point * p = &state->points[i];
                if (p->pos == p->rule->length) {
                    /* This is reduceable. */
                    if (set_has(p->rule->sym->follow, sym->id)) {
                        add_reduce(&rtab->reduce[nrtab], sym, p->rule);
                        ++nrtab;
                    }
                }
            }
        }
        assert(nrtab == rtab_size);
        rtab->nreduce = nrtab;
        ((struct lr0_state *)state)->reducetab = rtab;
    }
}
