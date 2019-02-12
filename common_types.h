#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// NOTE(stefanos): Taken from Casey
#define internal static
#define global_variable static
#define local_persist static

typedef uint8_t byte_t;

#if defined(DEBUG) && DEBUG > 1
  #define debug_printf(fmt, args...) fprintf(stderr, "%s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)
#elif defined(DEBUG)
  #define debug_printf(fmt, args...) printf(fmt, ##args);
#else
  #define debug_printf(fmt, args...)
#endif

#endif
