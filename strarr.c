#include "strarr.h"

#include "rc.h"

#include <stddef.h>

struct strarr {
    size_t       space;
    size_t       size;
    const char * data[];
};

strarr_t
strarr_create(size_t reserve)
{
    strarr_t arr = (strarr_t)rcalloc(sizeof(struct strarr) + reserve * (sizeof(const char*)));
    if (! arr)
        return NULL;
    arr->space = reserve;
    arr->size = 0;
    return arr;
}

strarr_t
strarr_shrink(strarr_t arr)
{
    arr = (strarr_t)rcrealloc(arr, sizeof(struct strarr) + arr->size * (sizeof(const char*)));
    if (! arr)
        return NULL;
    arr->space = arr->size;
    return arr;
}

strarr_t
strarr_push(strarr_t arr, const char *str)
{
    ++(arr->size);
    if (arr->size > arr->space) {
        arr->space *= 2;
        arr = (strarr_t)rcrealloc(arr, sizeof(struct strarr) + arr->space * (sizeof(const char*)));
        if (! arr)
            return NULL;
    }
    rcref((void*)str);
    arr->data[arr->size - 1] = str;
    return arr;
}

const char **
strarr_data(strarr_t arr)
{
    return arr->data;
}

size_t
strarr_size(strarr_t arr)
{
    return arr->size;
}

void
strarr_ref(strarr_t arr)
{
    rcref(arr);
}

static void
strarr_destroy(void *ptr)
{
    strarr_t arr = (strarr_t) ptr;
    size_t i;
    for (i = 0; i < arr->size; ++i)
        rcunref((void*)arr->data[i]);
}

void
strarr_unref(strarr_t arr)
{
    rcunref_free(arr, strarr_destroy);
}

