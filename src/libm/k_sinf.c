/* __kernel_sinf: seno su [-pi/4, pi/4]; y è la coda di x, iy indica se y è valida
 * (fdlibm, Sun Microsystems). */
#include "libm.h"

static const f32 half = 0.5f;
static const f32 S1 = -0.1666666716337204f;       /* 0xbe2aaaab */
static const f32 S2 = 0.008333333767950535f;      /* 0x3c088889 */
static const f32 S3 = -0.00019841270113829523f;   /* 0xb9500d01 */
static const f32 S4 = 2.7557314297155244e-06f;    /* 0x3638ef1b */
static const f32 S5 = -2.5050759688838298e-08f;   /* 0xb2d72f34 */
static const f32 S6 = 1.5896910177115586e-10f;    /* 0x2f2ec9d3 */

f32 __kernel_sinf(f32 x, f32 y, int iy)
{
    f32 z, r, v;
    s32 ix;

    ix = f32_bits(x) & 0x7fffffff;
    if (ix < 0x32000000) { /* |x| < 2**-27 */
        if ((int)x == 0)
            return x;
    }
    z = x * x;
    v = z * x;
    r = S2 + z * (S3 + z * (S4 + z * (S5 + z * S6)));
    if (iy == 0)
        return x + v * (S1 + z * r);
    return x - ((z * (half * y - v * r) - y) - v * S1);
}
