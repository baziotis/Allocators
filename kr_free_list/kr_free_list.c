#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "kr_free_list.h"

global_variable header_t base;             // empty list to get started with
global_variable header_t *freep = NULL;    // start of free list

// NOTE(stefanos): For alignment purposes, allocations are always
// a multiple of a 'unit', where a unit is just the size of the header_t .

#define NUNITS_MIN 1024

// morecore: A core is a big chunk of contiguous memory. When we run out in
// one, we ask for another big chunk and initialize it.
internal header_t *morecore(size_t nunits) {
    // An sbrk() call is expensive, so we only want to call it every once
    // in a while and so, we allocate large contiguous chunks
    if(nunits < NUNITS_MIN)
        nunits = NUNITS_MIN;

    size_t nbytes = nunits * sizeof(header_t);
    // NOTE - IMPORTANT(stefanos): sbrk() is not in the C standard.
    // We might at some point take the time to make this more platfrom-independent.
    uint8_t *p = sbrk(nbytes);
    if((void *) p == (void *) -1)  // out of memory
        return NULL;
    header_t *up = (header_t *) p;
    up->size = nunits;
    my_free(up + 1);
    return freep;
}

internal size_t count_units(size_t nbytes) {
    size_t unit_size = sizeof(header_t);
    size_t nunits = ((nbytes-1) + unit_size) / unit_size + 1;
    return nunits;
}

// my_alloc: general-purpose storage allocator
void *my_alloc(size_t nbytes) {
    if(nbytes == 0)
        return NULL;
    header_t *prevp, *currp;
    // number of units including the header
    size_t nunits = count_units(nbytes);
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

internal header_t *locate_in_free_list(header_t *bp) {
    // We want to stop when bp is between currp and currp->next without
    // any other free block interleaving. So:
    // currp / maybe some other allocated blocks / bp / maybe some other allocated blocks / currp->next
    // That is because then we can easily connect them as bp->next = currp->next, currp->next = bp;
    // It's important to think and understand why if we have any other other interleaving blocks, that can't
    // be done. Something like: currp / other free block / allocated data / bp / allocated data / currp->next

    // All this is covered by the stop_condition. But, because the list is cyclic, we can have bp be essentially
    // between currp and currp->next logically but not literally because the list wrapped. For example:
    // allocated data / bp / allocated data / currp->next / allocated data / currp
    // That is we what we call 'bp is at the start of the list' because it really should
    // be put at the start of the free list (Remember that the free list is composed only from the
    // free blocks).
    // In the same manner, we can have it at the end:
    // allocated data / currp->next / allocated data / currp / allocated data / bp / allocated data
    // NOTE: To read those lines, start from currp and move cyclically.
    header_t *currp;

#define stop_condition (currp < bp && bp < currp->next)
    for(currp = freep; !stop_condition; currp = currp->next) {
        // NOTE(stefanos): It's important that to get to this part of the code,
        // we have passed the stop condition.
        int wrapped = currp >= currp->next;
        int at_start = bp > currp;
        int at_end = bp < currp->next;
        if(wrapped && (at_start || at_end))
            break;
    }
    return currp;
}

// my_free: free the data pointed by ap, i.e.
// put the block pointed by 'ap' into the free list.
void my_free(void *ap) {
    header_t *currp, *bp;
    bp = ((header_t *)ap) - 1;  // get the block header

    currp = locate_in_free_list(bp);

    // We have 4 cases, because there's the possibility that we can coalesce
    // with an already existing free block. For example, if bp is exactly before
    // the currp->next free block. If that's the case, bp + bp->size will land us
    // to currp->next

    // NOTE - IMPORTANT(stefanos): Be very careful as these test cases are very
    // artistically constructed.
    // For example, a point of confusion might arise if bp is between currp and currp->next
    // without being adjacent to either. Then, it might make sense that the first
    // else case should be:
    // bp->next = currp->next; AND ALSO currp->next = bp->next;
    // _But_, note that this second statement will be executed because of the 2nd
    // else case (in the second if-else block).

    if(bp + bp->size == currp->next) {    // coalesce with right adjacent free block
        bp->size += currp->next->size;
        bp->next = currp->next->next;
    } else {    // handle it as just an interleaving block between currp and currp->next
        bp->next = currp->next;
    }

    // Simlarly, it might be exactly after the currp free block
    if(currp + currp->size == bp) {
        currp->size += bp->size;
        currp->next = bp->next;
    } else {    // handle it as just an interleaving block between currp and currp->next
        currp->next = bp;
    }

    // Note that here, currp->next is pretty much the newly allocated block (or a coalesced)
    // and setting freep = currp just means that the next time my_alloc() runs (or the)
    // next iteration of the basic loop in my_alloc() after the morecore() call) will
    // firstly test the new block. This makes sense as this new block has as many
    // chances as the rest of the free blocks to fit, if not more because this
    // my_free() call might have occured from a morecore() call, in which case,
    // no other block could fit.
    freep = currp;
}

internal header_t *shift_right(header_t *p, size_t size, size_t shamnt) {
    assert(shamnt > 0);
    size_t i, j;
    for(i = size-1 + shamnt, j = size-1; i > shamnt; --i, --j) {
        memcpy(&p[i], &p[j], sizeof(header_t));
    }
    return &p[i];
}

// my_realloc: resize the memory block pointed to by ptr
void *my_realloc(void *ap, size_t new_nbytes) {
    header_t *currp, *bp;
    bp = ((header_t *)ap) - 1;  // get the block header

    if(new_nbytes == 0) {
        my_free(ap);
        return NULL;
    }
    size_t new_size = count_units(new_nbytes);
    if(new_size == bp->size) {
        return ap;
    } else if(new_size < bp->size) {
        size_t diff =  bp->size - new_size;
        currp = locate_in_free_list(bp);
        int is_there_right_adjacent_free_block = (bp + bp->size == currp->next);
        int is_there_left_adjacent_free_block = (currp + currp->size == bp);
        if(!is_there_right_adjacent_free_block && is_there_left_adjacent_free_block) {
            // Do compaction.
            // That is, move the part of the block to be kept to the right,
            // and merge the remaining left part with the left free block.
            header_t *new_bp;
            new_bp = shift_right(bp, new_size, diff);
            new_bp->size = new_size;
            currp->size += diff;
            return ((void *)(new_bp+1));
        } else {
            bp->size = new_size;
            header_t *temp_bp = bp + new_size;
            temp_bp->size = diff;
            my_free(temp_bp+1);
            return ap;
        }
    } else {
        // TODO(stefanos): Test if there is a left adjacent free block so that
        // the new_size <= (old_block_size + left_adjacent_block_size)
        // If so, shift to the left to the start of this block and put the remaining
        // into the free list.
        header_t *ret = (header_t *) my_alloc((new_size-1) * sizeof(header_t));
        header_t *new_bp = ret - 1;
        if(new_bp == NULL) {
            my_free(bp+1);
            return NULL;
        }
        memcpy(new_bp, bp, bp->size * sizeof(header_t));
        new_bp->size = new_size;
        my_free(bp+1);
        return ((void *)(new_bp+1));
    }
}
