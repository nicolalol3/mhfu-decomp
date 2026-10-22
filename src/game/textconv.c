/* Conversione dei testi fra Shift-JIS, UTF-8 e UTF-16 (stringhe a 16 bit).
 * Sono metodi di una classe C++: il primo argomento ("self") non viene usato.
 * (Nomi ipotetici.) */
#include "libc.h"

extern const s16 D_08910968[];   /* tabella codice JIS -> Unicode */
extern void *D_089CBBE8;          /* istanza globale del convertitore */
int memcmp(const void *a, const void *b, size_t n);

/* legge un carattere Shift-JIS da *pp e lo restituisce come codice JIS
 * (bit 15 = piano 2 per i codici 0xF0-0xFC); 0x3F ('?') se non valido */
u32 sjis_decode(void *self, const u8 **pp)
{
    const u8 *p = *pp;
    u32 c = *p++;
    int plane2, row, col;

    if (c >= 0x80 && (c < 0xA0 || c >= 0xE0) && c < 0xFD) {
        plane2 = 0;
        row = 0;
        col = 0;
        if (c >= 0x81 && c <= 0x9F) {
            row = (c - 0x81) * 2 + 1;
        } else if (c >= 0xE0 && c <= 0xEF) {
            row = (c - 0xE0) * 2 + 0x3F;
        } else if (c >= 0xF0 && c < 0xFD) {
            plane2 = 1;
            row = (c - 0xF0) * 2 + 7;
            if (c > 0xF3)
                row = (c - 0xF0) * 2 + 0x45;
        }
        if (row != 0) {
            c = *p++;
            if (c >= 0x40 && c <= 0x7E) {
                col = c - 0x3F;
            } else if (c >= 0x80 && c <= 0x9E) {
                col = c - 0x40;
            } else if (c > 0x9E && c < 0xFD) {
                col = c - 0x9E;
                row++;
            }
            if (plane2) {
                if (row == 7)
                    row = 1;
                else if (row == 0xB)
                    row = 5;
                else if (row == 0x4D)
                    row = 0xF;
            }
        }
        if (row == 0 || col == 0)
            c = 0x3F;
        else
            c = ((col + 0x20) | (plane2 << 15) | ((row + 0x20) << 8)) & 0xFFFF;
    }
    *pp = p;
    return c;
}

int func_088167E0(void *self, u32 c);
int func_088167EC(void *self, u32 c);

/* legge un carattere UTF-8 da *pp; 0x3F se non valido o fuori dai 16 bit */
u32 utf8_decode(void *self, const u8 **pp)
{
    const u8 *p = *pp;
    u32 c = *p++;

    if (c >= 0x80) {
        if (c < 0xC2) {
            c = 0x3F;
        } else if (c < 0xF5) {
            u32 lim = 0x40, mask;

            c &= 0x7F;
            do {
                mask = lim - 1;
                lim <<= 5;
                c = (c & mask) << 6 | (*p++ & 0x3F);
            } while (lim <= c);
        } else {
            c = 0x3F;
        }
    }
    *pp = p;
    if (!func_088167E0(self, c))
        return 0x3F;
    return c & 0xFFFF;
}

/* scrive il carattere c in UTF-8 a *pp e avanza il puntatore */
void utf8_encode(void *self, u8 **pp, u32 c)
{
    u8 *p = *pp, *end, *q;
    int ok = func_088167EC(self, c);
    u32 v = ok ? c : 0x3F;

    if (!ok || c < 0x80) {
        *p = v;
        end = p + 1;
    } else {
        u32 lim = 0x800, mask;
        int n = 1;

        if (v > 0x7FF) {
            do {
                lim <<= 5;
                n++;
            } while (lim <= v);
        }
        end = p + n + 1;
        mask = (1 << (7 - n)) - 1;
        q = end;
        for (; n != 0; n--) {
            *--q = (v & 0x3F) | 0x80;
            v >>= 6;
        }
        q[-1] = ~(u8)mask | ((u8)v & (u8)((int)(mask & 0xFF) >> 1));
    }
    *pp = end;
}

