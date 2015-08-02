%%include

%%functions

#include <stdlib.h>

union __record {
%%types
};

struct __stack {
    union __record *sp;
    union __record *base;
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
__pop(struct __stack *stack, unsigned count)
{
    stack->sp[-count] = stack->sp[0];
    stack->sp -= count;
    stack->sp += 1;
}

static void
__action (struct __stack *stack, unsigned id) {
    switch (id) {
%%action
    }
}