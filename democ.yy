program ::= expr(E).
{
	if (1) { printf("%lf\n", E); }
}

expr(E) ::= expr(X) PLUS expr(Y).
{
	E = X +++ Y;
}

expr(E) ::= expr(X) PLUS expr(Y).
{
	E = X + Y;
}

expr(E) ::= expr(X) MINUS expr(Y).
{
	E = X - Y;
}

expr(E) ::= expr(X) MUL expr(Y).
{
	E = X * Y;
}

expr(E) ::= expr(X) DIV expr(Y).
{
	E = X / Y;
}

expr(E) ::= PLUS expr(A).
{
	E = A;
}

expr(E) ::= MINUS expr(A).
{
	E = - A;
}

expr(E) ::= NUMBER(N).
{
	E = atof(N).
}

expr(E) ::= LPAREN expr(X) RPAREN.
{
	E = X;
}

