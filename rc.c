#include "rc.h"

#include "common.h"

#include <stdlib.h>
#include <string.h>

#include <stdio.h>

struct refc {
    unsigned cnt;
    char     data[];
};

#define TO_REFC(x) container_of(x, struct refc, data)

void *
rcalloc(size_t size)
{
    struct refc *s = (struct refc *)calloc(1, offsetof(struct refc, data) + size);
    if (! s)
        return NULL;
    s->cnt = 1;
    return &s->data;
}

void
rcref(void *ptr)
{
    if (ptr) {
        struct refc *s = TO_REFC(ptr);
        ++(s->cnt);
    }
}

void
rcunref(void *ptr)
{
    if (ptr) {
        struct refc *s = TO_REFC(ptr);
        --(s->cnt);
        if (! s->cnt) {
            free(s);
        }
    }
}

void *
rcrealloc(void *ptr, size_t size)
{
    if (! ptr)
        return rcalloc(size);
    struct refc *s = TO_REFC(ptr);
    void *s1 = realloc(s, offsetof(struct refc, data) + size);
    if (! s1) {
        free(s);
        return NULL;
    }
    s = (struct refc *) s1;
    return &s->data;
}

char *
rcstrdup(const char *str)
{
    return rcstrndup(str, strlen(str));
}

char *
rcstrndup(const char *str, size_t len)
{
    char *s = (char *)rcalloc(len + 1);
    if (! s)
        return NULL;
    memcpy(s, str, len);
    s[len] = '\0';
    return s;
}
