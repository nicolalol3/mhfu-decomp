/* The font g_font (4): the quads of the glyphs in the display list, the conversion of the
 * formatted text to 16-bit characters, width class of the characters, the small objects
 * of the popup manager. (Guessed names.) */
#include "libc.h"
#include "font.h"
#include "vfpu.h"

#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define S16(p, off) (*(s16 *)((u8 *)(p) + (off)))
#define U16(p, off) (*(u16 *)((u8 *)(p) + (off)))
#define F(o) (self[o])

extern u8 *g_vram;
extern u8 *D_08A62C40;            /* display lists */
extern void *D_089CBBE8;          /* text converter */
extern const char *D_089AD404;    /* "0123456789ABCDEF...": glyphs always in the cache */
extern const char D_089AD408[];

int vram_get_surface(void *m, u32 id, void *out);
u32 *dl_cur(u8 *dl);
int dl_link(u8 *dl, u32 *p, s32 n, s32 layer);
void dl_set_cur(u8 *dl, u32 *p);
u32 pow2_log(u8 *dl, u32 v);
u32 utf8_decode(void *self, const u8 **pp);
u32 sjis_decode(void *self, const u8 **pp);
u32 jis_to_unicode(void *self, u32 c);
void heap_free(void *h, void *p);
int sprintf(char *buf, const char *fmt, ...);
void font_set_palette(u8 *self, s32 i, u32 alpha, u32 rgb);
void font_cache_string(u8 *self, const char *s);
void font_to_full_width_utf8(u8 *self, const char *s, char *d);
void font_to_full_width_sjis(u8 *self, const char *s, char *d);
u8 func_088952E4(u8 *self, u32 ch);
void scene_start(u8 **self, u8 slot, u32 kind);

u32 font_layer_dl(u8 *self);

/* quad of the glyph ch (half: width class) for the queued string e */
void glyph_draw(u8 *self, u32 ch, s32 half, u8 *e)
{
    u8 surf[0x18];
    u32 slot, col, u, v, w, layer, *p;
    s32 c;
    s16 dy;

    if (U16(self, FONT_SLOT_OF + (ch & 0xFFFF) * 2) == 0xFFFF)
        return;
    if ((s8)e[6] >= 0x30)
        e[6] = 0;
    c = (s8)e[6];
    col = c < 0 ? U32(self, FONT_PALETTE) : U32(self, FONT_COLORS + c * 4);
    slot = U16(self, FONT_SLOT_OF + (ch & 0xFFFF) * 2);
    u = F(FONT_CELL_W) * (slot % F(FONT_COLS));
    v = F(FONT_CELL_H) * ((slot / F(FONT_COLS)) % F(FONT_ROWS));
    vram_get_surface(g_vram, (slot / U16(self, FONT_CELLS) + 3) & 0xFF, surf);
    if (half == 0 || half == 2)
        w = e[4];
    else
        w = e[4] >> 1;
    layer = font_layer_dl(self);
    if (func_088952E4(self, ch) == 1) {
        if (e[5] < 0x10)
            dy = -2;
        else if (e[5] < 0x14)
            dy = -3;
        else if (e[5] < 0x1A)
            dy = -4;
        else if (e[5] < 0x1E)
            dy = -5;
        else
            dy = -6;
    } else {
        dy = 0;
    }
    p = dl_cur(D_08A62C40);
    p[0] = v << 16 | u;
    p[1] = (col & 0xFFFFFF) | 0xFF000000;
    p[2] = U16(self, FONT_DRAW_X) | (S16(self, FONT_DRAW_Y) + dy) << 16;
    p[3] = U16(self, FONT_DRAW_Z);
    p[4] = (v + F(FONT_CELL_H)) << 16 | (u + F(FONT_CELL_W));
    p[5] = (col & 0xFFFFFF) | 0xFF000000;
    p[6] = (e[5] + S16(self, FONT_DRAW_Y) + dy) << 16 | ((S16(self, FONT_DRAW_X) + (u8)w) & 0xFFFF);
    p[7] = U16(self, FONT_DRAW_Z);
    p[8] = ((u32)p & 0xFF000000) >> 8 | 0x10000000;
    p[9] = ((u32)p & 0xFFFFFF) | 0x01000000;
    p[10] = 0xC2000000;
    p[11] = U16(surf, 0xC) | 0xC3000000;
    p[12] = (U32(surf, 8) & 0xFFFFFF) | 0xA0000000;
    p[13] = (U32(surf, 8) & 0xFF000000) >> 8 | 0xA8000000 | U16(surf, 4);
    {
        u32 lh = pow2_log(D_08A62C40, U16(surf, 6));

        p[14] = lh << 8 | 0xB8000000 | pow2_log(D_08A62C40, U16(surf, 4));
    }
    p[15] = ((col & 0xFF000000) != 0) << 16 | 0xC500FF00 | U16(surf, 0x14);
    p[16] = (U32(surf, 0x10) & 0xFFFFFF) | 0xB0000000;
    p[17] = (U32(surf, 0x10) & 0xFF000000) >> 8 | 0xB1000000;
    p[18] = 0xC4000004;
    p[19] = 0xCB000000;
    p[20] = 0x1E000001;
    p[21] = 0x50000001;
    p[22] = 0x1280011E;
    p[23] = 0x04060002;
    p[24] = 0x10000000;
    p[25] = 0x08000000;
    dl_link(D_08A62C40, p + 8, 0x12, layer);
    dl_set_cur(D_08A62C40, p + 0x1A);
}

