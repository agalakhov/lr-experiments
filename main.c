
#include "yy.h"
#include "grammar.h"
#include "print.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void *bnfparserAlloc(void *(*mallocProc)(size_t));
void bnfparser(void *parser, int token, const char *val, grammar_t grammar);
void bnfparserFree(void *parser, void (*freeProc)(void *));

struct theparser {
    struct parser pars;
    void *lemon;
    grammar_t grammar;
};

static void
do_parse(struct parser *pars, int tok, const char *val) {
    struct theparser *tp = (struct theparser *) pars;
    bnfparser(tp->lemon, tok, val, tp->grammar);
}

int
main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s filename.yy\n", argv[0]);
        return 1;
    }

    print_options("grammar,symbols,first,follow");

    struct theparser tp;
    tp.pars.parse = do_parse;
    tp.lemon = bnfparserAlloc(malloc);
    tp.grammar = grammar_alloc();
    int e = parse_file((struct parser *)&tp, argv[1]);
    grammar_complete(tp.grammar);
    grammar_free(tp.grammar);
    bnfparserFree(tp.lemon, free);
    if (e < 0) {
        fprintf(stderr, "ERROR: %s: %s\n", argv[1], strerror(-e));
        return 2;
    }

    return 0;
}
