#ifndef ALLOCATORS_POOL_ALLOCATOR_H
#define ALLOCATORS_POOL_ALLOCATOR_H

#include <stdint.h>

typedef struct pool_allocator pool_allocator_t;

pool_allocator_t *pool_initialize(size_t total_chunks, size_t chunk_size);
void *pool_alloc(pool_allocator_t *allocator);
void pool_free(pool_allocator_t *allocator, void *ptr);

#endif //ALLOCATORS_POOL_ALLOCATOR_H
