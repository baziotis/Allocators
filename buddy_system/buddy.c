#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "buddy.h"

#define MAX_POWER 10
#define MIN_POWER 2
#define MAX_LEVEL (MAX_POWER-1)

global_variable byte_t *buffer;
global_variable byte_t *m_base;
global_variable size_t num_nodes;
global_variable size_t nodes_size;
global_variable size_t max_level;

// IMPORTANT(stefanos):
// TODO list:
// 1) The allocator currently allocates the biggest possible
// buffer that it will need (depending on the MAX_POWER), which
// may never be used and may actually case the allocator to fail
// if the system doesn't have enough memory.
// 2) The usable memory size should be 1 << MAX_POWER, not
// (1 << MAX_POWER) * num_levels, because that's a _total_
// waste of space. Basically, you want one buffer for all levels.
// Now we have num_levels buffers.
// 3) It's possible that computationally, some things are too expensive.

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
    *p = (*p) | (1 << in_byte_off);
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

void initialize_size_info() {
    size_t s = 1 << MAX_POWER;
    size_t nodes_per_lvl = 1;
    size_t off = 0;
    size_t index = 0;
    size_t level = 0;
    size_t num_levels = MAX_POWER - MIN_POWER + 1;
    while(level <= num_levels) {
        linfo[level].num_buddies = nodes_per_lvl;
        linfo[level].index = index;
        // A fun intrinsic. This is GCC only. MSVC has BitScanReverse
        linfo[level].size_pow = __builtin_ctz(s);
        linfo[level].off = off;
        off += nodes_per_lvl * s;
        index += nodes_per_lvl;
        nodes_per_lvl <<= 1;
        s >>= 1;
        ++level;
    }
}

// offsets start at 0
#define left(off) (2*off+1)
#define right(off) (2*off+2)
#define get_parent(index) ((index-1) >> 1)
#define is_free(index) (!get_bit(index))

// NOTE(stefanos): These may be too slow...
void set_downward(size_t off, size_t lvl) {
    if(lvl > MAX_LEVEL) return;
    set_bit(off);
    set_downward(left(off), lvl+1);
    set_downward(right(off), lvl+1);
}

void unset_downward(size_t off, size_t lvl) {
    if(lvl > MAX_LEVEL) return;
    unset_bit(off);
    unset_downward(left(off), lvl+1);
    unset_downward(right(off), lvl+1);
}

void set_upward(size_t index, size_t level) {
    if(level != 0) {
        for(size_t lvl = 0; lvl != level; ++lvl) {
            index = get_parent(index);
            set_bit(index);
            // printf("parent: %zd\n", index);
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

// NOTE(stefanos): Every level of the tree has the same total memory
// size.
size_t usable_memory_size(size_t max_power, size_t min_power) {
    size_t num_levels = max_power - min_power + 1;
    return ((1 << max_power) * num_levels);
}

void initialize_buddy(void) {
    num_nodes = num_nodes_complete_bin_tree(MAX_POWER, MIN_POWER);
    nodes_size = num_nodes/8 + 1;
    size_t usable_mem_size = usable_memory_size(MAX_POWER, MIN_POWER);
    size_t alloc_size = usable_mem_size + nodes_size;
    buffer = calloc(1, alloc_size);
    m_base = buffer + nodes_size;
    initialize_size_info();
}

void *balloc(size_t size) {
    if(buffer == NULL)
        initialize_buddy();
    void *ret_addr = NULL;
    size_t max_size = 1 << MAX_POWER;
    size_t min_size = 1 << MIN_POWER;
    if(size < min_size)
        size = min_size;
    if(size > max_size)
        return NULL;
    size = next_power_of_2(size);
    size_t level = level_for_size(size);
    level_info_t li = linfo[level];
    size_t buddy, ndx;
    // address where buddies of this size start
    byte_t *start_addr = m_base + li.off;
    for(buddy = 0, ndx = li.index; buddy != li.num_buddies; ++buddy, ++ndx) {
        if(is_free(ndx))
            break;
        start_addr += size;
    }
    if(buddy != li.num_buddies) {
        // printf("Marked: %zd, size: %zd\n", ndx, size);
        ret_addr = start_addr;
        // walk down the tree
        set_downward(ndx, level);
        set_upward(ndx, level);
    }
    return ret_addr;
}

size_t level_for_addr(void *ptr) {
    size_t s = 1 << MAX_POWER;
    size_t nodes_per_lvl = 1;
    size_t off = 0;
    size_t level = 0;
    while((void *)(m_base + off) <= ptr) {
        off += nodes_per_lvl * s;
        nodes_per_lvl <<= 1;
        s >>= 1;
        level++;
    }
    return level-1;
}

size_t index_for_addr(void *ptr, size_t lvl) {
    void *base = (void *) (m_base + linfo[lvl].off);
    size_t start_index = linfo[lvl].index;
    // Think it as division.
    start_index += ((ptr - base) >> linfo[lvl].size_pow);
    return start_index;
}

void bfree(void *ptr) {
    size_t ndx;
    size_t lvl = level_for_addr(ptr);
    size_t index = index_for_addr(ptr, lvl);
    unset_downward(index, lvl);
    unset_upward(index, lvl);
}
