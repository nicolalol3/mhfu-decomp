/* floorf: the largest integer <= x, computed on the bits (fdlibm, Sun Microsystems). */
#include "libm.h"

static const f32 huge = 1.0e30f;

f32 floorf(f32 x)
{
    s32 i0, j0;
    u32 i, ix;

    i0 = f32_bits(x);
    ix = i0 & 0x7fffffff;
    j0 = (ix >> 23) - 0x7f;
    if (j0 < 23) {
        if (j0 < 0) { /* |x| < 1 */
            if (huge + x > (f32)0.0) {
                if (i0 >= 0)
                    i0 = 0;
                else if (ix != 0)
                    i0 = 0xbf800000; /* -1.0 */
            }
        } else {
            i = (0x007fffff) >> j0;
            if ((i0 & i) == 0)
                return x; /* already an integer */
            if (huge + x > (f32)0.0) {
                if (i0 < 0)
                    i0 += (0x00800000) >> j0;
                i0 &= (~i);
            }
        }
    } else {
        if (ix >= 0x7f800000)
            return x + x; /* inf or NaN */
        return x;         /* already an integer */
    }
    return bits_f32(i0);
}
