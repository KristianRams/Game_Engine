#pragma once

#include <stdint.h>

#define null 0

typedef int8_t   s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t   u8;
typedef uint16_t u16;
typedef uint32_t u32; 
typedef uint64_t u64;

typedef float  f32;
typedef double f64;

typedef s32 b32;

union V2 {
    struct { f32 x, y; };
    struct { f32 u, v; };
    struct { f32 width, height; };
    f32 E[2];
};

