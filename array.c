#include "array.h"

#include "rc.h"

#include <string.h>

struct array
{
    array_free_func_t   freefunc;
    size_t              elem_size;
    size_t              space;
    size_t              size;
    char                data[];
};
static inline size_t sizeof_struct_array(size_t size, size_t elem_size) {
    return sizeof(struct array) + size * elem_size;
}

static void
array_destroy(void *ptr)
{
    array_t array = ptr;
    if (array->freefunc) {
        for (size_t i = 0; i < array->size; ++i)
            array->freefunc(array->data + (i * array->elem_size));
    }
}

array_t
array_create(size_t reserve, size_t elem_size, array_free_func_t freefunc)
{
    array_t array = rcalloc(sizeof_struct_array(reserve, elem_size));
    if (! array)
        return NULL;
    array->freefunc = freefunc;
    array->elem_size = elem_size;
    array->space = reserve;
    array->size = 0;
    return array;
}

array_t
array_push(array_t array, void *elem)
{
    ++(array->size);
    if (array->size > array->space) {
        array->space *= 2;
        array = rcrealloc_free(array, sizeof_struct_array(array->space, array->elem_size), array_destroy);
        if (! array)
            return NULL;
    }
    memcpy(array->data + ((array->size - 1) * array->elem_size), elem, array->elem_size);
    return array;
}

void *
array_data(array_t array)
{
    return array->data;
}

size_t
array_size(array_t array)
{
    return array->size;
}

void
array_ref(array_t array)
{
    rcref(array);
}

void
array_unref(array_t array)
{
    rcunref_free(array, array_destroy);
}
