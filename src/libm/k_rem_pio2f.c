/* __kernel_rem_pio2f: riduzione di argomenti molto grandi modulo pi/2, usando le cifre di 2/pi
 * passate in ipio2 (fdlibm, Sun Microsystems). Restituisce n & 7 e il resto in y[]. */
#include "libm.h"

f32 floorf(f32 x);

static const s32 init_jk[] = { 4, 7, 9 }; /* valore iniziale di jk per ogni precisione */

static const f32 PIo2[] = {
    1.5703125f,                /* 0x3fc90000 */
    0.000457763671875f,        /* 0x39f00000 */
    2.5987625122070312e-05f,   /* 0x37da0000 */
    7.543712854385376e-08f,    /* 0x33a20000 */
    6.002665031701326e-11f,    /* 0x2e840000 */
    7.389644451905042e-13f,    /* 0x2b500000 */
    5.384581669432009e-15f,    /* 0x27c20000 */
    5.637851296924623e-18f,    /* 0x22d00000 */
    8.300922883092143e-20f,    /* 0x1fc40000 */
    3.2756352257099896e-22f,   /* 0x1bc60000 */
    6.333101564859118e-25f,    /* 0x17440000 */
};

static const f32 zero = 0.0f;
static const f32 one = 1.0f;
static const f32 two8 = 256.0f;
static const f32 twon8 = 0.00390625f;

s32 __kernel_rem_pio2f(f32 *x, f32 *y, int e0, int nx, int prec, const s32 *ipio2)
{
    s32 jz, jx, jv, jp, jk, carry, n, iq[20], i, j, k, m, q0, ih;
    f32 z, fw, f[20], fq[20], q[20];

    jk = init_jk[prec];
    jp = jk;

    /* jx, jv, q0 (q0 < 3) */
    jx = nx - 1;
    jv = (e0 - 3) / 8;
    if (jv < 0)
        jv = 0;
    q0 = e0 - 8 * (jv + 1);

    /* f[0..jx+jk] = ipio2[jv-jx .. jv+jk] */
    j = jv - jx;
    m = jx + jk;
    for (i = 0; i <= m; i++, j++)
        f[i] = (j < 0) ? zero : (f32)ipio2[j];

    /* q[0..jk] */
    for (i = 0; i <= jk; i++) {
        for (j = 0, fw = 0.0f; j <= jx; j++)
            fw += x[j] * f[jx + i - j];
        q[i] = fw;
    }

    jz = jk;
recompute:
    /* distilla q[] in iq[] all'inverso */
    for (i = 0, j = jz, z = q[jz]; j > 0; i++, j--) {
        fw = (f32)((s32)(twon8 * z));
        iq[i] = (s32)(z - two8 * fw);
        z = q[j - 1] + fw;
    }

    /* calcola n */
    z = scalbnf(z, (int)q0);
    z -= (f32)8.0 * floorf(z * (f32)0.125); /* toglie la parte intera >= 8 */
    n = (s32)z;
    z -= (f32)n;
    ih = 0;
    if (q0 > 0) { /* serve iq[jz-1] per determinare n */
        i = (iq[jz - 1] >> (8 - q0));
        n += i;
        iq[jz - 1] -= i << (8 - q0);
        ih = iq[jz - 1] >> (7 - q0);
    } else if (q0 == 0) {
        ih = iq[jz - 1] >> 8; /* come nel gioco (fdlibm/newlib storica); la versione corretta usa >> 7 */
    } else if (z >= (f32)0.5) {
        ih = 2;
    }

    if (ih > 0) { /* q > 0.5 */
        n += 1;
        carry = 0;
        for (i = 0; i < jz; i++) { /* calcola 1-q */
            j = iq[i];
            if (carry == 0) {
                if (j != 0) {
                    carry = 1;
                    iq[i] = 0x100 - j;
                }
            } else {
                iq[i] = 0xff - j;
            }
        }
        if (q0 > 0) {
            switch (q0) {
            case 1:
                iq[jz - 1] &= 0x7f;
                break;
            case 2:
                iq[jz - 1] &= 0x3f;
                break;
            }
        }
        if (ih == 2) {
            z = one - z;
            if (carry != 0)
                z -= scalbnf(one, (int)q0);
        }
    }

    /* serve ricalcolare? */
    if (z == zero) {
        j = 0;
        for (i = jz - 1; i >= jk; i--)
            j |= iq[i];
        if (j == 0) {
            for (k = 1; iq[jk - k] == 0; k++)
                ; /* k = numero di termini necessari */

            for (i = jz + 1; i <= jz + k; i++) { /* aggiunge q[jz+1] .. q[jz+k] */
                f[jx + i] = (f32)ipio2[jv + i];
                for (j = 0, fw = 0.0f; j <= jx; j++)
                    fw += x[j] * f[jx + i - j];
                q[i] = fw;
            }
            jz += k;
            goto recompute;
        }
    }

    /* elimina i termini nulli */
    if (z == (f32)0.0) {
        jz -= 1;
        q0 -= 8;
        while (iq[jz] == 0) {
            jz--;
            q0 -= 8;
        }
    } else { /* spezza z in blocchi da 8 bit se serve */
        z = scalbnf(z, -(int)q0);
        if (z >= two8) {
            fw = (f32)((s32)(twon8 * z));
            iq[jz] = (s32)(z - two8 * fw);
            jz += 1;
            q0 += 8;
            iq[jz] = (s32)fw;
        } else {
            iq[jz] = (s32)z;
        }
    }

    /* converte i blocchi interi in virgola mobile */
    fw = scalbnf(one, (int)q0);
    for (i = jz; i >= 0; i--) {
        q[i] = fw * (f32)iq[i];
        fw *= twon8;
    }

    /* PIo2[0..jp] * q[jz..0] */
    for (i = jz; i >= 0; i--) {
        for (fw = 0.0f, k = 0; k <= jp && k <= jz - i; k++)
            fw += PIo2[k] * q[i + k];
        fq[jz - i] = fw;
    }

    /* comprime fq[] in y[] */
    switch (prec) {
    case 0:
        fw = 0.0f;
        for (i = jz; i >= 0; i--)
            fw += fq[i];
        y[0] = (ih == 0) ? fw : -fw;
        break;
    case 1:
    case 2:
        fw = 0.0f;
        for (i = jz; i >= 0; i--)
            fw += fq[i];
        y[0] = (ih == 0) ? fw : -fw;
        fw = fq[0] - fw;
        for (i = 1; i <= jz; i++)
            fw += fq[i];
        y[1] = (ih == 0) ? fw : -fw;
        break;
    case 3:
        for (i = jz; i > 0; i--) {
            fw = fq[i - 1] + fq[i];
            fq[i] += fq[i - 1] - fw;
            fq[i - 1] = fw;
        }
        for (i = jz; i > 1; i--) {
            fw = fq[i - 1] + fq[i];
            fq[i] += fq[i - 1] - fw;
            fq[i - 1] = fw;
        }
        for (fw = 0.0f, i = jz; i >= 2; i--)
            fw += fq[i];
        if (ih == 0) {
            y[0] = fq[0];
            y[1] = fq[1];
            y[2] = fw;
        } else {
            y[0] = -fq[0];
            y[1] = -fq[1];
            y[2] = -fw;
        }
    }
    return n & 7;
}
