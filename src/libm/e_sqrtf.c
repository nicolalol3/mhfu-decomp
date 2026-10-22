/* __ieee754_sqrtf: square root computed bit by bit (fdlibm, Sun Microsystems). */
#include "libm.h"

static const f32 one = 1.0f;
static const f32 tiny = 1.0e-30f;

f32 __ieee754_sqrtf(f32 x)
{
    f32 z;
    u32 r, hx;
    s32 ix, s, q, m, t, i;

    ix = f32_bits(x);
    hx = ix & 0x7fffffff;

    if (hx >= 0x7f800000)
        return x * x + x; /* sqrt(NaN) = NaN, sqrt(+inf) = +inf, sqrt(-inf) = NaN */
    if (hx == 0)
        return x; /* sqrt(+-0) = +-0 */
    if (ix < 0)
        return (x - x) / (x - x); /* sqrt(negative) = NaN */

    /* normalizza x */
    m = ix >> 23;
    if (hx < 0x00800000) { /* subnormale */
        for (i = 0; (ix & 0x00800000) == 0; i++)
            ix <<= 1;
        m -= i - 1;
    }
    m -= 127;
    ix = (ix & 0x007fffff) | 0x00800000;
    if (m & 1) /* odd exponent: doubles x */
        ix += ix;
    m >>= 1;

    /* generates sqrt(x) one bit at a time */
    ix += ix;
    q = s = 0;
    r = 0x01000000;
    while (r != 0) {
        t = s + r;
        if (t <= ix) {
            s = t + r;
            ix -= t;
            q += r;
        }
        ix += ix;
        r >>= 1;
    }

    /* rounding: the sums with tiny choose the direction */
    if (ix != 0) {
        z = one - tiny;
        if (z >= one) {
            z = one + tiny;
            if (z > one)
                q += 2;
            else
                q += (q & 1);
        }
    }
    ix = (q >> 1) + 0x3f000000;
    ix += (m << 23);
    return bits_f32(ix);
}
