/* Il motore di printf: interpreta il formato (%d, %s, %f, ...) e scrive il risultato
 * nel FILE a pezzi (lista di __siov svuotata con __sprint ogni 8 pezzi).
 * Da vfprintf.c di newlib (origine BSD). */
#include "libc.h"

typedef u16 wchar_t16;
typedef struct { int __count; int __value; } mbstate_t;
int _wcrtomb_r(struct _reent *ptr, char *s, wchar_t16 wc, mbstate_t *ps);
size_t _wcsrtombs_r(struct _reent *r, char *dst, const wchar_t16 **src, size_t len, mbstate_t *ps);

#define BUF     348  /* spazio per le cifre di un numero */
#define DEFPREC 6    /* precisione predefinita di %e %f %g */
#define NIOV    8    /* pezzi accumulati prima di scrivere */
#define PADSIZE 16

/* opzioni del formato */
#define ALT       0x001 /* '#' */
#define HEXPREFIX 0x002 /* aggiunge 0x / 0X */
#define LADJUST   0x004 /* '-': allinea a sinistra */
#define LONGDBL   0x008 /* 'L' */
#define LONGINT   0x010 /* 'l' */
#define QUADINT   0x020 /* 'll' o 'q' */
#define SHORTINT  0x040 /* 'h' */
#define ZEROPAD   0x080 /* '0' */
#define FPT       0x100 /* numero con la virgola */

enum { OCT, DEC, HEX };

#define to_digit(c) ((c) - '0')
#define is_digit(c) ((unsigned)to_digit(c) <= 9)
#define to_char(n)  ((n) + '0')

