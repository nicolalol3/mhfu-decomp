/* rint (double): rounds to the nearest integer in the current mode, using
 * the addition and subtraction of 2^52 (fdlibm, Sun Microsystems). */
#include "libm.h"

static const f64 TWO52[2] = {
    4.50359962737049600000e+15,  /* 0x43300000, 0x00000000 */
    -4.50359962737049600000e+15, /* 0xC3300000, 0x00000000 */
};

f64 rint(f64 x)
{
    s32 i0, j0, sx;
    u32 i, i1;
    f64 w, t;
    union { f64 d; s32 w[2]; } u;

    u.d = x;
    i0 = u.w[1];
    sx = (i0 >> 31) & 1;
    i1 = u.w[0];
    j0 = ((i0 >> 20) & 0x7ff) - 0x3ff;
    if (j0 < 20) {
        if (j0 < 0) { /* |x| < 1 */
            if (((i0 & 0x7fffffff) | i1) == 0)
                return x;
            i1 |= (i0 & 0x0fffff);
            i0 &= 0xfffe0000;
            i0 |= ((i1 | -i1) >> 12) & 0x80000;
            u.w[1] = i0;
            w = TWO52[sx] + u.d;
            u.d = w - TWO52[sx];
            u.w[1] = (u.w[1] & 0x7fffffff) | (sx << 31); /* keeps the sign of x */
            return u.d;
        } else {
            i = (0x000fffff) >> j0;
            if (((i0 & i) | i1) == 0)
                return x; /* already an integer */
            i >>= 1;
            if (((i0 & i) | i1) != 0) {
                if (j0 == 19)
                    i1 = 0x40000000;
                else
                    i0 = (i0 & (~i)) | ((0x20000) >> j0);
            }
        }
    } else if (j0 > 51) {
        if (j0 == 0x400)
            return x + x; /* inf or NaN */
        return x;         /* already an integer */
    } else {
        i = ((u32)(0xffffffff)) >> (j0 - 20);
        if ((i1 & i) == 0)
            return x; /* already an integer */
        i >>= 1;
        if ((i1 & i) != 0)
            i1 = (i1 & (~i)) | ((0x40000000) >> (j0 - 20));
    }
    u.w[1] = i0;
    u.w[0] = i1;
    w = TWO52[sx] + u.d;
    return w - TWO52[sx];
}
