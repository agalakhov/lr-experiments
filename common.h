#pragma once

#include <stddef.h>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

#define field_of(ptr, type, member) (*(type *)((char *)(ptr) + (member)))

void lsort(void ** list, ptrdiff_t nextoffset, signed (*compar) (const void *, const void *));
