/* __kernel_cosf: cosine on [-pi/4, pi/4]; y is the tail of x (fdlibm, Sun Microsystems). */
#include "libm.h"

static const f32 one = 1.0f;
static const f32 C1 = 0.0416666679084301f;       /* 0x3d2aaaab */
static const f32 C2 = -0.0013888889225199819f;   /* 0xbab60b61 */
static const f32 C3 = 2.4801587642286904e-05f;   /* 0x37d00d01 */
static const f32 C4 = -2.7557314297155244e-07f;  /* 0xb493f27c */
static const f32 C5 = 2.087572337217125e-09f;    /* 0x310f74f6 */
static const f32 C6 = -1.135964759757524e-11f;   /* 0xad47d74e */

f32 __kernel_cosf(f32 x, f32 y)
{
    f32 a, hz, z, r, qx;
    s32 ix;

    ix = f32_bits(x) & 0x7fffffff;
    if (ix < 0x32000000) { /* |x| < 2**-27 */
        if ((int)x == 0)
            return one;
    }
    z = x * x;
    r = z * (C1 + z * (C2 + z * (C3 + z * (C4 + z * (C5 + z * C6)))));
    if (ix < 0x3e99999a) /* |x| < 0.3 */
        return one - ((f32)0.5 * z - (z * r - x * y));
    if (ix > 0x3f480000) /* |x| > 0.78125 */
        qx = (f32)0.28125;
    else
        qx = bits_f32(ix - 0x01000000); /* x/4 */
    hz = (f32)0.5 * z - qx;
    a = one - qx;
    return a - (hz - (z * r - x * y));
}
