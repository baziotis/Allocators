#ifndef ALLOCATORS_POOL_ALLOCATOR_H
#define ALLOCATORS_POOL_ALLOCATOR_H

typedef struct pool_allocator pool_allocator_t;

pool_allocator_t *allocate_pool(size_t total_chunks, size_t chunk_size);
void *pool_allocate(pool_allocator_t *allocator);
void pool_free(pool_allocator_t *allocator, void *ptr);

#endif //ALLOCATORS_POOL_ALLOCATOR_H
