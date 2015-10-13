
#include "yy.h"

#include "codgen.h"
#include "conflict.h"
#include "grammar.h"
#include "lalr.h"
#include "lr0.h"
#include "print.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void *bnfparserAlloc(void *(*mallocProc)(size_t));
void bnfparser(void *parser, int token, const char *val, struct grammar *grammar);
void bnfparserFree(void *parser, void (*freeProc)(void *));

struct theparser {
    struct parser pars;
    void *lemon;
    struct grammar *grammar;
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

    print_options("all");

    struct theparser tp;
    tp.pars.parse = do_parse;
    tp.lemon = bnfparserAlloc(malloc);
    tp.grammar = grammar_alloc();
    int e = parse_file((struct parser *)&tp, argv[1]);
    if (! e) {
        grammar_complete(tp.grammar);
        lr0_machine_t lr0m = lr0_build(tp.grammar);
        lalr_reduce_search(lr0m);
        lr0_print(lr0m);
        conflicts(lr0m);

        FILE *fd = fopen("out.C", "w");
        codgen_c(fd, lr0m);
        fclose(fd);
        fd = fopen("out.H", "w");
        codgen_c_h(fd, lr0m);
        fclose(fd);

        lr0_free(lr0m);
    }
    grammar_free(tp.grammar);
    bnfparserFree(tp.lemon, free);
    if (e < 0) {
        fprintf(stderr, "ERROR: %s: %s\n", argv[1], strerror(-e));
        return 2;
    }

    return 0;
}
