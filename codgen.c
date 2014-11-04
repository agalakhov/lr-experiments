#include "codgen.h"

#include "grammar_i.h"
#include "lr0_i.h"

static const char*
symtype(const struct symbol *sym)
{
    return sym->host_type ? sym->host_type : "void *";
}

void
codgen_c(FILE *fd, lr0_machine_t machine)
{
    fprintf(fd, "/* GENERATED FILE - DO NOT EDIT */\n\n");

    const struct grammar *grammar = machine->grammar;
    for (const struct symbol *sym = grammar->symlist.first; sym; sym = sym->next) {
        if (sym->type != NONTERMINAL)
            continue;
        for (const struct rule *rule = sym->nt.rules; rule; rule = rule->next) {
            if (rule->host_code) {
                if (rule->ls_label) {
                    fprintf(fd, "#define %s (*__%s)\n", rule->ls_label, rule->ls_label);
                }
                for (unsigned i = 0; i < rule->length; ++i) {
                    if (rule->rs[i].label) {
                        fprintf(fd, "#define %s (*__%s)\n", rule->rs[i].label, rule->rs[i].label);
                    }
                }
                fprintf(fd, "static void __reduce_%s_%u (", sym->name, rule->id);
                unsigned irule = 0;
                if (rule->ls_label) {
                    fprintf(fd, "\n  %s *__%s", symtype(sym), rule->ls_label);
                    ++irule;
                }
                for (unsigned i = 0; i < rule->length; ++i) {
                    if (rule->rs[i].label) {
                        fprintf(fd, "%s\n  %s const *__%s", irule ? "," : "",
                                symtype(rule->rs[i].sym.sym),
                                rule->rs[i].label);
                        ++irule;
                    }
                }
                fprintf(fd, "%s) {%s}\n", irule ? "\n" : "", rule->host_code);
                if (rule->ls_label) {
                    fprintf(fd, "#undef %s\n", rule->ls_label);
                }
                for (unsigned i = 0; i < rule->length; ++i) {
                    if (rule->rs[i].label) {
                        fprintf(fd, "#undef  %s\n", rule->rs[i].label);
                    }
                }
                fprintf(fd, "\n");
            }
        }
    }

}
