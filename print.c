#include "print.h"

#include "common.h"
#include "bitset.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

struct propt {
    const char * const  name;
    const enum print    flag;
};

static const struct propt propts[] = {
    { "grammar",        P_GRAMMAR },
    { "symbols",        P_SYMBOLS },
    { "first",          P_FIRST },
    { "follow",         P_FOLLOW },
    { "lr0-kernels",    P_LR0_KERNELS },
    { "lr0-closures",   P_LR0_CLOSURES },
    { "lr0-goto",       P_LR0_GOTO },
    { "lr-reduce",      P_LR_REDUCE },
};

/* Global variable */
static bitset_t options = NULL;

bool
print_opt(enum print opt)
{
    return set_has(options, opt);
}

static void
do_print(const char *fmt, va_list ap)
{
    vprintf(fmt, ap);
}

void
print(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    do_print(fmt, ap);
    va_end(ap);
}

void
printo(enum print opt, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if (print_opt(opt))
        do_print(fmt, ap);
    va_end(ap);
}

static void
print_options_free(void)
{
    set_free(options);
}

void
print_options(const char *opts)
{
    if (options)
        set_free(options);
    options = set_alloc(P_MAX_);
    if (! options)
        abort();
    atexit(print_options_free);

    const char *end;
    while (*opts) {
        end = strchr(opts, ',');
        if (! end)
            end = opts + strlen(opts);
        for (unsigned i = 0; i < ARRAY_SIZE(propts); ++i) {
            if (! strncmp(propts[i].name, opts, end - opts))
                set_add(options, propts[i].flag);
        }
        opts = end;
        if (*opts)
            ++opts; /* skip the ',' char */
    }
}
