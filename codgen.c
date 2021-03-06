#include "codgen.h"

#include "grammar_i.h"
#include "lr0_i.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "blob_c.h"

struct argument {
    signed          stack_index;
    const char *    host_type;
    const char *    name;
    const char *    type;
};

struct reduce {
    unsigned        id;
    unsigned        symbol;
    const char *    name;
    const char *    host_code;
    const char *    extra_argument;
    unsigned        pop_size;
    unsigned        nargs;
    struct argument args[];
};
static inline size_t sizeof_struct_reduce(unsigned nargs) {
    return sizeof(struct reduce) + nargs * sizeof(struct argument);
}

typedef void (*emit_func_t)(FILE *fd, const struct reduce *reduce);

static const char*
termtype(const struct grammar *grammar)
{
    return grammar->terminal_host_type ? grammar->terminal_host_type : "int";
}

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
            reduce->symbol = sym->id;
            reduce->name = name;
            reduce->host_code = rule->host_code;
            reduce->extra_argument = grammar->extra_argument;
            reduce->pop_size = rule->length;
            reduce->nargs = 0;
            if (rule->ls_label) {
                struct argument *arg = &reduce->args[reduce->nargs++];
                arg->stack_index = 0;
                arg->host_type = symtype(rule->sym);
                arg->name = rule->ls_label;
                arg->type = sym->name;
            }
            for (unsigned i = 0; i < rule->length; ++i) {
                struct argument *arg = &reduce->args[reduce->nargs++];
                arg->stack_index = (signed)i - (signed)rule->length;
                arg->host_type = (rule->rs[i].sym->type == TERMINAL) ? termtype(grammar) : symtype(rule->rs[i].sym);
                arg->name = rule->rs[i].label;
                arg->type = (rule->rs[i].sym->type == TERMINAL) ? "__terminal" : rule->rs[i].sym->name;
            }
            func(fd, reduce);
        }
    }
}

static void
emit_functions_c(FILE *fd, const struct reduce *reduce)
{
    if (! reduce->host_code)
        return;
    for (unsigned i = 0; i < reduce->nargs; ++i) {
        if (reduce->args[i].name)
            fprintf(fd, "#define %s (*__%s)\n", reduce->args[i].name, reduce->args[i].name);
    }
    fprintf(fd, "static inline void\n__reduce_%s (", reduce->name);
    bool cont = false;
    if (reduce->extra_argument) {
        fprintf(fd, "%s", reduce->extra_argument);
        cont = true;
    }
    for (unsigned i = 0; i < reduce->nargs; ++i) {
        if (reduce->args[i].name) {
            fprintf(fd, "%s\n  %s %s*__%s", (cont ? "," : ""), reduce->args[i].host_type,
                    (i ? "const " : ""), reduce->args[i].name);
            cont = true;
        }
    }
    fprintf(fd, "%s) {%s}\n", (cont ? "\n" : ""), reduce->host_code);
    for (unsigned i = 0; i < reduce->nargs; ++i) {
        if (reduce->args[i].name) {
            fprintf(fd, "#undef %s\n", reduce->args[i].name);
        }
    }
    fprintf(fd, "\n");
}

static void
emit_destructor_c(FILE *fd, const char *name, const char *host_type, const char *host_code)
{
    fprintf(fd, "static inline void\n__destruct_%s(%s *__T)\n{", name, host_type);
    if (host_code) {
        const char *p = host_code;
        for (;;) {
            const char *q = strstr(p, "$$");
            if (! q) {
                fwrite(p, 1, strlen(p), fd);
                break;
            } else {
                fwrite(p, 1, (q - p), fd);
                fprintf(fd, "(*__T)");
                p = q + 2;
            }
        }
    } else {
        fprintf(fd, "\n    (void) __T;\n");
    }
    fprintf(fd, "}\n\n");
}

static void
emit_destructors_c(FILE *fd, lr0_machine_t machine)
{
    const struct grammar *grammar = machine->grammar;
    emit_destructor_c(fd, "__terminal", termtype(grammar), grammar->terminal_destructor_code);
    for (const struct symbol *sym = grammar->symlist.first; sym; sym = sym->next) {
        if (sym->type != NONTERMINAL)
            continue;
        emit_destructor_c(fd, sym->name, symtype(sym), sym->destructor_code);
    }
}

static void
emit_reduce_c(FILE *fd, const struct reduce *reduce)
{
    if (reduce->id == 0)
        return;
    fprintf(fd, "    case %u: /* %s */\n", reduce->id, reduce->name);
    fprintf(fd, "        __assert_stack(parser, %u);\n", reduce->pop_size);
    if (reduce->host_code) {
        fprintf(fd, "        __reduce_%s(__EXTRA_ARGUMENT_VALUE", reduce->name);
        bool cont = false;
        for (unsigned i = 0; i < reduce->nargs; ++i) {
            if (reduce->args[i].name) {
                fprintf(fd, "%s&parser->stack.sp[%i].item.%s",
                        (cont ? ", " : ""), reduce->args[i].stack_index, reduce->args[i].type);
                cont = true;
            }
        }
        fprintf(fd, ");\n");
        for (unsigned i = 0; i < reduce->nargs; ++i) {
            if (! reduce->args[i].name) {
                fprintf(fd, "        __destruct_%s(&parser->stack.sp[%i].item.%s);\n",
                        reduce->args[i].type, reduce->args[i].stack_index, reduce->args[i].type);
            }
        }
    }
    fprintf(fd, "        __pop(parser, %u, %u);\n", reduce->pop_size, reduce->symbol);
    fprintf(fd, "        break;\n");
}

