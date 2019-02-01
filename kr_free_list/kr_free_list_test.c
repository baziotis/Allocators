#include <stdio.h>
#include "kr_free_list.h"

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
    d = (double *) my_realloc(d, 50 * sizeof(double));
    printf("realloced d: %p\n", d);

    return 0;
}
