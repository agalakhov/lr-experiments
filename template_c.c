%%include

%%functions

%%defines

#include <stdlib.h>

static inline unsigned
__action(unsigned state, unsigned token)
{
    return __actions[state][token];
}

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
        struct __record *   base;
        unsigned size;
    }                   stack;
};

static inline void
__assert_stack(const struct __EXPORT(parser) *parser, unsigned count)
{
    if (stack->sp - stack->base < count) {
        abort(); /* BUG */
    }
    if (stack->sp - stack->base + 1 < stack->size + count) {
        // TODO reallocate stack
    }
}

static inline void
__pop(struct __EXPORT(parser) *parser, unsigned count, unsigned symbol)
{
    stack->sp[-count] = stack->sp[0];
    stack->sp -= count;
    stack->sp[0].state = __goto(stack->sp[-1].state, symbol);
    stack->sp += 1;
}

static inline void
__shift(struct __EXPORT(parser) *parser, unsigned token, __EXPORT(terminal_t) terminal)
{
    __assert_stack(stack, 0);
    stack->sp[0].state = __goto(stack->sp[-1].state, token);
    stack->sp[0].item.__terminal = terminal;
    stack->sp += 1;
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
        unsigned act = __action(stack->sp[-1].state, token);
        if (! act)
            break;
        __reduce(stack, act);
    }
    __shift(stack, token, terminal);
    return (state == 1);
}

