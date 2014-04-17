%include {
# include "bnf.h"
# include "rc.h"
# include "strarr.h"
# include "grammar.h"
# include <assert.h>
# include <stdio.h>
# include <stdlib.h>
}

%name {bnfparser}

%extra_argument { grammar_t grammar }
%token_type { const char * }
%token_destructor { rcunref((void*)$$); }

%start_symbol grammar

grammar ::= rules.
rules ::= .
rules ::= rules rule.



rule ::= PRAGMA_INCLUDE LCURL text RCURL.

rule ::= PRAGMA_NAME LCURL text RCURL.

rule ::= PRAGMA_TYPE WORD LCURL text RCURL.

rule ::= PRAGMA_DESTRUCTOR WORD LCURL text RCURL.

rule ::= PRAGMA_FALLBACK WORD word_list DOT.
word_list ::= .
word_list ::= word_list WORD.

rule ::= PRAGMA_START_SYMBOL WORD.



%fallback PRAGMA
    PRAGMA_NAME PRAGMA_INCLUDE PRAGMA_TOKEN_TYPE PRAGMA_TOKEN_DESTRUCTOR
    PRAGMA_EXTRA_ARGUMENT PRAGMA_SYNTAX_ERROR
.
rule ::= PRAGMA.
rule ::= PRAGMA LCURL text RCURL.


rule ::= left(L) IS right(R) DOT action(A).
{
    grammar_nonterminal(grammar, L, strarr_size(R), strarr_data(R), A);
    rcunref((void*)L);
    strarr_unref(R);
    rcunref((void*)A);
}

%type left { const char * }
%destructor left { rcunref((void*)$$); }

left(X) ::= WORD(W) specifier(S).
{
    X = W;
    rcunref((void*)S);
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

rightlist(X) ::= rightlist(R) WORD(W) specifier(S).
{
    X = strarr_push(R, W);
    rcunref((void*)W);
    rcunref((void*)S);
}

%type specifier { const char * }
%destructor specifier { rcunref((void*)$$); }

specifier(X) ::= .
{
    X = NULL;
}

specifier(X) ::= LPAREN WORD(W) RPAREN.
{
    X = W;
}

%type action { const char * }

%destructor action { rcunref((void*)$$); }

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

%syntax_error {
    printf("Syntax error\n");
    abort();
}
