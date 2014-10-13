#include "slr.h"

#include "grammar_i.h"
#include "lr0_i.h"

#include "print.h"

// FIXME FIXME FIXME this sholud NOT be here!
struct lr0_machine {
    grammar_t			grammar;
    unsigned                    grammar_size;
    unsigned                    nstates;
    struct lr0_state *          process_first;
};


void
slr_reduce_search(lr0_machine_t lr0_machine)
{
    for (const struct lr0_state * s = lr0_machine->process_first; s; s = s->next) { 
        for (unsigned i = 0; i < s->npoints; ++i) {
            const struct lr0_point * p = &s->points[i];
            if (p->pos == p->rule->length) {
                /* This is reduceable. */
                if (set_has(p->rule->sym->follow, 0))
                    printo(P_LR0_CLOSURES, "    [$] :< %s :: {%s}\n", p->rule->sym->name, p->rule->host_code);
                for (const struct symbol * sym = lr0_machine->grammar->symlist.first; sym; sym = sym->next) {
                    if (set_has(p->rule->sym->follow, sym->id))
                        printo(P_LR0_CLOSURES, "    [%s] :< %s :: {%s}\n", sym->name, p->rule->sym->name, p->rule->host_code);
                }
            }
        }
    }
}
