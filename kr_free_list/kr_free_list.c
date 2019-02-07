#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "kr_free_list.h"

#define __is_left_adjacent(left_pointer, pointer) (left_pointer + left_pointer->size == pointer)

#define __is_right_adjacent(right_pointer, pointer) (pointer + pointer->size == right_pointer)

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
    if((void *) p == (void *) -1)   // out of memory
        return NULL;
    header_t *up = (header_t *) p;
    up->size = nunits;
    fl_free(up + 1);
    return freep;
}

internal size_t count_units(size_t nbytes) {
    size_t unit_size = sizeof(header_t);
    size_t nunits = ((nbytes - 1) + unit_size) / unit_size + 1;
    return nunits;
}

// fl_alloc: general-purpose storage allocator
void *fl_alloc(size_t nbytes) {
    if(nbytes == 0)
        return NULL;
    header_t *prevp, *currp;
    // number of units including the header
    size_t nunits = count_units(nbytes);
    // TODO(stefanos): Immediately ask for memory on startup.
    if(freep == NULL) {   // list not initialized
        base.next = freep = &base;
        base.size = 0;
    }
    prevp = freep;
    currp = freep->next;
    while(true) {
        if(currp->size >= nunits) {     // found big enough block
            if(currp->size == nunits) {         // exact fit
                prevp->next = currp->next;
            } else if(currp->size > nunits) {   // allocate tail end
                currp->size -= nunits;
                currp += currp->size;
                currp->size = nunits;
            }
            freep = prevp;
            return ((void *) (currp + 1));
        }
        if(currp == freep) {     // list wrapped around, did not find space
            currp = morecore(nunits);
            if(currp == NULL) {    // no space left.
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

// TODO(stefanos): Make an internal free that takes
// the currp (to save the O(n) search in the free list that might
// have already been done).


// fl_free: free the data pointed by ap, i.e.
// put the block pointed by 'ap' into the free list.
void fl_free(void *ap) {
    header_t *currp, *bp;
    bp = ((header_t *) ap) - 1;  // get the block header

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

    if(bp + bp->size == currp->next) {     // coalesce with right adjacent free block
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
    // and setting freep = currp just means that the next time fl_alloc() runs (or the)
    // next iteration of the basic loop in fl_alloc() after the morecore() call) will
    // firstly test the new block. This makes sense as this new block has as many
    // chances as the rest of the free blocks to fit, if not more because this
    // fl_free() call might have occured from a morecore() call, in which case,
    // no other block could fit.
    freep = currp;
}

internal header_t *shift_right(header_t *p, size_t size, size_t shamnt) {
    assert(shamnt > 0);
    size_t i, j;
    for(i = size - 1 + shamnt, j = size - 1; i > shamnt; --i, --j) {
        memcpy(&p[i], &p[j], sizeof(header_t));
    }
    return &p[i];
}

// fl_realloc: resize the memory block pointed to by ptr
void *fl_realloc(void *ap, size_t new_nbytes) {
    header_t *currp, *bp;
    bp = ((header_t *) ap) - 1;  // get the block header

    if(new_nbytes == 0) {
        fl_free(ap);
        return NULL;
    }
    currp = locate_in_free_list(bp);
    size_t new_size = count_units(new_nbytes);
    if(new_size == bp->size) {
        return ap;
    } else if(new_size < bp->size) {
        size_t diff = bp->size - new_size;
        if(__is_right_adjacent(currp->next, bp) && __is_left_adjacent(currp, bp)) {
            // Copy all the bytes except the header of the current block to the start of the left block.
            // Create a new header and update the pointers.
            memmove(currp + 1, bp + 1, (bp->size - 1) * sizeof(header_t));
            header_t *new_header = currp + new_size;
            new_header->size = currp->size + currp->next->size + bp->size - new_size;
            new_header->next = currp->next->next;
            currp->next = new_header;
            currp->size = new_size;
            return currp + 1;
        } else if(__is_left_adjacent(currp, bp)) {
            // Do compaction.
            // That is, move the part of the block to be kept to the right,
            // and merge the remaining left part with the left free block.

            /* NOTE(george): Another way of moving the data to the right:
             * We could just advance the new_bp pointer to its final position
             * and then use memmove to copy the number of bytes we want.
             * Note that we MUST use memmove and not memcpy because the blocks of memory that we
             * are reading from and writing to, overlap. That means between reads and writes of the bytes
             * our data is going to be corrupted. For that reason we are using memmove which writes the data
             * in an intermediate buffer first and then copies it.
             * CODE:
             * header_t* new_bp = bp + diff;
             * memmove(new_bp, bp, bp->size * sizeof(header_t));
             */
            header_t *new_bp;
            new_bp = shift_right(bp, new_size, diff);
            new_bp->size = new_size;
            currp->size += diff;
            return ((void *) (new_bp + 1));
        } else if(__is_right_adjacent(currp->next, bp)) {
            // Shrink block (its end is more "left" now) and create a new free block.
            // Update the pointers
            header_t *new_header = bp + new_size;
            new_header->size = diff + currp->next->size;
            new_header->next = currp->next->next;
            currp->next = new_header;
            bp->size = new_size;
            return ap;
        } else {
            // Just create a new free block and shrink the current one. Update pointers.
            header_t *new_header = bp + new_size;
            new_header->size = diff;
            new_header->next = currp->next->next;
            currp->next = new_header;
            bp->size = new_size;
            return ap;
        }
    } else {
        size_t diff = new_size - bp->size;
        if(__is_left_adjacent(currp, bp) && __is_right_adjacent(currp->next, bp) &&
           diff <= (currp->size + currp->next->size)) {
            // Copy all the bytes except the header of the current block to the start of the left block.
            // If there is no space left from all the three blocks then the next free block is the one after
            // the right block. If there is space left, we create a new header and set the next free block to that
            // header
            memmove(currp + 1, bp + 1, (bp->size - 1) * sizeof(header_t));
            if(diff == (currp->size + currp->next->size)) {
                currp->next = currp->next->next;
            } else {
                header_t *new_header = currp + new_size;
                new_header->size = currp->size + currp->next->size + bp->size - new_size;
                new_header->next = currp->next->next;
                currp->next = new_header;
            }
            currp->size = new_size;
            return currp + 1;
        } else if(__is_left_adjacent(currp, bp) && diff <= currp->size) {
            // Move the block pointer "diff" units to the left. That's the new pointer.
            // Copy every byte that block pointer has in it and decrease the size of the left
            // adjacent free block
            header_t *new_bp = bp - diff;
            memmove(new_bp, bp, bp->size * sizeof(header_t));
            currp->size -= diff;
            return new_bp + 1;
        } else if(__is_right_adjacent(currp->next, bp) && diff <= currp->next->size) {
            // If we took all the free memory from the right adjacent block then the next free block
            // if the next free block after the one in the right
            // else we decrease the size of the right block, create a new header and set the next free block
            // to that header.
            if(diff == currp->next->size) {
                currp->next = currp->next->next;
            } else {
                header_t *new_header = currp->next + diff;
                new_header->next = currp->next->next;
                new_header->size = currp->next->size - diff;
                currp->next = new_header;
            }
            bp->size += diff;
            return ap;
        } else {
            // In that case we have neither left nor right free blocks. We just allocate a new block,
            // copy the current contents and we free the current block.
            header_t *ret = (header_t *) fl_alloc((new_size - 1) * sizeof(header_t));
            header_t *new_bp = ret - 1;
            if(new_bp == NULL) {
                fl_free(bp + 1);
                return NULL;
            }
            memcpy(new_bp, bp, bp->size * sizeof(header_t));
            new_bp->size = new_size;
            fl_free(bp + 1);
            return ((void *) (new_bp + 1));
        }
    }
}
