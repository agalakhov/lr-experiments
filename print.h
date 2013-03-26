#pragma once

#include <stdbool.h>

enum print {
    P_GRAMMAR,
    P_SYMBOLS,
    P_FIRST,
    P_FOLLOW,
    P_LR0_KERNELS,
    P_LR0_CLOSURES,

    P_MAX_ /* END */
};

void print_options(const char *opts);

bool print_opt(enum print opt);

void print(const char *fmt, ...);

void printo(enum print opt, const char *fmt, ...);
