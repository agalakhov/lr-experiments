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
    unsigned            state;
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
    parser->stack.sp[-count] = parser->stack.sp[0];
    parser->stack.sp -= count;
    parser->stack.sp[0].state = __goto(parser->stack.sp[-1].state, symbol);
    parser->stack.sp += 1;
}

static inline void
__shift(struct __EXPORT(parser) *parser, unsigned token, __EXPORT(terminal_t) terminal)
{
    __assert_stack(parser, 0);
    parser->stack.sp[0].state = __goto(parser->stack.sp[-1].state, token);
    parser->stack.sp[0].item.__terminal = terminal;
    parser->stack.sp += 1;
}

static inline void
__reduce(struct __EXPORT(parser) *parser, unsigned id)
{
    switch (id) {
%%reduce
    }
}

bool
__EXPORT(parse)(struct __EXPORT(parser) *parser, unsigned token, __EXPORT(terminal_t) terminal)
{
    while (true) {
        unsigned act = __action(parser->stack.sp[-1].state, token);
        if (! act)
            break;
        __reduce(parser, act);
    }
    __shift(parser, token, terminal);
    return (parser->stack.sp[-1].state == 1);
}

