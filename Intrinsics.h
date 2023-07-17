#pragma once

#include <math.h>
#include <intrin.h>
#include <windows.h>

inline s32 round_f32_to_s32(f32 value) {
    s32 result = (s32)roundf(value);
    return result;
}

inline u32 round_f32_to_u32(f32 value) {
    u32 result = (u32)roundf(value);
    return result;
}

inline s32 floor_f32_to_s32(f32 value) {
    s32 result = (s32)floorf(value);
    return result;
}

inline f32 sin(f32 angle) {
    f32 result = sinf(angle);
    return result;
}

inline f32 cos(f32 angle) {
    f32 result = cosf(angle);
    return result;
}

inline f32 atan2(f32 x, f32 y) {
    f32 result = atan2f(y, x);
    return result;
}

inline bool bsf(u32 *index, u32 mask) { 
    s32 result = _BitScanForward((unsigned long *)index, (unsigned long)mask);
    if (result) { return true; }
    return false;
}


