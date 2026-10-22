/* __kernel_cos: coseno double su [-pi/4, pi/4] (fdlibm, Sun Microsystems).
 * The PSP has no double-precision FPU: the operations become calls to __muldf3/__adddf3/__subdf3,
 * which are linked to the game's software routines. The game never calls this function. */
#include "libm.h"

static const f64 one = 1.0;
static const f64 C1 = 4.16666666666666019037e-02;  /* 0x3FA55555, 0x5555554C */
static const f64 C2 = -1.38888888888741095749e-03; /* 0xBF56C16C, 0x16C15177 */
static const f64 C3 = 2.48015872894767294178e-05;  /* 0x3EFA01A0, 0x19CB1590 */
static const f64 C4 = -2.75573143513906633035e-07; /* 0xBE927E4F, 0x809C52AD */
static const f64 C5 = 2.08757232129817482790e-09;  /* 0x3E21EE9E, 0xBDB4B1C4 */
static const f64 C6 = -1.13596475577881948265e-11; /* 0xBDA8FAE9, 0xBE8838D4 */

f64 __kernel_cos(f64 x, f64 y)
{
    f64 a, hz, z, r, qx;
    s32 ix;
    union { f64 d; u32 w[2]; } u;

    u.d = x;
    ix = u.w[1] & 0x7fffffff; /* high word of |x| */
    if (ix < 0x3e400000) { /* |x| < 2**-27 */
        if ((int)x == 0)
            return one;
    }
    z = x * x;
    r = z * (C1 + z * (C2 + z * (C3 + z * (C4 + z * (C5 + z * C6)))));
    if (ix < 0x3FD33333) /* |x| < 0.3 */
        return one - (0.5 * z - (z * r - x * y));
    if (ix > 0x3fe90000) { /* |x| > 0.78125 */
        qx = 0.28125;
    } else {
        u.w[1] = ix - 0x00200000; /* x/4 */
        u.w[0] = 0;
        qx = u.d;
    }
    hz = 0.5 * z - qx;
    a = one - qx;
    return a - (hz - (z * r - x * y));
}
