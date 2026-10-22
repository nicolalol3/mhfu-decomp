/* Conversione double -> cifre decimali (usata da printf per %f, %e, %g).
 * Da dtoa.c di newlib (David M. Gay, "Correctly Rounded Binary-Decimal Conversions"):
 * prima prova con l'aritmetica dei double, se non basta usa gli interi grandi (mprec.c). */
#include "mprec.h"

#define Ten_pmax  22
#define Quick_max 14
#define Int_max   14
#define Bletch    0x10
#define n_bigtens 5
#define Bndry_mask 0xfffff
#define Log2P     1
#define Frac_mask1 0xfffff
#define Exp_11    0x3ff00000

int __gedf2(f64 a, f64 b); /* confronto della libreria dei double: >= 0 se a >= b */

/* nella struct _reent: buffer del risultato precedente */
#define REENT_RESULT(r)   (*(_Bigint **)((u8 *)(r) + 0x40))
#define REENT_RESULT_K(r) (*(int *)((u8 *)(r) + 0x44))

/* stringhe originali (il chiamante riceve proprio questi indirizzi) */
extern char D_08910428[]; /* "Infinity" */
extern char D_08910434[]; /* "NaN" */
extern char D_08910438[]; /* "0" */

/* Divide b per S (quoziente < 10) lasciando il resto in b. */
int quorem(_Bigint *b, _Bigint *S)
{
    int n;
    s32 borrow, y, z;
    u32 carry, q, ys, si, zs;
    u32 *bx, *bxe, *sx, *sxe;

    n = S->_wds;
    if (b->_wds < n)
        return 0;
    sx = S->_x;
    sxe = sx + --n;
    bx = b->_x;
    bxe = bx + n;
    q = *bxe / (*sxe + 1); /* q <= quoziente vero */
    if (q) {
        borrow = 0;
        carry = 0;
        do {
            si = *sx++;
            ys = (si & 0xffff) * q + carry;
            zs = (si >> 16) * q + (ys >> 16);
            carry = zs >> 16;
            y = (*bx & 0xffff) - (ys & 0xffff) + borrow;
            borrow = y >> 16;
            z = (*bx >> 16) - (zs & 0xffff) + borrow;
            borrow = z >> 16;
            ((u16 *)bx)[1] = (u16)z;
            ((u16 *)bx)[0] = (u16)y;
            bx++;
        } while (sx <= sxe);
        if (!*bxe) {
            bx = b->_x;
            while (--bxe > bx && !*bxe)
                --n;
            b->_wds = n;
        }
    }
    if (cmp(b, S) >= 0) {
        q++;
        borrow = 0;
        carry = 0;
        bx = b->_x;
        sx = S->_x;
        do {
            si = *sx++;
            ys = (si & 0xffff) + carry;
            zs = (si >> 16) + (ys >> 16);
            carry = zs >> 16;
            y = (*bx & 0xffff) - (ys & 0xffff) + borrow;
            borrow = y >> 16;
            z = (*bx >> 16) - (zs & 0xffff) + borrow;
            borrow = z >> 16;
            ((u16 *)bx)[1] = (u16)z;
            ((u16 *)bx)[0] = (u16)y;
            bx++;
        } while (sx <= sxe);
        bx = b->_x;
        bxe = bx + n;
        if (!*bxe) {
            while (--bxe > bx && !*bxe)
                --n;
            b->_wds = n;
        }
    }
    return q;
}

/* Modi (come ecvt/fcvt):
 *  0: cifre minime che riletta danno esattamente d;  1: come 0 (senza arrotondamenti ROUND_BIASED)
 *  2: max(1, ndigits) cifre significative;  3: ndigits cifre dopo la virgola
 *  4, 5: come 2 e 3 ma con il metodo "leftright";  6-9: come 2-5 senza la via veloce.
 * Ritorna le cifre (senza zeri finali), *decpt = posizione della virgola, *sign = segno. */
