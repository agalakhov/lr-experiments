#include "conflict.h"

#include "grammar_i.h"
#include "lr0_i.h"

#include <stdio.h>

bool
conflicts(lr0_machine_t machine)
{
    bool ret = false;
    for (const struct lr0_state * state = machine->first_state; state; state = state->next) {
        const struct lr0_gototab * shifttab = state->gototab;
        const struct lr_reducetab * reducetab = state->reducetab;
        unsigned ishift = 0;
        unsigned symid = (unsigned)(-1);
        for (unsigned ireduce = 0; ireduce < reducetab->nreduce; ++ireduce) {
            unsigned rsymid = reducetab->reduce[ireduce].sym ? reducetab->reduce[ireduce].sym->id : 0;
            if (symid != rsymid) {
                symid = rsymid;
            } else {
                fprintf(stderr, "Reduce-reduce conflict in state %u at symbol %s.\n",
                        state->id, rsymid ? reducetab->reduce[ireduce].sym->name : "$");
                ret = true;
            }
            while (ishift < shifttab->ngo && shifttab->go[ishift].sym->id < symid)
                ++ishift;
            if (ishift < shifttab->ngo && shifttab->go[ishift].sym->id == symid) {
                fprintf(stderr, "Shift-reduce conflict in state %u at symbol %s.\n",
                        state->id, reducetab->reduce[ireduce].sym->name);
                ret = true;
            }
        }
    }
    return ret;
}
