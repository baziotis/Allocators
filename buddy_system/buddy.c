#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "buddy.h"

#define MAX_POWER 11
#define MIN_POWER 2
#define MAX_LEVEL (MAX_POWER - MIN_POWER)

global_variable byte_t *buffer;
global_variable byte_t *m_base;
global_variable size_t num_nodes;
global_variable size_t nodes_size;
global_variable size_t max_level;

typedef size_t header_t;

// IMPORTANT(stefanos):
// TODO list:
// 1) The allocator currently allocates the biggest possible
// buffer that it will need (depending on the MAX_POWER), which
// may never be used and may actually case the allocator to fail
// if the system doesn't have enough memory. It's actually very easy
// to expand the buffer only when we need more.
// 2) Computationally, some things are too expensive.

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

internal uint8_t *get_byte(size_t index) {
    size_t byte_off = index >> 3; // div 8
    uint8_t *p = (uint8_t *) (buffer + byte_off);
    return p;
}

void set_bit(size_t index) {
    uint8_t *p = get_byte(index);
    size_t in_byte_off = index & 7; // 7 == 0B111, aka mod 8
    *p = (*p) | (1U << in_byte_off);
}

void unset_bit(size_t index) {
    uint8_t *p = get_byte(index);
    size_t in_byte_off = index & 7; // 7 == 0B111, aka mod 8
    uint8_t temp = 0xFF ^ (1 << in_byte_off);
    *p = (*p) & temp;
}

uint8_t get_bit(size_t index) {
    size_t byte_off = index >> 3; // div 8
    size_t in_byte_off = index & 7; // 7 == 0B111, aka mod 8
    uint8_t *p = (uint8_t *) (buffer + byte_off);
    uint8_t ret_val = ((*p) & (1U << in_byte_off)) >> in_byte_off;
    return ret_val;
}

typedef struct size_info {
    size_t num_buddies, index, size_pow, off;
} level_info_t;

global_variable level_info_t linfo[MAX_POWER - MIN_POWER + 1];

void initialize_level_info() {
    size_t nodes_per_lvl = 1;
    size_t index = 0;
    size_t level = 0;
    size_t num_levels = MAX_POWER - MIN_POWER + 1;
    while(level <= num_levels) {
        linfo[level].num_buddies = nodes_per_lvl;
        linfo[level].index = index;
        index += nodes_per_lvl;
        nodes_per_lvl <<= 1;
        ++level;
    }
}

// offsets start at 0
#define left(index) (2*index+1)
#define right(index) (2*index+2)
#define get_parent(index) ((index-1) >> 1)
#define is_free(index) (!get_bit(index))

// NOTE(stefanos): These may be too slow...
void set_downward(size_t index, size_t lvl) {
    if(lvl > MAX_LEVEL) return;
    set_bit(index);
    set_downward(left(index), lvl+1);
    set_downward(right(index), lvl+1);
}

void unset_downward(size_t index, size_t lvl) {
    if(lvl > MAX_LEVEL) return;
    unset_bit(index);
    unset_downward(left(index), lvl+1);
    unset_downward(right(index), lvl+1);
}

void set_upward(size_t index, size_t level) {
    if(level != 0) {
        for(size_t lvl = 0; lvl != level; ++lvl) {
            index = get_parent(index);
            set_bit(index);
            debug_printf("parent: %zd\n", index);
        }
    }
}

void unset_upward(size_t index, size_t level) {
    if(level != 0) {
        for(size_t lvl = 0; lvl != level; ++lvl) {
            index = get_parent(index);
            if(is_free(left(index)) && is_free(right(index)))
                unset_bit(index);
        }
    }
}

size_t level_for_size(size_t size) {
    return (MAX_POWER - __builtin_ctz(size));
}

size_t usable_memory_size(size_t max_power) {
    // size_t num_levels = max_power - min_power + 1;
    return (1 << max_power);
}

void initialize_buddy(void) {
    num_nodes = num_nodes_complete_bin_tree(MAX_POWER, MIN_POWER);
    nodes_size = num_nodes/8 + 1;
    size_t usable_mem_size = usable_memory_size(MAX_POWER);
    size_t alloc_size = usable_mem_size + nodes_size;
    buffer = calloc(1, alloc_size);
    m_base = buffer + nodes_size;
    initialize_level_info();
}

void *balloc(size_t size) {
    if(buffer == NULL)
        initialize_buddy();
    void *ret_addr = NULL;
    size_t max_size = 1 << MAX_POWER;
    size += sizeof(header_t);
    if(size > max_size)
        return NULL;
    size = next_power_of_2(size);
    debug_printf("size to be allocated: %zd\n", size);
    size_t level = level_for_size(size);
    level_info_t li = linfo[level];
    size_t buddy, ndx;
    // address where buddies of this size start
    byte_t *start_addr = m_base;
    for(buddy = 0, ndx = li.index; buddy != li.num_buddies; ++buddy, ++ndx) {
        if(is_free(ndx))
            break;
        start_addr += size;
    }
    if(buddy != li.num_buddies) {
        debug_printf("Marked: %zd, size: %zd\n", ndx, size);
        header_t *set_header = (header_t *) start_addr;
        *set_header = size;
        ret_addr = start_addr + sizeof(header_t);
        set_downward(ndx, level);
        set_upward(ndx, level);
    }
    return ret_addr;
}

void index_and_level_for_addr(byte_t *p, size_t size, size_t *ndx, size_t *lvl) {
    // A fun intrinsic. Counts 0 bits (until an 1 is found ) starting
    // from LSB going to MSB. This is GCC only. MSVC has BitScanReverse.
    size_t size_pow = __builtin_ctz(size);
    size_t level = (MAX_POWER - size_pow);
    size_t index = linfo[level].index;
    size_t diff = (byte_t *)p - m_base;
    index += diff >> size_pow;
    *ndx = index;
    *lvl = level;
}

void bfree(void *ptr) {
    byte_t *p = ((byte_t *) ptr) - sizeof(header_t);
    size_t size = *((header_t *) p);
    debug_printf("size: %zd\n", size);
    size_t index, level;
    index_and_level_for_addr(p, size, &index, &level);
    debug_printf("index: %zd\n", index);
    unset_downward(index, level);
    unset_upward(index, level);
}
