#include "yy.h"
#include "rc.h"
#include "bnf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

%%{
    machine yylex;
    alphtype char;

    word = [A-Za-z_][A-Za-z0-9_]*;

    text := |*
                '{'      => { ++curllevel; token(TEXT); };
                '}'      => { --curllevel; if (! curllevel) { fhold; fret; } token(TEXT); };
                [^{}]+   => { token(TEXT); };
            *|;

    main := |*

                space;
                word     => { token(WORD); };
                '%' word => { token(PRAGMA); };
                '::='    => { token(IS); };
                '.'      => { token(DOT); };
                '('      => { token(LPAREN); };
                ')'      => { token(RPAREN); };
                '{'      => { token(LCURL); ++curllevel; fcall text; };
                '}'      => { token(RCURL); };

            *|;

}%%

%% write data;

#define token(x) emit(x, ts, te-ts)
#define emit(tok, t, l) pars->parse(pars, tok, rcstrndup(t, l))

static void
parse_machine(struct parser *pars, const char *s, size_t len)
{
    int cs;
    const char *p = s;
    const char *pe = s + len;
    const char *eof = pe;
    const char *ts, *te;
    int act;
    int top;
    int stack[128];

    int curllevel = 0;

    %% write init;
    %% write exec;

    pars->parse(pars, 0, NULL);
}

int
parse_file(struct parser *pars, const char *filename)
{
    const size_t bufsize = 1<<20;

    int ret = 0;

    char *buf = malloc(bufsize);
    if (buf) do {

        FILE *fd = fopen(filename, "r");
        if (fd) do {

            ssize_t s = fread(buf, 1, bufsize, fd);
            if (s < 0) {
                ret = -errno;
                break;
            }

            parse_machine(pars, buf, s);

            fclose(fd);
        } while (0);
        else ret = -errno;

        free(buf);
    } while (0);
    else ret = -errno;

    return ret;
}