/* the formatted text to the 16-bit string buffer (utf8: else Shift-JIS) */
void font_store_text(u8 *self, const char *s, s8 utf8)
{
    const u8 *p = (const u8 *)s;
    u16 *d = &U16(self, FONT_TEXT + U16(self, FONT_TEXT_POS));
    s32 over = 0;

    while (*p != 0) {
        if (utf8 == 1)
            *d = utf8_decode(D_089CBBE8, &p);
        else
            *d = jis_to_unicode(D_089CBBE8, sjis_decode(D_089CBBE8, &p));
        U16(self, FONT_TEXT_POS) += 2;
        d++;
        if (U16(self, FONT_TEXT_POS) > 0x2FFF) {
            U16(self, FONT_TEXT_POS) = 0x2FFE;
            over++;
            d = &U16(self, FONT_TEXT + U16(self, FONT_TEXT_POS));
        }
    }
    *d = 0;
    if (over == 0) {
        U16(self, FONT_TEXT_POS) += 2;
        if (U16(self, FONT_TEXT_POS) > 0x2FFF)
            U16(self, FONT_TEXT_POS) = 0x2FFE;
    }
}

/* 1: half-width character, 0: full width, 2: half-width katakana */
u32 font_char_kind(u8 *self, u16 c)
{
    if (c == 0x3035)
        return 1;
    if (c >= 0x250 && c < 0x2000)
        return 0;
    if (c >= 0x2000 && c < 0x2070)
        return 1;
    if (c >= 0x2070 && c < 0x20A0)
        return 0;
    if (c >= 0x20A0 && c < 0x20D0)
        return 1;
    if (c >= 0x20D0 && c < 0xFF60)
        return 0;
    if (c >= 0xFF60 && c < 0xFFE0)
        return 2;
    if (c < 0xFFE0)
        return 1;
    if (c < 0xFFF0)
        return 0;
    return 1;
}

/* layer of the display lists for the font layer being drawn */
u32 font_layer_dl(u8 *self)
{
    switch (F(FONT_DRAW_LAYER)) {
    case 0: return 9;
    case 1: return 10;
    case 2: return 11;
    case 3: return 12;
    case 4: return 13;
    case 5: return 14;
    }
    return 9;
}

/* the color -1 blinks (red-orange) */
void font_blink_color(u8 *self)
{
    f32 x = (f32)(u16)((F(FONT_BLINK) & 0x1F) << 11);
    f32 s = vfpu_sinf(3.1415927f * x / 32768.0f);
    u32 r = (u8)((s8)(s32)(s * 64.0f) + 0x50);
    u32 g = (u8)((s8)(s32)(s * 20.0f) + 0xE4);
    u32 b = (u8)((s8)(s32)(s * 7.0f) + 0xF7);

    font_set_palette(self, -1, 0, b | (r << 16 | 0xFF000000) | g << 8);
}

