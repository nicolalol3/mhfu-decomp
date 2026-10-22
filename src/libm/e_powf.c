/* powf: x elevato alla y, versione float della libreria matematica fdlibm (Sun Microsystems).
 * Le costanti sono quelle presenti nel gioco (cp e cp_h differiscono dalla fdlibm classica). */
#include "libm.h"

static const f32 bp[] = { 1.0f, 1.5f };
static const f32 dp_h[] = { 0.0f, 0.5849609375f };          /* 0x3f15c000 */
static const f32 dp_l[] = { 0.0f, 1.5632208487659227e-06f }; /* 0x35d1cfdc */
static const f32 one = 1.0f;
static const f32 two = 2.0f;
static const f32 two24 = 16777216.0f;
static const f32 huge = 1.0e30f;
static const f32 tiny = 1.0e-30f;
/* coefficienti del polinomio per (3/2)*(log(x)-2s-2/3*s**3) */
static const f32 L1 = 0.6000000238418579f;     /* 0x3f19999a */
static const f32 L2 = 0.4285714328289032f;     /* 0x3edb6db7 */
static const f32 L3 = 0.3333333432674408f;     /* 0x3eaaaaab */
static const f32 L4 = 0.2727281153202057f;     /* 0x3e8ba305 */
static const f32 L5 = 0.23066075146198273f;    /* 0x3e6c3255 */
static const f32 L6 = 0.20697501301765442f;    /* 0x3e53f142 */
static const f32 P1 = 0.1666666716337204f;     /* 0x3e2aaaab */
static const f32 P2 = -0.0027777778450399637f; /* 0xbb360b61 */
static const f32 P3 = 6.61375597701408e-05f;   /* 0x388ab355 */
static const f32 P4 = -1.6533901998627698e-06f; /* 0xb5ddea0e */
static const f32 P5 = 4.138136944220605e-08f;  /* 0x3331bb4c */
static const f32 lg2 = 0.6931471824645996f;    /* 0x3f317218 */
static const f32 lg2_h = 0.693145751953125f;   /* 0x3f317200 */
static const f32 lg2_l = 1.4286065379565116e-06f; /* 0x35bfbe8c */
static const f32 ovt = 4.299566569443414e-08f; /* -(128-log2(ovfl+.5ulp)) */
static const f32 cp = 0.9617967009544373f;     /* 0x3f76384f = 2/(3ln2) */
static const f32 cp_h = 0.9617919921875f;      /* 0x3f763800 */
static const f32 cp_l = 4.701738362200558e-06f; /* 0x369dc3a0 */
static const f32 ivln2 = 1.4426950216293335f;  /* 0x3fb8aa3b = 1/ln2 */
static const f32 ivln2_h = 1.44268798828125f;  /* 0x3fb8aa00 */
static const f32 ivln2_l = 7.052607543300837e-06f; /* 0x36eca570 */

