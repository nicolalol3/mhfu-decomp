/* The font g_font (1): setup, colors, text state, widths of the strings and the queue of the
 * strings to draw (6 layers of up to 128 strings). (Guessed names.) */
#include "menu.h"
#include "font.h"

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
void font_open_system(u8 *self);
void font_cache_digits(u8 *self);
void font_blink_color(u8 *self);
s32 font_char_kind(u8 *self, u32 ch);
const char *font_parse_tag(u8 *self, const char *s, s16 *out, int a, int b);

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
    font_open_system(self);
    F(FONT_GLYPH_W) = U16(self, FONT_SYS_W);
    F(FONT_GLYPH_W) = (F(FONT_GLYPH_W) + 1) & 0xFE;
    F(FONT_GLYPH_H) = U16(self, FONT_SYS_H);
    F(FONT_GLYPH_H) = (F(FONT_GLYPH_H) + 1) & 0xFE;
    F(FONT_PAD_X) = 0;
    F(FONT_PAD_X) = (F(FONT_PAD_X) + 1) & 0xFE;
    F(FONT_PAD_Y) = 2;
    F(FONT_PAD_Y) = (F(FONT_PAD_Y) + 1) & 0xFE;
    F(FONT_CELL_W) = F(FONT_GLYPH_W) + F(FONT_PAD_X);
    F(FONT_CELL_H) = F(FONT_GLYPH_H) + F(FONT_PAD_Y);
    F(FONT_COLS) = 0x100 / F(FONT_CELL_W);
    F(FONT_ROWS) = 0x100 / F(FONT_CELL_H);
    U16(self, FONT_CELLS) = (u16)F(FONT_COLS) * (u16)F(FONT_ROWS);
    U16(self, FONT_CELLS_ALL) = U16(self, FONT_CELLS) * 3;
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
        U32(self, FONT_LAYER_BUF + i * 4) = (u32)(self + FONT_LAYERS + i * 0x600);
    F(FONT_FIXED) = 0;
    font_clear_cache(self);
    font_begin_frame(self);
    font_init_palette(self);
    font_cache_digits(self);
}

void font_clear_cache(u8 *self)
{
    u16 *p = &U16(self, FONT_SLOT_OF);
    s32 i;

    for (i = 0; i < 0xFFF0; i++)
        p[i] = 0xFFFF;
    U16(self, FONT_CACHE_NEXT) = 0;
    memset(self + FONT_SLOT_AGE, 0, 0x1B0);
    U16(self, FONT_CACHE_N) = 0;
    U16(self, FONT_CACHE_OLD) = 0;
}

/* start of a frame: empty queues, default state */
void font_begin_frame(u8 *self)
{
    s32 i;

    for (i = 0; i < 6; i++) {
        F(FONT_LAYER_N + i) = 0;
        F(FONT_ICON_N + i) = 0;
    }
    U16(self, FONT_TEXT_POS) = 0;
    for (i = 0; i < 0x1B0; i++)
        if (F(FONT_SLOT_AGE + i) != 0)
            F(FONT_SLOT_AGE + i)--;
    U16(self, FONT_X) = 0;
    U16(self, FONT_Y) = 0;
    U16(self, FONT_Z) = 0;
    F(FONT_SIZE_W) = 0x14;
    F(FONT_SIZE_H) = 0x14;
    F(FONT_LINE_H) = F(FONT_SIZE_H);
    F(FONT_COLOR) = 0;
    F(FONT_LAYER) = 0;
    F(FONT_ON) = 1;
    F(FONT_TAGS) = 0x20;
    font_blink_color(self);
    F(FONT_BLINK)++;
}

/* color i (alpha from alpha, the rest from the 0x00BBGGRR rgb turned to RGB) */
void font_set_palette(u8 *self, s32 i, u32 alpha, u32 rgb)
{
    u32 *p;

    if (i < 0)
        p = &U32(self, FONT_PALETTE);
    else
        p = &U32(self, FONT_COLORS + i * 4);
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
    s32 i = (s8)F(FONT_TAGS);

    font_set_palette(self, i, 0, rgb);
    font_set_color(self, i);
    F(FONT_TAGS)++;
    return i;
}

