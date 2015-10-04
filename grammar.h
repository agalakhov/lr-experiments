#pragma once

typedef struct grammar * grammar_t;

struct grammar_element {
    const char * name;
    const char * label;
};

grammar_t grammar_alloc(void);
void grammar_free(grammar_t grammar);

void grammar_name(grammar_t grammar, const char *name);

void grammar_start_symbol(grammar_t grammar, const char *start);

void grammar_assign_terminal_type(grammar_t grammar, const char *type);
void grammar_assign_type(grammar_t grammar, const char *name, const char *type);
void grammar_assign_terminal_destructor(grammar_t grammar, const char *destructor_code);
void grammar_assign_destructor(grammar_t grammar, const char *name, const char *destructor_code);

void grammar_nonterminal(grammar_t grammar,
                         const struct grammar_element *ls,
                         unsigned rsn, const struct grammar_element rs[],
                         const char *host_code);

void grammar_add_host_code(grammar_t grammar, const char *host_code);

void grammar_complete(grammar_t grammar);