/* codice JIS -> Unicode (0x3F se non presente nella tabella) */
u16 jis_to_unicode(void *self, u32 c)
{
    u16 u;

    if (c == 0)
        return 0;
    u = D_08910968[c & 0xFFFF];
    if (u == 0)
        u = 0x3F;
    return u;
}

/* converte una stringa Shift-JIS in UTF-8 (al massimo size-1 byte + zero finale);
 * restituisce il numero di caratteri convertiti */
int sjis_to_utf8(void *self, u8 *dst, int size, const u8 *src)
{
    u8 buf[4], *w, *s;
    int count = 0, n;
    u32 c;

    if (size <= 0)
        return 0;
    size--;
    c = sjis_decode(self, &src) & 0xFFFF;
    if (c == 0) {
        *dst = 0;
        return 0;
    }
    do {
        u32 u;

        w = buf;
        u = (u16)jis_to_unicode(self, c);
        if (u == 0)
            break;
        utf8_encode(self, &w, u);
        n = w - buf;
        if (size < n)
            break;
        size -= n;
        for (s = buf; n != 0; n--)
            *dst++ = *s++;
        count++;
        c = sjis_decode(self, &src) & 0xFFFF;
    } while (c != 0);
    *dst = 0;
    return count;
}

/* converte una stringa UTF-8 in UTF-16; restituisce il numero di caratteri */
int utf8_to_utf16(void *self, s16 *dst, int size, const u8 *src)
{
    int count = 0;
    s16 c;

    if (size <= 0)
        return 0;
    c = utf8_decode(self, &src);
    if (c == 0) {
        *dst = 0;
        return 0;
    }
    do {
        size--;
        if (size <= 0)
            break;
        *dst++ = c;
        count++;
        c = utf8_decode(self, &src);
    } while (c != 0);
    *dst = 0;
    return count;
}

int func_088167D4(void *self, u32 c)
{
    return c < 0x80000000;
}

int func_088167E0(void *self, u32 c)
{
    return c < 0x10000;
}

int func_088167EC(void *self, u32 c)
{
    return func_088167D4(self, c);
}

/* lunghezza di una stringa a 16 bit */
int wstr_len(void *self, const s16 *s)
{
    int n = 0;

    while (*s != 0) {
        s++;
        n++;
    }
    return n;
}

/* cerca la stringa a 16 bit "needle" dentro "hay" */
const s16 *wstr_find(void *self, const s16 *hay, const s16 *needle)
{
    int left = wstr_len(D_089CBBE8, hay);
    int nlen = wstr_len(D_089CBBE8, needle);

    if (left < nlen)
        return NULL;
    while (*hay != 0) {
        if ((u16)*hay == (u16)*needle) {
            if (left < nlen)
                return NULL;
            if (memcmp(hay, needle, nlen * 2) == 0)
                return hay;
        }
        left--;
        hay++;
    }
    return NULL;
}

/* il carattere a 16 bit è una lettera minuscola a-z? */
int wchar_is_lower(void *self, u32 c_arg)
{
    u16 c = c_arg;
    return c >= 0x61 && c < 0x7B;
}

/* lunghezza in byte della sequenza UTF-8 che inizia con il byte c (0 se non valido) */
int utf8_seq_len(void *self, u32 c_arg)
{
    u8 c = c_arg;
    if (c == 0)
        return 0;
    if ((c & 0x80) == 0)
        return 1;
    if ((c & 0xE0) == 0xC0)
        return 2;
    if ((c & 0xF0) == 0xE0)
        return 3;
    if ((c & 0xF8) == 0xF0)
        return 4;
    return 0;
}

/* numero di byte di una stringa UTF-8 (somma delle sequenze) */
int utf8_byte_len(void *self, const u8 *s)
{
    int n = 0;

    while (*s != 0) {
        int len = utf8_seq_len(self, *s);

        n += len;
        s += len;
    }
    return n;
}
