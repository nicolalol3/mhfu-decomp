/* Funzioni matematiche float (fdlibm) presenti nel gioco. */
#ifndef LIBM_H
#define LIBM_H
#include "common.h"

f32 __ieee754_powf(f32 x, f32 y);
f32 __ieee754_sqrtf(f32 x);
f32 fabsf(f32 x);
f32 scalbnf(f32 x, int n);

/* Accesso ai bit di un float. */
static inline s32 f32_bits(f32 f) { union { f32 f; s32 i; } u; u.f = f; return u.i; }
static inline f32 bits_f32(s32 i) { union { f32 f; s32 i; } u; u.i = i; return u.f; }

#endif
