#include <stdio.h>
#include <stdlib.h>

#include "mem_arena.h"

typedef struct {
	double d;
	int i;
} composite_type_t;

int main(void) {

	// Allocate memory
	size_t s = 100;
	void *base = malloc(s);
	if(base == NULL) exit(1);

	// Initialize arena
	mem_arena_t mem_arena;
	mem_arena_initialize(&mem_arena, base, s);

	// Push single type
	composite_type_t *ct = mem_arena_push_type(&mem_arena, composite_type_t);
	ct->d = 1.4;
	ct->i = 7;
	printf("%lf %d\n\n", ct->d, ct->i);

	// Push array of a type
	int *array = mem_arena_push_array(&mem_arena, 10, int);
	for(size_t i = 0; i != 10; ++i)
		array[i] = i;
	for(size_t i = 0; i != 10; ++i)
		printf("%d\n", array[i]);

	return 0;
}
