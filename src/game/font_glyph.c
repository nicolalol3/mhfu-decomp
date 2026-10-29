/* The font g_font (3): the system font (sceFont) and the cache of the glyphs drawn in the 3
 * font textures (cells of 0x118 x 0x119 pixels, slot table 0x2628 by character, characters
 * of the slots 0x22608, age of the slots 0x22968). (Guessed names.) */
#include "libc.h"
#include "font.h"

#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define S16(p, off) (*(s16 *)((u8 *)(p) + (off)))
#define U16(p, off) (*(u16 *)((u8 *)(p) + (off)))
#define F(o) (self[o])

extern u8 *g_vram;
extern u8 *D_08A62C40;            /* display lists */
extern s32 D_08A8B1E0, D_08A8B1E4;
extern const u8 D_089AD380[];     /* parameters of the font library */

void *memset(void *p, int c, size_t n);
void *heap_alloc(void *h, u32 size, u32 align);
int vram_get_surface(void *m, u32 id, void *out);
u32 *dl_cur(u8 *dl);
int dl_link(u8 *dl, u32 *p, s32 n, s32 layer);
void dl_set_cur(u8 *dl, u32 *p);
u32 sceFontNewLib(const void *params, s32 *err);
s32 sceFontGetNumFontList(u32 lib, s32 *err);
u32 sceFontFindOptimumFont(u32 lib, void *style, s32 *err);
u32 sceFontOpen(u32 lib, u32 index, int mode, s32 *err);
s32 sceFontGetFontInfo(u32 font, void *info);
s32 sceFontGetCharInfo(u32 font, u32 ch, s32 *info);
s32 sceFontGetCharGlyphImage(u32 font, u32 ch, void *img);
s32 font_char_kind(u8 *self, u32 ch);
u32 font_layer_dl(u8 *self);
void glyph_draw(u8 *self, u32 ch, s32 half, u8 *e);
u32 font_char_advance(u8 *self, u32 ch, s32 half, u32 w);

u32 glyph_cache_add(u8 *self, u16 ch);
u32 glyph_cache_slot(u8 *self);
void glyph_render_all(u8 *self);
void glyph_render(u8 *self, u32 ch, u32 tex, s32 x, s32 y);
void glyph_mark_layer(u8 *self);

/* opens the system font: 0 library, 4 index, 8 font, 0xC font info */
void font_open_system(u32 *self)
{
    u8 style[0xA8];
    s32 err;

    self[0] = sceFontNewLib(D_089AD380, &err);
    sceFontGetNumFontList(self[0], &err);
    memset(style, 0, 0xA8);
    U16(style, 0x14) = 0;
    U16(style, 0x16) = 0;
    U16(style, 0x1A) = 1;
    self[1] = sceFontFindOptimumFont(self[0], style, &err);
    self[2] = sceFontOpen(self[0], self[1], 0, &err);
    err = sceFontGetFontInfo(self[2], self + 3);
}

/* memory callback of the font library: counts the memory used */
void *font_lib_alloc(u8 *self, u32 size)
{
    u8 *p;

    if (size == 0)
        size = 4;
    p = heap_alloc((void *)0x08A631B8, size, 4);
    if (p == NULL)
        return NULL;
    D_08A8B1E0 += U32(p, -0x10) * 0x10;
    if (D_08A8B1E4 < D_08A8B1E0)
        D_08A8B1E4 = D_08A8B1E0;
    return p;
}

/* GE states for the text, then the glyphs of the layer */
void font_begin_layer(u8 *self)
{
    u32 layer = font_layer_dl(self);
    u32 *p = dl_cur(D_08A62C40);

    p[0] = 0x23000000;
    p[1] = 0xDE000000;
    p[2] = 0x21000001;
    p[3] = 0xDF000032;
    p[4] = 0x22000001;
    p[5] = 0xDBFF0003;
    p[6] = 0xC9000100;
    p[7] = 0xC0000000;
    p[8] = 0xC7000000;
    p[9] = 0xC6000101;
    p[10] = 0x10000000;
    p[11] = 0x08000000;
    dl_link(D_08A62C40, p, 0xC, layer);
    dl_set_cur(D_08A62C40, p + 0xC);
    glyph_mark_layer(self);
    glyph_render_all(self);
}

