%include {
# include <stdio.h>
# include <stdlib.h>
}

%token_type { double }
%type program { double }
%type expr { double }
%type term { double }
%type fac { double }

program ::= expr(E).
{
	if (1) { printf("%lf\n", E); }
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
	Y = atof(N);
}

fac(Y) ::= LPAREN expr(X) RPAREN.
{
	Y = X;
}