f32 __ieee754_powf(f32 x, f32 y)
{
    f32 z, ax, z_h, z_l, p_h, p_l;
    f32 y1, t1, t2, r, s, t, u, v, w;
    s32 i, j, k, yisint, n;
    s32 hx, hy, ix, iy;

    hx = f32_bits(x);
    hy = f32_bits(y);
    ix = hx & 0x7fffffff;
    iy = hy & 0x7fffffff;

    /* y == 0: x**0 = 1 */
    if (iy == 0)
        return one;

    /* x o y NaN */
    if (ix > 0x7f800000 || iy > 0x7f800000)
        return y + x;

    /* yisint = 0: y non intero, 1: dispari, 2: pari (calcolato solo per x < 0) */
    yisint = 0;
    if (hx < 0) {
        if (iy >= 0x4b800000) {
            yisint = 2;
        } else if (iy >= 0x3f800000) {
            k = (iy >> 23) - 0x7f;
            j = iy >> (23 - k);
            if ((j << (23 - k)) == iy)
                yisint = 2 - (j & 1);
        }
    }

    /* valori speciali di y */
    if (iy == 0x7f800000) { /* y = +-inf */
        if (ix == 0x3f800000)
            return y - y; /* inf**+-1 = NaN */
        else if (ix > 0x3f800000)
            return (hy >= 0) ? y : 0.0f;
        else
            return (hy < 0) ? -y : 0.0f;
    }
    if (iy == 0x3f800000) { /* y = +-1 */
        if (hy < 0)
            return one / x;
        return x;
    }
    if (hy == 0x40000000) /* y = 2 */
        return x * x;
    if (hy == 0x3f000000) { /* y = 0.5 */
        if (hx >= 0)
            return __ieee754_sqrtf(x);
    }

    ax = fabsf(x);
    /* valori speciali di x: +-0, +-inf, +-1 */
    if (ix == 0x7f800000 || ix == 0 || ix == 0x3f800000) {
        z = ax;
        if (hy < 0)
            z = one / z;
        if (hx < 0) {
            if (((ix - 0x3f800000) | yisint) == 0)
                z = (z - z) / (z - z); /* (-1)**non-intero = NaN */
            else if (yisint == 1)
                z = -z;
        }
        return z;
    }

    /* (x<0)**(non intero) = NaN */
    if (((((u32)hx >> 31) - 1) | yisint) == 0)
        return (x - x) / (x - x);

    if (iy > 0x4d000000) { /* |y| > 2**27 */
        if (ix < 0x3f7ffff8)
            return (hy < 0) ? huge * huge : tiny * tiny;
        if (ix > 0x3f800007)
            return (hy > 0) ? huge * huge : tiny * tiny;
        /* |1-x| piccolissimo: log(x) = x-x^2/2+x^3/3-x^4/4 */
        t = ax - one;
        w = (t * t) * (0.5f - t * (0.3333333432674408f - t * 0.25f));
        u = ivln2_h * t;
        v = t * ivln2_l - w * ivln2;
        t1 = u + v;
        t1 = bits_f32(f32_bits(t1) & 0xfffff000);
        t2 = v - (t1 - u);
    } else {
        f32 s2, s_h, s_l, t_h, t_l;

        n = 0;
        if (ix < 0x00800000) { /* subnormale */
            ax *= two24;
            n -= 24;
            ix = f32_bits(ax);
        }
        n += (ix >> 23) - 0x7f;
        j = ix & 0x007fffff;
        ix = j | 0x3f800000;
        if (j <= 0x1cc471) {
            k = 0; /* |x| < sqrt(3/2) */
        } else if (j < 0x5db3d7) {
            k = 1; /* |x| < sqrt(3) */
        } else {
            k = 0;
            n += 1;
            ix -= 0x00800000;
        }
        ax = bits_f32(ix);

        /* s = s_h + s_l = (x-1)/(x+1) oppure (x-1.5)/(x+1.5) */
        u = ax - bp[k];
        v = one / (ax + bp[k]);
        s = u * v;
        s_h = bits_f32(f32_bits(s) & 0xfffff000);
        t_h = bits_f32(((ix >> 1) | 0x20000000) + 0x0040000 + (k << 21));
        t_l = ax - (t_h - bp[k]);
        s_l = v * ((u - s_h * t_h) - s_h * t_l);
        /* log(ax) */
        s2 = s * s;
        r = s2 * s2 * (L1 + s2 * (L2 + s2 * (L3 + s2 * (L4 + s2 * (L5 + s2 * L6)))));
        r += s_l * (s_h + s);
        s2 = s_h * s_h;
        t_h = 3.0f + s2 + r;
        t_h = bits_f32(f32_bits(t_h) & 0xfffff000);
        t_l = r - ((t_h - 3.0f) - s2);
        u = s_h * t_h;
        v = s_l * t_h + t_l * s;
        p_h = u + v;
        p_h = bits_f32(f32_bits(p_h) & 0xfffff000);
        p_l = v - (p_h - u);
        z_h = cp_h * p_h;
        z_l = cp_l * p_h + p_l * cp + dp_l[k];
        /* log2(ax) = n + dp_h + z_h + z_l */
        t = (f32)n;
        t1 = (((z_h + z_l) + dp_h[k]) + t);
        t1 = bits_f32(f32_bits(t1) & 0xfffff000);
        t2 = z_l - (((t1 - t) - dp_h[k]) - z_h);
    }

    s = one; /* segno del risultato */
    if (((((u32)hx >> 31) - 1) | (yisint - 1)) == 0)
        s = -one; /* (negativo)**(dispari) */

    /* (y1+y2)*(t1+t2) */
    y1 = bits_f32(f32_bits(y) & 0xfffff000);
    p_l = (y - y1) * t1 + y * t2;
    p_h = y1 * t1;
    z = p_l + p_h;
    j = f32_bits(z);
    if (j > 0x43000000) {
        return s * huge * huge; /* overflow */
    } else if (j == 0x43000000) {
        if (p_l + ovt > z - p_h)
            return s * huge * huge;
    } else if ((j & 0x7fffffff) > 0x43160000) {
        return s * tiny * tiny; /* underflow */
    } else if ((u32)j == 0xc3160000) {
        if (p_l <= z - p_h)
            return s * tiny * tiny;
    }

    /* 2**(p_h+p_l) */
    i = j & 0x7fffffff;
    k = (i >> 23) - 0x7f;
    n = 0;
    if (i > 0x3f000000) { /* |z| > 0.5: n = [z+0.5] */
        n = j + (0x00800000 >> (k + 1));
        k = ((n & 0x7fffffff) >> 23) - 0x7f;
        t = bits_f32(n & ~(0x007fffff >> k));
        n = ((n & 0x007fffff) | 0x00800000) >> (23 - k);
        if (j < 0)
            n = -n;
        p_h -= t;
    }
    t = p_l + p_h;
    t = bits_f32(f32_bits(t) & 0xfffff000);
    u = t * lg2_h;
    v = (p_l - (t - p_h)) * lg2 + t * lg2_l;
    z = u + v;
    w = v - (z - u);
    t = z * z;
    t1 = z - t * (P1 + t * (P2 + t * (P3 + t * (P4 + t * P5))));
    r = (z * t1) / (t1 - two) - (w + z * w);
    z = one - (r - z);
    j = f32_bits(z);
    j += (n << 23);
    if ((j >> 23) <= 0)
        z = scalbnf(z, n); /* risultato subnormale */
    else
        z = bits_f32(j);
    return s * z;
}
