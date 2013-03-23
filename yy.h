#pragma once

struct parser {
    void (*parse)(struct parser *pars, int token, const char *val);
};

int parse_file(struct parser *pars, const char *filename);