void font_cache_digits(u8 *self)
{
    font_cache_string(self, D_089AD404);
}

/* the text of a number tag: kind 2 "0" (full width), kind 0/1 a format */
void font_tag_number(u8 *self, char *d, s16 kind, u8 utf8)
{
    char t[0x300];

    if (kind == 2) {
        t[0] = '0';
        t[1] = 0;
        if (utf8 == 1)
            font_to_full_width_utf8(self, t, d);
        else
            font_to_full_width_sjis(self, t, d);
    } else if (kind == 1 || kind == 0) {
        sprintf(d, D_089AD408, kind == 0 ? 0x720 : 0x244);
    }
}

/* icon in the text (up to 10 for each layer) */
void font_add_icon(u8 *self, u16 x, u16 y, u16 id, u8 a, s16 b)
{
    u8 n = F(FONT_ICON_N + (s8)F(FONT_LAYER));
    u8 *e;

    if (n >= 10)
        return;
    if (b == 0xC)
        b = 0xB;
    e = self + (s8)F(FONT_LAYER) * 0x50 + n * 8;
    U16(e, 0x22B18) = x;
    U16(e, 0x22B1A) = y;
    U16(e, 0x22B1C) = id;
    e[0x22B1E] = b;
    e[0x22B1F] = a;
    F(FONT_ICON_N + (s8)F(FONT_LAYER))++;
}

/* advance of the character (w: width of the string entry) */
u32 font_char_advance(u8 *self, u32 ch, s32 half, u32 w)
{
    if (F(FONT_FIXED) == 0) {
        if (half == 0)
            return w & 0xFF;
        return (w & 0xFF) >> 1;
    }
    if (half == 0)
        return w & 0xFF;
    return (w & 0xFF) >> 1;
}

typedef void (*ObjFn)(void *self);
#define OBJ_CALL(o, off) ((ObjFn)((void **)*(void **)(o))[(off) / 4])(o)

/* the 4 scenes: update, and delete the finished ones (then start the next) */
void scenes_update(u8 **self)
{
    u8 **pp = self;
    u8 *o;
    u32 next;
    s32 i;

    for (i = 0; i < 4; i++, pp++) {
        o = *pp;
        if (o == NULL)
            continue;
        if (o[0x11] != 1)
            OBJ_CALL(o, 0xC);
        if (o[0x11] != 1)
            continue;
        next = U32(o, 0x18);
        if (*pp != NULL)
            OBJ_CALL(*pp, 8);
        heap_free((u8 *)self + 0x14010, *pp);
        *pp = NULL;
        if (next != 0)
            scene_start(self, i & 0xFF, next);
    }
}

u32 scene_slot_used(u8 **self, u32 i)
{
    if ((i & 0xFF) >= 4)
        return 0;
    return self[i & 0xFF] != NULL;
}

/* ---- tags in the texts: <%, <Bnn, <Ann, <Cnn, <~ ----
 * mode 0: code of the tag, 1: change of the length, 2: characters shown */

s32 font_count_utf8(u8 *self, const char *s);
s32 font_count_sjis(u8 *self, const u8 *s);
int font_char_count(void *font, const char *s);
size_t font_strlen(u8 *self, const char *s);
s32 utf8_seq_len(void *self, s32 c);
u32 font_first_char_width(u8 *self, const char *s);

static const char *tag_digits(const char *p, u16 *v)
{
    s8 c = *p;

    *v = 0;
    if (c > '/' && c < ':') {
        *v = c - '0';
        p++;
        c = *p;
        if (c > '/' && c < ':') {
            *v = c - '0' + *v * 10;
            p++;
        }
    }
    return p;
}

