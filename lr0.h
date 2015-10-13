#pragma once

#include "grammar.h"

typedef struct lr0_machine * lr0_machine_t;

lr0_machine_t lr0_build(const struct grammar *grammar);
void lr0_free(lr0_machine_t lr0_machine);

void lr0_print(lr0_machine_t mach);
