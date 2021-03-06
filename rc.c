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
rcunref_free(void *ptr, rc_free_func_t freefunc)
{
    if (ptr) {
        struct refc *s = TO_REFC(ptr);
        --(s->cnt);
        if (! s->cnt) {
            if (freefunc)
                freefunc(ptr);
            free(s);
        }
    }
}

void
rcunref(void *ptr)
{
    rcunref_free(ptr, NULL);
}

void *
rcrealloc_free(void *ptr, size_t size, rc_free_func_t freefunc)
{
    if (! ptr)
        return rcalloc(size);
    struct refc *s = TO_REFC(ptr);
    void *s1 = realloc(s, offsetof(struct refc, data) + size);
    if (! s1) {
        if (freefunc)
            freefunc(ptr);
        free(s);
        return NULL;
    }
    s = (struct refc *) s1;
    return &s->data;
}

void *
rcrealloc(void *ptr, size_t size)
{
    return rcrealloc_free(ptr, size, NULL);
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

char *rcstrconcat(const char* str1, const char* str2)
{
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    char *s = (char *)rcalloc(len1 + len2 + 1);
    if (! s)
        return NULL;
    memcpy(s, str1, len1);
    memcpy(s + len1, str2, len2);
    s[len1 + len2] = '\0';
    return s;
}
