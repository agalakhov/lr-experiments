#include "grammar.h"

#include "conflict.h"
#include "find.h"
#include "lr0.h"
#include "grammar_i.h"
#include "slr.h"
#include "lalr.h"
#include "strhash.h"
#include "bitset.h"

#include "print.h"

#include "codgen.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/*
 *  Constructor for grammar_t
 */
grammar_t
grammar_alloc(void)
{
    grammar_t ret = (grammar_t) calloc(1, sizeof(struct grammar));
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
            destroy_nonterminal(&sym->nt);
            break;
    }
    if (sym->first)
        set_free(sym->first);
    if (sym->follow)
        set_free(sym->follow);
    if (sym->host_type)
        free((void *)sym->host_type);
    free(sym);
}

static void
commit_symbol(grammar_t grammar, struct symbol *sym)
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
            sym->next = grammar->symlist.last->next;
            grammar->symlist.last->next = sym;
            grammar->symlist.last = sym;
            break;
    }
}

/*
 *  Destructor for grammar_t
 */
void
grammar_free(grammar_t grammar)
{
    strhash_free(grammar->hash, destroy_symbol);
    free(grammar);
}

/*
 * Assign host type to a symbol.
 */
void
grammar_assign_type(grammar_t grammar, const char *name, const char *type)
{
    struct symbol **s = (struct symbol **) strhash_find(grammar->hash, name);
    if (! *s) {
        *s = calloc(1, sizeof(struct symbol));
        if (! *s)
            abort();
        (*s)->type = UNKNOWN;
        (*s)->name = strhash_key((void **)s);
    }
    if ((*s)->host_type) {
        print("error: reassiginig symbol type\n");
        return;
    }
    (*s)->host_type = strdup(type);
}

/*
 * Add a new nonterminal rule to the grammar while building it.
 */
void
grammar_nonterminal(grammar_t grammar,
                    const struct grammar_element *ls,
                    unsigned rsn, const struct grammar_element rs[],
                    const char *host_code)
{
    struct symbol **s = (struct symbol **) strhash_find(grammar->hash, ls->name);
    if (! *s) {
        *s = calloc(1, sizeof(struct symbol));
        if (! *s)
            abort();
        (*s)->type = NONTERMINAL;
        (*s)->name = strhash_key((void **)s);
        commit_symbol(grammar, *s);
    }
    if ((*s)->type == UNKNOWN) {
        (*s)->type = NONTERMINAL;
        commit_symbol(grammar, *s);
    }
    if ((*s)->type != NONTERMINAL) {
        print("error: symbol `%s' is not a nonterminal\n", ls->name);
        return;
    }

    struct rule *rule = calloc(1, sizeof_struct_rule(rsn));
    if (! rule)
        abort();
    rule->next = (*s)->nt.rules;
    rule->sym = (*s);
    (*s)->nt.rules = rule;
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
        rule->rs[i].sym.tmp.raw = strhash_key(strhash_find(grammar->hash, rs[i].name));
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
grammar_start_symbol(grammar_t grammar, const char *start)
{
    grammar->start.tmp.raw = strhash_key(strhash_find(grammar->hash, start));
}


static void
resolve_symbols(grammar_t grammar)
{
    for (struct symbol * sym = grammar->symlist.first; sym; sym = sym->next) {
        for (const struct rule * rule = sym->nt.rules; rule; rule = rule->next) {
            for (unsigned i = 0; i < rule->length; ++i) {
                struct symbol ** s = (struct symbol **) strhash_find(grammar->hash, rule->rs[i].sym.tmp.raw);
                if (! *s) {
                    *s = calloc(1, sizeof(struct symbol));
                    if (! *s)
                        abort();
                    (*s)->name = strhash_key((void **)s);
                }
                if ((*s)->type == UNKNOWN) {
                    (*s)->type = TERMINAL;
                    commit_symbol(grammar, *s);
                }
                ++((*s)->use_count);
                ((struct rule *) rule)->rs[i].sym.sym = *s;
            }
        }
    }
    if (grammar->start.tmp.raw) {
        struct symbol ** sym = (struct symbol **) strhash_find(grammar->hash, grammar->start.tmp.raw);
        if (! *sym)
            print("error: grammar has no symbol `%s\n'", grammar->start.tmp.raw);
        grammar->start.sym = *sym;
    }
}

static void
add_sentinel_rule(grammar_t grammar)
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
    (*start)->type = NONTERMINAL;
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
    rule->rs[0].sym.sym = grammar->start.sym;
    rule->rs[1].sym.sym = *eof;
    for (unsigned i = 0; i < rule->length; ++i)
        ++(((struct symbol*)rule->rs[i].sym.sym)->use_count);

    grammar->start.sym = *start;
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
            return "nonterminal";
    }
    return "BUG";
}

static void
count_symbols(grammar_t grammar)
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
                ++(grammar->n_nonterminals);
                break;
        }
    }
}

static void
determine_start(grammar_t grammar)
{
    for (struct symbol * sym = grammar->symlist.first; sym; sym = sym->next) {
        if (! sym->use_count) {
            if (! grammar->start.sym && sym->type == NONTERMINAL) {
                print("note: using `%s' as start symbol\n", sym->name);
                grammar->start.sym = sym;
            } else if (sym != grammar->start.sym && sym->type != UNKNOWN) {
                print("warning: unused %s symbol `%s'\n", strtype(sym), sym->name);
            }
        }
    }
}

static void
dump_grammar(grammar_t grammar)
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
                for (const struct rule * r = sym->nt.rules; r; r = r->next) {
                    print("// %i\n%s ::=", r->id, sym->name);
                    for (unsigned i = 0; i < r->length; ++i)
                        print(" %s", r->rs[i].sym.sym->name);
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
dump_symbols(grammar_t grammar)
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
grammar_complete(grammar_t grammar)
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

    find_nullable(grammar);
    find_first(grammar);
    find_follow(grammar);

    if (print_opt(P_NULLABLE))
        dump_nullable(grammar);
    if (print_opt(P_FIRST))
        dump_first(grammar);
    if (print_opt(P_FOLLOW))
        dump_follow(grammar);

    lr0_machine_t lr0m = lr0_build(grammar);
    slr_reduce_search(lr0m);
    lalr_reduce_search(lr0m);
    conflicts(lr0m);
//    FILE *fd = fopen("out.C", "w");
//    codgen_c(fd, lr0m);
//    fclose(fd);
    lr0_free(lr0m);
}

