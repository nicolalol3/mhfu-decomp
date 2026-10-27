/* The font g_font (1): setup, colors, text state, widths of the strings and the queue of the
 * strings to draw (6 layers of up to 128 strings). (Guessed names.) */
#include "menu.h"

extern u8 *g_vram;
extern u32 D_08A8B1E0, D_08A8B1E4;
extern void *D_089CBBE8;          /* text converter */
extern const u16 D_08931ABC[0x40];   /* palette of the font textures */
extern const u32 D_08931B3C[0x20];   /* the 32 standard colors */
extern const char D_089AD26C[];   /* "<" (start of a tag) */
extern const char D_089AD270[];   /* "\n" */

void heap_init(void *a, void *b, int n);
int vram_create_surface(void *m, u32 id, u32 w, u32 h, u32 format, u32 user, u32 offset);
u32 utf8_decode(void *self, const u8 **pp);
size_t strlen(const char *s);
char *strstr(const char *s, const char *t);
void *memset(void *p, int c, size_t n);
void func_08892E7C(u8 *self);
void func_08894B00(u8 *self);
void func_088949EC(u8 *self);
s32 func_08894874(u8 *self, u32 ch);
const char *func_08894B0C(u8 *self, const char *s, s16 *out, int a, int b);

void font_init_textures(u8 *self);
void font_clear_cache(u8 *self);
void font_begin_frame(u8 *self);
void font_init_palette(u8 *self);
void font_set_palette(u8 *self, s32 i, u32 alpha, u32 rgb);
int font_char_count(void *font, const char *s);
s32 font_line_char_count(u8 *self, const char *s);
void font_push_text(u8 *self, const u16 *s);

#define F(o) U8(self, o)

void font_init(u8 *self)
{
    heap_init((void *)0x08A631B8, (void *)0x08A631E0, 0x28000);
    D_08A8B1E0 = 0;
    D_08A8B1E4 = 0;
    func_08892E7C(self);
    F(0x114) = U16(self, 0x5C);
    F(0x114) = (F(0x114) + 1) & 0xFE;
    F(0x115) = U16(self, 0x5E);
    F(0x115) = (F(0x115) + 1) & 0xFE;
    F(0x116) = 0;
    F(0x116) = (F(0x116) + 1) & 0xFE;
    F(0x117) = 2;
    F(0x117) = (F(0x117) + 1) & 0xFE;
    F(0x118) = F(0x114) + F(0x116);
    F(0x119) = F(0x115) + F(0x117);
    F(0x11A) = 0x100 / F(0x118);
    F(0x11B) = 0x100 / F(0x119);
    U16(self, 0x11C) = (u16)F(0x11A) * (u16)F(0x11B);
    U16(self, 0x11E) = U16(self, 0x11C) * 3;
    font_init_textures(self);
}

void font_init_textures(u8 *self)
{
    s32 k, i;
    u16 *d;

    vram_create_surface(g_vram, 3, 0x100, 0x100, 4, 0, -1);
    vram_create_surface(g_vram, 4, 0x100, 0x100, 4, 0, -1);
    vram_create_surface(g_vram, 5, 0x100, 0x100, 4, 0, -1);
    for (k = 0; k < 3; k++) {
        U16(g_vram, 0x5C + k * 0x18) = 2;
        d = (u16 *)U32(g_vram, 0x58 + k * 0x18);
        for (i = 0; i < 0x40; i++)
            d[i] = D_08931ABC[i];
    }
    for (i = 0; i < 6; i++)
        U32(self, 0x140 + i * 4) = (u32)(self + 0x228 + i * 0x600);
    F(0x130) = 0;
    font_clear_cache(self);
    font_begin_frame(self);
    font_init_palette(self);
    func_08894B00(self);
}

void font_clear_cache(u8 *self)
{
    u16 *p = &U16(self, 0x2628);
    s32 i;

    for (i = 0; i < 0xFFF0; i++)
        p[i] = 0xFFFF;
    U16(self, 0x158) = 0;
    memset(self + 0x22968, 0, 0x1B0);
    U16(self, 0x15A) = 0;
    U16(self, 0x162) = 0;
}

/* start of a frame: empty queues, default state */
void font_begin_frame(u8 *self)
{
    s32 i;

    for (i = 0; i < 6; i++) {
        F(0x134 + i) = 0;
        F(0x13A + i) = 0;
    }
    U16(self, 0x160) = 0;
    for (i = 0; i < 0x1B0; i++)
        if (F(0x22968 + i) != 0)
            F(0x22968 + i)--;
    U16(self, 0x120) = 0;
    U16(self, 0x122) = 0;
    U16(self, 0x124) = 0;
    F(0x12C) = 0x14;
    F(0x12D) = 0x14;
    F(0x15F) = F(0x12D);
    F(0x12E) = 0;
    F(0x12F) = 0;
    F(0x15C) = 1;
    F(0x131) = 0x20;
    func_088949EC(self);
    F(0x15E)++;
}

/* color i (alpha from alpha, the rest from the 0x00BBGGRR rgb turned to RGB) */
void font_set_palette(u8 *self, s32 i, u32 alpha, u32 rgb)
{
    u32 *p;

    if (i < 0)
        p = &U32(self, 0x164);
    else
        p = &U32(self, 0x168 + i * 4);
    *p = (rgb & 0xFF) << 16 | (rgb & 0xFF00) | (alpha & 0xFF000000) | (rgb & 0xFF0000) >> 16;
}

