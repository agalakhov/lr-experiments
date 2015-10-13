#pragma once

struct grammar;

struct grammar_element {
    const char * name;
    const char * label;
};


struct grammar * grammar_alloc(void);
void grammar_free(struct grammar *grammar);

void grammar_complete(struct grammar *grammar);

void grammar_name(struct grammar *grammar, const char *name);
void grammar_add_host_code(struct grammar *grammar, const char *host_code);
void grammar_set_extra_argument(struct grammar *grammar, const char *extra_argument);



void grammar_start_symbol(struct grammar *grammar, const char *start);

void grammar_assign_terminal_type(struct grammar *grammar, const char *type);
void grammar_assign_type(struct grammar *grammar, const char *name, const char *type);
void grammar_assign_terminal_destructor(struct grammar *grammar, const char *destructor_code);
void grammar_assign_destructor(struct grammar *grammar, const char *name, const char *destructor_code);

void grammar_deduce_type(struct grammar *grammar, const char *ls_name, const char *rs_name);

void grammar_nonterminal(struct grammar *grammar,
                         const struct grammar_element *ls,
                         unsigned rsn, const struct grammar_element rs[],
                         const char *host_code);


