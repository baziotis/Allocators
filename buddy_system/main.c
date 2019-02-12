#include <assert.h>
#include <stdio.h>
#include "buddy.h"

global_variable void *base;

#define diff(p) ((void *) p - base)

int main(void) {

    // Based on Alexis Delis' homework

    void *p1 = (void *) balloc(159);
    base = p1; // First allocation should always give the base.
    printf("alloc p1: %ld\n", diff(p1));
    void *p2 = (void *) balloc(79);
    printf("alloc p2: %ld\n", diff(p2));
    void *p3 = (void *) balloc(490);
    printf("alloc p3: %ld\n", diff(p3));
    void *p4 = (void *) balloc(170);
    printf("alloc p4: %ld\n", diff(p4));
    void *p5 = (void *) balloc(81);
    printf("alloc p5: %ld\n", diff(p5));
    printf("\nfree p3\n");
    bfree(p3);
    printf("\nfree p1\n");
    bfree(p1);
    void *p6 = (void *) balloc(274);
    printf("\nalloc p6: %ld\n", diff(p6));
    printf("\nfree p2\n");
    bfree(p2);
    printf("\nfree p5\n");
    bfree(p5);
    printf("\nfree p4\n");
    bfree(p4);
    printf("\nfree p6\n");
    bfree(p6);
}
