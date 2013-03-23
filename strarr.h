#pragma once

#include <stddef.h>

typedef struct strarr *strarr_t;

strarr_t      strarr_create(size_t reserve);
strarr_t      strarr_shrink(strarr_t arr);

strarr_t      strarr_push(strarr_t arr, const char *str);
const char ** strarr_data(strarr_t arr);
size_t        strarr_size(strarr_t arr);

void          strarr_ref(strarr_t arr);
void          strarr_unref(strarr_t arr);
