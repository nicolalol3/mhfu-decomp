/* The font g_font (5): the formatter of the texts. Like vsnprintf with the arguments in an
 * array of words; each conversion is done by sprintf on a copy of its "%..." part. Extra
 * conversions: "%N%" N times '%', "%Nb" the N low bits in binary. The text between the
 * conversions is copied whole characters at a time (UTF-8 or Shift-JIS). (Guessed names.) */
#include "libc.h"

extern void *D_089CBBE8;          /* text converter */

void *memcpy(void *d, const void *s, size_t n);
char *strcpy(char *d, const char *s);
char *strcat(char *d, const char *s);
size_t strlen(const char *s);
int sprintf(char *buf, const char *fmt, ...);
s32 utf8_seq_len(void *self, s32 c);
s32 utf8_byte_len(void *self, const char *s);

/* number written in spec[1 .. len - 1] (any characters, as digits) */
static s32 spec_number(const char *spec, s32 len, s32 v)
{
    s32 m = 1, i;

    if (len + 1 > 2) {
        v = 0;
        for (i = len - 1; i > 0; i--) {
            v += (spec[i] - '0') * m;
            m *= 10;
        }
    }
    return v;
}

s32 text_format(u8 *self, char *out, s32 size, const u8 *fmt, u32 *args, s8 utf8)
{
    char spec[0x301];
    char res[0x300];
    s32 n = 0, len, w, i, k;
    s32 go = 1, more;
    u8 c = *fmt;
    u32 a;

    for (;;) {
        if (c == '%') {
            more = 1;
            res[0] = 0;
            len = 1;
            do {
                c = fmt[len];
                if (c == 0) {
                    memcpy(spec, fmt, len + 1);
                    spec[len + 1] = 0;
                    strcpy(res, spec);
                    more = 0;
                    go = 0;
                } else if (c == '%') {
                    memcpy(spec, fmt, len + 1);
                    spec[len + 1] = 0;
                    w = spec_number(spec, len, 1);
                    for (i = 0; i < w; i++)
                        res[i] = '%';
                    res[w < 1 ? 0 : w] = 0;
                    more = 0;
                } else if (c == 's' || c == 'o' || c == 'X' || c == 'x' || c == 'u' || c == 'd') {
                    memcpy(spec, fmt, len + 1);
                    spec[len + 1] = 0;
                    a = *args++;
                    sprintf(res, spec, a);
                    more = 0;
                } else if (c == 'c') {
                    memcpy(spec, fmt, len + 1);
                    spec[len + 1] = 0;
                    a = *args++;
                    sprintf(res, spec, (s32)(s8)a);
                    more = 0;
                } else if (c == 'f') {
                    memcpy(spec, fmt, len + 1);
                    spec[len + 1] = 0;
                    if (((u32)args & 7) != 0)
                        args = (u32 *)((u32)args + (8 - ((u32)args & 7)));
                    args += 2;
                    sprintf(res, spec, args[-2], args[-1]);
                    more = 0;
                } else if (c == 'b') {
                    memcpy(spec, fmt, len + 1);
                    spec[len + 1] = 0;
                    a = *args++;
                    w = spec_number(spec, len, 0x20);
                    if (w > 0x20)
                        w = 0x20;
                    for (i = 0; i < w; i++)
                        res[i] = (a & (1u << ((w - 1 - i) & 0x1F))) != 0 ? '1' : '0';
                    res[w < 1 ? 0 : w] = 0;
                    more = 0;
                } else if ((c < '0' || c > '9') && c != '.' && c != '-' && c != '+' && c != '#' &&
                           c != ' ') {
                    memcpy(spec, fmt, len + 1);
                    spec[len + 1] = 0;
                    strcpy(res, spec);
                    more = 0;
                }
                len++;
            } while (more);
            if (utf8 == 1)
                k = utf8_byte_len(D_089CBBE8, res);
            else
                k = strlen(res);
            if (n + k < size - 1) {
                strcat(out, res);
                fmt += len;
                n += k;
            } else {
                out[n] = 0;
                go = 0;
            }
        } else if (c == 0) {
            out[n] = 0;
            go = 0;
        } else {
            if (utf8 == 1) {
                k = utf8_seq_len(D_089CBBE8, c);
                if (k >= 2 && k <= 4) {
                    if (n + k < size - 1) {
                        for (i = 0; i < k; i++)
                            out[n + i] = fmt[i];
                        fmt += k - 1;
                        n += k;
                    } else {
                        out[n] = 0;
                        go = 0;
                    }
                } else if (k == 1) {
                    out[n++] = c;
                } else {
                    out[n] = 0;
                    go = 0;
                }
            } else if ((c < 0x20 || c > 0x7E) && c != '\n') {
                if ((c < 0x81 || c > 0x9F) && c < 0xE0) {
                    out[n++] = c;
                } else if (n + 2 < size - 1) {
                    out[n] = c;
                    fmt++;
                    c = *fmt;
                    out[n + 1] = c;
                    n += 2;
                    if (c == 0)
                        go = 0;
                } else {
                    out[n] = 0;
                    go = 0;
                }
            } else {
                out[n++] = c;
            }
            fmt++;
            if (n >= size - 1) {
                out[size - 1] = 0;
                go = 0;
                n = size - 1;
            }
        }
        if (!go)
            return n;
        c = *fmt;
    }
}
