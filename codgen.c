#include "codgen.h"

#include "grammar_i.h"
#include "lr0_i.h"


void
codgen_c(FILE *fd, lr0_machine_t machine)
{
    fprintf(fd, "/* GENERATED FILE - DO NOT EDIT */\n");

    const struct grammar *grammar = machine->grammar;
    for (const struct symbol *sym = grammar->symlist.first; sym; sym = sym->next) {
        if (sym->type != NONTERMINAL)
            continue;
        for (const struct rule *rule = sym->nt.rules; rule; rule = rule->next) {
            if (rule->host_code) {
                fprintf(fd, "\nstatic void __reduce_%s_%u(\n", sym->name, rule->id);
                for (unsigned i = 0; i < rule->length; ++i) {
                    fprintf(fd, "  %s%s\n", rule->rs[i].label, (i + 1 == rule->length) ? "" : ",");
                }
                fprintf(fd, ") {%s}\n", rule->host_code);
            }
        }
    }

}