void font_init_palette(u8 *self)
{
    const u32 *c = D_08931B3C;
    s32 i;

    for (i = 0; i < 0x1E; i++, c++)
        font_set_palette(self, i, 0, *c);
    for (i = 0x1E; i < 0x20; i++, c++)
        font_set_palette(self, i, 0xFF000000, *c);
    for (i = 0x20; i < 0x30; i++)
        font_set_palette(self, i, 0, 0);
}

/* new color for this frame; it becomes the current one */
s32 font_add_color(u8 *self, u32 rgb)
{
    s32 i = (s8)F(0x131);

    font_set_palette(self, i, 0, rgb);
    font_set_color(self, i);
    F(0x131)++;
    return i;
}

void font_set_size(void *font, int w, int h)
{
    u8 *self = font;

    F(0x12C) = w;
    F(0x12D) = h;
    F(0x15F) = F(0x12D);
}

void font_set_pos(void *font, int x, int y)
{
    u8 *self = font;

    U16(self, 0x120) = x;
    U16(self, 0x122) = y;
}

void font_set_layer(void *font, int l)
{
    u8 *self = font;
    s8 v = l;

    if (v < 0)
        v = 0;
    if (v < 6)
        F(0x12F) = v;
    else
        F(0x12F) = 5;
}

void font_set_color(void *font, int color)
{
    U8(font, 0x12E) = color;
}

void font_set_height2(u8 *self, u8 h)
{
    F(0x15F) = h;
}

/* width of the string in half characters (the full-width ones count 2) */
int font_char_count(void *font, const char *s)
{
    const u8 *p = (const u8 *)s;
    s32 n = 0;
    u32 c;

    for (c = (u16)utf8_decode(D_089CBBE8, &p); c != 0; c = (u16)utf8_decode(D_089CBBE8, &p))
        n += func_08894874(font, c) == 0 ? 2 : 1;
    return n;
}

/* the same up to the end of the line */
s32 font_line_char_count(u8 *self, const char *s)
{
    const u8 *p = (const u8 *)s;
    s32 n = 0;
    u32 c;

    for (c = (u16)utf8_decode(D_089CBBE8, &p); c != 10; c = (u16)utf8_decode(D_089CBBE8, &p))
        n += func_08894874(self, c) == 0 ? 2 : 1;
    return n;
}

size_t font_strlen(u8 *self, const char *s)
{
    return strlen(s);
}

/* width of the text with its tags (<...>: the tags 0x8xx switch the counting) */
s32 font_text_width(void *font, const char *s)
{
    s32 n = font_char_count(font, s);
    s32 skip = 0;
    const char *t;
    s16 v;

    while (*s != 0) {
        t = strstr(s, D_089AD26C);
        if (t == NULL)
            break;
        func_08894B0C(font, t, &v, 0, 1);
        if ((v & 0xFF00) == 0x800) {
            skip ^= 1;
            s = func_08894B0C(font, t, &v, 1, 1);
            n += v;
        } else if (skip) {
            s = t + 1;
        } else {
            s = func_08894B0C(font, t, &v, 1, 1);
            n += v;
        }
    }
    return n;
}

/* the same for the first line */
s32 font_line_width(u8 *self, const char *s)
{
    s32 n = font_line_char_count(self, s);
    s32 skip = 0;
    const char *nl, *t;
    s16 v;

    for (;;) {
        if (*s == 0 || *s == '\n')
            return n;
        nl = strstr(s, D_089AD270);
        t = strstr(s, D_089AD26C);
        if (t == NULL)
            return n;
        if ((u32)nl <= (u32)t)
            return n;
        func_08894B0C(self, t, &v, 0, 1);
        if ((v & 0xFF00) == 0x800) {
            skip ^= 1;
            s = func_08894B0C(self, t, &v, 1, 1);
            n += v;
        } else if (skip) {
            s = t + 1;
        } else {
            s = func_08894B0C(self, t, &v, 1, 1);
            n += v;
        }
    }
}

/* adds the string (16-bit units) to the queue of the current layer with the current state */
void font_push_text(u8 *self, const u16 *s)
{
    s32 over = 0;
    u8 l = F(0x12F);
    u8 n = F(0x134 + (s8)l);
    u8 *e;
    u16 *d;

    if (n >= 0x80)
        return;
    e = (u8 *)U32(self, 0x140 + (s8)l * 4) + n * 0xC;
    F(0x134 + (s8)l) = n + 1;
    U16(e, 0) = U16(self, 0x120);
    U16(e, 2) = U16(self, 0x122);
    e[4] = F(0x12C);
    e[5] = F(0x12D);
    e[7] = F(0x15F);
    e[6] = F(0x12E);
    d = &U16(self, 0x22CF8 + U16(self, 0x160));
    U32(e, 8) = (u32)d;
    if (*s == 0) {
        *d = 0;
    } else {
        *d = *s;
        for (;;) {
            d++;
            U16(self, 0x160) += 2;
            s++;
            if (U16(self, 0x160) > 0x2FFF) {
                U16(self, 0x160) = 0x2FFE;
                over++;
                d = &U16(self, 0x22CF8 + U16(self, 0x160));
            }
            if (*s == 0)
                break;
            *d = *s;
        }
        *d = 0;
    }
    if (over == 0) {
        U16(self, 0x160) += 2;
        if (U16(self, 0x160) > 0x2FFF)
            U16(self, 0x160) = 0x2FFE;
    }
}

void font_puts_at(void *font, int x, int y, const char *s)
{
    font_set_pos(font, x, y);
    font_push_text(font, (const u16 *)s);
}

void font_puts_at_color(void *font, int x, int y, int color, const char *s)
{
    font_set_pos(font, x, y);
    font_set_color(font, color);
    font_push_text(font, (const u16 *)s);
}
