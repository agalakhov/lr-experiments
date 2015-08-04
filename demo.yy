%include {
# include <stdio.h>
# include <stdlib.h>
}

%token_type { const char * }
%type program { double }
%type expr { double }
%type term { double }
%type fac { double }

program ::= expr(E).
{
	if (1) { printf("\033[1;33m%lf\033[0m\n", E); }
}

expr(Y) ::= expr(X) PLUS term(A).
{
	Y = X + A;
}

expr(Y) ::= expr(X) MINUS term(A).
{
	Y = X - A;
}

expr(Y) ::= PLUS term(A).
{
	Y = A;
}

expr(Y) ::= MINUS term(A).
{
	Y = - A;
}

expr(Y) ::= term(A).
{
	Y = A;
}

term(Y) ::= term(X) MUL fac(A).
{
	Y = X * A;
}

term(Y) ::= term(X) DIV fac(A).
{
	Y = X / A;
}

term(Y) ::= fac(A).
{
	Y = A;
}

fac(Y) ::= NUMBER(N).
{
	Y = strtod(N, NULL);
}

fac(Y) ::= LPAREN expr(X) RPAREN.
{
	Y = X;
}

