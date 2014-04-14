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
rule ::= PRAGMA LCURL text RCURL.

rule ::= WORD(W) IS right(R) DOT action(A).
{
    grammar_nonterminal(grammar, W, strarr_size(R), strarr_data(R), A);
    rcunref((void*)W);
    strarr_unref(R);
    rcunref((void*)A);
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

rightlist(X) ::= rightlist(R) WORD(W) specifier.
{
    X = strarr_push(R, W);
    rcunref((void*)W);
}

specifier ::= .
specifier ::= LPAREN WORD RPAREN.

%type action { const char * }

action(X) ::= .
{
    X = NULL;
}

action(X) ::= LCURL RCURL.
{
    X = rcstrdup("");
}

action(X) ::= LCURL text(T) RCURL.
{
    X = T;
}

%type text { const char * }
%destructor text { rcunref((void*)$$); }

text(X) ::= TEXT(T).
{
    X = T;
}

text(X) ::= text(Y) TEXT(T).
{
    X = rcstrconcat(Y, T);
    rcunref((void *)Y);
    rcunref((void *)T);
}

