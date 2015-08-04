#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef const char * terminal_t;
struct parser;
struct parser *parser_alloc(void);
bool parser_parse(struct parser *parser, unsigned token, terminal_t terminal);
void parser_free(struct parser *parser);

enum {
  NUMBER = 1,
  RPAREN = 2,
  LPAREN = 3,
  MUL = 4,
  DIV = 5,
  PLUS = 6,
  MINUS = 7
};


int
main()
{

    while (true) {
        char buf[1024];
        if (! fgets(buf, sizeof(buf), stdin))
            break;

        struct parser *parser = parser_alloc();

        char *pos = buf;
        bool done = false;
        while (! done) {
            unsigned id = 0;
            switch (*pos) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                case '.':
                    id = NUMBER;
                    break;
                case '+':
                    id = PLUS;
                    break;
                case '-':
                    id = MINUS;
                    break;
                case '*':
                    id = MUL;
                    break;
                case '/':
                    id = DIV;
                    break;
                case '(':
                    id = LPAREN;
                    break;
                case ')':
                    id = RPAREN;
                    break;
                case ' ':
                case '\t':
                    ++pos;
                    continue;
                case '\r':
                case '\n':
                    id = 0;
                    break;
                default:
                    {
                        fputs("Syntax error:\n", stderr);
                        fputs(buf, stderr);
                        for (; pos != buf; --pos)
                            fputs(" ", stderr);
                        fputs("^\n", stderr);
                        done = true;
                    }
                    break;
            }

            done = done || parser_parse(parser, id, pos);

            if (id != NUMBER)
                ++pos;
            else
                strtod(pos, &pos);
        }

        parser_free(parser);
    }

    return 0;
}
