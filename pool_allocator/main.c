#include <stdio.h>
#include "pool_allocator.h"

#define CHUNK_N 10

int main() {
    pool_allocator_t *pool = allocate_pool(CHUNK_N, sizeof(int));
    int *array_of_ten = pool_allocate(pool);
    printf("Pointer = %p\n", array_of_ten);
    int *array_of_ten2 = pool_allocate(pool);
    printf("Pointer2 = %p\n", array_of_ten2);
    printf("Pointer2 - Pointer (should be 12) %zu\n", ((void*)array_of_ten2) - ((void*)array_of_ten));
    for (size_t i = 0U; i != CHUNK_N; ++i) {
        array_of_ten[i] = (int) i;
    }
    for (size_t i = 0U; i != CHUNK_N; ++i) {
        printf("Array of ten [%zu] = %d\n", i, array_of_ten[i]);
    }
    pool_free(pool, array_of_ten2);
    array_of_ten = pool_allocate(pool);
    printf("array of ten = %p\n", array_of_ten);
    return 0;
}