static void
emit_types_c(FILE *fd, lr0_machine_t machine)
{
    const struct grammar *grammar = machine->grammar;
    for (const struct symbol *sym = grammar->symlist.first; sym; sym = sym->next) {
        if (sym->type != NONTERMINAL)
            continue;
        fprintf(fd, "    %s %s;\n", symtype(sym), sym->name);
    }
    fprintf(fd, "    %s __terminal;\n", termtype(grammar));
}

static void
emit_defines_c(FILE *fd, lr0_machine_t machine)
{
    const struct grammar *grammar = machine->grammar;
    const char *machine_name = grammar->machine_name;
    const char *prefix = machine_name ? machine_name : "";
    const char *delim = machine_name ? "_##" : "";
    fprintf(fd, "#define __EXPORT(x) %s%sx\n", prefix, delim);
    if (grammar->extra_argument) {
        fprintf(fd, "#define __EXTRA_ARGUMENT %s,\n", grammar->extra_argument);
        const char *value = grammar->extra_argument;
        value += strlen(value);
        bool seenchr = false;
        for (; value != grammar->extra_argument; --value) {
            if (!strchr(" \t", *value)) {
                seenchr = true;
            }
            if (seenchr && strchr(" \t*&", *value)) {
                ++value;
                break;
            }
        }
        fprintf(fd, "#define __EXTRA_ARGUMENT_VALUE %s,\n", value);
    } else {
        fprintf(fd, "#define __EXTRA_ARGUMENT\n");
        fprintf(fd, "#define __EXTRA_ARGUMENT_VALUE\n");
    }
    fprintf(fd, "typedef %s __EXPORT(terminal_t);\n", termtype(machine->grammar));
}

static void
emit_actions_c(FILE *fd, lr0_machine_t machine)
{
    fprintf(fd, "static const unsigned __actions[][%u] = {\n",
            machine->grammar->n_terminals + machine->grammar->n_nonterminals);
    for (const struct lr0_state * s = machine->first_state; s; s = s->next) {
        fprintf(fd, "    {");
        unsigned id = 0;
        for (unsigned i = 0; i < s->reducetab->nreduce; ++i) {
            const struct lr_reduce *rdc = &s->reducetab->reduce[i];
            for (; id < rdc->sym->id; ++id)
                fprintf(fd, " 0,");
            fprintf(fd, " %u,", rdc->rule->id);
            ++id;
        }
        for (; id < machine->grammar->n_terminals + machine->grammar->n_nonterminals; ++id)
            fprintf(fd, " 0,");
        fprintf(fd, " },\n");
    }
    fprintf(fd, "};\n");
}

static void
emit_gotos_c(FILE *fd, lr0_machine_t machine)
{
    fprintf(fd, "static const unsigned __gotos[][%u] = {\n",
            machine->grammar->n_terminals + machine->grammar->n_nonterminals);
    for (const struct lr0_state * s = machine->first_state; s; s = s->next) {
        fprintf(fd, "    {");
        unsigned id = 0;
        for (unsigned i = 0; i < s->gototab->ngo; ++i) {
            const struct lr0_state *go = s->gototab->go[i];
            for (; id < go->access_sym->id; ++id)
                fprintf(fd, " 0,");
            fprintf(fd, " %u,", go->id);
            ++id;
        }
        for (; id < machine->grammar->n_terminals + machine->grammar->n_nonterminals; ++id)
            fprintf(fd, " 0,");
        fprintf(fd, " },\n");
    }
    fprintf(fd, "};\n");
}

void
codgen_c(FILE *fd, lr0_machine_t machine)
{
    FILE *tmpl = fmemopen((void*)blob_c, sizeof(blob_c), "rb");
    if (! tmpl)
        abort();

    fprintf(fd, "/* GENERATED FILE - DO NOT EDIT */\n\n");

    bool noeol = false;

    while (true) {
        char line[1024];
        errno = 0;
        if (! fgets(line, sizeof(line), tmpl)) {
            if (errno)
                abort();
            break;
        }

        if (noeol || line[0] != '%' || line[1] != '%') {
            fputs(line, fd);
            noeol = (line[strlen(line) - 1] != '\n');
        } else if (! strcmp(line, "%%include\n")) {
            if (machine->grammar->host_code)
                fputs(machine->grammar->host_code, fd);
        } else if (! strcmp(line, "%%types\n")) {
            emit_types_c(fd, machine);
        } else if (! strcmp(line, "%%functions\n")) {
            foreach_rule(machine, emit_functions_c, fd);
            emit_destructors_c(fd, machine);
        } else if (! strcmp(line, "%%reduce\n")) {
            foreach_rule(machine, emit_reduce_c, fd);
        } else if (! strcmp(line, "%%actions\n")) {
            emit_actions_c(fd, machine);
        } else if (! strcmp(line, "%%gotos\n")) {
            emit_gotos_c(fd, machine);
        } else if (! strcmp(line, "%%defines\n")) {
            emit_defines_c(fd, machine);
        } else {
            fprintf(fd, "//");
            fputs(line, fd);
        }
    }

    fclose(tmpl);
}

void
codgen_c_h(FILE *fd, lr0_machine_t machine)
{
    fprintf(fd, "/* GENERATED FILE - DO NOT EDIT */\n");
    fprintf(fd, "#pragma once\n\n");

    fprintf(fd, "enum {");
    bool cont = false;
    const struct grammar *grammar = machine->grammar;
    for (const struct symbol * sym = grammar->symlist.first; sym; sym = sym->next) {
        if (sym->type == TERMINAL) {
            if (sym->name[0] != '%') {
                fprintf(fd, "%s\n    %s = %u", (cont ? "," : ""), sym->name, sym->id);
                cont = true;
            }
        }
    }
    fprintf(fd, "\n};\n");
}

