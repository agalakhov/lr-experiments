#include "nullable.h"

#include "grammar_i.h"
#include "bitset.h"

#include "print.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void
nullable_dump(grammar_t grammar)
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
nullable_find(grammar_t grammar)
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
