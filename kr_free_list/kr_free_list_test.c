#include <stdio.h>
#include "kr_free_list.h"

int main(void) {

    void *ret = my_alloc(10);
    printf("main: %p\n", ret);

    return 0;
}