void font_set_size(void *font, int w, int h)
{
    u8 *self = font;

    F(FONT_SIZE_W) = w;
    F(FONT_SIZE_H) = h;
    F(FONT_LINE_H) = F(FONT_SIZE_H);
}

void font_set_pos(void *font, int x, int y)
{
    u8 *self = font;

    U16(self, FONT_X) = x;
    U16(self, FONT_Y) = y;
}

void font_set_layer(void *font, int l)
{
    u8 *self = font;
    s8 v = l;

    if (v < 0)
        v = 0;
    if (v < 6)
        F(FONT_LAYER) = v;
    else
        F(FONT_LAYER) = 5;
}

void font_set_color(void *font, int color)
{
    U8(font, FONT_COLOR) = color;
}

void font_set_height2(u8 *self, u8 h)
{
    F(FONT_LINE_H) = h;
}

/* width of the string in half characters (the full-width ones count 2) */
int font_char_count(void *font, const char *s)
{
    const u8 *p = (const u8 *)s;
    s32 n = 0;
    u32 c;

    for (c = (u16)utf8_decode(D_089CBBE8, &p); c != 0; c = (u16)utf8_decode(D_089CBBE8, &p))
        n += font_char_kind(font, c) == 0 ? 2 : 1;
    return n;
}

/* the same up to the end of the line */
s32 font_line_char_count(u8 *self, const char *s)
{
    const u8 *p = (const u8 *)s;
    s32 n = 0;
    u32 c;

    for (c = (u16)utf8_decode(D_089CBBE8, &p); c != 10; c = (u16)utf8_decode(D_089CBBE8, &p))
        n += font_char_kind(self, c) == 0 ? 2 : 1;
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
        font_parse_tag(font, t, &v, 0, 1);
        if ((v & 0xFF00) == 0x800) {
            skip ^= 1;
            s = font_parse_tag(font, t, &v, 1, 1);
            n += v;
        } else if (skip) {
            s = t + 1;
        } else {
            s = font_parse_tag(font, t, &v, 1, 1);
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
        font_parse_tag(self, t, &v, 0, 1);
        if ((v & 0xFF00) == 0x800) {
            skip ^= 1;
            s = font_parse_tag(self, t, &v, 1, 1);
            n += v;
        } else if (skip) {
            s = t + 1;
        } else {
            s = font_parse_tag(self, t, &v, 1, 1);
            n += v;
        }
    }
}

/* adds the string (16-bit units) to the queue of the current layer with the current state */
void font_push_text(u8 *self, const u16 *s)
{
    s32 over = 0;
    u8 l = F(FONT_LAYER);
    u8 n = F(FONT_LAYER_N + (s8)l);
    u8 *e;
    u16 *d;

    if (n >= 0x80)
        return;
    e = (u8 *)U32(self, FONT_LAYER_BUF + (s8)l * 4) + n * 0xC;
    F(FONT_LAYER_N + (s8)l) = n + 1;
    U16(e, 0) = U16(self, FONT_X);
    U16(e, 2) = U16(self, FONT_Y);
    e[4] = F(FONT_SIZE_W);
    e[5] = F(FONT_SIZE_H);
    e[7] = F(FONT_LINE_H);
    e[6] = F(FONT_COLOR);
    d = &U16(self, FONT_TEXT + U16(self, FONT_TEXT_POS));
    U32(e, 8) = (u32)d;
    if (*s == 0) {
        *d = 0;
    } else {
        *d = *s;
        for (;;) {
            d++;
            U16(self, FONT_TEXT_POS) += 2;
            s++;
            if (U16(self, FONT_TEXT_POS) > 0x2FFF) {
                U16(self, FONT_TEXT_POS) = 0x2FFE;
                over++;
                d = &U16(self, FONT_TEXT + U16(self, FONT_TEXT_POS));
            }
            if (*s == 0)
                break;
            *d = *s;
        }
        *d = 0;
    }
    if (over == 0) {
        U16(self, FONT_TEXT_POS) += 2;
        if (U16(self, FONT_TEXT_POS) > 0x2FFF)
            U16(self, FONT_TEXT_POS) = 0x2FFE;
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
