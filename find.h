#pragma once

#include "grammar.h"

void find_nullable(grammar_t grammar);
void find_first(grammar_t grammar);
void find_follow(grammar_t grammar);
