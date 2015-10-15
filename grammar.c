#include "grammar.h"

#include "grammar_i.h"
#include "nullable.h"
#include "strhash.h"

#include "print.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/*
 *  Constructor for struct grammar *
 */
struct grammar *
grammar_alloc(void)
{
    struct grammar *ret = (struct grammar *) calloc(1, sizeof(struct grammar));
    if (! ret)
        return NULL;
    ret->hash = strhash_alloc();
    if (! ret->hash) {
        free(ret);
        return NULL;
    }
    return ret;
}

static void
destroy_terminal(struct terminal *t)
{
    (void) t;
}

static void
destroy_rule(struct rule *rule)
{
    if (rule->host_code)
        free((void *)rule->host_code);
    if (rule->ls_label)
        free((void *)rule->ls_label);
    for (unsigned i = 0; i < rule->length; ++i) {
        if (rule->rs[i].label)
            free((void *)rule->rs[i].label);
    }
    free(rule);
}

static void
destroy_nonterminal(struct nonterminal *nt)
{
    struct rule *r = (struct rule *) nt->rules;
    while (r) {
        struct rule *n = (struct rule *) r->next;
        destroy_rule(r);
        r = n;
    }
}

static void
destroy_symbol(void *ptr)
{
    struct symbol *sym = (struct symbol *) ptr;
    if (! sym)
        return;
    switch (sym->type) {
        case UNKNOWN:
            break;
        case TERMINAL:
            destroy_terminal(&sym->t);
            break;
        case NONTERMINAL:
        case START_NONTERMINAL:
            destroy_nonterminal(&sym->nt);
            break;
    }
    if (sym->host_type)
        free((void *)sym->host_type);
    if (sym->destructor_code)
        free((void *)sym->destructor_code);
    free(sym);
}

/*
 *  Destructor for struct grammar *
 */
void
grammar_free(struct grammar *grammar)
{
    if (grammar->machine_name)
        free((void *)grammar->machine_name);
    if (grammar->terminal_host_type)
        free((void *)grammar->terminal_host_type);
    if (grammar->terminal_destructor_code)
        free((void *)grammar->terminal_destructor_code);
    if (grammar->host_code)
        free((void *)grammar->host_code);
    if (grammar->extra_argument)
        free((void *)grammar->extra_argument);
    strhash_free(grammar->hash, destroy_symbol);
    free(grammar);
}

static void
commit_symbol(struct grammar *grammar, struct symbol *sym)
{
    if (! grammar->symlist.first) {
        sym->next = NULL;
        grammar->symlist.first = sym;
        grammar->symlist.last = sym;
        return;
    }
    switch (sym->type) {
        case UNKNOWN:
            break;
        case TERMINAL:
            sym->next = grammar->symlist.first;
            grammar->symlist.first = sym;
            break;
        case NONTERMINAL:
        case START_NONTERMINAL:
            sym->next = grammar->symlist.last->next;
            grammar->symlist.last->next = sym;
            grammar->symlist.last = sym;
            break;
    }
}

struct symbol *
grammar_symbol(struct grammar *grammar, const char *name, enum symbol_type type)
{
    struct symbol **s = (struct symbol **) strhash_find(grammar->hash, name);
    if (! *s) {
        *s = calloc(1, sizeof(struct symbol));
        if (! *s)
            abort();
        (*s)->type = (enum symbol_typex)type;
        (*s)->name = strhash_key((void **)s);
        if (type != SYMBOL_UNKNOWN)
            commit_symbol(grammar, *s);
    } else if ((*s)->type == UNKNOWN) {
        if (type != SYMBOL_UNKNOWN) {
            (*s)->type = type;
            commit_symbol(grammar, *s);
        }
    } else if ((type != SYMBOL_UNKNOWN) && ((*s)->type != (enum symbol_typex)type)) {
        print("error: reassigning type for symbol %s\n", name);
        return NULL;
    }
    return *s;
}

/*
 * Assign name to the grammar
 */
void
grammar_name(struct grammar *grammar, const char *name)
{
    if (grammar->machine_name) {
        print("error: grammar already has a name\n");
        return;
    }
    grammar->machine_name = strdup(name);
}

/*
 * Add raw host code to the grammar
 */
void
grammar_add_host_code(struct grammar *grammar, const char *host_code)
{
    size_t len = strlen(host_code);
    if (grammar->host_code)
        len += strlen(grammar->host_code);
    char *str = calloc(1, len + 1);
    if (grammar->host_code)
        strcpy(str, grammar->host_code);
    strcat(str, host_code);
    if (grammar->host_code)
        free((void *)grammar->host_code);
    grammar->host_code = str;
}

