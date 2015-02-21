#include "find.h"

#include "grammar_i.h"
#include "bitset.h"

#include "print.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void
dump_bitset(grammar_t grammar, const bitset_t set)
{
    for (struct symbol * sym = grammar->symlist.first ; sym; sym = sym->next) {
        if (set_has(set, sym->id))
            print(" %s", sym->name);
    }
    print("\n");
}

void
dump_first(grammar_t grammar)
{
    print("-- First:\n");
    for (struct symbol * sym = grammar->symlist.first ; sym; sym = sym->next) {
        print("  %s :%s", sym->name, sym->nullable ? " (e)" : "");
        dump_bitset(grammar, sym->first);
    }
    print("--\n");
}

void
dump_follow(grammar_t grammar)
{
    print("-- Follow:\n");
    for (struct symbol * sym = grammar->symlist.first ; sym; sym = sym->next) {
        print("  %s :", sym->name);
        dump_bitset(grammar, sym->follow);
    }
    print("--\n");
}

void
dump_nullable(grammar_t grammar)
{
    print("-- Nullable:\n");
    for (const struct symbol * sym = grammar->symlist.first; sym; sym = sym->next) {
        print("  %s : %s\n", sym->name, sym->nullable ? "yes" : "no");
        if (sym->type != NONTERMINAL)
            continue;
        for (const struct rule * rule = sym->nt.rules; rule; rule = rule->next) {
            print("    ");
            for (unsigned i = 0; i < rule->length; ++i)
                print("%s ", rule->rs[i].sym.sym->name);
            print(". (%u)\n", rule->nnl);
        }
    }
}

static void
enqueue_rule_if_needed(const struct grammar * grammar, struct symbol * * queue, struct rule * relations[], struct rule * rule)
{
    struct symbol * sym = (struct symbol *) rule->sym;
    if (sym->nullable)
        return;
    if (rule->nnl == 0) {
        sym->nullable = true;
        sym->tmp.que_next = *queue;
        *queue = sym;
    } else if (rule->rs[rule->nnl - 1].sym.sym->type == NONTERMINAL) {
        unsigned i = rule->rs[rule->nnl - 1].sym.sym->id - grammar->n_terminals;
        rule->tmp.que_next = relations[i];
        relations[i] = rule;
    }
}

void
find_nullable(grammar_t grammar)
{
    struct symbol * queue = NULL;
    struct rule * relations[grammar->n_nonterminals];
    memset(relations, 0, sizeof(relations));
    for (struct symbol * sym = grammar->symlist.first; sym; sym = sym->next) {
        sym->nullable = false;
        if (sym->type != NONTERMINAL)
            continue;
        for (struct rule * rule = (struct rule *) sym->nt.rules; rule; rule = (struct rule *) rule->next) {
            rule->nnl = rule->length;
            enqueue_rule_if_needed(grammar, &queue, relations, rule);
        }
    }

    while (queue) {
        const unsigned i = queue->id - grammar->n_terminals;
        assert(queue->nullable);
        queue = queue->tmp.que_next;
        while (relations[i]) {
            struct rule * rule = relations[i];
            relations[i] = rule->tmp.que_next;
            assert(rule->nnl);
            do {
                --(rule->nnl);
            } while ((rule->nnl != 0) && rule->rs[rule->nnl - 1].sym.sym->nullable);
            enqueue_rule_if_needed(grammar, &queue, relations, rule);
        }
    }
}

void
find_first(grammar_t grammar)
{
    for (struct symbol * sym = grammar->symlist.first ; sym; sym = sym->next) {
        if (sym->first)
            set_free(sym->first);
        sym->first = set_alloc(grammar->n_terminals);
        if (! sym->first)
            abort();
    }
    bool chg;
    do {
        chg = false;
        for (struct symbol * sym = grammar->symlist.first ; sym; sym = sym->next) {
            switch (sym->type) {
                case UNKNOWN:
                    break;
                case TERMINAL:
                    /* Terminal's FIRST is the terminal itself */
                    set_add(sym->first, sym->id);
                    break;
                case NONTERMINAL:
                    {
                        for (const struct rule *r = sym->nt.rules; r; r = r->next) {
                            for (unsigned i = 0; i < r->length; ++i) {
                                const struct symbol * rs = r->rs[i].sym.sym;
                                chg = set_union(sym->first, rs->first) || chg;
                                if (! rs->nullable)
                                    break;
                            }
                        }
                    }
                    break;
            }
        }
    } while (chg);
}

void
find_follow(grammar_t grammar)
{
    for (struct symbol * sym = grammar->symlist.first ; sym; sym = sym->next) {
        if (sym->follow)
            set_free(sym->follow);
        sym->follow = set_alloc(grammar->n_terminals);
        if (! sym->follow)
            abort();
    }
    bool chg;
    do {
        chg = false;
        for (struct symbol * lsym = grammar->symlist.first ; lsym; lsym = lsym->next) {
            if (lsym->type != NONTERMINAL)
                continue;
            for (const struct rule *r = lsym->nt.rules; r; r = r->next) {
                if (r->length) {
                    for (unsigned i = r->length - 1; i > 0; --i) {
                        const struct symbol * rs1 = r->rs[i-1].sym.sym;
                        const struct symbol * rsi = r->rs[i].sym.sym;
                        chg = set_union(rs1->follow, rsi->first) || chg;
                        if (rsi->nullable)
                            chg = set_union(rs1->follow, rsi->follow) || chg;
                    }
                        chg = set_union(r->rs[r->length - 1].sym.sym->follow, lsym->follow) || chg;
                }
            }
        }
    } while (chg);
}
