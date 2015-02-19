#include "find.h"

#include "grammar_i.h"
#include "bitset.h"

#include "print.h"

#include <string.h>
#include <stdlib.h>

static void
dump_bitset(grammar_t grammar, const bitset_t set)
{
    if (set_has(set, 0))
        print(" $");
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

void
find_nullable(grammar_t grammar)
{
    struct symbol * null_queue = NULL;
    struct rule * relations[grammar->n_nonterminals];
    memset(relations, 0, sizeof(relations));
    for (struct symbol * sym = grammar->symlist.first; sym; sym = sym->next) {
        if (sym->type != NONTERMINAL) {
            sym->nullable = false;
            continue;
        }
        sym->nullable = false;
        for (struct rule * rule = (struct rule *) sym->nt.rules; rule; rule = (struct rule *) rule->next) {
            rule->nnl = rule->length;
            if ((rule->nnl == 0) && ! sym->nullable) {
                sym->nullable = true;
                sym->tmp.que_next = null_queue;
                null_queue = sym;
            } else if (rule->rs[rule->nnl - 1].sym.sym->type == NONTERMINAL) {
                unsigned i = rule->rs[rule->nnl - 1].sym.sym->id - grammar->n_terminals - 1;
                rule->tmp.que_next = relations[i];
                relations[i] = rule;
            }
        }
    }

    while (null_queue) {
        const unsigned i = null_queue->id - grammar->n_terminals - 1;
        for (struct rule * rule = relations[i]; rule; rule = rule->tmp.que_next) {
            --(rule->nnl);
            while (rule->nnl != 0) {
                if (! rule->rs[rule->nnl].sym.sym->nullable)
                    break;
                --(rule->nnl);
            }
            if ((rule->nnl != 0) && ! rule->sym->nullable) {
                struct symbol * sym = (struct symbol *) rule->sym;
                sym->nullable = true;
                sym->tmp.que_next = null_queue;
                null_queue = sym;
            } else if (rule->rs[rule->nnl - 1].sym.sym->type == NONTERMINAL) {
                unsigned j = rule->rs[rule->nnl - 1].sym.sym->id - grammar->n_terminals - 1;
                rule->tmp.que_next = relations[j];
                relations[j] = rule;
            }
        }
        null_queue = null_queue->tmp.que_next;
    }
    dump_nullable(grammar);
}

void
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
        sym->follow = set_alloc(grammar->n_terminals + 1);
        if (! sym->follow)
            abort();
    }
    set_add(grammar->start.sym->follow, 0);
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
