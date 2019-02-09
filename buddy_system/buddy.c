#include <stdio.h>
#include "../common_types.h"

#define MAX_POWER 5
#define MIN_POWER 2

global_variable byte_t *buffer;
global_variable size_t num_nodes;
global_variable size_t nodes_size;


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

// 2-bit values:
// 00: free
// 01: split
// 10: used

int num_nodes_complete_bin_tree(int max, int min) {
    return ((1U << (max-min+1)) - 1);
}

// offsets start at 0
#define left(off) (2*off+1)
#define right(off) (2*off+2)

void set_bitfield(size_t off, uint8_t v) {
    // size_t byte_off = (bitfield_size*off)/8;
    // size_t in_byte_off = (bitfield_size*off)%8;
    size_t mult_value = off << 1;
    size_t byte_off = mult_value >> 3; // div 8
    size_t in_byte_off = mult_value & 7;    // 7 == 0B111, mod 8
    uint8_t *p = (uint8_t *) (buffer + byte_off);
    *p = (*p) | (v << in_byte_off);
    //uint8_t print_value = (*p) & 0xFF;
    //printf("mv: %zd, bo: %zd, ibo: %zd, *p: %u\n", mult_value, byte_off, in_byte_off, print_value);
}

uint8_t get_bitfield(size_t off) {
    size_t mult_value = off << 1;
    size_t byte_off = mult_value >> 3; // div 8
    size_t in_byte_off = mult_value & 7; // 7 == 0B111, mod 8
    uint8_t *p = (uint8_t *) (buffer + byte_off);
    uint8_t ret_val = ((*p) & (3U << in_byte_off)) >> in_byte_off;
    return ret_val;
}

#define is_free(off) (get_bitfield(off) == 0)

int get_level(size_t off) {
    for(int i = MAX_POWER; i >= MIN_POWER-1; --i) {
        size_t up = ((1U << i) - 1);
        size_t down = ((1U << (i-1)) - 1);
        if(off < up && off >= down)
            return i;

    }
    // should never get here
    return -1;
}

size_t get_buddy_size(size_t off) {
    return (1U << (MAX_POWER - (get_level(off)-1)));
}

int is_appropriate(size_t off, size_t req_size) {
    uint32_t next_power = next_power_of_2((uint32_t) req_size);
    size_t buddy_size = get_buddy_size(off);
    return ((uint32_t) buddy_size == next_power);
}

// TODO(stefanos): Find a mathematical way to compute that.
void *actual_mem_addr(size_t off) {
    uint8_t *am_base = buffer + nodes_size;
    uint8_t *p = am_base;
    for(int i = 0; i <= off; ++i) {
        p += get_buddy_size(i);
    }
    return ((void *) p);
}

void *ret_addr;

int traverse(size_t off, size_t req_size) {
    // printf("node: %zd, size: %zd\n", off, get_buddy_size(off));

    if(is_appropriate(off, req_size)) {
        if(is_free(off)) {
            printf("%zd marked!\n", off);
            set_bitfield(off, 2);
            ret_addr = actual_mem_addr(off);
            return 1;
        }
        return 0;
    }

    size_t left_buddy = left(off);
    size_t right_buddy = right(off);
    size_t other_buddy = left_buddy;
    // TODO(stefanos): Weird C, change it.
    size_t best_buddy = is_free(left_buddy) ?
                        other_buddy = right_buddy, left_buddy : right_buddy;

    if(traverse(best_buddy, req_size) == 0) {
        if(traverse(other_buddy, req_size) == 0) {
            return 0;
        }
        set_bitfield(off, 1);
        return 1;
    } else {
        set_bitfield(off, 1);
        return 1;
    }
}

void *balloc(size_t size) {
    size_t max_size = 1 << MAX_POWER;
    size_t min_size = 1 << MIN_POWER;
    if(size < min_size)
        size = min_size;
    if(size > max_size)
        size = max_size;
    int ret = traverse(0, size);
    // printf("ret: %d\n", ret);
    if(ret == 0)
        return NULL;
    return ret_addr;
}

int main(void) {

    num_nodes = num_nodes_complete_bin_tree(MAX_POWER, MIN_POWER);
    nodes_size = (num_nodes * 2) / 8 + 1;
    size_t alloc_size = (1U << MAX_POWER) + nodes_size;
    buffer = calloc(1, alloc_size);
    size_t off = 1;
    printf("%zd l: %zd r: %zd\n", off, left(off), right(off));

    printf("%d\n", num_nodes_complete_bin_tree(MAX_POWER, MIN_POWER));
    // balloc(31);
    // balloc(17);
    balloc(1);
    balloc(17);
    balloc(15);

    return 0;
}