const char *font_parse_tag(u8 *self, const char *s, u16 *out, s16 mode, u32 utf8)
{
    char buf[0x300];
    const char *p = s + 1;
    u16 v;

    *out = 0;
    switch (*p) {
    case '%':
        p = s + 2;
        if (mode == 2)
            *out = 0;
        else if (mode == 1)
            *out = 0xFFFE;
        else if (mode == 0)
            *out = 0x800;
        return p;
    case 'B':
        p = s + 2;
        if (mode == 2) {
            *out = 1;
            return s + 4;
        }
        if (mode == 1) {
            *out = 0xFFFE;
            return s + 4;
        }
        if (mode != 0)
            return p;
        p = tag_digits(p, &v);
        *out = v | 0x400;
        return p;
    case 'A':
        p = tag_digits(s + 2, &v);
        if (mode == 2) {
            font_tag_number(self, buf, (s16)v, utf8);
            if ((utf8 & 0xFF) == 1)
                *out = font_count_utf8(self, buf);
            else
                *out = font_count_sjis(self, (const u8 *)buf);
            return p;
        }
        if (mode == 1) {
            font_tag_number(self, buf, (s16)v, utf8);
            if ((utf8 & 0xFF) == 1)
                *out = font_char_count(self, buf) - 4;
            else
                *out = font_strlen(self, buf) - 4;
            return p;
        }
        if (mode == 0)
            *out = v | 0x200;
        return p;
    case 'C':
        p = s + 2;
        if (mode == 2) {
            *out = 0;
            return s + 4;
        }
        if (mode == 1) {
            *out = 0xFFFC;
            return s + 4;
        }
        if (mode != 0)
            return p;
        p = tag_digits(p, &v);
        *out = v | 0x100;
        return p;
    case '~':
        if (mode == 2) {
            *out = 1;
            return s + 2;
        }
        if (mode == 1) {
            *out = 0xFFFF;
            return s + 2;
        }
        if (mode == 0)
            *out = 0;
        return p;
    }
    if (mode == 2)
        *out = 0;
    else if (mode == 1)
        *out = 0xFFFF;
    else if (mode == 0)
        *out = 0;
    return p;
}

/* copies the character number n of *pp to out and moves *pp after it; 1 if full width,
 * 4 at the end of the string */
u8 font_get_char(u8 *self, s16 n, u8 *out, const u8 **pp, s8 utf8)
{
    const u8 *p = *pp;
    s16 i = 0;
    s32 k, j, w;

    for (;; i++) {
        if (*p == 0) {
            *out = 0;
            *pp = p;
            return 4;
        }
        if (utf8 == 1) {
            k = utf8_seq_len(D_089CBBE8, *p);
            if (i == n) {
                w = font_first_char_width(self, (const char *)p);
                for (j = 0; j < k; j++)
                    *out++ = *p++;
                *out = 0;
                *pp = p;
                return w == 2;
            }
            for (j = 0; j < k; j++)
                p++;
        } else {
            u32 c = *p;

            if ((c < 0x81 || c > 0x9F) && (c < 0xE0 || c > 0xFF)) {
                if (i == n) {
                    out[0] = c;
                    out[1] = 0;
                    *pp = p + 1;
                    return 0;
                }
            } else {
                if (i == n) {
                    out[0] = c;
                    out[1] = p[1];
                    out[2] = 0;
                    *pp = p + 2;
                    return 1;
                }
                p++;
            }
            p++;
        }
    }
}

extern void *g_heap;
extern const u8 D_08931BFC[];     /* icons: {u, v, w, h} */
s32 temp_get(void *a, u32 b);
int tmh_get_image(void *unused, const char *f, s32 tex, u32 img, u32 pal, u32 *out);

