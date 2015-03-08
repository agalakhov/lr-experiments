#pragma once

#include <stdbool.h>

enum print {
    P_GRAMMAR,
    P_SYMBOLS,
    P_NULLABLE,
    P_LR0_KERNELS,
    P_LR0_CLOSURES,
    P_LR0_GOTO,
    P_LR_REDUCE,

    P_MAX_ /* END */
};

void print_options(const char *opts);

bool print_opt(enum print opt);

void print(const char *fmt, ...);

void printo(enum print opt, const char *fmt, ...);
