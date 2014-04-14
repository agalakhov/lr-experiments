#pragma once

#include <stddef.h>

typedef void (*rc_free_func_t)(void *);

void *rcalloc(size_t size);
void rcref(void *ptr);
void rcunref(void *ptr);
void rcunref_free(void *ptr, rc_free_func_t freefunc);
void *rcrealloc(void *ptr, size_t size);

char *rcstrdup(const char *str);
char *rcstrndup(const char *str, size_t len);
char *rcstrconcat(const char* str1, const char* str2);
