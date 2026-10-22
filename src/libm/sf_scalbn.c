/* scalbnf: x * 2^n calcolato modificando l'esponente (fdlibm/newlib). */
#include "libm.h"

f32 copysignf(f32 x, f32 y);

static const f32 two25 = 33554432.0f;            /* 0x4c000000 */
static const f32 twom25 = 2.9802322387695312e-08f; /* 0x33000000 */
static const f32 huge = 1.0e30f;
static const f32 tiny = 1.0e-30f;

f32 scalbnf(f32 x, int n)
{
    s32 k, ix;
    u32 hx;

    ix = f32_bits(x);
    hx = ix & 0x7fffffff;
    k = hx >> 23; /* esponente */
    if (hx == 0)
        return x;
    if (hx >= 0x7f800000)
        return x + x; /* NaN o inf */
    if (hx < 0x00800000) { /* subnormale */
        x *= two25;
        ix = f32_bits(x);
        k = ((ix & 0x7f800000) >> 23) - 25;
        if (n < -50000)
            return tiny * x; /* underflow */
    }
    k = k + n;
    if (k > 254)
        return huge * copysignf(huge, x); /* overflow */
    if (k > 0) /* risultato normale */
        return bits_f32((ix & 0x807fffff) | (k << 23));
    if (k < -22) {
        if (n > 50000) /* nel caso n+k sia andato in overflow */
            return huge * copysignf(huge, x);
        return tiny * copysignf(tiny, x);
    }
    k += 25; /* risultato subnormale */
    x = bits_f32((ix & 0x807fffff) | (k << 23));
    return x * twom25;
}
