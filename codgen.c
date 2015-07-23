#include "codgen.h"

#include "grammar_i.h"
#include "lr0_i.h"

#include <string.h>

struct argument {
    signed          stack_index;
    const char *    host_type;
    const char *    name;
};

struct reduce {
    unsigned        id;
    const char *    name;
    const char *    host_code;
    unsigned        pop_size;
    unsigned        nargs;
    struct argument args[];
};
static inline size_t sizeof_struct_reduce(unsigned nargs) {
    return sizeof(struct reduce) + nargs * sizeof(struct argument);
}

typedef void (*emit_func_t)(FILE *fd, const struct reduce *reduce);

static const char*
symtype(const struct symbol *sym)
{
    return sym->host_type ? sym->host_type : "void *";
}


static void
foreach_rule(lr0_machine_t machine, emit_func_t func, FILE *fd)
{
    const struct grammar *grammar = machine->grammar;
    for (const struct symbol *sym = grammar->symlist.first; sym; sym = sym->next) {
        if (sym->type != NONTERMINAL)
            continue;
        for (const struct rule *rule = sym->nt.rules; rule; rule = rule->next) {
            char pool[sizeof_struct_reduce(rule->length + 1)];
            memset(pool, 0, sizeof(pool));
            struct reduce *reduce = (struct reduce*)pool;
            size_t namelen = 1 + snprintf(NULL, 0, "%s_%u", rule->sym->name, rule->id);
            char name[namelen];
            snprintf(name, namelen, "%s_%u", rule->sym->name, rule->id);
            reduce->id = rule->id;
            reduce->name = name;
            reduce->host_code = rule->host_code;
            reduce->pop_size = rule->length;
            reduce->nargs = 0;
            if (rule->ls_label) {
                struct argument *arg = &reduce->args[reduce->nargs++];
                arg->stack_index = 0;
                arg->host_type = symtype(rule->sym);
                arg->name = rule->ls_label;
            }
            for (unsigned i = 0; i < rule->length; ++i) {
                if (rule->rs[i].label) {
                    struct argument *arg = &reduce->args[reduce->nargs++];
                    arg->stack_index = i - rule->length - 1;
                    arg->host_type = symtype(rule->rs[i].sym.sym);
                    arg->name= rule->rs[i].label;
                }
            }
            func(fd, reduce);
        }
    }
}

static void
emit_reduce_c(FILE *fd, const struct reduce *reduce)
{
    if (! reduce->host_code)
        return;
    for (unsigned i = 0; i < reduce->nargs; ++i) {
        fprintf(fd, "#define %s (*__%s)\n", reduce->args[i].name, reduce->args[i].name);
    }
    fprintf(fd, "static inline void __reduce_%s (", reduce->name);
    for (unsigned i = 0; i < reduce->nargs; ++i) {
        fprintf(fd, "\n  %s %s*__%s%s", reduce->args[i].host_type,
                (i ? "const " : ""), reduce->args[i].name,
                (i + 1 < reduce->nargs ? "," : ""));
    }
    fprintf(fd, "%s) {%s}\n", (reduce->nargs ? "\n" : ""), reduce->host_code);
    for (unsigned i = 0; i < reduce->nargs; ++i) {
        fprintf(fd, "#undef %s\n", reduce->args[i].name);
    }
    fprintf(fd, "\n");
}

static void
emit_action_c(FILE *fd, const struct reduce *reduce)
{
    if (reduce->id == 0)
        return;
    fprintf(fd, "    case %u: /* %s */\n", reduce->id, reduce->name);
    fprintf(fd, "        __assert_stack(stack, %u);\n", reduce->pop_size);
    if (reduce->host_code) {
        fprintf(fd, "        __reduce_%s(", reduce->name);
        for (unsigned i = 0; i < reduce->nargs; ++i) {
            fprintf(fd, "%s&stack->data[%i]", (i ? ", " : ""), reduce->args[i].stack_index);
        }
        fprintf(fd, ");\n");
    }
    fprintf(fd, "        __pop(stack, %u);\n", reduce->pop_size);
    fprintf(fd, "        break;\n");
}

void
codgen_c(FILE *fd, lr0_machine_t machine)
{
    fprintf(fd, "/* GENERATED FILE - DO NOT EDIT */\n\n");
    foreach_rule(machine, emit_reduce_c, fd);
    fprintf(fd, "static void __action (struct __stack *stack, unsigned id) {\n");
    fprintf(fd, "    switch (id) {\n");
    foreach_rule(machine, emit_action_c, fd);
    fprintf(fd, "    }\n");
    fprintf(fd, "}\n\n");
}