/* draws the n icons of the layer (entries {x, y, size, icon, color} of 8 bytes) */
void font_draw_icons(u8 *self, u8 *e, s8 n, u32 layer)
{
    u32 t[6];
    u32 *p, col;
    u32 u, v, w, h;
    s32 id;
    s8 c;

    tmh_get_image(D_08A62C40, (const char *)temp_get(g_heap, 0), 1, 0, 0, t);
    p = dl_cur(D_08A62C40);
    p[0] = 0x23000000;
    p[1] = 0xDE000000;
    p[2] = 0x21000001;
    p[3] = 0xDF000032;
    p[4] = 0x22000001;
    p[5] = 0xDBFF0003;
    p[6] = 0xC9000100;
    p[7] = 0xC0000000;
    p[8] = 0xC7000101;
    p[9] = 0xC6000101;
    p[10] = 0xC2000001;
    p[11] = t[1] | 0xC3000000;
    p[12] = (t[0] & 0xFFFFFF) | 0xA0000000;
    p[13] = (t[0] & 0xFF000000) >> 8 | 0xA8000000 | U16(t, 8);
    {
        u32 lh = pow2_log(D_08A62C40, U16(t, 0xA));

        p[14] = lh << 8 | 0xB8000000 | pow2_log(D_08A62C40, U16(t, 8));
    }
    p[15] = t[4] | 0xC500FF00;
    p[16] = (t[3] & 0xFFFFFF) | 0xB0000000;
    p[17] = (t[3] & 0xFF000000) >> 8 | 0xB1000000;
    p[18] = t[5] >> 3 | 0xC4000000;
    p[19] = 0xCB000000;
    p[20] = 0x10000000;
    p[21] = 0x08000000;
    dl_link(D_08A62C40, p, 0x16, layer);
    dl_set_cur(D_08A62C40, p + 0x16);
    for (; n != 0; n--, e += 8) {
        id = e[6] * 4;
        u = D_08931BFC[id];
        v = D_08931BFC[id + 1];
        w = D_08931BFC[id + 2];
        h = D_08931BFC[id + 3];
        if ((u32)(e[6] - 10) < 2) {
            if ((s8)e[7] >= 0x30)
                e[7] = 0;
            c = e[7];
            col = (c < 0 ? U32(self, FONT_PALETTE) : U32(self, FONT_COLORS + c * 4)) | 0xFF000000;
        } else {
            col = 0xFFFFFFFF;
        }
        p = dl_cur(D_08A62C40);
        p[0] = v << 16 | u;
        p[1] = col;
        p[2] = U32(e, 0);
        p[3] = U16(self, FONT_DRAW_Z);
        p[4] = (v + h) << 16 | (u + w);
        p[5] = col;
        p[6] = (S16(e, 2) + S16(e, 4)) << 16 | ((S16(e, 0) + S16(e, 4)) & 0xFFFF);
        p[7] = U16(self, FONT_DRAW_Z);
        p[8] = ((u32)p & 0xFF000000) >> 8 | 0x10000000;
        p[9] = ((u32)p & 0xFFFFFF) | 0x01000000;
        p[10] = 0x1E000001;
        p[11] = 0x50000001;
        p[12] = 0x1280011E;
        p[13] = 0x04060002;
        p[14] = 0x10000000;
        p[15] = 0x08000000;
        dl_link(D_08A62C40, p + 8, 8, layer);
        dl_set_cur(D_08A62C40, p + 0x10);
    }
}

/* the accented capital letters (drawn higher) */
u8 font_is_accented_capital(u8 *self, u16 c)
{
    if (c < 0xA0 || c >= 0x200)
        return 0;
    switch (c) {
    case 0x1D3: case 0x1D1: case 0x1CF: case 0x1CD: case 0x17D: case 0x178: case 0x16E:
    case 0x16A: case 0x168: case 0x160: case 0x14C: case 0x12A: case 0x128: case 0x11A:
    case 0x112: case 0x100: case 0xDD: case 0xDC: case 0xDB: case 0xDA: case 0xD9: case 0xD8:
    case 0xD6: case 0xD5: case 0xD4: case 0xD3: case 0xD2: case 0xD1: case 0xD0: case 0xCF:
    case 0xCE: case 0xCD: case 0xCC: case 0xCB: case 0xCA: case 0xC9: case 0xC8: case 0xC6:
    case 0xC5: case 0xC4: case 0xC3: case 0xC2: case 0xC1: case 0xC0:
        return 1;
    }
    return 0;
}
