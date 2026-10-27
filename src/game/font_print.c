/* The font g_font (2): formatted text (the arguments are the register ones, copied to an
 * array for the formatter func_08893C6C), drawing of the queued layers, conversions of the
 * strings. Not including menu.h: there these functions are declared with "...".
 * (Guessed names.) */
#include "libc.h"

#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define S16(p, off) (*(s16 *)((u8 *)(p) + (off)))
#define U16(p, off) (*(u16 *)((u8 *)(p) + (off)))

extern void *D_089CBBE8;          /* text converter */
extern const char D_089AD274[];   /* "%s" */
extern const u8 D_08931C2C[];     /* ASCII -> UTF-8 full width (3 bytes each) */
extern const u8 *D_089AD37C;      /* ASCII -> SJIS full width (2 bytes each) */

void *memset(void *p, int c, size_t n);
void font_set_pos(u8 *font, int x, int y);
void font_set_color(u8 *font, int color);
u32 utf8_decode(void *self, const u8 **pp);
u32 sjis_decode(void *self, const u8 **pp);
u32 jis_to_unicode(void *self, u32 c);
s32 utf8_seq_len(void *self, s32 c);
void func_08893C6C(u8 *self, char *buf, u32 size, const char *fmt, u32 *args, int a);
void func_08894754(u8 *self, char *buf, int a);
void func_088921FC(u8 *self, s32 a, char *buf);
void func_08892FB0(u8 *self, s32 l);
void func_0889377C(u8 *self, u8 *e);
u32 func_08894988(u8 *self);
void func_08892A88(u8 *self, u8 *p, u32 n, u32 a);
void func_088930A0(u8 *self);
s32 func_08893198(u8 *self, u32 c);
void func_08893390(u8 *self);
s32 func_08894874(u8 *self, u32 ch);

#define F(o) (self[o])

/* new entry of the queue of the current layer for the text in the string buffer */
static int queue_entry(u8 *self, u32 n)
{
    s8 l = F(0x12F);
    u8 *e = (u8 *)U32(self, 0x140 + l * 4) + n * 0xC;

    F(0x134 + (s8)F(0x12F))++;
    U16(e, 0) = U16(self, 0x120);
    U16(e, 2) = U16(self, 0x122);
    e[4] = F(0x12C);
    e[5] = F(0x12D);
    e[7] = F(0x15F);
    e[6] = F(0x12E);
    U32(e, 8) = (u32)(self + 0x22CF8 + U16(self, 0x160));
    return 1;
}

void font_printf(u8 *self, const char *fmt, u32 a, u32 b, u32 c, u32 d, u32 e, u32 f)
{
    u32 args[6];
    char buf[0x300];
    u8 n = F(0x134 + (s8)F(0x12F));

    if (n >= 0x80)
        return;
    queue_entry(self, n);
    args[0] = a;
    args[1] = b;
    args[2] = c;
    args[3] = d;
    args[4] = e;
    args[5] = f;
    memset(buf, 0, 0x300);
    func_08893C6C(self, buf, 0x300, fmt, args, 1);
    func_08894754(self, buf, 1);
}

void font_printf_at(u8 *self, int x, int y, const char *fmt, u32 a, u32 b, u32 c, u32 d)
{
    u32 args[4];
    char buf[0x300];
    u8 n = F(0x134 + (s8)F(0x12F));

    if (n >= 0x80)
        return;
    args[0] = a;
    args[1] = b;
    args[2] = c;
    args[3] = d;
    font_set_pos(self, x, y);
    queue_entry(self, n);
    memset(buf, 0, 0x300);
    func_08893C6C(self, buf, 0x300, fmt, args, 1);
    func_08894754(self, buf, 1);
}

void font_printf_at_color(u8 *self, int x, int y, int color, const char *fmt, u32 a, u32 b, u32 c)
{
    u32 args[3];
    char buf[0x300];
    u8 n = F(0x134 + (s8)F(0x12F));

    if (n >= 0x80)
        return;
    args[0] = a;
    args[1] = b;
    args[2] = c;
    font_set_pos(self, x, y);
    font_set_color(self, color);
    queue_entry(self, n);
    memset(buf, 0, 0x300);
    func_08893C6C(self, buf, 0x300, fmt, args, 1);
    func_08894754(self, buf, 1);
}