static const char blanks[PADSIZE] = { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                                      ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' };
static const char zeroes[PADSIZE] = { '0', '0', '0', '0', '0', '0', '0', '0',
                                      '0', '0', '0', '0', '0', '0', '0', '0' };

/* Cifre di value per %e/%f/%g (tramite _dtoa_r); per %e/%f e con '#' completa con zeri. */
static inline char *cvt(struct _reent *data, f64 value, int ndigits, int flags, char *sign,
                        int *decpt, int ch, int *length)
{
    int mode, dsgn;
    char *digits, *bp, *rve;
    union { f64 d; u32 i[2]; } tmp;

    if (ch == 'f') {
        mode = 3; /* ndigits cifre dopo la virgola */
    } else {
        /* %e ha una cifra in più (prima della virgola) */
        if (ch == 'e' || ch == 'E')
            ndigits++;
        mode = 2; /* ndigits cifre significative */
    }
    tmp.d = value;
    if (tmp.i[1] & 0x80000000) { /* anche -0.0 */
        tmp.i[1] ^= 0x80000000;
        value = tmp.d;
        *sign = '-';
    } else {
        *sign = '\0';
    }
    digits = _dtoa_r(data, value, mode, ndigits, decpt, &dsgn, &rve);
    if ((ch != 'g' && ch != 'G') || (flags & ALT)) {
        /* zeri finali */
        bp = digits + ndigits;
        if (ch == 'f') {
            if (*digits == '0' && value != 0)
                *decpt = -ndigits + 1;
            bp += *decpt;
        }
        if (value == 0) /* dtoa per lo 0 dà una sola cifra */
            rve = bp;
        while (rve < bp)
            *rve++ = '0';
    }
    *length = rve - digits;
    return digits;
}

/* esponente di %e: "e+05", "e-123", ... */
static inline int exponent(char *p0, int exp, int fmtch)
{
    char *p, *t;
    char expbuf[40];

    p = p0;
    *p++ = fmtch;
    if (exp < 0) {
        exp = -exp;
        *p++ = '-';
    } else {
        *p++ = '+';
    }
    t = expbuf + 40;
    if (exp > 9) {
        do {
            *--t = to_char(exp % 10);
        } while ((exp /= 10) > 9);
        *--t = to_char(exp);
        for (; t < expbuf + 40; *p++ = *t++)
            ;
    } else {
        *p++ = '0';
        *p++ = to_char(exp);
    }
    return p - p0;
}

int _vfprintf_r(struct _reent *data, FILE *fp, const char *fmt0, va_list ap)
{
    char *fmt;           /* posizione nel formato */
    int ch;              /* carattere del formato */
    int n, m;
    char *cp;            /* testo da stampare */
    struct __siov *iovp;
    int flags;
    int ret;             /* caratteri scritti finora */
    int width;           /* larghezza minima */
    int prec;            /* precisione ('.'), -1 se assente */
    char sign;           /* segno ('-', '+', ' ' o 0) */
    char *decimal_point = *(char **)localeconv();
    char softsign;       /* segno di un numero con la virgola */
    f64 _fpvalue = 0;
    int expt;            /* esponente decimale da cvt */
    int expsize = 0;     /* lunghezza dell'esponente */
    int ndig = 0;        /* cifre da cvt */
    char expstr[7];
    u64 _uquad;          /* intero da stampare */
    int base;
    int dprec;           /* precisione dei numeri interi */
    int realsz;          /* lunghezza del campo senza la larghezza */
    int size;            /* lunghezza di cp */
    char *xdigs = NULL;  /* cifre esadecimali */
    struct __suio uio;
    struct __siov iov[NIOV];
    char buf[BUF];
    char ox[2];          /* "0x" o cifra + '.' */
    char *malloc_buf = NULL; /* stringa convertita da %S */

/* aggiunge un pezzo; ogni NIOV pezzi li scrive */
#define PRINT(ptr, len)                              \
    {                                                \
        iovp->iov_base = (ptr);                      \
        iovp->iov_len = (len);                       \
        uio.uio_resid += (len);                      \
        iovp++;                                      \
        if (++uio.uio_iovcnt >= NIOV) {              \
            if (__sprint(fp, &uio))                  \
                goto error;                          \
            iovp = iov;                              \
        }                                            \
    }
#define PAD(howmany, with)                           \
    {                                                \
        if ((n = (howmany)) > 0) {                   \
            while (n > PADSIZE) {                    \
                PRINT(with, PADSIZE);                \
                n -= PADSIZE;                        \
            }                                        \
            PRINT(with, n);                          \
        }                                            \
    }
#define FLUSH()                                      \
    {                                                \
        if (uio.uio_resid && __sprint(fp, &uio))     \
            goto error;                              \
        uio.uio_iovcnt = 0;                          \
        iovp = iov;                                  \
    }

/* argomenti interi con segno / senza segno, secondo 'h', 'l', 'll' */
#define SARG()                                                   \
    (flags & QUADINT ? va_arg(ap, s64)                           \
     : flags & LONGINT ? (s64)va_arg(ap, s32)                    \
     : flags & SHORTINT ? (s64)(short)va_arg(ap, int)            \
     : (s64)va_arg(ap, int))
#define UARG()                                                   \
    (flags & QUADINT ? va_arg(ap, u64)                           \
     : flags & LONGINT ? (u64)va_arg(ap, u32)                    \
     : flags & SHORTINT ? (u64)(u16)va_arg(ap, int)              \
     : (u64)va_arg(ap, unsigned int))

    /* bisogna poter scrivere */
    if (((fp->_flags & __SWR) == 0 || fp->_bf._base == NULL) && __swsetup(fp))
        return -1;

    /* file senza buffer: meglio un buffer temporaneo */
    if ((fp->_flags & (__SNBF | __SWR | __SRW)) == (__SNBF | __SWR) && fp->_file >= 0)
        return __sbprintf(fp, fmt0, ap);

    fmt = (char *)fmt0;
    uio.uio_iov = iovp = iov;
    uio.uio_resid = 0;
    uio.uio_iovcnt = 0;
    ret = 0;

    for (;;) {
        /* testo normale fino al prossimo % */
        for (cp = fmt; (ch = *fmt) != '\0' && ch != '%'; fmt++)
            ;
        if ((m = fmt - cp) != 0) {
            PRINT(cp, m);
            ret += m;
        }
        if (ch == '\0')
            goto done;
        fmt++; /* salta il % */

        flags = 0;
        dprec = 0;
        width = 0;
        prec = -1;
        sign = '\0';

    rflag:
        ch = *fmt++;
    reswitch:
        switch (ch) {
        case ' ':
            /* "se ci sono sia ' ' sia '+', ' ' viene ignorato" (ANSI) */
            if (!sign)
                sign = ' ';
            goto rflag;
        case '#':
            flags |= ALT;
            goto rflag;
        case '*':
            /* larghezza negativa = '-' più larghezza positiva */
            if ((width = va_arg(ap, int)) >= 0)
                goto rflag;
            width = -width;
            /* continua */
        case '-':
            flags |= LADJUST;
            goto rflag;
        case '+':
            sign = '+';
            goto rflag;
        case '.':
            if ((ch = *fmt++) == '*') {
                n = va_arg(ap, int);
                prec = n < 0 ? -1 : n;
                goto rflag;
            }
            n = 0;
            while (is_digit(ch)) {
                n = 10 * n + to_digit(ch);
                ch = *fmt++;
            }
            prec = n < 0 ? -1 : n;
            goto reswitch;
        case '0':
            /* "0 prima della larghezza = riempi di zeri" (ANSI) */
            flags |= ZEROPAD;
            goto rflag;
        case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            n = 0;
            do {
                n = 10 * n + to_digit(ch);
                ch = *fmt++;
            } while (is_digit(ch));
            width = n;
            goto reswitch;
        case 'L':
            flags |= LONGDBL;
            goto rflag;
        case 'h':
            flags |= SHORTINT;
            goto rflag;
        case 'l':
            if (*fmt == 'l') {
                fmt++;
                flags |= QUADINT;
            } else {
                flags |= LONGINT;
            }
            goto rflag;
        case 'q':
            flags |= QUADINT;
            goto rflag;
        case 'c':
        case 'C':
            cp = buf;
            if (ch == 'C' || (flags & LONGINT)) {
                mbstate_t ps;

                memset(&ps, 0, sizeof(ps));
                if ((size = _wcrtomb_r(data, cp, (wchar_t16)va_arg(ap, int), &ps)) == -1)
                    goto error;
            } else {
                *cp = va_arg(ap, int);
                size = 1;
            }
            sign = '\0';
            break;
        case 'D':
            flags |= LONGINT;
            /* continua */
        case 'd':
        case 'i':
            _uquad = SARG();
            if ((s64)_uquad < 0) {
                _uquad = -_uquad;
                sign = '-';
            }
            base = DEC;
            goto number;
        case 'e':
        case 'E':
        case 'f':
        case 'g':
        case 'G':
            if (prec == -1)
                prec = DEFPREC;
            else if ((ch == 'g' || ch == 'G') && prec == 0)
                prec = 1;
            _fpvalue = va_arg(ap, f64); /* (long double == double) */

            /* infinito e NaN prima dei calcoli sulla precisione */
            if (isinf(_fpvalue)) {
                if (_fpvalue < 0)
                    sign = '-';
                cp = "Inf";
                size = 3;
                break;
            }
            if (isnan(_fpvalue)) {
                cp = "NaN";
                size = 3;
                break;
            }
            flags |= FPT;
            cp = cvt(data, _fpvalue, prec, flags, &softsign, &expt, ch, &ndig);

            if (ch == 'g' || ch == 'G') {
                if (expt <= -4 || expt > prec)
                    ch = (ch == 'g') ? 'e' : 'E';
                else
                    ch = 'g';
            }
            if (ch <= 'e') { /* %e o %E */
                --expt;
                expsize = exponent(expstr, expt, ch);
                size = expsize + ndig;
                if (ndig > 1 || (flags & ALT))
                    ++size;
            } else if (ch == 'f') {
                if (expt > 0) {
                    size = expt;
                    if (prec || (flags & ALT))
                        size += prec + 1;
                } else { /* "0.X" */
                    size = (prec || (flags & ALT)) ? prec + 2 : 1;
                }
            } else if (expt >= ndig) { /* %g senza virgola */
                size = expt;
                if (flags & ALT)
                    ++size;
            } else {
                size = ndig + (expt > 0 ? 1 : 2 - expt);
            }
            if (softsign)
                sign = '-';
            break;
        case 'n':
            if (flags & QUADINT)
                *va_arg(ap, s64 *) = ret;
            else if (flags & LONGINT)
                *va_arg(ap, s32 *) = ret;
            else if (flags & SHORTINT)
                *va_arg(ap, short *) = ret;
            else
                *va_arg(ap, int *) = ret;
            continue; /* non stampa niente */
        case 'O':
            flags |= LONGINT;
            /* continua */
        case 'o':
            _uquad = UARG();
            base = OCT;
            goto nosign;
        case 'p':
            /* il puntatore come numero esadecimale con 0x */
            _uquad = (u32)va_arg(ap, void *);
            base = HEX;
            xdigs = "0123456789abcdef";
            flags |= HEXPREFIX;
            ch = 'x';
            goto nosign;
        case 's':
        case 'S':
            sign = '\0';
            if ((cp = va_arg(ap, char *)) == NULL) {
                cp = "(null)";
                size = 6;
            } else if (ch == 'S' || (flags & LONGINT)) {
                /* stringa di caratteri larghi: la converte in multibyte */
                mbstate_t ps;
                const wchar_t16 *wcp = (const wchar_t16 *)cp;

                size = m = 0;
                memset(&ps, 0, sizeof(ps));
                if (prec >= 0) {
                    /* solo i caratteri che stanno in prec byte */
                    for (;;) {
                        if (wcp[m] == 0)
                            break;
                        if ((n = _wcrtomb_r(data, buf, wcp[m], &ps)) == -1)
                            goto error;
                        if (n + size > prec)
                            break;
                        m += 1;
                        size += n;
                        if (size == prec)
                            break;
                    }
                } else {
                    if ((size = _wcsrtombs_r(data, NULL, &wcp, 0, &ps)) == -1)
                        goto error;
                    wcp = (const wchar_t16 *)cp;
                }
                if (size == 0)
                    break;
                if ((malloc_buf = _malloc_r(data, size + 1)) == NULL)
                    goto error;
                memset(&ps, 0, sizeof(ps));
                if (_wcsrtombs_r(data, malloc_buf, &wcp, size, &ps) != (size_t)size)
                    goto error;
                cp = malloc_buf;
                cp[size] = '\0';
            } else if (prec >= 0) {
                /* niente strlen: lo 0 si cerca solo nei primi prec caratteri */
                char *p = memchr(cp, 0, prec);

                if (p != NULL) {
                    size = p - cp;
                    if (size > prec)
                        size = prec;
                } else {
                    size = prec;
                }
            } else {
                size = strlen(cp);
            }
            break;
        case 'U':
            flags |= LONGINT;
            /* continua */
        case 'u':
            _uquad = UARG();
            base = DEC;
            goto nosign;
        case 'X':
            xdigs = "0123456789ABCDEF";
            goto hex;
        case 'x':
            xdigs = "0123456789abcdef";
        hex:
            _uquad = UARG();
            base = HEX;
            /* 0x davanti solo se il numero non è 0 */
            if ((flags & ALT) && _uquad != 0)
                flags |= HEXPREFIX;
        nosign:
            sign = '\0';
        number:
            /* "con una precisione il flag 0 si ignora" (ANSI) */
            if ((dprec = prec) >= 0)
                flags &= ~ZEROPAD;
            /* "lo 0 con precisione 0 non dà caratteri" (ANSI) */
            cp = buf + BUF;
            if (_uquad != 0 || prec != 0) {
                switch (base) {
                case OCT:
                    do {
                        *--cp = to_char(_uquad & 7);
                        _uquad >>= 3;
                    } while (_uquad);
                    /* '#' per l'ottale: uno 0 davanti */
                    if ((flags & ALT) && *cp != '0')
                        *--cp = '0';
                    break;
                case DEC:
                    /* molti numeri hanno una cifra sola */
                    while (_uquad >= 10) {
                        *--cp = to_char(_uquad % 10);
                        _uquad /= 10;
                    }
                    *--cp = to_char(_uquad);
                    break;
                case HEX:
                    do {
                        *--cp = xdigs[_uquad & 15];
                        _uquad >>= 4;
                    } while (_uquad);
                    break;
                default:
                    cp = "bug in vfprintf: bad base";
                    size = strlen(cp);
                    goto skipsize;
                }
            } else if (base == OCT && (flags & ALT)) {
                /* printf("%#.0o", 0) stampa "0" */
                *--cp = '0';
            }
            size = buf + BUF - cp;
        skipsize:
            break;
        default: /* "%?" stampa ?, se non è lo 0 finale */
            if (ch == '\0')
                goto done;
            cp = buf;
            *cp = ch;
            size = 1;
            sign = '\0';
            break;
        }

        /* Qui cp punta al testo da stampare (size caratteri). Prima eventuali spazi a
         * sinistra, poi segno/prefisso, zeri di riempimento, il testo, e spazi a destra. */
        realsz = dprec > size ? dprec : size;
        if (sign)
            realsz++;
        else if (flags & HEXPREFIX)
            realsz += 2;

        /* spazi a sinistra */
        if ((flags & (LADJUST | ZEROPAD)) == 0)
            PAD(width - realsz, blanks);

        /* prefisso */
        if (sign) {
            PRINT(&sign, 1);
        } else if (flags & HEXPREFIX) {
            ox[0] = '0';
            ox[1] = ch;
            PRINT(ox, 2);
        }

        /* zeri a sinistra */
        if ((flags & (LADJUST | ZEROPAD)) == ZEROPAD)
            PAD(width - realsz, zeroes);

        /* zeri della precisione degli interi */
        PAD(dprec - size, zeroes);

        /* il testo */
        if ((flags & FPT) == 0) {
            PRINT(cp, size);
        } else { /* numero con la virgola: si mettono insieme i pezzi */
            if (ch >= 'f') { /* %f o %g */
                if (_fpvalue == 0) {
                    /* dtoa dà "0" */
                    PRINT("0", 1);
                    if (expt < ndig || (flags & ALT) != 0) {
                        PRINT(decimal_point, 1);
                        PAD(ndig - 1, zeroes);
                    }
                } else if (expt <= 0) {
                    PRINT("0", 1);
                    if (expt || ndig) {
                        PRINT(decimal_point, 1);
                        PAD(-expt, zeroes);
                        PRINT(cp, ndig);
                    }
                } else if (expt >= ndig) {
                    PRINT(cp, ndig);
                    PAD(expt - ndig, zeroes);
                    if (flags & ALT)
                        PRINT(".", 1);
                } else {
                    PRINT(cp, expt);
                    cp += expt;
                    PRINT(".", 1);
                    PRINT(cp, ndig - expt);
                }
            } else { /* %e o %E */
                if (ndig > 1 || (flags & ALT)) {
                    ox[0] = *cp++;
                    ox[1] = '.';
                    PRINT(ox, 2);
                    if (_fpvalue != 0) {
                        PRINT(cp, ndig - 1);
                    } else { /* 0.000... */
                        PAD(ndig - 1, zeroes);
                    }
                } else { /* XeYYY */
                    PRINT(cp, 1);
                }
                PRINT(expstr, expsize);
            }
        }

        /* spazi a destra */
        if (flags & LADJUST)
            PAD(width - realsz, blanks);

        ret += width > realsz ? width : realsz;

        FLUSH();
        if (malloc_buf != NULL) {
            free(malloc_buf);
            malloc_buf = NULL;
        }
    }
done:
    FLUSH();
error:
    if (malloc_buf != NULL)
        free(malloc_buf);
    return (fp->_flags & __SERR) ? -1 : ret;
}