void font_end_layer(u8 *self)
{
}

/* the characters of the queued strings of the current layer get a slot */
void glyph_mark_layer(u8 *self)
{
    u8 *e;
    u32 n;
    s32 i;
    s16 *s;
    s16 c;

    memset(self + FONT_SLOT_CHAR, 0, 0x360);
    e = (u8 *)U32(self, FONT_LAYER_BUF + (s8)F(FONT_DRAW_LAYER) * 4);
    n = F(FONT_LAYER_N + (s8)F(FONT_DRAW_LAYER));
    for (i = 0; i < (s32)n; i++, e += 0xC) {
        for (s = (s16 *)U32(e, 8); (c = *s++) != 0;) {
            if (c == 10 || c == 0x20 || c == 0xA0 || c == 0x2002 || c == 0x2003 || c == 0x3000)
                continue;
            glyph_cache_add(self, c);
        }
    }
}

/* slot of the character ch (1: new, it must be drawn) */
u32 glyph_cache_add(u8 *self, u16 ch)
{
    u16 *t;
    u32 s;

    if (ch >= 0xFFF0)
        return 0;
    t = &U16(self, FONT_SLOT_OF + ch * 2);
    s = *t;
    if (s != 0xFFFF) {
        F(FONT_SLOT_AGE + s) = 2;
        return 0;
    }
    U16(self, FONT_CACHE_NEXT) = glyph_cache_slot(self);
    *t = U16(self, FONT_CACHE_NEXT);
    U16(self, FONT_SLOT_CHAR + U16(self, FONT_CACHE_NEXT) * 2) = ch;
    F(FONT_SLOT_AGE + U16(self, FONT_CACHE_NEXT)) = 2;
    U16(self, FONT_CACHE_N)++;
    if (U16(self, FONT_CACHE_N) > U16(self, FONT_CELLS_ALL))
        U16(self, FONT_CACHE_N) = U16(self, FONT_CELLS_ALL);
    if (U16(self, FONT_CACHE_N) < U16(self, FONT_CELLS_ALL))
        U16(self, FONT_CACHE_NEXT)++;
    return 1;
}

/* a free slot: the next one, or one not used for 2 frames, or the oldest */
u32 glyph_cache_slot(u8 *self)
{
    u32 n = U16(self, FONT_CELLS_ALL);
    u32 first = U16(self, FONT_CACHE_OLD);
    u32 s;
    s32 i;
    s16 v;

    if (U16(self, FONT_CACHE_N) < n)
        return U16(self, FONT_CACHE_NEXT);
    for (s = first; s < n; s++) {
        if (F(FONT_SLOT_AGE + s) == 0) {
            for (i = 0; i < 0xFFF0; i++) {
                if (U16(self, FONT_SLOT_OF + i * 2) == s) {
                    U16(self, FONT_SLOT_OF + i * 2) = 0xFFFF;
                    break;
                }
            }
            return s & 0xFFFF;
        }
    }
    if (first != 0)
        S16(self, FONT_CACHE_OLD)--;
    v = S16(self, FONT_CACHE_OLD);
    for (i = 0; i < 0xFFF0; i++) {
        if (S16(self, FONT_SLOT_OF + i * 2) == v) {
            U16(self, FONT_SLOT_OF + i * 2) = 0xFFFF;
            break;
        }
    }
    return U16(self, FONT_CACHE_OLD);
}

/* draws the glyphs of the new characters in their cells */
void glyph_render_all(u8 *self)
{
    s32 i;
    s16 c;

    for (i = 0; i < U16(self, FONT_CELLS_ALL); i++) {
        c = S16(self, FONT_SLOT_CHAR + i * 2);
        if (c == 0)
            continue;
        glyph_render(self, c, i / U16(self, FONT_CELLS), F(FONT_CELL_W) * (i % F(FONT_COLS)),
                     F(FONT_CELL_H) * ((i / F(FONT_COLS)) % F(FONT_ROWS)));
    }
}