/* the same, the format without the conversion of the characters */
void font_printf_at_raw(u8 *self, int x, int y, const char *fmt, u32 a, u32 b, u32 c, u32 d)
{
    u32 args[4];
    char buf[0x300];
    u8 n = F(0x134 + (s8)F(0x12F));

    if (n >= 0x80)
        return;
    args[0] = a;
    args[1] = b;
    args[2] = c;
    args[3] = d;
    font_set_pos(self, x, y);
    queue_entry(self, n);
    memset(buf, 0, 0x300);
    func_08893C6C(self, buf, 0x300, fmt, args, 0);
    func_08894754(self, buf, 0);
}

void font_printf_at_color_raw(u8 *self, int x, int y, int color, const char *fmt, u32 a, u32 b,
                              u32 c)
{
    u32 args[3];
    char buf[0x300];
    u8 n = F(0x134 + (s8)F(0x12F));

    if (n >= 0x80)
        return;
    args[0] = a;
    args[1] = b;
    args[2] = c;
    font_set_pos(self, x, y);
    font_set_color(self, color);
    queue_entry(self, n);
    memset(buf, 0, 0x300);
    func_08893C6C(self, buf, 0x300, fmt, args, 0);
    func_08894754(self, buf, 0);
}

/* text with a shadow of color c1 moved by (dx, dy) */
void font_print_shadow(u8 *self, int x, int y, int c1, int c2, const char *s, s16 dx, s16 dy)
{
    font_printf_at_color(self, (s16)((s16)x + dx), (s16)((s16)y + dy), c1, D_089AD274, (u32)s,
                         0, 0);
    font_printf_at_color(self, x, y, c2, D_089AD274, (u32)s, 0, 0);
}

/* draws the queued strings of the layer l */
void font_draw_layer(u8 *self, s8 l)
{
    u32 n;
    u8 *e;
    s32 i;

    if (l < 0 || l >= 6)
        return;
    n = F(0x134 + l);
    if (n != 0) {
        F(0x15D) = l;
        func_08892FB0(self, l);
        e = (u8 *)U32(self, 0x140 + l * 4);
        for (i = 0; i < (s32)n; i++, e += 0xC)
            func_0889377C(self, e);
    }
    n = F(0x13A + l);
    if (n != 0) {
        func_08892A88(self, self + l * 0x50 + 0x22B18, n, func_08894988(self));
        func_088930A0(self);
    }
}

void font_draw_layers(u8 *self)
{
    s32 i;

    for (i = 0; i < 6; i++)
        font_draw_layer(self, (s8)i);
}

/* width (1 or 2 halves) of the first character of s */
u32 font_first_char_width(u8 *self, const char *s)
{
    const u8 *p = (const u8 *)s;

    if (s == NULL || *s == 0)
        return 0;
    return func_08894874(self, utf8_decode(D_089CBBE8, &p)) == 0 ? 2 : 1;
}

/* copies s to d with the ASCII characters made full width (UTF-8) */
void font_to_full_width_utf8(u8 *self, const char *s, char *d)
{
    s32 n, i;
    s8 c = *s;

    for (;;) {
        if (c == 0)
            break;
        n = utf8_seq_len(D_089CBBE8, (u8)c);
        if (n == 0)
            break;
        if (n == 1) {
            c = *s;
            if (c < 0x20) {
                *d++ = c;
                s++;
            } else if (c < 0x7F) {
                s++;
                d[0] = D_08931C2C[c * 3];
                d[1] = D_08931C2C[c * 3 + 1];
                d[2] = D_08931C2C[c * 3 + 2];
                d += 3;
            } else {
                *d++ = c;
                s++;
            }
        } else {
            for (i = 0; i < n; i++)
                *d++ = *s++;
        }
        c = *s;
    }
    *d = 0;
}

/* the same in Shift-JIS */
void font_to_full_width_sjis(u8 *self, const char *s, char *d)
{
    const u8 *t = D_089AD37C;
    s32 c = (s8)*s;

    if (c == 0) {
        *d = 0;
        return;
    }
    do {
        if (c >= 0x81 && c <= 0x9F) {
            d[0] = c;
            d[1] = s[1];
            s += 2;
            d += 2;
        } else if (c > 0xDF && c < 0x100) {
            d[0] = c;
            d[1] = s[1];
            s += 2;
            d += 2;
        } else if (c < 0x20 || c > 0x7E) {
            *d++ = c;
            s++;
        } else {
            s++;
            d[0] = t[c * 2];
            d[1] = t[c * 2 + 1];
            d += 2;
        }
        c = (s8)*s;
    } while (c != 0);
    *d = 0;
}

