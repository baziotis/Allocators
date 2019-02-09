#include <stdio.h>
#include "../common_types.h"

#define MAX_POWER 5

/*
1-bit value
0: free
1: dirty
*/
typedef uint8_t header_t;

byte_t *buffer;

#define left(st, si) (st)
#define right(st, si) (st + (si >> 1))

int is_free(size_t off) {
    header_t *hp = (header_t *) (buffer + off);
    return hp == 0;
}

// TODO(stefanos): Note that v should be 32-bit value.
// Examine if this works for 64-bit values as well.
uint32_t next_power_of_2(uint32_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    return v;
}

int is_appropriate(size_t buddy_size, size_t req_size) {
    uint32_t next_power = next_power_of_2((uint32_t) req_size);
    return ((uint32_t) buddy_size == next_power);
}

int traverse(size_t start, size_t size, size_t req_size) {
    if(size < (1U << 2)) {
        return 0;
    }

    header_t *hp = (header_t *) (buffer + start);

    printf("father: %zd, size: %zd\n", start, size);

    if(is_appropriate(size, req_size)) {
        if(*hp == 0) {
            printf("Marked!\n");
            return 1;
        }
        return 0;
    }

    size_t left_buddy = left(start, size);
    size_t right_buddy = right(start, size);
    size_t other_buddy = left_buddy;
    // TODO(stefanos): Weird C, change it.
    size_t best_buddy = is_free(left_buddy) ?
                        other_buddy = right_buddy, left_buddy : right_buddy;

    size_t new_size = size >> 1;
    if(traverse(best_buddy, new_size, req_size) == 0) {
        if(traverse(other_buddy, new_size, req_size) == 0)
            return 0;
        return 1;
    } else {
        *hp = 1;
        return 1;
    }
}

int main(void) {

    size_t size = 1 << MAX_POWER;
    buffer = calloc(1, size);
    header_t *hp = (header_t *) buffer;
    *hp = 0;
    traverse(0, size, 13);

    return 0;
}
