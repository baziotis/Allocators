#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common_types.h"

#define MAX_POWER 5
#define MIN_POWER 2
#define MAX_LEVEL (MAX_POWER-1)

global_variable byte_t *buffer;
global_variable size_t num_nodes;
global_variable size_t nodes_size;
global_variable size_t max_level;


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

// 1-bit values:
// 0: free
// 1: split
// Split might be that either only one is used or both
// are used.

int num_nodes_complete_bin_tree(int max, int min) {
    return ((1U << (max-min+1)) - 1);
}

void set_bitfield(size_t off, uint8_t v) {
    assert(v <= 1U);
    size_t byte_off = off >> 3; // div 8
    size_t in_byte_off = off & 7; // 7 == 0B111, aka mod 8
    uint8_t *p = (uint8_t *) (buffer + byte_off);
    *p = (*p) | (v << in_byte_off);
}

uint8_t get_bitfield(size_t off) {
    size_t byte_off = off >> 3; // div 8
    size_t in_byte_off = off & 7; // 7 == 0B111, aka mod 8
    uint8_t *p = (uint8_t *) (buffer + byte_off);
    uint8_t ret_val = ((*p) & (1U << in_byte_off)) >> in_byte_off;
    return ret_val;
}

// int get_level(size_t off) {
//     for(int i = MAX_POWER; i >= MIN_POWER-1; --i) {
//         size_t up = ((1U << i) - 1);
//         size_t down = ((1U << (i-1)) - 1);
//         if(off < up && off >= down)
//             return i;
//
//     }
//     // should never get here
//     return -1;
// }

// size_t get_buddy_size(size_t off) {
//     return (1U << (MAX_POWER - (get_level(off)-1)));
// }

// TODO(stefanos): Find a mathematical way to compute that.
// void *addr_for_off(size_t off) {
//     uint8_t *m_base = buffer + nodes_size;
//     uint8_t *p = m_base;
//     for(int i = 0; i <= off; ++i) {
//         p += get_buddy_size(i);
//     }
//     return ((void *) p);
// }

// address where buddies of 'size' size start.
// Also, return how many buddies of this size exist and the starting index
// of those buddies.
void *start_addr_for_size(size_t size, size_t *num_buddies, size_t *index, size_t *lvl) {
    uint32_t j = 1 << MAX_POWER;
    uint32_t k = 1;
    size_t off = 0;
    size_t ndx = 0;
    size_t level = 1;
    while(size < j) {
        off += k * j;
        ndx += k;
        k <<= 1;
        j >>= 1;
        level++;
    }
    *num_buddies = k;
    *index = ndx;
    *lvl = level;
    uint8_t *m_base = buffer + nodes_size;
    return (void *)(m_base + off);
}

// offsets start at 0
#define left(off) (2*off+1)
#define right(off) (2*off+2)
#define get_parent(index) ((index-1) >> 1)
#define is_free(index) (!get_bitfield(index))

// NOTE(stefanos): This may be too slow...
void set_downward(size_t off, size_t lvl) {
    if(lvl > MAX_LEVEL) return;
    set_bitfield(off, 1);
    set_downward(left(off), lvl+1);
    set_downward(right(off), lvl+1);
}

void *traverse(size_t req_size) {
    void *ret_addr;
    size_t num_buddies, index, level, i, j;
    uint8_t *p = (uint8_t *) start_addr_for_size(req_size, &num_buddies, &index, &level);
    for(i = 0, j = index; i != num_buddies; ++i, ++j) {
        if(is_free(j))
            break;
        p += req_size;
    }
    if(i != num_buddies) {
        printf("Marked: %zd\n", j);
        ret_addr = p;
        set_bitfield(j, 1);
        if(level != MAX_LEVEL) {
            // walk down the tree
            set_downward(j, level);
        }
        // walk the tree up to the parent.
        for(i = 1; i != level; ++i) {
            j = get_parent(j);
            set_bitfield(j, 1);
            printf("parent: %zd\n", j);
        }
        return ret_addr;
    }
    return NULL;
}

void *balloc(size_t size) {
    size_t max_size = 1 << MAX_POWER;
    size_t min_size = 1 << MIN_POWER;
    if(size < min_size)
        size = min_size;
    if(size > max_size)
        size = max_size;
    size = next_power_of_2(size);
    return traverse(size);
}

int main(void) {

    num_nodes = num_nodes_complete_bin_tree(MAX_POWER, MIN_POWER);
    nodes_size = (num_nodes * 2) / 8 + 1;
    printf("nodes_size: %zd\n", nodes_size);
    size_t alloc_size = (1U << MAX_POWER) + nodes_size;
    buffer = calloc(1, alloc_size);
    size_t off = 1;

    printf("num_nodes: %d\n", num_nodes_complete_bin_tree(MAX_POWER, MIN_POWER));
    // balloc(15);
    // balloc(15);
    // balloc(4); // in this call we should have run out space.
    char *p = balloc(14);
    strcpy(p, "stefanos");
    printf("%s\n", p);

    return 0;
}