void
grammar_set_extra_argument(struct grammar *grammar, const char *extra_argument)
{
    if (grammar->extra_argument) {
        print("error: grammar already has extra_argument\n");
        return;
    }
    grammar->extra_argument = strdup(extra_argument);
}

/* 
 * Assign host type to terminals.
 */
void 
grammar_assign_terminal_type(struct grammar *grammar, const char *type)
{
    if (grammar->terminal_host_type) {
        print("error: reassigning terminal type\n");
        return;
    }
    grammar->terminal_host_type = strdup(type);
}

/*
 * Assign host type to a symbol.
 */
void
grammar_assign_type(struct grammar *grammar, const char *name, const char *type)
{
    struct symbol *s = grammar_symbol(grammar, name, SYMBOL_NONTERMINAL);
    if (! s)
        return;
    if (s->host_type) {
        print("error: reassiginig symbol type for %s\n", name);
        return;
    }
    s->host_type = strdup(type);
}

/* 
 * Assign host type to terminals.
 */
void
grammar_assign_terminal_destructor(struct grammar *grammar, const char *destructor_code)
{
    if (grammar->terminal_destructor_code) {
        print("error: reassigning terminal destructor code\n");
        return;
    }
    grammar->terminal_destructor_code = strdup(destructor_code);
}

/*
 * Assign destructor code to a symbol.
 */
void
grammar_assign_destructor(struct grammar *grammar, const char *name, const char *destructor_code)
{
    struct symbol *s = grammar_symbol(grammar, name, SYMBOL_NONTERMINAL);
    if (! s)
        return;
    if (s->destructor_code) {
        print("error: reassiginig symbol destructor for %s\n", name);
        return;
    }
    s->destructor_code = strdup(destructor_code);
}

void
grammar_deduce_type(struct grammar *grammar, const char *ls_name, const char *rs_name)
{
    struct symbol *ls = grammar_symbol(grammar, ls_name, SYMBOL_UNKNOWN);
    struct symbol *rs = grammar_symbol(grammar, rs_name, SYMBOL_UNKNOWN);
    if (! ls->host_type) {
        if (rs->host_type)
            ls->host_type = strdup(rs->host_type);
    } else {
        if (! rs->host_type || strcmp(ls->host_type, rs->host_type))
            print("error: overriding type for %s\n", ls_name);
    }
}

/*
 * Add a new nonterminal rule to the grammar while building it.
 */
void
grammar_nonterminal(struct grammar *grammar,
                    const struct grammar_element *ls,
                    unsigned rsn, const struct grammar_element rs[],
                    const char *host_code)
{
    struct symbol *s = grammar_symbol(grammar, ls->name, SYMBOL_NONTERMINAL);
    if (! s) {
        print("error: symbol `%s' is not a nonterminal\n", ls->name);
        return;
    }

    struct rule *rule = calloc(1, sizeof_struct_rule(rsn));
    if (! rule)
        abort();
    rule->next = s->nt.rules;
    rule->sym = s;
    s->nt.rules = rule;
    rule->id = ++(grammar->n_rules);
    if (ls->label) {
        const char *lbl = strdup(ls->label);
        if (! lbl)
            abort();
        rule->ls_label = lbl;
    }
    if (host_code) {
        const char *hc = strdup(host_code);
        if (! hc)
            abort();
        rule->host_code = hc;
    }

    rule->length = rsn;
    for (unsigned i = 0; i < rsn; ++i) {
        rule->rs[i].sym = grammar_symbol(grammar, rs[i].name, SYMBOL_UNKNOWN);
        if (rs[i].label) {
                const char *lbl = strdup(rs[i].label);
            if (! lbl)
                abort();
            rule->rs[i].label = lbl;
        }
    }
}


/*
 * Set grammar start symbol
 */
void
grammar_start_symbol(struct grammar *grammar, const char *start)
{
    grammar->start = grammar_symbol(grammar, start, SYMBOL_NONTERMINAL);
}


static void
resolve_symbols(struct grammar *grammar)
{
    for (struct symbol * sym = grammar->symlist.first; sym; sym = sym->next) {
        for (const struct rule * rule = sym->nt.rules; rule; rule = rule->next) {
            for (unsigned i = 0; i < rule->length; ++i) {
                struct symbol * s = rule->rs[i].sym;
                if (s->type == UNKNOWN) {
                    s->type = TERMINAL;
                    commit_symbol(grammar, s);
                }
                ++(s->use_count);
            }
        }
    }
}

