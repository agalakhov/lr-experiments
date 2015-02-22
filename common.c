#include "common.h"

#include <stddef.h>

static void
do_lsort(void ** list, ptrdiff_t nextoffset, signed (*compar) (const void *, const void *), signed order)
{
    if (! list || ! *list || ! field_of(*list, void *, nextoffset))
        return;
    void *even = NULL;
    void *odd = NULL;
    unsigned i = 0;
    for (void *p = *list; p; ++i) {
        void **l = (i % 2 == 0) ? &even : &odd;
        void *s = p;
        p = field_of(p, void *, nextoffset);
        field_of(s, void *, nextoffset) = *l;
        *l = s;
    }
    do_lsort(&even, nextoffset, compar, -order);
    do_lsort(&odd, nextoffset, compar, -order);
    void *out = NULL;
    while (even || odd) {
        void **l;
        if (! even) {
            l = &odd;
        } else if (! odd) {
            l = &even;
        } else {
            l = (order * compar(even, odd) < 0) ? &odd : &even;
        }
        void *s = *l;
        *l = field_of(*l, void *, nextoffset);
        field_of(s, void *, nextoffset) = out;
        out = s;
    }
    *list = out;
}

void
lsort(void ** list, ptrdiff_t nextoffset, signed (*compar) (const void *, const void *))
{
    do_lsort(list, nextoffset, compar, 1);
}
