/* isinf / isnan (double) senza confronti in virgola mobile (newlib/fdlibm). */
#include "libm.h"

struct exception;

int isinf(f64 x)
{
    s32 hx, lx;
    union { f64 d; s32 w[2]; } u;

    u.d = x;
    hx = u.w[1] & 0x7fffffff;
    lx = u.w[0];
    hx |= (u32)(lx | (-lx)) >> 31;
    hx = 0x7ff00000 - hx;
    return 1 - (int)((u32)(hx | (-hx)) >> 31);
}

int isnan(f64 x)
{
    s32 hx, lx;
    union { f64 d; s32 w[2]; } u;

    u.d = x;
    hx = u.w[1] & 0x7fffffff;
    lx = u.w[0];
    hx |= (u32)(lx | (-lx)) >> 31;
    hx = 0x7ff00000 - hx;
    return (int)((u32)hx >> 31);
}

/* Gestore degli errori matematici dell'utente: nella libreria predefinita non fa nulla. */
int matherr(struct exception *exc)
{
    return 0;
}
