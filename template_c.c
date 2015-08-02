%%include

%%functions

#include <stdlib.h>

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
    // TODO stack->sp[0].state = goto[ stack->sp[-1].state, symbol ]
    stack->sp += 1;
}

static void
__reduce (struct __stack *stack, unsigned id) {
    switch (id) {
%%reduce
    }
}
