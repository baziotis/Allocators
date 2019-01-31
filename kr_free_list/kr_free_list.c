
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "kr_free_list.h"

global_variable header_t base;             // empty list to get started with
global_variable header_t *freep = NULL;    // start of free list

// NOTE(stefanos): For alignment purposes, allocations are always
// a multiple of a 'unit', where a unit is just the size of the header_t .

internal header_t *morecore(size_t nunits) {
    return NULL;
}

// my_alloc: general-purpose storage allocator
void *my_alloc(size_t nbytes) {
    if(nbytes == 0)
        return NULL;
    header_t *prevp, *currp;
    // number of units including the header
    size_t nunits = ((nbytes-1) + sizeof(header_t)) / sizeof(header_t) + 1;
    if(freep == NULL) {  // list not initialized
        base.next = freep = prevp = &base;
        base.size = 0;
    }
    prevp = freep;
    currp = freep->next;
    while(true) {
        if(currp->size >= nunits) {    // found big enough block
            if(currp->size == nunits) {        // exact fit
                prevp->next = currp->next;
            } else if(currp->size > nunits) {  // allocate tail end
                currp->size -= nunits;
                currp += currp->size;
                currp->size = nunits;
            }
            freep = prevp;
            return ((void *)(currp+1));
        }
        if(currp == freep) {    // list wrapped around, did not find space
            currp = morecore(nunits);
            if(currp == NULL) {   // no space left.
                return NULL;
            }
        }

        prevp = currp;
        currp = currp->next;
    }
}

// NOTE(stefanos): To be continued...
