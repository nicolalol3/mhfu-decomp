/* Conversion of texts between Shift-JIS, UTF-8 and UTF-16 (16-bit strings).
 * These are methods of a C++ class: the first argument ("self") is not used.
 * (Guessed names.) */
#include "libc.h"

extern const s16 D_08910968[];   /* table: JIS code -> Unicode */
extern void *D_089CBBE8;          /* global instance of the converter */
int memcmp(const void *a, const void *b, size_t n);

/* reads a Shift-JIS character from *pp and returns it as a JIS code
 * (bit 15 = plane 2 for codes 0xF0-0xFC); 0x3F ('?') if invalid */
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

/* reads a UTF-8 character from *pp; 0x3F if invalid or outside the 16 bits */
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

/* writes character c in UTF-8 at *pp and advances the pointer */
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

/* JIS code -> Unicode (0x3F if not present in the table) */
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

/* converts a Shift-JIS string to UTF-8 (at most size-1 bytes + final zero);
 * returns the number of characters converted */
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

/* converts a UTF-8 string to UTF-16; returns the number of characters */
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

/* length of a 16-bit string */
int wstr_len(void *self, const s16 *s)
{
    int n = 0;

    while (*s != 0) {
        s++;
        n++;
    }
    return n;
}

/* searches for the 16-bit string "needle" inside "hay" */
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

/* is the 16-bit character a lowercase letter a-z? */
int wchar_is_lower(void *self, u32 c_arg)
{
    u16 c = c_arg;
    return c >= 0x61 && c < 0x7B;
}

/* length in bytes of the UTF-8 sequence starting with byte c (0 if invalid) */
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

/* number of bytes of a UTF-8 string (sum of the sequences) */
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