/* glyph of ch in the texture tex at the cell (x, y) */
void glyph_render(u8 *self, u32 ch, u32 tex, s32 x, s32 y)
{
    u8 surf[0x18];
    s32 info[0xF];
    struct {
        u32 format;
        s32 x64, y64;
        u16 w, h, pitch, pad;
        u32 buf;
    } img;
    u8 *d;
    s32 r, i, gx, gy, ry, base, top;
    u32 c;

    vram_get_surface(g_vram, (u8)(tex + 3), surf);
    img.w = 0x100;
    img.h = 0x100;
    img.format = 0;
    img.pad = 0;
    img.pitch = 0x80;
    img.buf = U32(surf, 8);
    d = (u8 *)img.buf + x / 2 + y * 0x80;
    for (r = 0; r < F(FONT_CELL_H); r++) {
        for (i = 0; i < (F(FONT_CELL_W) >> 1); i++)
            *d++ = 0;
        d += (0x100 - F(FONT_CELL_W)) / 2;
    }
    c = ch & 0xFFFF;
    if (c == 0x20 || c == 0xA0 || c == 0x2002 || c == 0x2003 || c == 0x3000)
        return;
    if (c == 0x5C)
        ch = 0x3035;
    if (sceFontGetCharInfo(U32(self, FONT_HANDLE), ch, info) != 0)
        return;
    gx = info[2] << 6;
    base = (F(FONT_PAD_Y) >> 1) * 0x40 + (s32)U32(self, FONT_PIXELS);
    gy = (s32)U32(self, FONT_PIXELS) + (y + (F(FONT_PAD_Y) >> 1)) * 0x40;
    ry = (x + (F(FONT_PAD_X) >> 1)) << 6;
    top = -(info[3] << 6);
    if (font_char_kind(self, ch) == 1 && F(FONT_FIXED) == 0)
        gx = ((u32)(F(FONT_GLYPH_W) - info[0]) >> 1) << 6;
    if (U32(self, FONT_SYS) == 0 && base + top < 0) {
        gy = (y + (F(FONT_PAD_Y) >> 1)) << 6;
        top = (F(FONT_GLYPH_H) - info[1]) << 6;
    }
    c = ch & 0xFFFF;
    if (c == 0x2A || c == 0xD7) {
        gy = (y + (F(FONT_PAD_Y) >> 1)) << 6;
        top = ((u32)(F(FONT_GLYPH_H) - info[1]) >> 1) << 6;
    }
    if (c == 0xC6 || c == 0xD0 || c == 0xD8) {
        gy = (y + (F(FONT_PAD_Y) >> 1)) << 6;
        top = (F(FONT_GLYPH_H) - info[1]) << 6;
    }
    img.x64 = ry + gx;
    img.y64 = gy + top;
    if (sceFontGetCharGlyphImage(U32(self, FONT_HANDLE), ch, &img) == 0 && U32(self, FONT_SYS) == 0 &&
        font_char_kind(self, ch) == 1) {
        img.x64 -= 0x1F;
        sceFontGetCharGlyphImage(U32(self, FONT_HANDLE), ch, &img);
    }
}

/* draws a queued string (entry e: x, y, w, h, color, line height, text) */
void font_draw_entry(u8 *self, u8 *e)
{
    u16 *s = (u16 *)U32(e, 8);
    u32 c;
    s32 half;

    S16(self, FONT_DRAW_X) = S16(e, 0);
    S16(self, FONT_DRAW_Y) = S16(e, 2);
    S16(self, FONT_DRAW_Z) = S16(self, FONT_Z);
    while ((c = *s++) != 0) {
        if (c == 10) {
            S16(self, FONT_DRAW_X) = S16(e, 0);
            S16(self, FONT_DRAW_Y) += e[7];
            continue;
        }
        half = font_char_kind(self, c);
        if (c != 0x20 && c != 0xA0 && c != 0x2002 && c != 0x2003 && c != 0x3000)
            glyph_draw(self, c, half, e);
        S16(self, FONT_DRAW_X) += (u8)font_char_advance(self, c, half, e[4]);
    }
}
