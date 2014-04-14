#pragma once

typedef struct grammar * grammar_t;

struct grammar_rs {
    const char * name;
    const char * label;
};

grammar_t grammar_alloc(void);
void grammar_free(grammar_t grammar);

void grammar_nonterminal(grammar_t grammar,
                         const char *ls,
                         unsigned rsn, const char *rs[],
                         const char *host_code);

void grammar_complete(grammar_t grammar);
