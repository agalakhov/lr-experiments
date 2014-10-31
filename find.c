#include "find.h"

#include "grammar_i.h"
#include "bitset.h"

#include "print.h"

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
find_nullable(grammar_t grammar)
{
    bool chg;
    do {
        chg = false;
        for (struct symbol * sym = grammar->symlist.first ; sym; sym = sym->next) {
            switch (sym->type) {
                case UNKNOWN:
                case TERMINAL:
                    break;
                case NONTERMINAL:
                    if (! sym->nullable) {
                        for (const struct rule *r = sym->nt.rules; r; r = r->next) {
                            bool rule_nullable = true;
                            for (unsigned i = 0; i < r->length; ++i) {
                                const struct symbol * rs = r->rs[i].sym.sym;
                                if (rs->nullable)
                                    continue;
                                rule_nullable = false;
                            }
                            if (rule_nullable) {
                                sym->nullable = true;
                                chg = true;
                            }
                        }
                    }
                    break;
            }
        }
    } while (chg);
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
