/* Piccole funzioni float che lavorano direttamente sui bit (fdlibm/newlib). */
#include "libm.h"

f32 copysignf(f32 x, f32 y)
{
    return bits_f32((f32_bits(x) & 0x7fffffff) | (f32_bits(y) & 0x80000000));
}

f32 fabsf(f32 x)
{
    return bits_f32(f32_bits(x) & 0x7fffffff);
}

int finitef(f32 x)
{
    return (f32_bits(x) & 0x7fffffff) < 0x7f800000;
}

int isnanf(f32 x)
{
    return (f32_bits(x) & 0x7fffffff) > 0x7f800000;
}
