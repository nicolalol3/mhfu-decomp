/* floor (double): the largest integer <= x, computed on the bits (fdlibm, Sun Microsystems).
 * The double operations become calls to the game's software routines. */
#include "libm.h"

static const f64 huge = 1.0e300;

f64 floor(f64 x)
{
    s32 i0, i1, j0;
    u32 i, j;
    union { f64 d; s32 w[2]; } u;

    u.d = x;
    i0 = u.w[1];
    i1 = u.w[0];
    j0 = ((i0 >> 20) & 0x7ff) - 0x3ff;
    if (j0 < 20) {
        if (j0 < 0) { /* |x| < 1 */
            if (huge + x > 0.0) {
                if (i0 >= 0) {
                    i0 = i1 = 0;
                } else if (((i0 & 0x7fffffff) | i1) != 0) {
                    i0 = 0xbff00000; /* -1.0 */
                    i1 = 0;
                }
            }
        } else {
            i = (0x000fffff) >> j0;
            if (((i0 & i) | i1) == 0)
                return x; /* x is already an integer */
            if (huge + x > 0.0) {
                if (i0 < 0)
                    i0 += (0x00100000) >> j0;
                i0 &= (~i);
                i1 = 0;
            }
        }
    } else if (j0 > 51) {
        if (j0 == 0x400)
            return x + x; /* inf or NaN */
        return x;         /* x is already an integer */
    } else {
        i = ((u32)(0xffffffff)) >> (j0 - 20);
        if ((i1 & i) == 0)
            return x;
        if (huge + x > 0.0) {
            if (i0 < 0) {
                if (j0 == 20) {
                    i0 += 1;
                } else {
                    j = i1 + (1 << (52 - j0));
                    if (j < (u32)i1)
                        i0 += 1; /* riporto */
                    i1 = j;
                }
            }
            i1 &= (~i);
        }
    }
    u.w[1] = i0;
    u.w[0] = i1;
    return u.d;
}
