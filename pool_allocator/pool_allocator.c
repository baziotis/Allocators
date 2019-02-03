#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include "pool_allocator.h"

typedef struct header {
    struct header *next;
} header_t;

typedef struct pool_allocator {
    header_t *head;
} pool_allocator_t;

pool_allocator_t *pool_initialize(size_t total_chunks, size_t chunk_size) {
    size_t nbytes = total_chunks * (sizeof(header_t) + chunk_size) + sizeof(pool_allocator_t);
    pool_allocator_t *pool = sbrk(nbytes);
    if (pool == (void *) -1) return NULL;
    pool->head = ((void *) pool) + sizeof(pool_allocator_t);
    header_t *header;
    size_t i;
    for (i = 0U, header = pool->head; i != total_chunks; ++i, header = header->next) {
        header->next = ((void *) header) + chunk_size + sizeof(header_t);
    }
    header->next = NULL;
    return pool;
}

void *pool_alloc(pool_allocator_t *allocator) {
    if (allocator == NULL || allocator->head == NULL) return NULL;
    void *chunk = ((void *) allocator->head) + sizeof(header_t);
    allocator->head = allocator->head->next;
    return chunk;
}

void pool_free(pool_allocator_t *allocator, void *ptr) {
    if (allocator == NULL || ptr == NULL) return;
    header_t *old_head = allocator->head;
    uint8_t *byte = (uint8_t *) ptr;
    allocator->head = (void *) (byte - sizeof(header_t));
    allocator->head->next = old_head;
}
