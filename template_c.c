%%include

%%functions

#include <stdbool.h>
#include <stdlib.h>

%%defines

%%actions

static inline unsigned
__action(unsigned state, unsigned token)
{
    return __actions[state][token];
}

%%gotos

static inline unsigned
__goto(unsigned state, unsigned token)
{
    return __gotos[state][token];
}

struct __record {
    unsigned state;
    union {
%%types
    } item;
};

struct __EXPORT(parser) {
    unsigned state;
    struct {
        struct __record *   sp;
        unsigned size;
        struct __record     base[];
    }                   stack;
};

static inline void
__assert_stack(const struct __EXPORT(parser) *parser, unsigned count)
{
    if (parser->stack.sp - parser->stack.base < count) {
        abort(); /* BUG */
    }
    if (parser->stack.sp - parser->stack.base + 1 < parser->stack.size + count) {
        // TODO reallocate stack
    }
}

static inline void
__pop(struct __EXPORT(parser) *parser, unsigned count, unsigned symbol)
{
    parser->stack.sp[-(signed)count].item = parser->stack.sp[0].item;
    parser->stack.sp -= count;
    parser->state = parser->stack.sp[0].state;
    parser->stack.sp += 1;
    parser->state = __goto(parser->state, symbol);
}

static inline void
__shift(struct __EXPORT(parser) *parser, unsigned token, __EXPORT(terminal_t) terminal)
{
    __assert_stack(parser, 0);
    parser->stack.sp[0].state = parser->state;
    parser->stack.sp[0].item.__terminal = terminal;
    parser->stack.sp += 1;
    parser->state = __goto(parser->state, token);
}

static inline void
__reduce(__EXTRA_ARGUMENT struct __EXPORT(parser) *parser, unsigned id)
{
    switch (id) {
%%reduce
    }
}

struct __EXPORT(parser) *
__EXPORT(parser_alloc)(void)
{
    struct __EXPORT(parser) *parser = calloc(1, sizeof(struct __EXPORT(parser)) + (16 << 10)); // FIXME
    if (! parser)
        return NULL;
    parser->stack.sp = parser->stack.base;
    parser->stack.size = 1024; // FIXME
    parser->state = 0;
    return parser;
}

void
__EXPORT(parser_free)(struct __EXPORT(parser) *parser)
{
    if (parser)
        free(parser);
}

bool
__EXPORT(parser_parse)(__EXTRA_ARGUMENT struct __EXPORT(parser) *parser, unsigned token, __EXPORT(terminal_t) terminal)
{
    while (true) {
        unsigned act = __action(parser->state, token);
        if (! act)
            break;
        __reduce(__EXTRA_ARGUMENT_VALUE parser, act);
    }
    __shift(parser, token, terminal);
    return (parser->state == 1);
}

