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
                fprintf(fd, "\nstatic void __reduce_%s_%u(", sym->name, rule->id);
                unsigned irule = 0;
                for (unsigned i = 0; i < rule->length; ++i) {
                    if (rule->rs[i].label) {
                        fprintf(fd, "%s\n  %s %s", irule ? "," : "",
                                rule->rs[i].sym.sym->host_type ? rule->rs[i].sym.sym->host_type : "void*",
                                rule->rs[i].label);
                        ++irule;
                    }
                }
                fprintf(fd, "%s) {%s}\n", irule ? "\n" : "", rule->host_code);
            }
        }
    }

}
