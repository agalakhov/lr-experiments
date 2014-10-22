#pragma once

#include <stddef.h>

typedef struct array * array_t;

typedef void (*array_free_func_t)(void*);

array_t array_create(size_t reserve, size_t elem_size, array_free_func_t freefunc);

array_t array_push(array_t array, void *elem);
void *  array_data(array_t array);
size_t  array_size(array_t array);

void    array_ref(array_t array);
void    array_unref(array_t array);
