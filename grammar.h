#pragma once

typedef struct grammar * grammar_t;

grammar_t grammar_alloc(void);
void grammar_free(grammar_t grammar);

void grammar_nonterminal(grammar_t grammar,
                         const char *ls,
                         unsigned rsn, const char *rs[],
                         const char *host_code);

void grammar_complete(grammar_t grammar);
