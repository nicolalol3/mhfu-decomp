/* Aritmetica sugli interi grandi (Bigint) per la conversione esatta double <-> testo.
 * Da mprec.c di newlib (algoritmi di David M. Gay); le cifre sono parole da 32 bit
 * elaborate a metà (16 bit) per evitare moltiplicazioni a 64 bit. */
#include "mprec.h"

/* Un Bigint con spazio per 1 << k parole, preso dalla lista dei liberi se possibile. */
_Bigint *Balloc(struct _reent *ptr, int k)
{
    int x;
    _Bigint *rv;

    if (REENT_FREELIST(ptr) == NULL) {
        REENT_FREELIST(ptr) = _calloc_r(ptr, sizeof(_Bigint *), 16);
        if (REENT_FREELIST(ptr) == NULL)
            return NULL;
    }
    if ((rv = REENT_FREELIST(ptr)[k]) != NULL) {
        REENT_FREELIST(ptr)[k] = rv->_next;
    } else {
        x = 1 << k;
        rv = _calloc_r(ptr, 1, sizeof(_Bigint) + (x - 1) * sizeof(u32));
        if (rv == NULL)
            return NULL;
        rv->_k = k;
        rv->_maxwds = x;
    }
    rv->_sign = rv->_wds = 0;
    return rv;
}

void Bfree(struct _reent *ptr, _Bigint *v)
{
    if (v != NULL) {
        v->_next = REENT_FREELIST(ptr)[v->_k];
        REENT_FREELIST(ptr)[v->_k] = v;
    }
}

/* b = b * m + a */
_Bigint *multadd(struct _reent *ptr, _Bigint *b, int m, int a)
{
    int i, wds;
    u32 *x, y, xi, z;
    _Bigint *b1;

    wds = b->_wds;
    x = b->_x;
    i = 0;
    do {
        xi = *x;
        y = (xi & 0xffff) * m + a;
        z = (xi >> 16) * m + (y >> 16);
        a = (int)(z >> 16);
        *x++ = (z << 16) + (y & 0xffff);
    } while (++i < wds);
    if (a) {
        if (wds >= b->_maxwds) {
            b1 = Balloc(ptr, b->_k + 1);
            Bcopy(b1, b);
            Bfree(ptr, b);
            b = b1;
        }
        b->_x[wds++] = a;
        b->_wds = wds;
    }
    return b;
}

/* bit a zero in testa (32 se x == 0) */
int hi0bits(u32 x)
{
    return x != 0 ? __builtin_clz(x) : 32;
}

/* bit a zero in coda; toglie quei bit da *y (32 e *y invariato se *y == 0) */
int lo0bits(u32 *y)
{
    u32 x = *y;
    int k = x != 0 ? __builtin_ctz(x) : 32;

    *y = x >> (k & 31);
    return k;
}

_Bigint *i2b(struct _reent *ptr, int i)
{
    _Bigint *b = Balloc(ptr, 1);

    b->_x[0] = i;
    b->_wds = 1;
    return b;
}

/* scrive due metà da 16 bit in una parola e avanza */
#define Storeinc(a, b, c) \
    (((u16 *)(a))[1] = (u16)(b), ((u16 *)(a))[0] = (u16)(c), (a)++)

