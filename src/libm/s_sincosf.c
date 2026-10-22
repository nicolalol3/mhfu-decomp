/* sinf and cosf: reduce the angle to [-pi/4, pi/4] and use the kernels __kernel_sinf/__kernel_cosf
 * (fdlibm, Sun Microsystems). */
#include "libm.h"

s32 __ieee754_rem_pio2f(f32 x, f32 *y);
f32 __kernel_sinf(f32 x, f32 y, int iy);
f32 __kernel_cosf(f32 x, f32 y);

f32 cosf(f32 x)
{
    f32 y[2], z = 0.0f;
    s32 n, ix;

    ix = f32_bits(x) & 0x7fffffff;
    if (ix <= 0x3f490fd8) /* |x| <= pi/4 */
        return __kernel_cosf(x, z);
    if (ix >= 0x7f800000) /* cos(inf or NaN) = NaN */
        return x - x;
    n = __ieee754_rem_pio2f(x, y);
    switch (n & 3) {
    case 0: return __kernel_cosf(y[0], y[1]);
    case 1: return -__kernel_sinf(y[0], y[1], 1);
    case 2: return -__kernel_cosf(y[0], y[1]);
    default: return __kernel_sinf(y[0], y[1], 1);
    }
}

f32 sinf(f32 x)
{
    f32 y[2], z = 0.0f;
    s32 n, ix;

    ix = f32_bits(x) & 0x7fffffff;
    if (ix <= 0x3f490fd8) /* |x| <= pi/4 */
        return __kernel_sinf(x, z, 0);
    if (ix >= 0x7f800000) /* sin(inf or NaN) = NaN */
        return x - x;
    n = __ieee754_rem_pio2f(x, y);
    switch (n & 3) {
    case 0: return __kernel_sinf(y[0], y[1], 1);
    case 1: return __kernel_cosf(y[0], y[1]);
    case 2: return -__kernel_sinf(y[0], y[1], 1);
    default: return -__kernel_cosf(y[0], y[1]);
    }
}
