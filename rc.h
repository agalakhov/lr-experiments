#pragma once

#include <stddef.h>

void *rcalloc(size_t size);
void rcref(void *ptr);
void rcunref(void *ptr);
void *rcrealloc(void *ptr, size_t size);

char *rcstrdup(const char *str);
char *rcstrndup(const char *str, size_t len);
