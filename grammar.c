#include "grammar.h"

#include "lr0.h"
#include "grammar_i.h"
#include "strhash.h"
#include "bitset.h"

#include <stdio.h>
#include <stdlib.h>

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
}

static void
destroy_nonterminal(struct nonterminal *nt)
{
    struct rule *r = nt->rules;
    while (r) {
        struct rule *n = r->next;
        free(r);
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
        case TERMINAL:
            destroy_terminal(&sym->s.t);
            break;
        case NONTERMINAL:
            destroy_nonterminal(&sym->s.nt);
            break;
    }
    if (sym->first)
        set_free(sym->first);
    if (sym->follow)
        set_free(sym->follow);
    free(sym);
}

static void
commit_symbol(grammar_t grammar, struct symbol * sym)
{
    if (! grammar->symlist.first) {
        sym->next = NULL;
        grammar->symlist.first = sym;
        grammar->symlist.last = sym;
        return;
    }
    switch (sym->type) {
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
 * Add a new nonterminal rule to the grammar while building it.
 */
void
grammar_nonterminal(grammar_t grammar,
                    const char *ls,
                    unsigned rsn, const char *rs[])
{
    struct symbol **s = (struct symbol **) strhash_find(grammar->hash, ls);
    if (! *s) {
        *s = calloc(1, sizeof(struct symbol));
        if (! *s)
            abort();
        (*s)->type = NONTERMINAL;
        (*s)->name = strhash_key((void **)s);
        (*s)->s.nt.nullable = false;
        commit_symbol(grammar, *s);
    }

    struct rule *rule = calloc(1, sizeof_struct_rule(rsn));
    if (! rule)
        abort();
    rule->next = (*s)->s.nt.rules;
    rule->sym = (*s);
    (*s)->s.nt.rules = rule;
    rule->id = ++(grammar->n_rules);

    rule->length = rsn;
    for (unsigned i = 0; i < rsn; ++i)
        rule->rs[i].raw = strhash_key(strhash_find(grammar->hash, rs[i]));
}


static void
resolve_symbols(grammar_t grammar)
{
    for (struct symbol * sym = grammar->symlist.first; sym; sym = sym->next) {
        for (struct rule *rule = sym->s.nt.rules; rule; rule = rule->next) {
            for (unsigned i = 0; i < rule->length; ++i) {
                struct symbol ** s = (struct symbol **) strhash_find(grammar->hash, rule->rs[i].raw);
                if (! *s) {
                    *s = calloc(1, sizeof(struct symbol));
                    if (! *s)
                        abort();
                    (*s)->type = TERMINAL;
                    (*s)->name = rule->rs[i].raw;
                    commit_symbol(grammar, *s);
                }
                ++((*s)->use_count);
                rule->rs[i].sym = *s;
            }
        }
    }
}

static const char *
strtype(const struct symbol * sym)
{
    switch (sym->type) {
        case TERMINAL:
            return "terminal";
        case NONTERMINAL:
            return "nonterminal";
    }
}

static void
count_symbols(grammar_t grammar)
{
    unsigned id = 1;
    for (struct symbol * sym = grammar->symlist.first; sym; sym = sym->next, ++id) {
        sym->id = id;
        switch (sym->type) {
            case TERMINAL:
                ++(grammar->n_terminals);
                break;
            case NONTERMINAL:
                ++(grammar->n_nonterminals);
                break;
        }
        if (! sym->use_count) {
            if (! grammar->start.sym && sym->type == NONTERMINAL) {
                fprintf(stderr, "note: using `%s' as start symbol\n", sym->name);
                grammar->start.sym = sym;
            } else {
                fprintf(stderr, "warning: unused %s symbol `%s'\n", strtype(sym), sym->name);
            }
        }
    }
}


static void
dump_grammar(grammar_t grammar)
{
    for (struct symbol * sym = grammar->symlist.first; sym; sym = sym->next) {
        switch (sym->type) {
            case TERMINAL:
                printf("%%terminal %s. // %i\n", sym->name, sym->id);
                break;
            case NONTERMINAL:
                for (struct rule *r = sym->s.nt.rules; r; r = r->next) {
                    printf("%s ::=", sym->name);
                    for (unsigned i = 0; i < r->length; ++i)
                        printf(" %s", r->rs[i].sym->name);
                    printf(". // %i\n", sym->id);
                }
                break;
        }
    }
}

static void
find_nullable(grammar_t grammar)
{
    bool chg;
    do {
        chg = false;
        for (struct symbol * sym = grammar->symlist.first ; sym; sym = sym->next) {
            switch (sym->type) {
                case TERMINAL:
                    break;
                case NONTERMINAL:
                    if (! sym->s.nt.nullable) {
                        for (struct rule *r = sym->s.nt.rules; r; r = r->next) {
                            bool rule_nullable = true;
                            for (unsigned i = 0; i < r->length; ++i) {
                                const struct symbol * rs = r->rs[i].sym;
                                if ((rs->type == NONTERMINAL) && rs->s.nt.nullable)
                                    continue;
                                rule_nullable = false;
                            }
                            if (rule_nullable) {
                                sym->s.nt.nullable = true;
                                chg = true;
                            }
                        }
                    }
                    break;
            }
        }
    } while (chg);
}

static void
find_first(grammar_t grammar)
{
    for (struct symbol * sym = grammar->symlist.first ; sym; sym = sym->next) {
        if (sym->first)
            set_free(sym->first);
        sym->first = set_alloc(grammar->n_terminals + 1);
        if (! sym->first)
            abort();
    }
    bool chg;
    do {
        chg = false;
        for (struct symbol * sym = grammar->symlist.first ; sym; sym = sym->next) {
            switch (sym->type) {
                case TERMINAL:
                    /* Terminal's FIRST is the terminal itself */
                    set_add(sym->first, sym->id);
                    break;
                case NONTERMINAL:
                    {
                        for (struct rule *r = sym->s.nt.rules; r; r = r->next) {
                            for (unsigned i = 0; i < r->length; ++i) {
                                const struct symbol * rs = r->rs[i].sym;
                                chg = set_union(sym->first, rs->first) || chg;
                                if (rs->type == TERMINAL)
                                    break;
                                if ((rs->type == NONTERMINAL) && ! rs->s.nt.nullable)
                                    break;
                            }
                        }
                    }
                    break;
            }
        }
    } while (chg);
}

/*
 * Finish building grammar
 */
void
grammar_complete(grammar_t grammar)
{
    resolve_symbols(grammar);
    count_symbols(grammar);
    /* At this point we have complete symbol graph */

    dump_grammar(grammar);

    printf("===\n");
    printf("Terminals: %u\nNonterminals: %u\nRules: %u\n",
           grammar->n_terminals, grammar->n_nonterminals, grammar->n_rules);
    printf("===\n");

    find_nullable(grammar);
    find_first(grammar);

    build_lr0(grammar);

}