_Bigint *mult(struct _reent *ptr, _Bigint *a, _Bigint *b)
{
    _Bigint *c;
    int k, wa, wb, wc;
    u32 carry, y, z, z2;
    u32 *x, *xa, *xae, *xb, *xbe, *xc, *xc0;

    if (a->_wds < b->_wds) {
        c = a;
        a = b;
        b = c;
    }
    k = a->_k;
    wa = a->_wds;
    wb = b->_wds;
    wc = wa + wb;
    if (wc > a->_maxwds)
        k++;
    c = Balloc(ptr, k);
    for (x = c->_x, xa = x + wc; x < xa; x++)
        *x = 0;
    xa = a->_x;
    xae = xa + wa;
    xb = b->_x;
    xbe = xb + wb;
    xc0 = c->_x;
    for (; xb < xbe; xb++, xc0++) {
        if ((y = *xb & 0xffff) != 0) {
            x = xa;
            xc = xc0;
            carry = 0;
            do {
                z = (*x & 0xffff) * y + (*xc & 0xffff) + carry;
                carry = z >> 16;
                z2 = (*x++ >> 16) * y + (*xc >> 16) + carry;
                carry = z2 >> 16;
                Storeinc(xc, z2, z);
            } while (x < xae);
            *xc = carry;
        }
        if ((y = *xb >> 16) != 0) {
            x = xa;
            xc = xc0;
            carry = 0;
            z2 = *xc;
            do {
                z = (*x & 0xffff) * y + (*xc >> 16) + carry;
                carry = z >> 16;
                Storeinc(xc, z, z2);
                z2 = (*x++ >> 16) * y + (*xc & 0xffff) + carry;
                carry = z2 >> 16;
            } while (x < xae);
            *xc = z2;
        }
    }
    for (xc0 = c->_x, xc = xc0 + wc; wc > 0 && !*--xc; --wc)
        ;
    c->_wds = wc;
    return c;
}

/* b = b * 5^k; le potenze 5^4, 5^8, 5^16... restano in ptr per le volte successive */
_Bigint *pow5mult(struct _reent *ptr, _Bigint *b, int k)
{
    _Bigint *b1, *p5, *p51;
    int i;
    static const int p05[3] = { 5, 25, 125 };

    if ((i = k & 3) != 0)
        b = multadd(ptr, b, p05[i - 1], 0);

    if (!(k >>= 2))
        return b;
    if (!(p5 = REENT_P5S(ptr))) {
        p5 = REENT_P5S(ptr) = i2b(ptr, 625);
        p5->_next = NULL;
    }
    for (;;) {
        if (k & 1) {
            b1 = mult(ptr, b, p5);
            Bfree(ptr, b);
            b = b1;
        }
        if (!(k >>= 1))
            break;
        if (!(p51 = p5->_next)) {
            p51 = p5->_next = mult(ptr, p5, p5);
            p51->_next = NULL;
        }
        p5 = p51;
    }
    return b;
}

/* b << k (libera b) */
_Bigint *lshift(struct _reent *ptr, _Bigint *b, int k)
{
    int i, k1, n, n1;
    _Bigint *b1;
    u32 *x, *x1, *xe, z;

    n = k >> 5;
    k1 = b->_k;
    n1 = n + b->_wds + 1;
    for (i = b->_maxwds; n1 > i; i <<= 1)
        k1++;
    b1 = Balloc(ptr, k1);
    x1 = b1->_x;
    for (i = 0; i < n; i++)
        *x1++ = 0;
    x = b->_x;
    xe = x + b->_wds;
    if (k &= 0x1f) {
        k1 = 32 - k;
        z = 0;
        do {
            *x1++ = *x << k | z;
            z = *x++ >> k1;
        } while (x < xe);
        if ((*x1 = z) != 0)
            ++n1;
    } else {
        do
            *x1++ = *x++;
        while (x < xe);
    }
    b1->_wds = n1 - 1;
    Bfree(ptr, b);
    return b1;
}

/* confronto: <0, 0, >0 */
int cmp(_Bigint *a, _Bigint *b)
{
    u32 *xa, *xa0, *xb;
    int i, j;

    i = a->_wds;
    j = b->_wds;
    if (i -= j)
        return i;
    xa0 = a->_x;
    xa = xa0 + j;
    xb = b->_x + j;
    for (;;) {
        if (*--xa != *--xb)
            return *xa < *xb ? -1 : 1;
        if (xa <= xa0)
            break;
    }
    return 0;
}