/* prepares the glyphs of the characters of the SJIS string s */
void font_cache_string(u8 *self, const char *s)
{
    const u8 *p = (const u8 *)s;
    u32 c;

    memset(self + 0x22608, 0, 0x360);
    while (*p != 0) {
        c = (u16)jis_to_unicode(D_089CBBE8, (u16)sjis_decode(D_089CBBE8, &p));
        if (c == 0)
            break;
        if (c == 10 || c == 0x20 || c == 0xA0 || c == 0x2002 || c == 0x2003 || c == 0x3000)
            continue;
        if ((s32)U16(self, 0x162) < U16(self, 0x11E) - 1 && func_08893198(self, c) != 0)
            U16(self, 0x162)++;
    }
    func_08893390(self);
}

/* characters of the UTF-8 string (the new lines are not counted) */
s32 font_count_utf8(u8 *self, const char *s)
{
    const u8 *p = (const u8 *)s;
    s32 n = 0;

    while (*p != 0) {
        if (*p == '\n') {
            p++;
        } else {
            utf8_decode(D_089CBBE8, &p);
            n++;
        }
    }
    return n;
}

/* characters of the SJIS string (the new lines are not counted) */
s32 font_count_sjis(u8 *self, const u8 *s)
{
    u8 c = *s;
    s32 n = 0;

    for (;;) {
        if (c == 0)
            return n;
        if (c >= 0x81 && c <= 0x9F) {
            s++;
            n++;
        } else if (c > 0xDF) {
            s++;
            n++;
        } else if (c != 10) {
            n++;
        }
        s++;
        c = *s;
    }
}

const char *font_parse_tag(u8 *self, const char *s, u16 *out, s16 mode, u32 utf8);
void font_add_icon(u8 *self, u16 x, u16 y, u16 id, u8 a, s16 b);
void font_tag_number(u8 *self, char *d, s16 kind, u8 utf8);
u32 font_first_char_width(u8 *self, const char *s);
u8 font_get_char(u8 *self, s16 n, u8 *out, const u8 **pp, s8 utf8);
s32 utf8_byte_len(void *self, const char *s);
char *strncpy(char *d, const char *s, size_t n);
char *strncat(char *d, const char *s, size_t n);
void font_printf(u8 *self, const char *fmt, u32 a, u32 b, u32 c, u32 d, u32 e, u32 f);

#define FLUSH(out, n) do { (out)[n] = 0; font_printf(self, D_089AD274, (u32)(out), 0, 0, 0, 0, 0); } while (0)

/* draws the text with its tags; limit: characters to show (-1: all) */
void font_draw_tagged(u8 *self, s16 limit, const char *src)
{
    char a[0x300], b[0x300];
    char *buf[2];
    s32 cur = 0, skip = 0, n = 0, kind, k, i;
    s16 x0 = S16(self, 0x120), x = x0, y = S16(self, 0x122);
    const char *p, *q;
    char *out;
    u16 v;
    u32 t;

    buf[0] = a;
    buf[1] = b;
    strncpy(a, src, 0x300);
    a[0x2FF] = 0;
    memset(b, 0, 0x300);
    p = a;
    for (;;) {
        out = buf[cur ^ 1];
        q = p;
        if (*q == 0) {
            FLUSH(out, n);
            return;
        }
        if (*q == '~') {
            if (skip) {
                p = font_parse_tag(self, q, &v, 0, 1);
                if ((v & 0xFF00) == 0x800) {
                    skip = 0;
                    continue;
                }
                p = q;
            } else {
                FLUSH(out, n);
                n = 0;
                font_set_pos(self, x, y);
                p = font_parse_tag(self, q, &v, 0, 1);
                t = v & 0xFF00;
                if (t == 0x800) {
                    skip = 1;
                    continue;
                }
                if (t == 0x400) {
                    font_add_icon(self, x, y, F(0x12D), (s8)F(0x12E), v & 0xFF);
                    x = x + F(0x12C);
                    font_set_pos(self, x, y);
                    if (limit > 0)
                        limit--;
                    if (limit == 0)
                        *(char *)p = 0;
                    continue;
                }
                if (t == 0x200) {
                    cur ^= 1;
                    font_tag_number(self, buf[cur], v & 0xFF, 1);
                    strncat(buf[cur], p, 0x300 - utf8_byte_len(D_089CBBE8, buf[cur]));
                    p = buf[cur];
                    buf[cur][0x2FF] = 0;
                    continue;
                }
                if (t == 0x100) {
                    font_set_color(self, (s8)v);
                    continue;
                }
                if (*p == 0) {
                    FLUSH(out, n);
                    return;
                }
            }
        }
        /* a character */
        kind = 2;
        if (font_first_char_width(self, p) == 2) {
            kind = 0;
            if (limit > 0)
                limit--;
        } else if (*p != '\n') {
            kind = 1;
            if (limit > 0)
                limit--;
        }
        k = utf8_seq_len(D_089CBBE8, (u8)*p);
        if (n + k > 0x2FE) {
            FLUSH(out, n);
            return;
        }
        for (i = 0; i < k; i++)
            out[n++] = *p++;
        if (n > 0x2FE) {
            n = 0x2FF;
            FLUSH(out, n);
            return;
        }
        if (kind == 0) {
            x = x + F(0x12C);
        } else if (kind == 1) {
            x = x + (F(0x12C) >> 1);
        } else {
            y = y + F(0x15F);
            out[n - 1] = 0;
            x = x0;
            font_printf(self, D_089AD274, (u32)out, 0, 0, 0, 0, 0);
            n = 0;
            font_set_pos(self, x0, y);
        }
        if (limit == 0)
            *(char *)p = 0;
    }
}

