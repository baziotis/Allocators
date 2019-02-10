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

typedef struct size_info {
    size_t num_buddies, index, level, off;
} size_info_t;

// wasting extra space for easy indexing
global_variable size_info_t sinfo[MAX_POWER + 1];

// address where buddies of 'size' size start.
// Also, return how many buddies of this size exist and the starting index
// of those buddies.
uint8_t *start_addr_for_size(size_t size) {
    uint8_t *m_base = buffer + nodes_size;
    return (m_base + sinfo[size].off);
}

void initialize_size_info() {
    size_t s = 1 << MAX_POWER;
    size_t nodes_per_lvl = 1;
    size_t off = 0;
    size_t index = 0;
    size_t level = 0;
    size_t num_levels = MAX_POWER - MIN_POWER + 1;
    while(level <= num_levels) {
        sinfo[level].num_buddies = nodes_per_lvl;
        sinfo[level].index = index;
        sinfo[level].level = level;
        sinfo[level].off = off;
        off += nodes_per_lvl * s;
        index += nodes_per_lvl;
        nodes_per_lvl <<= 1;
        s >>= 1;
        ++level;
    }
}

// TODO(stefanos): Probably there's better way to do that.
size_info_t get_size_info(size_t size) {
    size_t i = 0;
    size_t max = 1 << MAX_POWER;
    while(size < max) {
        ++i;
        size <<= 1;
    }
    return sinfo[i];
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
    size_info_t si = get_size_info(req_size);
    size_t i, j;
    uint8_t *p = (uint8_t *) start_addr_for_size(req_size);
    for(i = 0, j = si.index; i != si.num_buddies; ++i, ++j) {
        if(is_free(j))
            break;
        p += req_size;
    }
    if(i != si.num_buddies) {
        printf("Marked: %zd\n", j);
        ret_addr = p;
        set_bitfield(j, 1);
        if(si.level != MAX_LEVEL) {
            // walk down the tree
            set_downward(j, si.level);
        }
        if(si.level != 0) {
            // walk the tree up to the parent.
            for(i = 1; i != si.level; ++i) {
                j = get_parent(j);
                set_bitfield(j, 1);
                printf("parent: %zd\n", j);
            }
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

// NOTE(stefanos): Every level of the tree has the same total memory
// size.
size_t usable_memory_size(size_t max_power, size_t min_power) {
    size_t num_levels = max_power - min_power + 1;
    return ((1 << max_power) * num_levels);
}

int main(void) {

    num_nodes = num_nodes_complete_bin_tree(MAX_POWER, MIN_POWER);
    nodes_size = num_nodes/8 + 1;
    printf("nodes_size: %zd\n", nodes_size);
    size_t usable_mem_size = usable_memory_size(MAX_POWER, MIN_POWER);
    printf("usable memory size: %zd\n", usable_mem_size);
    size_t alloc_size = usable_mem_size + nodes_size;
    buffer = calloc(1, alloc_size);

    initialize_size_info();

    printf("num_nodes: %d\n", num_nodes_complete_bin_tree(MAX_POWER, MIN_POWER));
    // balloc(15);
    // balloc(15);
    // balloc(4); // in this call we should have run out space.
    char *p = balloc(32);
    // strcpy(p, "stefanos");
    // printf("%s\n", p);

    return 0;
}
