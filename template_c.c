%%include

%%functions

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

struct __stack {
    struct __record *sp;
    struct __record *base;
    unsigned size;
};

static inline void
__assert_stack(const struct __stack *stack, unsigned count)
{
    if (stack->sp - stack->base < count) {
        abort(); /* BUG */
    }
    if (stack->sp - stack->base + 1 < stack->size + count) {
        // TODO reallocate stack
    }
}

static inline void
__pop(struct __stack *stack, unsigned count, unsigned symbol)
{
    stack->sp[-count] = stack->sp[0];
    stack->sp -= count;
    stack->sp[0].state = __goto(stack->sp[-1].state, symbol);
    stack->sp += 1;
}

static inline void
__shift(struct stack *stack, unsigned symbol, terminal_t terminal, unsigned state)
{
    __assert_stack(stack, 0);
    stack->sp[0].state = state;
    stack->sp[0].item.__terminal = terminal;
    stack->sp += 1;
}

static inline void
__reduce(struct __stack *stack, unsigned id)
{
    switch (id) {
%%reduce
    }
}

bool
parse(struct __stack *stack, unsigned token, terminal_t terminal)
{
    unsigned go = __goto(stack->sp[-1].state, token);

}