static void
add_sentinel_rule(struct grammar *grammar)
{
    struct symbol ** eof = (struct symbol **) strhash_find(grammar->hash, "%eof");
    assert(! *eof);
    *eof = calloc(1, sizeof(struct symbol));
    if (! *eof)
        abort();
    (*eof)->type = TERMINAL;
    (*eof)->name = strhash_key((void **) eof);
    commit_symbol(grammar, *eof);

    struct symbol ** start = (struct symbol **) strhash_find(grammar->hash, "%start");
    assert(! *start);
    *start = calloc(1, sizeof(struct symbol));
    if (! *start)
        abort();
    (*start)->type = START_NONTERMINAL;
    (*start)->name = strhash_key((void **) start);
    commit_symbol(grammar, *start);

    struct rule * rule = calloc(1, sizeof_struct_rule(2));
    if (! rule)
        abort();
    (*start)->nt.rules = rule;

    ++(grammar->n_rules);
    rule->sym = *start;
    rule->id = 0;
    rule->length = 2;
    rule->rs[0].sym = (struct symbol *) grammar->start;
    rule->rs[1].sym = *eof;
    for (unsigned i = 0; i < rule->length; ++i)
        ++(((struct symbol*)rule->rs[i].sym)->use_count);

    grammar->start = *start;
}

static const char *
strtype(const struct symbol * sym)
{
    switch (sym->type) {
        case UNKNOWN:
            return "!undefined!";
        case TERMINAL:
            return "terminal";
        case NONTERMINAL:
        case START_NONTERMINAL:
            return "nonterminal";
    }
    return "BUG";
}

static void
count_symbols(struct grammar *grammar)
{
    unsigned id = 0;
    for (struct symbol * sym = grammar->symlist.first; sym; sym = sym->next, ++id) {
        sym->id = id;
        switch (sym->type) {
            case UNKNOWN:
                // FIXME this is actually impossible, UNKNOWN are not listed here
                print("warning: symbol `%s' not defined in grammar\n", sym->name);
                break;
            case TERMINAL:
                ++(grammar->n_terminals);
                break;
            case NONTERMINAL:
            case START_NONTERMINAL:
                ++(grammar->n_nonterminals);
                break;
        }
    }
}

static void
determine_start(struct grammar * grammar)
{
    for (struct symbol * sym = grammar->symlist.first; sym; sym = sym->next) {
        if (! sym->use_count) {
            if (! grammar->start && sym->type == NONTERMINAL) {
                print("note: using `%s' as start symbol\n", sym->name);
                grammar->start = sym;
            } else if (sym != grammar->start && sym->type != UNKNOWN) {
                print("warning: unused %s symbol `%s'\n", strtype(sym), sym->name);
            }
        }
    }
}

static void
dump_grammar(const struct grammar *grammar)
{
    print("-- Grammar:\n");
    for (struct symbol * sym = grammar->symlist.first; sym; sym = sym->next) {
        switch (sym->type) {
            case UNKNOWN:
                break;
            case TERMINAL:
                print("%%terminal %s.\n", sym->name);
                break;
            case NONTERMINAL:
            case START_NONTERMINAL:
                for (const struct rule * r = sym->nt.rules; r; r = r->next) {
                    print("// %i\n%s ::=", r->id, sym->name);
                    for (unsigned i = 0; i < r->length; ++i)
                        print(" %s", r->rs[i].sym->name);
                    print(".");
                    if (r->host_code)
                        print(" {%s}", r->host_code);
                    print("\n");
                }
                break;
        }
    }
    print("--\n");
}

static void
dump_symbols(const struct grammar *grammar)
{
    print("-- Terminals (%u):\n", grammar->n_terminals);
    for (struct symbol * sym = grammar->symlist.first; sym; sym = sym->next) {
        if (sym->id == grammar->n_terminals) {
            print("-- Nonterminals (%u):\n", grammar->n_nonterminals);
        }
        print("  %s = %u\n", sym->name, sym->id);
    }
    print("--\n");
}

/*
 * Finish building grammar
 */
void
grammar_complete(struct grammar *grammar)
{
    resolve_symbols(grammar);
    determine_start(grammar);
    add_sentinel_rule(grammar);
    count_symbols(grammar);
    /* At this point we have complete symbol graph */

    if (print_opt(P_GRAMMAR))
        dump_grammar(grammar);
    if (print_opt(P_SYMBOLS))
        dump_symbols(grammar);

    nullable_find(grammar);

    if (print_opt(P_NULLABLE))
        nullable_dump(grammar);
}