/* |a - b|, con _sign = 1 se a < b */
_Bigint *diff(struct _reent *ptr, _Bigint *a, _Bigint *b)
{
    _Bigint *c;
    int i, wa, wb;
    s32 borrow, y, z;
    u32 *xa, *xae, *xb, *xbe, *xc;

    i = cmp(a, b);
    if (!i) {
        c = Balloc(ptr, 0);
        c->_wds = 1;
        c->_x[0] = 0;
        return c;
    }
    if (i < 0) {
        c = a;
        a = b;
        b = c;
        i = 1;
    } else {
        i = 0;
    }
    c = Balloc(ptr, a->_k);
    c->_sign = i;
    wa = a->_wds;
    xa = a->_x;
    xae = xa + wa;
    wb = b->_wds;
    xb = b->_x;
    xbe = xb + wb;
    xc = c->_x;
    borrow = 0;
    do {
        y = (*xa & 0xffff) - (*xb & 0xffff) + borrow;
        borrow = y >> 16;
        z = (*xa++ >> 16) - (*xb++ >> 16) + borrow;
        borrow = z >> 16;
        Storeinc(xc, z, y);
    } while (xb < xbe);
    while (xa < xae) {
        y = (*xa & 0xffff) + borrow;
        borrow = y >> 16;
        z = (*xa++ >> 16) + borrow;
        borrow = z >> 16;
        Storeinc(xc, z, y);
    }
    while (!*--xc)
        wa--;
    c->_wds = wa;
    return c;
}

/* Bigint -> double con mantissa normalizzata in [1, 2); *e = numero di bit */
f64 b2d(_Bigint *a, int *e)
{
    u32 *xa, *xa0, w, y, z;
    int k;
    double_union d;

    xa0 = a->_x;
    xa = xa0 + a->_wds;
    y = *--xa;
    k = hi0bits(y);
    *e = 32 - k;
    if (k < Ebits) {
        word0(d) = Exp_1 | y >> (Ebits - k);
        w = xa > xa0 ? *--xa : 0;
        word1(d) = y << ((32 - Ebits) + k) | w >> (Ebits - k);
        return d.d;
    }
    z = xa > xa0 ? *--xa : 0;
    if (k -= Ebits) {
        word0(d) = Exp_1 | y << k | z >> (32 - k);
        y = xa > xa0 ? *--xa : 0;
        word1(d) = z << k | y >> (32 - k);
    } else {
        word0(d) = Exp_1 | y;
        word1(d) = z;
    }
    return d.d;
}

/* double -> Bigint intero b, con valore = b * 2^*e; *bits = bit significativi */
_Bigint *d2b(struct _reent *ptr, f64 _d, int *e, int *bits)
{
    double_union d;
    _Bigint *b;
    int de, i, k;
    u32 *x, y, z;

    d.d = _d;
    b = Balloc(ptr, 1);
    x = b->_x;

    z = word0(d) & Frac_mask;
    word0(d) &= 0x7fffffff; /* il segno non conta */
    if ((de = (int)(word0(d) >> Exp_shift)) != 0)
        z |= Exp_msk1;
    if ((y = word1(d)) != 0) {
        if ((k = lo0bits(&y)) != 0) {
            x[0] = y | z << (32 - k);
            z >>= k;
        } else {
            x[0] = y;
        }
        i = b->_wds = (x[1] = z) ? 2 : 1;
    } else {
        k = lo0bits(&z);
        x[0] = z;
        i = b->_wds = 1;
        k += 32;
    }
    if (de) {
        *e = de - Bias - (P - 1) + k;
        *bits = P - k;
    } else {
        *e = de - Bias - (P - 1) + 1 + k;
        *bits = 32 * i - hi0bits(x[i - 1]);
    }
    return b;
}

/* 10^dig */
f64 _mprec_log10(int dig)
{
    f64 v = 1.0;

    if (dig < 24)
        return __mprec_tens[dig];
    while (dig > 0) {
        v *= 10;
        dig--;
    }
    return v;
}
