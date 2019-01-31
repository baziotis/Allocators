#ifndef MEM_ARENA_H
#define MEM_ARENA_H

#include <stdint.h>

typedef struct {
	uint8_t *base;
	size_t size;
	size_t used;
} mem_arena_t;

void mem_arena_initialize(mem_arena_t *, void *, size_t);
void *__mem_arena_push_bytes(mem_arena_t *, size_t);

// NOTE(stefanos): It's important to think and understand why we use the following macros.
// What are the alternatives and how are they better or worse?

#define mem_arena_push_type(mem_arena, type) (type *) __mem_arena_push_bytes(mem_arena, sizeof(type))
#define mem_arena_push_array(mem_arena, count, type) (type *) __mem_arena_push_bytes(mem_arena, ((size_t)(count))*sizeof(type))

#endif
