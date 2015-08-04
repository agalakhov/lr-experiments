%include {
# include "bnf.h"
# include "rc.h"
# include "array.h"
# include "strarr.h"
# include "grammar.h"
# include <assert.h>
# include <stdio.h>
# include <stdlib.h>

static void
grammar_element_free(void *ptr)
{
    struct grammar_element *elem = ptr;
    rcunref((void*)elem->name);
    if (elem->label)
        rcunref((void*)elem->label);
}

}

%name {bnfparser}

%extra_argument { grammar_t grammar }
%token_type { const char * }
%token_destructor { rcunref((void*)$$); }

%start_symbol grammar

grammar ::= rules.
rules ::= .
rules ::= rules rule.



rule ::= PRAGMA_NAME LCURL text RCURL.

rule ::= PRAGMA_TYPE WORD(N) LCURL text(T) RCURL.
{
    grammar_assign_type(grammar, N, T);
    rcunref((void*)N);
    rcunref((void*)T);
}

rule ::= PRAGMA_DESTRUCTOR WORD LCURL text RCURL.

rule ::= PRAGMA_FALLBACK WORD word_list DOT.
word_list ::= .
word_list ::= word_list WORD.

rule ::= PRAGMA_START_SYMBOL WORD(S).
{
    grammar_start_symbol(grammar, S);
    rcunref((void*)S);
}

rule ::= PRAGMA_TOKEN_TYPE LCURL text(T) RCURL.
{
    grammar_assign_terminal_type(grammar, T);
    rcunref((void*)T);
}

rule ::= PRAGMA_INCLUDE LCURL text(T) RCURL.
{
    grammar_add_host_code(grammar, T);
    rcunref((void*)T);
}

%fallback PRAGMA
    PRAGMA_NAME PRAGMA_TOKEN_DESTRUCTOR
    PRAGMA_EXTRA_ARGUMENT PRAGMA_SYNTAX_ERROR
.
rule ::= PRAGMA.
rule ::= PRAGMA LCURL text RCURL.

rule ::= left(L) IS right(R) DOT action(A).
{
    grammar_nonterminal(grammar, L, array_size(R), array_data(R), A);
    grammar_element_free((void*)L);
    rcunref((void*)L);
    array_unref(R);
    rcunref((void*)A);
}

%type left { const struct grammar_element * }
%destructor left {
  grammar_element_free((void*)$$);
  free((void*)$$);
}

left(X) ::= WORD(W) specifier(S).
{
    struct grammar_element * x = rcalloc(sizeof(struct grammar_element));
    x->name = W;
    x->label = S;
    X = x;
}

%type right { array_t }
%destructor right { array_unref($$); }

%type rightlist { array_t }
%destructor rightlist { array_unref($$); }

right(X) ::= rightlist(Y).
{
    X = Y;
}

rightlist(X) ::= .
{
    X = array_create(32, sizeof(struct grammar_element), grammar_element_free);
}

rightlist(X) ::= rightlist(R) WORD(W) specifier(S).
{
    struct grammar_element ge = { W, S };
    X = array_push(R, &ge);
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
