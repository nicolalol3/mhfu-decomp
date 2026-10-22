/* Numeri interi grandi ("Bigint") usati per convertire i double in testo (dtoa) con
 * precisione esatta. Da mprec.c di newlib (David M. Gay). */
#ifndef MPREC_H
#define MPREC_H
#include "libc.h"

typedef struct _Bigint {
    struct _Bigint *_next;  /* lista dei liberi / catena delle potenze di 5 */
    int _k;                 /* può contenere 1 << _k parole */
    int _maxwds;
    int _sign;
    int _wds;               /* parole usate */
    u32 _x[1];              /* cifre in base 2^32, dalla meno significativa */
} _Bigint;

/* nella struct _reent */
#define REENT_P5S(r)      (*(_Bigint **)((u8 *)(r) + 0x48))  /* potenze 5^(4*2^n) */
#define REENT_FREELIST(r) (*(_Bigint ***)((u8 *)(r) + 0x4C)) /* liste dei Bigint liberi */

/* parti di un double (little endian: word0 = metà alta con segno ed esponente) */
typedef union { f64 d; u32 i[2]; } double_union;
#define word0(x) ((x).i[1])
#define word1(x) ((x).i[0])

#define Exp_shift  20
#define Exp_msk1   0x100000
#define Exp_mask   0x7ff00000
#define Frac_mask  0xfffff
#define Exp_1      0x3ff00000
#define Bias       1023
#define P          53
#define Ebits      11
#define Sign_bit   0x80000000

#define Bcopy(x, y) memcpy(&(x)->_sign, &(y)->_sign, (y)->_wds * sizeof(u32) + 2 * sizeof(u32))

_Bigint *Balloc(struct _reent *ptr, int k);
void Bfree(struct _reent *ptr, _Bigint *v);
_Bigint *multadd(struct _reent *ptr, _Bigint *b, int m, int a);
int hi0bits(u32 x);
int lo0bits(u32 *y);
_Bigint *i2b(struct _reent *ptr, int i);
_Bigint *mult(struct _reent *ptr, _Bigint *a, _Bigint *b);
_Bigint *pow5mult(struct _reent *ptr, _Bigint *b, int k);
_Bigint *lshift(struct _reent *ptr, _Bigint *b, int k);
int cmp(_Bigint *a, _Bigint *b);
_Bigint *diff(struct _reent *ptr, _Bigint *a, _Bigint *b);
f64 b2d(_Bigint *a, int *e);
_Bigint *d2b(struct _reent *ptr, f64 d, int *e, int *bits);
f64 _mprec_log10(int dig);

extern const f64 __mprec_tens[];   /* 1e0 .. 1e22 */
extern const f64 __mprec_bigtens[];  /* 1e16, 1e32, 1e64, 1e128, 1e256 */
extern const f64 __mprec_tinytens[]; /* 1e-16 ... 1e-256 */

#endif
