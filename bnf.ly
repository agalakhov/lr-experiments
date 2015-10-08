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

rule ::= PRAGMA_NAME LCURL text(T) RCURL.
{
    grammar_name(grammar, T);
    rcunref((void*)T);
}

rule ::= PRAGMA_INCLUDE LCURL text(T) RCURL.
{
    grammar_add_host_code(grammar, T);
    rcunref((void*)T);
}

rule ::= PRAGMA_START_SYMBOL WORD(S).
{
    grammar_start_symbol(grammar, S);
    rcunref((void*)S);
}

rule ::= PRAGMA_EXTRA_ARGUMENT LCURL text(T) RCURL.
{
    grammar_set_extra_argument(grammar, T);
    rcunref((void*)T);
}

rule ::= PRAGMA_TOKEN_TYPE LCURL text(T) RCURL.
{
    grammar_assign_terminal_type(grammar, T);
    rcunref((void*)T);
}

rule ::= PRAGMA_TYPE WORD(N) LCURL text(T) RCURL.
{
    grammar_assign_type(grammar, N, T);
    rcunref((void*)N);
    rcunref((void*)T);
}

rule ::= PRAGMA_TOKEN_DESTRUCTOR LCURL text(T) RCURL.
{
    grammar_assign_terminal_destructor(grammar, T);
    rcunref((void*)T);
}

rule ::= PRAGMA_DESTRUCTOR WORD(N) LCURL text(T) RCURL.
{
    grammar_assign_destructor(grammar, N, T);
    rcunref((void*)N);
    rcunref((void*)T);
}

rule ::= PRAGMA_FALLBACK WORD(L) word_list(R) DOT.
{
    size_t s = strarr_size(R);
    const char** r = strarr_data(R);
    for (size_t i = 0; i < s; ++i) {
      struct grammar_element ll = { .name = L, .label = "LL" };
      struct grammar_element rr = { .name = r[i], .label = "RR" };
      grammar_deduce_type(grammar, L, r[i]);
      grammar_nonterminal(grammar, &ll, 1, &rr, "LL = RR;");
    }
    rcunref((void*)L);
    strarr_unref(R);
}

%type word_list { strarr_t }
%destructor word_list { strarr_unref($$); }

word_list(X) ::= WORD(W).
{
  strarr_t arr = strarr_create(32);
  X = strarr_push(arr, W);
  rcunref((void*)W);
}

word_list(X) ::= word_list(R) WORD(W).
{
  X = strarr_push(R, W);
  rcunref((void*)W);
}


%fallback PRAGMA
    PRAGMA_SYNTAX_ERROR
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
