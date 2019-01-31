#include <stdint.h>
#include <string.h>

#include "mem_arena.h"

// *******

// A memory arena is probably the simplest allocator. It is generally
// a big chunk of contiguous memory that is then used to do memory
// management manually. Here, we just push items one after the other
// in continuous places in this chunk. So, for example, if I push
// one int32 'a' and one double 'b', it will look like this:
// base
//    |----|--------|
//      a      b
// To the user, 'base' will be returned for 'a' and 'base' + 4 for 'b'.

// The interface contract is that the memory arena is not
// concerned with the memory allocation (and deallocation). Thus,
// 'base' is supposed to already contain valid memory address in which
// 'size' bytes can be used. 

// *******

// Initialize, zero memory
void mem_arena_initialize(mem_arena_t *arena, void *base, size_t size) {
	if(arena == NULL) return;
	if(base == NULL) return;
	arena->size = size;
	memset(base, 0, size);
	arena->base = (uint8_t *) base;
	arena->used = 0;
}

void *__mem_arena_push_bytes(mem_arena_t *arena, size_t size) {
	if(arena == NULL) return NULL;
	if(arena->base == NULL) return NULL;
	if(arena->used + size > arena->size)
		return NULL;
	void *ret = arena->base + arena->used;
	arena->used += size;
	return ret;
}
