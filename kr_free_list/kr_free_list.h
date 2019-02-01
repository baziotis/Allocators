#ifndef KR_FREE_LIST_H
#define KR_FREE_LIST_H

#include <unistd.h>

#include "../common_types.h"

typedef uint64_t force_align;

typedef union header {        // block header_t
    struct {
        union header *next;   // next free block
        size_t size;          // size of this block
    };
    force_align x;            // force alignment
} header_t;

void *my_alloc(size_t nbytes);
void my_free(void *ap);
void *my_realloc(void *ap, size_t new_size);

#endif