char *_dtoa_r(struct _reent *ptr, f64 _d, int mode, int ndigits, int *decpt, int *sign, char **rve)
{
    int bbits, b2, b5, be, dig, i, ieps, ilim, ilim0, ilim1, j, j1, k, k0, k_check, leftright,
        m2, m5, s2, s5, spec_case, try_quick, denorm;
    double_union d, d2, eps;
    s32 L;
    u32 x;
    _Bigint *b, *b1, *delta, *mlo, *mhi, *S;
    f64 ds;
    char *s, *s0;

    d.d = _d;
    ilim = ilim1 = 0;
    mlo = mhi = S = NULL;

    if (REENT_RESULT(ptr)) {
        REENT_RESULT(ptr)->_k = REENT_RESULT_K(ptr);
        REENT_RESULT(ptr)->_maxwds = 1 << REENT_RESULT_K(ptr);
        Bfree(ptr, REENT_RESULT(ptr));
        REENT_RESULT(ptr) = NULL;
    }

    if (word0(d) & Sign_bit) {
        *sign = 1;
        word0(d) &= ~Sign_bit;
    } else {
        *sign = 0;
    }

    if ((word0(d) & Exp_mask) == Exp_mask) { /* infinito o NaN */
        *decpt = 9999;
        s = !word1(d) && !(word0(d) & 0xfffff) ? D_08910428 : D_08910434;
        if (rve)
            *rve = s[3] ? s + 8 : s + 3;
        return s;
    }
    if (!(d.d != 0)) {
        *decpt = 1;
        s = D_08910438;
        if (rve)
            *rve = s + 1;
        return s;
    }

    b = d2b(ptr, d.d, &be, &bbits);
    if ((i = (int)(word0(d) >> Exp_shift & (Exp_mask >> Exp_shift))) != 0) {
        d2.d = d.d;
        word0(d2) &= Frac_mask1;
        word0(d2) |= Exp_11;
        i -= Bias;
        denorm = 0;
    } else {
        /* denormalizzato */
        i = bbits + be + (Bias + (P - 1) - 1);
        x = i > 32 ? (word0(d) << (64 - i)) | (word1(d) >> (i - 32)) : (word1(d) << (32 - i));
        d2.d = (s32)x; /* (u32 -> double come nell'originale) */
        if ((s32)x < 0)
            d2.d += 4294967296.0;
        word0(d2) -= 31 * Exp_msk1;
        i -= (Bias + (P - 1) - 1) + 1;
        denorm = 1;
    }
    /* stima di k = floor(log10(d)) */
    ds = (d2.d - 1.5) * 0.289529654602168 + 0.1760912590558 + i * 0.301029995663981;
    k = (int)ds;
    if (ds < 0. && ds != k)
        k--;
    k_check = 1;
    if (k >= 0 && k <= Ten_pmax) {
        if (d.d < __mprec_tens[k])
            k--;
        k_check = 0;
    }
    j = bbits - i - 1;
    if (j >= 0) {
        b2 = 0;
        s2 = j;
    } else {
        b2 = -j;
        s2 = 0;
    }
    if (k >= 0) {
        b5 = 0;
        s5 = k;
        s2 += k;
    } else {
        b2 -= k;
        b5 = -k;
        s5 = 0;
    }
    if (mode < 0 || mode > 9)
        mode = 0;
    try_quick = 1;
    if (mode > 5) {
        mode -= 4;
        try_quick = 0;
    }
    leftright = 1;
    switch (mode) {
    case 0:
    case 1:
        ilim = ilim1 = -1;
        i = 18;
        ndigits = 0;
        break;
    case 2:
        leftright = 0;
        /* continua */
    case 4:
        if (ndigits <= 0)
            ndigits = 1;
        ilim = ilim1 = i = ndigits;
        break;
    case 3:
        leftright = 0;
        /* continua */
    case 5:
        i = ndigits + k + 1;
        ilim = i;
        ilim1 = i - 1;
        if (i <= 0)
            i = 1;
    }
    j = sizeof(u32);
    for (REENT_RESULT_K(ptr) = 0; (int)(sizeof(_Bigint) - sizeof(u32)) + j <= i; j <<= 1)
        REENT_RESULT_K(ptr)++;
    REENT_RESULT(ptr) = Balloc(ptr, REENT_RESULT_K(ptr));
    s = s0 = (char *)REENT_RESULT(ptr);

    if (ilim >= 0 && ilim <= Quick_max && try_quick) {
        /* prova con l'aritmetica dei double */
        i = 0;
        d2.d = d.d;
        k0 = k;
        ilim0 = ilim;
        ieps = 2;
        if (k > 0) {
            ds = __mprec_tens[k & 0xf];
            j = k >> 4;
            if (j & Bletch) { /* evita l'overflow */
                j &= Bletch - 1;
                d.d /= __mprec_bigtens[n_bigtens - 1];
                ieps++;
            }
            for (; j; j >>= 1, i++) {
                if (j & 1) {
                    ieps++;
                    ds *= __mprec_bigtens[i];
                }
            }
            d.d /= ds;
        } else if ((j1 = -k) != 0) {
            d.d *= __mprec_tens[j1 & 0xf];
            for (j = j1 >> 4; j; j >>= 1, i++) {
                if (j & 1) {
                    ieps++;
                    d.d *= __mprec_bigtens[i];
                }
            }
        }
        if (k_check && d.d < 1. && ilim > 0) {
            if (ilim1 <= 0)
                goto fast_failed;
            ilim = ilim1;
            k--;
            d.d *= 10.;
            ieps++;
        }
        eps.d = ieps * d.d + 7.;
        word0(eps) -= (P - 1) * Exp_msk1;
        if (ilim == 0) {
            S = mhi = NULL;
            d.d -= 5.;
            if (d.d > eps.d)
                goto one_digit;
            if (d.d < -eps.d)
                goto no_digits;
            goto fast_failed;
        }
        if (leftright) {
            /* metodo di Steele & White: solo le cifre necessarie */
            eps.d = 0.5 / __mprec_tens[ilim - 1] - eps.d;
            for (i = 0;;) {
                L = d.d;
                d.d -= L;
                *s++ = '0' + (int)L;
                if (d.d < eps.d)
                    goto ret1;
                if (1. - d.d < eps.d)
                    goto bump_up;
                if (++i >= ilim)
                    break;
                eps.d *= 10.;
                d.d *= 10.;
            }
        } else {
            /* ilim cifre, poi le sistema */
            eps.d *= __mprec_tens[ilim - 1];
            for (i = 1;; i++, d.d *= 10.) {
                L = d.d;
                d.d -= L;
                *s++ = '0' + (int)L;
                if (i == ilim) {
                    if (d.d > 0.5 + eps.d)
                        goto bump_up;
                    else if (d.d < 0.5 - eps.d) {
                        while (*--s == '0')
                            ;
                        s++;
                        goto ret1;
                    }
                    break;
                }
            }
        }
    fast_failed:
        s = s0;
        d.d = d2.d;
        k = k0;
        ilim = ilim0;
    }

    /* un intero "piccolo"? */
    if (be >= 0 && k <= Int_max) {
        ds = __mprec_tens[k];
        if (ndigits < 0 && ilim <= 0) {
            S = mhi = NULL;
            /* (d <= 5 * ds: il gioco non ha __ledf2, si confronta con __gedf2 come l'originale) */
            if (ilim < 0 || __gedf2(5 * ds, d.d) >= 0)
                goto no_digits;
            goto one_digit;
        }
        for (i = 1;; i++) {
            L = d.d / ds;
            d.d -= L * ds;
            *s++ = '0' + (int)L;
            if (!(d.d != 0))
                break;
            if (i == ilim) {
                d.d += d.d;
                if (d.d > ds || (d.d == ds && L & 1)) {
                bump_up:
                    while (*--s == '9') {
                        if (s == s0) {
                            k++;
                            *s = '0';
                            break;
                        }
                    }
                    ++*s++;
                }
                break;
            }
            d.d *= 10.;
        }
        goto ret1;
    }

    m2 = b2;
    m5 = b5;
    mhi = mlo = NULL;
    if (leftright) {
        if (mode < 2) {
            i = denorm ? be + (Bias + (P - 1) - 1 + 1) : 1 + P - bbits;
        } else {
            j = ilim - 1;
            if (m5 >= j) {
                m5 -= j;
            } else {
                s5 += j -= m5;
                b5 += j;
                m5 = 0;
            }
            if ((i = ilim) < 0) {
                m2 -= i;
                i = 0;
            }
        }
        b2 += i;
        s2 += i;
        mhi = i2b(ptr, 1);
    }
    if (m2 > 0 && s2 > 0) {
        i = m2 < s2 ? m2 : s2;
        b2 -= i;
        m2 -= i;
        s2 -= i;
    }
    if (b5 > 0) {
        if (leftright) {
            if (m5 > 0) {
                mhi = pow5mult(ptr, mhi, m5);
                b1 = mult(ptr, mhi, b);
                Bfree(ptr, b);
                b = b1;
            }
            if ((j = b5 - m5) != 0)
                b = pow5mult(ptr, b, j);
        } else {
            b = pow5mult(ptr, b, b5);
        }
    }
    S = i2b(ptr, 1);
    if (s5 > 0)
        S = pow5mult(ptr, S, s5);

    /* caso speciale: d è una potenza di 2 normalizzata */
    spec_case = 0;
    if (mode < 2) {
        if (!word1(d) && !(word0(d) & Bndry_mask) && (word0(d) & Exp_mask)) {
            b2 += Log2P;
            s2 += Log2P;
            spec_case = 1;
        }
    }

    /* il divisore deve avere 4 bit a zero in testa (per quorem) */
    if ((i = ((s5 ? 32 - hi0bits(S->_x[S->_wds - 1]) : 1) + s2) & 0x1f) != 0)
        i = 32 - i;
    if (i > 4) {
        i -= 4;
        b2 += i;
        m2 += i;
        s2 += i;
    } else if (i < 4) {
        i += 28;
        b2 += i;
        m2 += i;
        s2 += i;
    }
    if (b2 > 0)
        b = lshift(ptr, b, b2);
    if (s2 > 0)
        S = lshift(ptr, S, s2);
    if (k_check) {
        if (cmp(b, S) < 0) {
            k--;
            b = multadd(ptr, b, 10, 0); /* la stima di k era sbagliata */
            if (leftright)
                mhi = multadd(ptr, mhi, 10, 0);
            ilim = ilim1;
        }
    }
    if (ilim <= 0 && mode > 2) {
        if (ilim < 0 || cmp(b, S = multadd(ptr, S, 5, 0)) <= 0) {
        no_digits:
            k = -1 - ndigits;
            goto ret;
        }
    one_digit:
        *s++ = '1';
        k++;
        goto ret;
    }
    if (leftright) {
        if (m2 > 0)
            mhi = lshift(ptr, mhi, m2);

        mlo = mhi;
        if (spec_case) {
            mhi = Balloc(ptr, mhi->_k);
            Bcopy(mhi, mlo);
            mhi = lshift(ptr, mhi, Log2P);
        }

        for (i = 1;; i++) {
            dig = quorem(b, S) + '0';
            /* abbiamo già la stringa più corta che riletta dà d? */
            j = cmp(b, mlo);
            delta = diff(ptr, S, mhi);
            j1 = delta->_sign ? 1 : cmp(b, delta);
            Bfree(ptr, delta);
            if (j1 == 0 && !mode && !(word1(d) & 1)) {
                if (dig == '9')
                    goto round_9_up;
                if (j > 0)
                    dig++;
                *s++ = dig;
                goto ret;
            }
            if (j < 0 || (j == 0 && !mode && !(word1(d) & 1))) {
                if (j1 > 0) {
                    b = lshift(ptr, b, 1);
                    j1 = cmp(b, S);
                    if ((j1 > 0 || (j1 == 0 && dig & 1)) && dig++ == '9')
                        goto round_9_up;
                }
                *s++ = dig;
                goto ret;
            }
            if (j1 > 0) {
                if (dig == '9') {
                round_9_up:
                    *s++ = '9';
                    goto roundoff;
                }
                *s++ = dig + 1;
                goto ret;
            }
            *s++ = dig;
            if (i == ilim)
                break;
            b = multadd(ptr, b, 10, 0);
            if (mlo == mhi) {
                mlo = mhi = multadd(ptr, mhi, 10, 0);
            } else {
                mlo = multadd(ptr, mlo, 10, 0);
                mhi = multadd(ptr, mhi, 10, 0);
            }
        }
    } else {
        for (i = 1;; i++) {
            *s++ = dig = quorem(b, S) + '0';
            if (i >= ilim)
                break;
            b = multadd(ptr, b, 10, 0);
        }
    }

    /* arrotonda l'ultima cifra */
    b = lshift(ptr, b, 1);
    j = cmp(b, S);
    if (j > 0 || (j == 0 && dig & 1)) {
    roundoff:
        while (*--s == '9') {
            if (s == s0) {
                k++;
                *s++ = '1';
                goto ret;
            }
        }
        ++*s++;
    } else {
        while (*--s == '0')
            ;
        s++;
    }
ret:
    Bfree(ptr, S);
    if (mhi) {
        if (mlo && mlo != mhi)
            Bfree(ptr, mlo);
        Bfree(ptr, mhi);
    }
ret1:
    Bfree(ptr, b);
    *s = 0;
    *decpt = k + 1;
    if (rve)
        *rve = s;
    return s0;
}
