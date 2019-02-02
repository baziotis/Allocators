#include <stdio.h>
#include "kr_free_list.h"

int main(void) {

    // Create on purpose compaction case in realloc.

    int *a = (int *) my_alloc(10 * sizeof(int));
    printf("a: %p\n", a);
    int *b = (int *) my_alloc(10 * sizeof(int));
    printf("b: %p\n", b);
    int *c = (int *) my_alloc(10 * sizeof(int));
    printf("c: %p\n", c);

    my_free(c);
    for(size_t i = 0; i != 10; ++i) {
        b[i] = i;
    }
    for(size_t i = 0; i != 10; ++i) {
        printf("%d\n", b[i]);
    }
    b = my_realloc(b, 3 * sizeof(int));
    printf("b: %p\n", b);
    for(size_t i = 0; i != 10; ++i) {
        printf("%d\n", b[i]);
    }

    return 0;
}
