#include <assert.h>
#include <stdio.h>
#include "buddy.h"

int main(void) {
    size_t i = 0;
    int *p1 = balloc(10 * sizeof(int));
    assert(p1 != NULL);
    for(i = 0; i != 10; ++i) {
        p1[i] = i;
    }
    double *p2 = balloc(30 * sizeof(double));
    assert(p2 != NULL);
    for(i = 0; i != 30; ++i) {
        p2[i] = 1.3 + i;
    }
    bfree(p2);
    for(i = 0; i != 10; ++i) {
        printf("%d\n", p1[i]);
    }
    bfree(p1);
}
