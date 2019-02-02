#include <stdio.h>
#include "kr_free_list.h"

// NOTE(stefanos): Crappy code!

global_variable size_t used;
global_variable size_t capacity;
global_variable int *arr;

void initialize() {
    used = 0;
    capacity = 1;
    arr = my_alloc(capacity * sizeof(int));
}

void add(int v) {
    if(used == capacity) {
        capacity *= 2;
        arr = my_realloc(arr, capacity * sizeof(int));
    }
    arr[used++] = v;
}

int main(void) {

    // TODO(stefanos): More testing.

    int *a = (int *) my_alloc(10 * sizeof(int));
    printf("a: %p\n", a);
    a[3] = 5;
    double *b = (double *) my_alloc(20 * sizeof(double));
    printf("b: %p\n", b);
    b[7] = 1.2;
    char *c = (char *) my_alloc(15 * sizeof(char));
    printf("c: %p\n", c);
    c[4] = 'c';
    my_free(b);
    double *d = (double *) my_alloc(20 * sizeof(double));
    // We should get the same address with b.
    printf("d: %p\n", d);
    initialize();
    printf("arr: %p\n", arr);
    for(int i = 0; i != 100; ++i) {
        add(i);
        printf("%d\n", arr[i]);
    }
    for(int i = 0; i != 100; ++i) {
        printf("%d\n", arr[i]);
    }
    printf("\n\n");
    arr = my_realloc(arr, 9000 * sizeof(int));
    for(int i = 300; i != 400; ++i) {
        add(i);
    }
    for(int i = 0; i != 200; ++i) {
        printf("%d\n", arr[i]);
    }

    return 0;
}
