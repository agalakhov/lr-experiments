#pragma once

#include <stdio.h>

typedef struct lr0_machine * lr0_machine_t;

void codgen_c(FILE *fd, lr0_machine_t machine);
void codgen_c_h(FILE *fd, lr0_machine_t machine);