/* the character number idx of the text as shown (the tags count as their characters);
 * *next after it. 1 full width, 2 icon, 4 end */
s32 font_tagged_char(u8 *self, s16 idx, char *out, const char *s, const char **next)
{
    char buf[0x302];
    const u8 *t;
    const char *q;
    s32 skip = 0, k, i, w;
    s16 count = 0;
    u16 v;
    u32 code;

    for (;;) {
        q = s;
        if (*q == 0) {
            *out = 0;
            *next = q;
            return 4;
        }
        if (*q == '~') {
            s = font_parse_tag(self, q, &v, 0, 1);
            if ((v & 0xFF00) == 0x800) {
                skip ^= 1;
                continue;
            }
            if (!skip) {
                s = font_parse_tag(self, q, &v, 2, 1);
                if (count + (s16)v <= idx) {
                    count += v;
                    continue;
                }
                font_parse_tag(self, q, &v, 0, 1);
                code = v & 0xFF00;
                if (code == 0x400) {
                    out[0] = v;
                    out[1] = 0;
                    *next = s;
                    return 2;
                }
                if (code == 0x200) {
                    t = (const u8 *)buf;
                    font_tag_number(self, buf, v & 0xFF, 1);
                    i = (s16)font_get_char(self, (s16)(idx - count), (u8 *)out, &t, 1);
                    *next = (const char *)t;
                    return i;
                }
                if (code == 0x100)
                    continue;
                if (code == 0) {
                    font_parse_tag(self, q, &v, 2, 1);
                    if (v != 0) {
                        out[0] = '~';
                        out[1] = 0;
                        *next = s;
                        return 0;
                    }
                } else {
                    count += v;
                }
                continue;
            }
        }
        k = utf8_seq_len(D_089CBBE8, (u8)*q);
        if (count == idx) {
            w = (s16)font_first_char_width(self, q);
            for (i = 0; i < k; i++)
                *out++ = *q++;
            *out = 0;
            *next = q;
            return (s16)(w == 2);
        }
        for (i = 0; i < k; i++)
            q++;
        count++;
        s = q;
    }
}

void font_draw(u8 *self, const char *fmt, u32 a, u32 b, u32 c, u32 d, u32 e, u32 f)
{
    u32 args[6];
    char buf[0x300];

    args[0] = a;
    args[1] = b;
    args[2] = c;
    args[3] = d;
    args[4] = e;
    args[5] = f;
    memset(buf, 0, 0x300);
    func_08893C6C(self, buf, 0x300, fmt, args, 1);
    func_088921FC(self, -1, buf);
}

void font_draw_at(u8 *self, int x, int y, const char *fmt, u32 a, u32 b, u32 c, u32 d)
{
    u32 args[4];
    char buf[0x300];

    args[0] = a;
    args[1] = b;
    args[2] = c;
    args[3] = d;
    font_set_pos(self, x, y);
    memset(buf, 0, 0x300);
    func_08893C6C(self, buf, 0x300, fmt, args, 1);
    func_088921FC(self, -1, buf);
}

void font_draw_at_color(u8 *self, int x, int y, int color, const char *fmt, u32 a, u32 b, u32 c)
{
    u32 args[3];
    char buf[0x300];

    args[0] = a;
    args[1] = b;
    args[2] = c;
    font_set_pos(self, x, y);
    font_set_color(self, color);
    memset(buf, 0, 0x300);
    func_08893C6C(self, buf, 0x300, fmt, args, 1);
    func_088921FC(self, -1, buf);
}
