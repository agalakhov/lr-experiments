%include {
# include "bnf.h"
# include "rc.h"
# include "strarr.h"
# include "grammar.h"
# include <assert.h>
}

%name {bnfparser}

%extra_argument { grammar_t grammar }
%token_type { const char * }
%token_destructor { rcunref((void*)$$); }

%start_symbol grammar

grammar ::= rules.
rules ::= .
rules ::= rules rule.

rule ::= PRAGMA.

rule ::= WORD(W) IS right(R) DOT.
{
    grammar_nonterminal(grammar, W, strarr_size(R), strarr_data(R));
    rcunref((void*)W);
    strarr_unref(R);
}

%type right { strarr_t }
%destructor right { rcunref($$); }

%type rightlist { strarr_t }
%destructor rightlist { rcunref($$); }

right(X) ::= rightlist(Y).
{
    X = strarr_shrink(Y);
}

rightlist(X) ::= .
{
    X = strarr_create(128);
}

rightlist(X) ::= rightlist(R) WORD(W).
{
    X = strarr_push(R, W);
    rcunref((void*)W);
}

