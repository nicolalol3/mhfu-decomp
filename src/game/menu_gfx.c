/* Menu: 2D drawing primitives (sprites, rectangles, lines) written directly as
 * GE commands in the display list of the menu. Same class as menu_box.c. (Guessed names.)
 *
 * Common pattern: nh words of vertex data followed by nc commands, among which
 * BASE/VADDR (pointing at the vertex data) at positions bi and bi + 1; the
 * commands are then linked in the layer self[MENU_LAYER] by dl_link. */
#include "menu.h"
#include "vfpu.h"
#undef menu_draw_sprite
#undef gfx_fill_rect
#undef gfx_draw_window
#undef gfx_draw_subwindow
#undef gfx_draw_frame
#undef gfx_draw_item_icon_alpha
#undef gfx_fill_rect_pulse
#undef layout_draw_from
#undef layout_build
#undef layout_draw_pages
#undef layout_draw

extern u8 *D_08A62C40;            /* display list manager */
extern const s16 D_089391A8[][6];  /* menu sprites: s16 u, v, w, h, u16 texture[2] */

u32 *dl_cur(void *dl);
void dl_link(void *dl, u32 *p, int n, u8 layer);
void dl_set_cur(void *dl, u32 *end);
void tex_select(void *self, u16 a, u16 b);
void tex_select_sprite(void *self, const s16 *sprite);

/* copies the nh + nc words of w at the end of the display list and links them */
#define DL_PUSH(dl, layer, w, nh, nc, bi) do {                                         \
        u32 cur_ = U32(dl, 0x80198);                                                \
        s32 used_ = (s32)(cur_ - (u32)((dl) + U32(dl, 4) * 0x40000 + 0x198)) / 4;   \
        if ((u32)(used_ + (nh) + (nc)) < 0x10000) {                                 \
            int i_;                                                                 \
            (w)[bi] = ((cur_ & 0xFF000000) >> 8) | 0x10000000;               \
            (w)[(bi) + 1] = (U32(dl, 0x80198) & 0xFFFFFF) | 0x01000000;            \
            for (i_ = 0; i_ < (nh) + (nc); i_++) {                                  \
                **(u32 **)((dl) + 0x80198) = (w)[i_];                               \
                U32(dl, 0x80198) += 4;                                              \
            }                                                                       \
            dl_link(dl, (u32 *)(cur_ + (nh) * 4), nc, layer);                \
        }                                                                           \
    } while (0)

/* texture rectangle (0,0)-(w,h) of the texture of sprite id, at (x, y) */
void gfx_draw_tex_rect(u8 *self, u32 x, u32 y, u32 w, u32 h, s32 id)
{
    u32 *p;

    tex_select_sprite(self, D_089391A8[id]);
    p = dl_cur(D_08A62C40);
    p[0] = 0;
    p[1] = ((x & 0xFFFF) << 16) | 0xFFFF;
    p[2] = y & 0xFFFF;
    p[3] = ((h & 0xFFFF) << 16) | (w & 0xFFFF);
    p[4] = (((x & 0xFFFF) + (w & 0xFFFF)) << 16) | 0xFFFF;
    p[5] = ((y & 0xFFFF) + (h & 0xFFFF)) & 0xFFFF;
    p[6] = (((u32)p & 0xFF000000) >> 8) | 0x10000000;
    p[7] = ((u32)p & 0xFFFFFF) | 0x01000000;
    p[8] = 0x1E000001;
    p[9] = 0x50000001;
    p[10] = 0x12800116;
    p[11] = 0x04060002;
    p[12] = 0x10000000;
    p[13] = 0x08000000;
    dl_link(D_08A62C40, p + 6, 8, self[MENU_LAYER]);
    dl_set_cur(D_08A62C40, p + 14);
}

/* colored texture rectangle (0,0)-(w,h) of the texture of sprite id */
void gfx_draw_tex_rect_color(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 r, u32 g, u32 b, u8 a, s32 id)
{
    u32 *p;
    u32 c;

    tex_select_sprite(self, D_089391A8[id]);
    p = dl_cur(D_08A62C40);
    p[0] = 0x10000000;
    p[1] = 0x08000000;
    dl_link(D_08A62C40, p, 2, self[MENU_LAYER]);
    dl_set_cur(D_08A62C40, p + 2);
    p = dl_cur(D_08A62C40);
    c = ((u32)a << 24) | ((b & 0xFF) << 16) | (r & 0xFF) | ((g & 0xFF) << 8);
    p[0] = 0;
    p[1] = c;
    p[2] = ((y & 0xFFFF) << 16) | (x & 0xFFFF);
    p[3] = 0;
    p[4] = ((h & 0xFFFF) << 16) | (w & 0xFFFF);
    p[5] = c;
    p[6] = (((y & 0xFFFF) + (h & 0xFFFF)) << 16) | (((x & 0xFFFF) + (w & 0xFFFF)) & 0xFFFF);
    p[7] = 0;
    p[8] = (((u32)p & 0xFF000000) >> 8) | 0x10000000;
    p[9] = ((u32)p & 0xFFFFFF) | 0x01000000;
    p[10] = 0x1E000001;
    p[11] = 0x50000001;
    p[12] = 0x1280011E;
    p[13] = 0x04060002;
    p[14] = 0x10000000;
    p[15] = 0x08000000;
    dl_link(D_08A62C40, p + 8, 8, self[MENU_LAYER]);
    dl_set_cur(D_08A62C40, p + 16);
}

/* sprite id stretched to w x h at (x, y) */
void gfx_draw_sprite_scaled(u8 *self, u32 x, u32 y, u32 w, u32 h, s32 id)
{
    const s16 *t = D_089391A8[id];
    u8 *dl;
    u32 v[15];
    u8 layer;

    tex_select(self, t[4], t[5]);
    dl = D_08A62C40;
    v[4] = (((x & 0xFFFF) + (w & 0xFFFF)) << 16) | 0xFFFF;
    v[5] = ((y & 0xFFFF) + (h & 0xFFFF)) & 0xFFFF;
    v[6] = 0x1E000001;
    v[8] = 0x50000001;
    v[9] = 0x12800116;
    v[12] = 0x04060002;
    v[13] = 0x10000000;
    v[14] = 0x08000000;
    v[2] = y & 0xFFFF;
    v[7] = 0xC9000100;
    v[1] = ((x & 0xFFFF) << 16) | 0xFFFF;
    v[3] = ((u32)(t[1] + t[3]) << 16) | (u32)(t[0] + t[2]);
    v[0] = ((u32)t[1] << 16) | (u32)(s32)t[0];
    layer = self[MENU_LAYER];
    if (layer < 20)
        DL_PUSH(dl, layer, v, 6, 9, 10);
}

/* sprite id with vertex color (r, g, b, a); in language 2 the sprite 0x2F gets an
 * extra piece of texture */
void gfx_draw_sprite_color(u8 *self, u32 x, u32 y, u32 r, u32 g, u32 b, u32 a, s32 id)
{
    const s16 *t = D_089391A8[id];
    u8 *dl;
    u32 v[17], e[17];
    u32 c, xm, ym;
    s32 h;
    u8 layer;

    tex_select(self, t[4], t[5]);
    dl = D_08A62C40;
    c = (a << 24) | ((b & 0xFF) << 16) | (r & 0xFF) | ((g & 0xFF) << 8);
    xm = x & 0xFFFF;
    ym = y & 0xFFFF;
    v[8] = e[8] = 0x1E000001;
    v[10] = e[10] = 0x50000001;
    v[11] = e[11] = 0x1280011E;
    v[14] = e[14] = 0x04060002;
    v[15] = e[15] = 0x10000000;
    v[16] = e[16] = 0x08000000;
    v[9] = e[9] = 0xC9000100;
    h = t[3];
    v[0] = ((u32)t[1] << 16) | (u32)(s32)t[0];
    v[2] = (ym << 16) | xm;
    v[4] = ((u32)(t[1] + h) << 16) | (u32)(t[0] + t[2]);
    v[6] = ((ym + h) << 16) | ((xm + t[2]) & 0xFFFF);
    v[1] = c;
    v[3] = 0;
    v[5] = c;
    v[7] = 0;
    if (LANG == 2 && id == 0x2F) {
        e[4] = ((u32)(h + 152) << 16) | 12;
        e[0] = 0x00980000;
        e[2] = (ym << 16) | ((xm + 104) & 0xFFFF);
        e[6] = ((xm + 116) & 0xFFFF) | ((ym + h) << 16);
        e[1] = c;
        e[5] = c;
        e[3] = 0;
        e[7] = 0;
        layer = self[MENU_LAYER];
        if (layer < 20)
            DL_PUSH(dl, layer, e, 8, 9, 12);
    }
    dl = D_08A62C40;
    layer = self[MENU_LAYER];
    if (layer < 20)
        DL_PUSH(dl, layer, v, 8, 9, 12);
}

/* sprite id with a 16-bit color; flip: 1 horizontal, 2 vertical, 3 both */
void gfx_draw_sprite(u8 *self, u32 x, u32 y, u32 r, u32 g, u32 b, s32 id, s32 flip)
{
    const s16 *t = D_089391A8[id];
    u8 *dl;
    u32 v[15];
    u32 c, u0, v0, u1, v1, ua, ub, va, vb;
    u8 layer;

    tex_select(self, t[4], t[5]);
    dl = D_08A62C40;
    u0 = (u16)t[0];
    c = (((b & 0xFF) >> 3) << 10) | ((r & 0xFF) >> 3) | (((g & 0xFF) >> 3) << 5) | 0x8000;
    v0 = (u16)t[1];
    u1 = (u0 + t[2]) & 0xFFFF;
    v1 = (v0 + t[3]) & 0xFFFF;
    if (flip == 3 || flip == 1) {
        ua = u1;
        ub = u0;
    } else {
        ua = u0;
        ub = u1;
    }
    if ((u32)(flip - 2) < 2) {
        va = v1;
        vb = v0;
    } else {
        va = v0;
        vb = v1;
    }
    v[0] = (va << 16) | ua;
    v[1] = (x << 16) | c;
    v[2] = y & 0xFFFF;
    v[3] = (vb << 16) | ub;
    v[4] = (((x & 0xFFFF) + t[2]) << 16) | c;
    v[5] = ((y & 0xFFFF) + t[3]) & 0xFFFF;
    v[6] = 0x1E000001;
    v[8] = 0x50000001;
    v[9] = 0x12800116;
    v[12] = 0x04060002;
    v[13] = 0x10000000;
    v[14] = 0x08000000;
    v[7] = 0xC9000100;
    layer = self[MENU_LAYER];
    if (layer < 20)
        DL_PUSH(dl, layer, v, 6, 9, 10);
}

/* fills the vertex data of a colored sprite: uv of sprite t, positions (x, y)-(x2, y2) */
#define SPRITE_VERTS_C(v, t, c, x, y, x2, y2) do {                        \
        (v)[0] = ((u32)(t)[1] << 16) | (u32)(s32)(t)[0];                     \
        (v)[1] = (c);                                                       \
        (v)[2] = ((u32)(y) << 16) | ((x) & 0xFFFF);                          \
        (v)[3] = 0;                                                         \
        (v)[4] = ((u32)((t)[1] + (t)[3]) << 16) | (u32)((t)[0] + (t)[2]);   \
        (v)[5] = (c);                                                       \
        (v)[6] = ((u32)(y2) << 16) | ((x2) & 0xFFFF);                        \
        (v)[7] = 0;                                                         \
    } while (0)

#define CMDS_C(v) do {                                                      \
        (v)[8] = 0x1E000001; (v)[9] = 0xC9000100; (v)[10] = 0x50000001;     \
        (v)[11] = 0x1280011E; (v)[14] = 0x04060002; (v)[15] = 0x10000000;   \
        (v)[16] = 0x08000000;                                               \
    } while (0)

/* sprite id with vertex color (r, g, b, a) */
void gfx_draw_sprite_rgba(u8 *self, u32 x, u32 y, u32 r, u32 g, u32 b, u32 a, s32 id)
{
    const s16 *t = D_089391A8[id];
    u8 *dl;
    u32 v[17];
    u32 c;
    u8 layer;

    tex_select(self, t[4], t[5]);
    dl = D_08A62C40;
    c = (a << 24) | ((b & 0xFF) << 16) | (r & 0xFF) | ((g & 0xFF) << 8);
    CMDS_C(v);
    SPRITE_VERTS_C(v, t, c, x, y & 0xFFFF, (x & 0xFFFF) + t[2], (y & 0xFFFF) + t[3]);
    layer = self[MENU_LAYER];
    if (layer < 20)
        DL_PUSH(dl, layer, v, 8, 9, 12);
}

/* sprite id with the vertex color read from c (r, g, b, a bytes) */
void gfx_draw_sprite_rgba_ptr(u8 *self, u32 x, u32 y, const u8 *cp, s32 id)
{
    const s16 *t = D_089391A8[id];
    u8 *dl;
    u32 v[17];
    u32 c;
    u8 layer;

    tex_select(self, t[4], t[5]);
    dl = D_08A62C40;
    CMDS_C(v);
    c = ((u32)cp[3] << 24) | ((u32)cp[2] << 16) | cp[0] | ((u32)cp[1] << 8);
    SPRITE_VERTS_C(v, t, c, x, y & 0xFFFF, (x & 0xFFFF) + t[2], (y & 0xFFFF) + t[3]);
    layer = self[MENU_LAYER];
    if (layer < 20)
        DL_PUSH(dl, layer, v, 8, 9, 12);
}

/* sprite id stretched to w x h, with a 16-bit color */
void gfx_draw_sprite_scaled_color(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 r, u32 g, u32 b, s32 id)
{
    const s16 *t = D_089391A8[id];
    u8 *dl;
    u32 v[15];
    u32 c;
    u8 layer;

    tex_select(self, t[4], t[5]);
    dl = D_08A62C40;
    c = (((b & 0xFF) >> 3) << 10) | ((r & 0xFF) >> 3) | (((g & 0xFF) >> 3) << 5) | 0x8000;
    v[4] = (((x & 0xFFFF) + (w & 0xFFFF)) << 16) | c;
    v[5] = ((y & 0xFFFF) + (h & 0xFFFF)) & 0xFFFF;
    v[6] = 0x1E000001;
    v[8] = 0x50000001;
    v[9] = 0x12800116;
    v[12] = 0x04060002;
    v[13] = 0x10000000;
    v[14] = 0x08000000;
    v[2] = y & 0xFFFF;
    v[7] = 0xC9000100;
    v[1] = (x << 16) | c;
    v[3] = ((u32)(t[1] + t[3]) << 16) | (u32)(t[0] + t[2]);
    v[0] = ((u32)t[1] << 16) | (u32)(s32)t[0];
    layer = self[MENU_LAYER];
    if (layer < 20)
        DL_PUSH(dl, layer, v, 6, 9, 10);
}

/* sprite id stretched to w x h, with vertex color (r, g, b, a) */
void gfx_draw_sprite_scaled_rgba(u8 *self, u32 x, u32 y, s32 w_arg, s32 h_arg, u32 r, u32 g, u32 b,
                                 u8 a, s32 id)
{
    s16 w = w_arg;
    s16 h = h_arg;
    const s16 *t = D_089391A8[id];
    u8 *dl;
    u32 v[17];
    u32 c;
    u8 layer;

    tex_select(self, t[4], t[5]);
    dl = D_08A62C40;
    CMDS_C(v);
    c = ((u32)a << 24) | ((b & 0xFF) << 16) | (r & 0xFF) | ((g & 0xFF) << 8);
    SPRITE_VERTS_C(v, t, c, x, y, (x & 0xFFFF) + w, (y & 0xFFFF) + h);
    layer = self[MENU_LAYER];
    if (layer < 20)
        DL_PUSH(dl, layer, v, 8, 9, 12);
}

/* part of sprite id stretched to w x h, with a 16-bit color:
 * part 0 right half, 1 left half, 2 right quarter, 3 left quarter */
void gfx_draw_sprite_part(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 r, u32 g, u32 b, s32 part, s32 id)
{
    const s16 *t = D_089391A8[id];
    u8 *dl;
    u32 v[15];
    u32 c;
    u8 layer;

    tex_select(self, t[4], t[5]);
    dl = D_08A62C40;
    v[6] = 0x1E000001;
    v[8] = 0x50000001;
    v[9] = 0x12800116;
    v[12] = 0x04060002;
    v[13] = 0x10000000;
    v[14] = 0x08000000;
    v[7] = 0xC9000100;
    if (part == 3) {
        v[0] = ((u32)t[1] << 16) | (u32)(s32)t[0];
        v[3] = ((u32)(t[1] + t[3]) << 16) | (u32)(t[0] + t[2] / 4);
    } else if (part == 2) {
        v[0] = ((u32)t[1] << 16) | (u32)(t[0] + t[2] - t[2] / 4);
        v[3] = ((u32)(t[1] + t[3]) << 16) | (u32)(t[0] + t[2]);
    } else if (part == 1) {
        v[0] = ((u32)t[1] << 16) | (u32)(s32)t[0];
        v[3] = ((u32)(t[1] + t[3]) << 16) | (u32)(t[0] + t[2] / 2);
    } else if (part == 0) {
        v[0] = ((u32)t[1] << 16) | (u32)(t[0] + t[2] / 2);
        v[3] = ((u32)(t[1] + t[3]) << 16) | (u32)(t[0] + t[2]);
    }
    c = (((b & 0xFF) >> 3) << 10) | ((r & 0xFF) >> 3) | (((g & 0xFF) >> 3) << 5) | 0x8000;
    v[4] = (((x & 0xFFFF) + (w & 0xFFFF)) << 16) | c;
    v[5] = ((y & 0xFFFF) + (h & 0xFFFF)) & 0xFFFF;
    v[2] = y & 0xFFFF;
    v[1] = (x << 16) | c;
    layer = self[MENU_LAYER];
    if (layer < 20)
        DL_PUSH(dl, layer, v, 6, 9, 10);
}

/* textured quadrilateral (4 points, triangle strip) with a 16-bit color */
void gfx_draw_quad(u8 *self, const s16 *pt, u32 r, u32 g, u32 b, s32 id)
{
    const s16 *t = D_089391A8[id];
    u8 *dl;
    u32 v[20];
    u32 c, u0, v0, v1;
    u8 layer;

    tex_select(self, t[4], t[5]);
    dl = D_08A62C40;
    v[12] = 0x1E000001;
    v[13] = 0x50000001;
    v[14] = 0x12800116;
    v[17] = 0x04040004;
    v[18] = 0x10000000;
    v[19] = 0x08000000;
    u0 = (u32)(s32)t[0];
    v0 = (u32)t[1] << 16;
    v[0] = v0 | u0;
    c = (((b & 0xFF) >> 3) << 10) | ((r & 0xFF) >> 3) | (((g & 0xFF) >> 3) << 5) | 0x8000;
    v[1] = ((u32)pt[0] << 16) | c;
    v[2] = (u16)pt[1];
    v1 = (u32)(t[1] + t[3]) << 16;
    v[3] = u0 | v1;
    v[6] = v0 | (u0 + t[2]);
    v[9] = v1 | (u0 + t[2]);
    v[5] = (u16)pt[3];
    v[7] = ((u32)pt[4] << 16) | c;
    v[4] = ((u32)pt[2] << 16) | c;
    v[10] = ((u32)pt[6] << 16) | c;
    v[11] = (u16)pt[7];
    v[8] = (u16)pt[5];
    layer = self[MENU_LAYER];
    if (layer < 20)
        DL_PUSH(dl, layer, v, 12, 8, 15);
}

/* texture coordinate word (v << 16 | u), with the sign of u kept as the original */
#define UV(u, v) (((u32)(v) << 16) | (u32)(s32)(u))

/* sprite id stretched to w x h with vertex color, mirrored / rotated by mode (0..5) */
void gfx_draw_sprite_mode(u8 *self, u32 x, u32 y, s32 w_arg, s32 h_arg, u32 r, u32 g, u32 b, u8 a,
                          s32 id, u8 mode)
{
    s16 w = w_arg;
    s16 h = h_arg;
    const s16 *t = D_089391A8[id];
    u8 *dl;
    u32 v[17];
    u32 c, pa, pb;
    u8 layer;

    tex_select(self, t[4], t[5]);
    dl = D_08A62C40;
    CMDS_C(v);
    v[3] = 0;
    c = ((u32)a << 24) | ((b & 0xFF) << 16) | (r & 0xFF) | ((g & 0xFF) << 8);
    v[1] = c;
    v[7] = 0;
    v[5] = c;
    pa = (y << 16) | (x & 0xFFFF);
    pb = (((y & 0xFFFF) + h) << 16) | (((x & 0xFFFF) + w) & 0xFFFF);
    switch (mode) {
    case 0:
        v[2] = pa;
        v[6] = pb;
        v[0] = UV(t[0], t[1]);
        v[4] = UV(t[0] + t[2], t[1] + t[3]);
        break;
    case 1:
        v[2] = pa;
        v[6] = pb;
        v[0] = UV(t[0] + t[2], t[1]);
        v[4] = UV(t[0], t[1] + t[3]);
        break;
    case 2:
        v[2] = (x & 0xFFFF) | (((y & 0xFFFF) + h) << 16);
        v[6] = (y << 16) | (((x & 0xFFFF) + w) & 0xFFFF);
        v[0] = UV(t[0], t[1]);
        v[4] = UV(t[0] + t[2], t[1] + t[3]);
        break;
    case 3:
        v[2] = (x & 0xFFFF) | (((y & 0xFFFF) + h) << 16);
        v[6] = (y << 16) | (((x & 0xFFFF) + w) & 0xFFFF);
        v[0] = UV(t[0] + t[2], t[1]);
        v[4] = UV(t[0], t[1] + t[3]);
        break;
    case 4:
        v[2] = pa;
        v[6] = pb;
        v[0] = UV(t[0], t[1] + t[3]);
        v[4] = UV(t[0] + t[2], t[1]);
        break;
    case 5:
        v[2] = pa;
        v[6] = pb;
        v[0] = UV(t[0] + t[2], t[1] + t[3]);
        v[4] = UV(t[0], t[1]);
        break;
    }
    layer = self[MENU_LAYER];
    if (layer < 20)
        DL_PUSH(dl, layer, v, 8, 9, 12);
}

/* textured quadrilateral with vertex color; pt = 4 points (x, y) */
void gfx_draw_quad_rgba(u8 *self, const s16 *pt, u32 r, u32 g, u32 b, u32 a, s32 id)
{
    const s16 *t = D_089391A8[id];
    u8 *dl;
    u32 v[24];
    u32 c;
    u8 layer;

    tex_select(self, t[4], t[5]);
    dl = D_08A62C40;
    c = (a << 24) | ((b & 0xFF) << 16) | (r & 0xFF) | ((g & 0xFF) << 8);
    v[1] = v[5] = v[9] = v[13] = c;
    v[16] = 0x1E000001;
    v[17] = 0x50000001;
    v[18] = 0x1280011E;
    v[21] = 0x04040004;
    v[22] = 0x10000000;
    v[23] = 0x08000000;
    v[0] = UV(t[0], t[1]);
    v[2] = (((u32)pt[1] << 16) | (u16)pt[0]);
    v[4] = UV(t[0], t[1] + t[3]);
    v[8] = UV(t[0] + t[2], t[1]);
    v[12] = UV(t[0] + t[2], t[1] + t[3]);
    v[6] = (((u32)pt[3] << 16) | (u16)pt[2]);
    v[14] = (((u32)pt[7] << 16) | (u16)pt[6]);
    v[3] = 0;
    v[7] = 0;
    v[11] = 0;
    v[15] = 0;
    v[10] = (((u32)pt[5] << 16) | (u16)pt[4]);
    layer = self[MENU_LAYER];
    if (layer < 20)
        DL_PUSH(dl, layer, v, 16, 8, 19);
}

extern const f32 D_08938718[4][2];  /* corners of a rotated sprite (fractions of w, h) */
extern const f32 D_08938738[4][2];

/* sprite id of size w x h rotated by ang (0x10000 = full turn) around (x, y) */
void gfx_draw_sprite_rotated(void *self, u32 x, u32 y, u32 w, u32 h, s32 ang, u32 a, s32 id)
{
    s16 pt[8];
    f32 r = (3.14159265f / 32768.0f) * -(f32)ang;
    f32 s = vfpu_sinf(r);
    f32 c = vfpu_cosf(r);
    f32 fw = (f32)w;
    f32 fh = (f32)h;
    int i;

    for (i = 0; i < 4; i++) {
        f32 dx = fw * D_08938718[i][0];
        f32 dy = fh * D_08938718[i][1];

        pt[i * 2] = (u16)x + (s16)(int)(c * dx + s * dy);
        pt[i * 2 + 1] = (u16)y + (s16)(int)(-s * dx + c * dy);
    }
    gfx_draw_quad_rgba(self, pt, 255, 255, 255, a, id);
}

/* sprite id of size w x h rotated by ang around (x, y), with the pivot (px, py) */
void gfx_draw_sprite_rotated_pivot(void *self, u32 x, u32 y, s32 px, s32 py, u32 w, u32 h, s32 ang,
                                   u8 a, s32 id)
{
    s16 pt[8];
    f32 r = (3.14159265f / 32768.0f) * -(f32)ang;
    f32 s = vfpu_sinf(r);
    f32 c = vfpu_cosf(r);
    f32 fw = (f32)w;
    f32 fh = (f32)h;
    int i;

    for (i = 0; i < 4; i++) {
        f32 dx = fw * D_08938738[i][0] - (f32)px;
        f32 dy = fh * D_08938738[i][1] - (f32)py;

        pt[i * 2] = (u16)x + (s16)(int)(c * dx + s * dy);
        pt[i * 2 + 1] = (u16)y + (s16)(int)(-s * dx + c * dy);
    }
    gfx_draw_quad_rgba(self, pt, 255, 255, 255, a, id);
}

/* solid rectangle with a 16-bit color */
void gfx_fill_rect(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 r, u32 g, u32 b)
{
    u8 *dl = D_08A62C40;
    u32 v[12];
    u32 c;
    u8 layer;

    c = (((b & 0xFF) >> 3) << 10) | ((r & 0xFF) >> 3) | (((g & 0xFF) >> 3) << 5) | 0x8000;
    v[3] = ((y & 0xFFFF) + (h & 0xFFFF)) & 0xFFFF;
    v[4] = 0x1E000000;
    v[5] = 0x50000001;
    v[6] = 0x12800114;
    v[9] = 0x04060002;
    v[10] = 0x10000000;
    v[11] = 0x08000000;
    v[0] = (x << 16) | c;
    v[2] = (((x & 0xFFFF) + (w & 0xFFFF)) << 16) | c;
    v[1] = y & 0xFFFF;
    layer = self[MENU_LAYER];
    if (layer < 20)
        DL_PUSH(dl, layer, v, 4, 8, 7);
}

/* solid rectangle with color (r, g, b, a) */
void gfx_fill_rect_rgba(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 r, u32 g, u32 b, u8 a)
{
    u8 *dl = D_08A62C40;
    u32 v[14];
    u8 layer;

    v[6] = 0x1E000000;
    v[7] = 0x50000001;
    v[8] = 0x1280011C;
    v[11] = 0x04060002;
    v[12] = 0x10000000;
    v[13] = 0x08000000;
    v[0] = ((u32)a << 24) | ((b & 0xFF) << 16) | (r & 0xFF) | ((g & 0xFF) << 8);
    v[4] = (((y & 0xFFFF) + (h & 0xFFFF)) << 16) | (((x & 0xFFFF) + (w & 0xFFFF)) & 0xFFFF);
    v[1] = (y << 16) | (x & 0xFFFF);
    v[3] = v[0];
    v[2] = 0;
    v[5] = 0;
    layer = self[MENU_LAYER];
    if (layer < 20)
        DL_PUSH(dl, layer, v, 6, 8, 9);
}

/* line from (x0, y0) to (x1, y1) with a 16-bit color */
void gfx_draw_line(u8 *self, u32 x0, u32 y0, u32 x1, u32 y1, u32 r, u32 g, u32 b)
{
    u8 *dl = D_08A62C40;
    u32 v[12];
    u32 c;
    u8 layer;

    c = (((b & 0xFF) >> 3) << 10) | ((r & 0xFF) >> 3) | (((g & 0xFF) >> 3) << 5) | 0x8000;
    v[0] = (x0 << 16) | c;
    v[1] = y0 & 0xFFFF;
    v[3] = y1 & 0xFFFF;
    v[4] = 0x1E000000;
    v[5] = 0x50000001;
    v[6] = 0x12800114;
    v[9] = 0x04010002;
    v[10] = 0x10000000;
    v[11] = 0x08000000;
    v[2] = (x1 << 16) | c;
    layer = self[MENU_LAYER];
    if (layer < 20)
        DL_PUSH(dl, layer, v, 4, 8, 7);
}

void menu_draw_sprite(u8 *self, u32 x, u32 y, s32 id);

/* window frame made of the 9 sprites base..base+8 (corners, edges, center) */
void gfx_draw_frame(u8 *self, u32 x, u32 y, u32 w, u32 h_arg, u32 alpha, s32 base)
{
    u16 h = h_arg;
    const s16 *t = D_089391A8[base];
    u32 x2, x3, y2, yb, w16;

    if ((alpha & 0xFF) == 0xFF) {
        x2 = (x & 0xFFFF) + t[2];
        menu_draw_sprite(self, x, y, base);
        w16 = w & 0xFFFF;
        gfx_draw_tex_rect(self, x2 & 0xFFFF, y, (w16 - t[2] * 2) & 0xFFFF, (u16)t[3], base + 1);
        x3 = (x & 0xFFFF) + w16;
        menu_draw_sprite(self, (x3 - t[2]) & 0xFFFF, y, base + 2);
        y2 = y & 0xFFFF;
        gfx_draw_tex_rect(self, x, (y2 + t[3]) & 0xFFFF, (u16)t[2], (h - t[3] * 2) & 0xFFFF, base + 3);
        gfx_draw_tex_rect(self, (x3 - t[2]) & 0xFFFF, (y2 + t[3]) & 0xFFFF, (u16)t[2],
                          (h - t[3] * 2) & 0xFFFF, base + 4);
        gfx_draw_tex_rect(self, x2 & 0xFFFF, (y2 + t[3]) & 0xFFFF, (w16 - t[2] * 2) & 0xFFFF,
                          (h - t[3] * 2) & 0xFFFF, base + 8);
        yb = y2 + h - t[3];
        menu_draw_sprite(self, x, yb & 0xFFFF, base + 5);
        gfx_draw_tex_rect(self, x2 & 0xFFFF, yb & 0xFFFF, (w16 - t[2] * 2) & 0xFFFF, (u16)t[3], base + 6);
        menu_draw_sprite(self, (x3 - t[2]) & 0xFFFF, yb & 0xFFFF, base + 7);
    } else {
        x2 = (x & 0xFFFF) + t[2];
        gfx_draw_sprite_rgba(self, x, y, 255, 255, 255, alpha, base);
        w16 = w & 0xFFFF;
        gfx_draw_tex_rect_color(self, x2 & 0xFFFF, y, (w16 - t[2] * 2) & 0xFFFF, (u16)t[3],
                                255, 255, 255, alpha, base + 1);
        x3 = (x & 0xFFFF) + w16;
        gfx_draw_sprite_rgba(self, (x3 - t[2]) & 0xFFFF, y, 255, 255, 255, alpha, base + 2);
        y2 = y & 0xFFFF;
        gfx_draw_tex_rect_color(self, x, (y2 + t[3]) & 0xFFFF, (u16)t[2], (h - t[3] * 2) & 0xFFFF,
                                255, 255, 255, alpha, base + 3);
        gfx_draw_tex_rect_color(self, (x3 - t[2]) & 0xFFFF, (y2 + t[3]) & 0xFFFF, (u16)t[2],
                                (h - t[3] * 2) & 0xFFFF, 255, 255, 255, alpha, base + 4);
        gfx_draw_tex_rect_color(self, x2 & 0xFFFF, (y2 + t[3]) & 0xFFFF, (w16 - t[2] * 2) & 0xFFFF,
                                (h - t[3] * 2) & 0xFFFF, 255, 255, 255, alpha, base + 8);
        yb = y2 + h - t[3];
        gfx_draw_sprite_rgba(self, x, yb & 0xFFFF, 255, 255, 255, alpha, base + 5);
        gfx_draw_tex_rect_color(self, x2 & 0xFFFF, yb & 0xFFFF, (w16 - t[2] * 2) & 0xFFFF, (u16)t[3],
                                255, 255, 255, alpha, base + 6);
        gfx_draw_sprite_rgba(self, (x3 - t[2]) & 0xFFFF, yb & 0xFFFF, 255, 255, 255, alpha, base + 7);
    }
}

/* menu window (frame sprites 64..72) */
void gfx_draw_window(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 alpha)
{
    gfx_draw_frame(self, x, y, w, h, alpha, 64);
}

/* sub-window (frame sprites 158..166) */
void gfx_draw_subwindow(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 alpha)
{
    gfx_draw_frame(self, x, y, w, h, alpha, 158);
}

extern void *g_equip_data;          /* equipment data */
extern const u8 D_0893AA5C[];      /* item colors (r, g, b, -) */
s32 func_088269C0(void *self, u32 id);
u32 item_icon_sprite(void *self, s32 icon, int small);
void gfx_set_filter(void *self, int on);
u8 equip_rarity(void *a, int type, u16 id);
u32 func_088D7D6C(void *hunter);

/* icon of item id (greyed out if not usable, with a mark if it is new);
 * mode != 0 draws half of the icon */
void gfx_draw_item_icon(u8 *self, u32 id, u32 x, u32 y, u32 w, u32 h, s8 mode)
{
    const u8 *e = D_0899D198 + (id & 0xFFFF) * 0x18;
    const u8 *c;
    u32 col, tex;
    int st = 0;

    if (e[5] == 0xFF)
        return;
    if ((u8)func_088269C0(self, id) == 0) {
        col = 10;
        st = -1;
    } else {
        col = e[6];
        if (e[4] & 1)
            st = 1;
    }
    tex = item_icon_sprite(self, e[5], (w & 0xFFFF) < 21);
    c = D_0893AA5C + col * 4;
    if (mode == 0)
        gfx_draw_sprite_scaled_color(self, x, y, w, h, c[0], c[1], c[2], tex);
    else
        gfx_draw_sprite_part(self, x, y, w, h, c[0], c[1], c[2], mode <= 0, tex);
    if (st == 0)
        return;
    if (st > 0)
        gfx_draw_sprite_scaled(self, x, y, w, h, (w & 0xFFFF) < 21 ? 238 : 132);
    else
        gfx_draw_sprite_scaled_color(self, x, y, w, h, 255, 0, 0, 133);
}

/* icon of item id with alpha; with mode 1 adds the "new" mark or the red cross
 * when the hunter cannot use the item with the current weapon */
void gfx_draw_item_icon_alpha(u8 *self, u32 id, u32 x, u32 y, u32 w, s32 h_arg, u8 mode, u8 alpha)
{
    s16 h = h_arg;
    const u8 *e = D_0899D198 + (id & 0xFFFF) * 0x18;
    const u8 *c;
    u32 w16, tex;

    if (e[5] == 0xFF)
        return;
    gfx_set_filter(self, 1);
    w16 = w & 0xFFFF;
    tex = item_icon_sprite(self, e[5], w16 < 21);
    c = D_0893AA5C + e[6] * 4;
    gfx_draw_sprite_scaled_rgba(self, x, y, (s16)w, h, c[0], c[1], c[2], alpha, tex);
    if (mode == 1) {
        if ((e[4] & 1) != 0) {
            gfx_draw_sprite_scaled_rgba(self, x, y, (s16)w, h, 255, 255, 255, alpha, w16 < 21 ? 238 : 132);
        } else if (w16 > 20) {
            u8 *hunter = *(u8 **)(self + MENU_PLAYER);
            s8 k = equip_rarity(g_equip_data, hunter[0x4E9], U16(hunter, 0x4EA));
            u8 m6, m3;
            int bad = 0;

            if (k == 10) {
                m6 = 4;
                m3 = 2;
            } else if (k == 5 || k == 1) {
                m6 = 2;
                m3 = 4;
            } else {
                m6 = 0;
                m3 = 6;
            }
            if ((e[1] & m3) != 0)
                bad = 1;
            else if ((e[1] & m6) != 0
                     && ((1 << (S16(e, 8) & 31)) & func_088D7D6C(*(u8 **)(self + MENU_PLAYER))) == 0)
                bad = 1;
            if (bad)
                gfx_draw_sprite_scaled_color(self, x, y, w, h, 255, 0, 0, 133);
        }
    }
    gfx_set_filter(self, 0);
}

/* icon number icon (-1 = none) with the color col */
void gfx_draw_icon(u8 *self, s32 icon_arg, u32 col, u32 x, u32 y, u32 w, u32 h)
{
    s8 icon = icon_arg;
    const u8 *c;

    if (icon == -1)
        return;
    c = D_0893AA5C + (col & 0xFF) * 4;
    gfx_draw_sprite_scaled_color(self, x, y, w, h, c[0], c[1], c[2],
                                 item_icon_sprite(self, icon, (w & 0xFFFF) < 21));
}

extern const s16 D_08939854, D_08939856;   /* size of the cursor arrows */

/* pair of cursor arrows (mirrored) at x and x + dx */
void gfx_draw_arrows(u8 *self, u32 x, u32 y, u32 dx, u32 r, u32 g, u32 b, u8 a)
{
    gfx_draw_sprite_mode(self, x, y, D_08939854, D_08939856, r, g, b, a, 142, 1);
    gfx_draw_sprite_scaled_rgba(self, ((x & 0xFFFF) + (dx & 0xFFFF)) & 0xFFFF, y, D_08939854, D_08939856,
                                r, g, b, a, 142);
}

/* pulsing value 0..: sine of the frame counter (period 64 frames) */
static s32 gfx_pulse(void)
{
    f32 s = vfpu_sinf((f32)(u16)((*(u32 *)(g_sys + 0xF7A554) & 0x3F) << 10) * (3.14159265f / 32768.0f));

    return (s8)(int)(48.0f * s) + 175;
}

/* blinking cursor arrows */
void gfx_draw_arrows_blink(u8 *self, u32 x, u32 y, u32 dx, u8 a)
{
    gfx_draw_arrows(self, x, y, dx, 32, gfx_pulse() & 0xFF, 32, a);
}

/* blinking color (green pulse) */
u32 gfx_blink_color(void)
{
    return (gfx_pulse() << 8) | 0xF0200020;
}

/* rectangle with a pulsing alpha; color = 0x00BBGGRR */
void gfx_fill_rect_pulse(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 color)
{
    f32 s = vfpu_sinf((f32)(u16)((*(u32 *)(g_sys + 0xF7A554) & 0x3F) << 10) * (3.14159265f / 32768.0f));
    u32 v = ((color << 8) >> 8) | (((s8)(int)(48.0f * s) + 191) << 24);

    gfx_fill_rect_rgba(self, x, y, w, h, (v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF, v >> 24);
}

extern const char D_08938540[];   /* "%d/%d" */
void font_set_height2(void *font, int v);
void gfx_draw_window2(void *self, u32 x, u32 y, u32 w, u32 h, u32 alpha);

/* builds the text box out from the description src (offset by dx, dy); with title != 0
 * leaves room for a title line */
void layout_build(Layout *out, const u8 *src, int title, s32 dx_arg, s32 dy_arg)
{
    s16 dx = dx_arg;
    s16 dy = dy_arg;
    s16 lh;

    title = title != 0;
    out->x = U16(src, 0) + dx;
    out->y = U16(src, 2) + dy;
    out->cw = src[4];
    out->b9 = src[5];
    out->ba = src[6];
    out->bb = src[7];
    out->e = U16(src, 10);
    out->ptr = *(void **)(src + 12);
    out->color = U32(src, 16);
    lh = src[5] + 2;
    out->w = (((s32)(src[4] * U16(src, 8)) >> 1) + 23) & 0x7FF8;
    out->h = lh * (U16(src, 10) + title) + 16;
    out->tx = (u16)out->x + 8;
    out->ty = (u16)out->y + (title ? 25 : 10);
    out->lh = lh;
    out->f[0] = (u16)out->x + 6;
    out->f[1] = (u16)out->ty - 2;
    out->f[2] = (u16)out->w - 12;
    out->f[3] = lh + 1;
    out->f[4] = (u16)out->x + 8;
    out->f[5] = (u16)out->y + 7;
}

/* builds the box from src and draws it with the cursor */
void layout_draw_from(void *self, const u8 *src, int title, int a, int alpha, int c)
{
    Layout l;

    layout_build(&l, src, title, 0, 0);
    layout_draw_box(self, &l, title, a, alpha, c);
}

/* page number "n/total" and page arrows of the box l */
void layout_draw_pages(void *self, Layout *l, u32 cur, u32 total_arg, s32 color, s32 alt)
{
    u16 total = total_arg;
    char buf[32];

    if (total > 0) {
        s32 n;

        sprintf(buf, D_08938540, (cur & 0xFFFF) + 1, total);
        n = font_char_count(g_font, buf);
        font_printf_at_color(g_font, (s16)((u16)l->x + (u16)l->w - 21 - (l->cw >> 1) * n), l->f[5],
                      (s8)color, fmt_s, buf);
    }
    if ((s8)alt == 0)
        gfx_draw_arrows_blink(self, (u16)l->x, (u16)l->f[5], (u16)((u16)l->w - 14), 240);
    else
        gfx_draw_arrows(self, (u16)l->x, (u16)l->f[5], (u16)((u16)l->w - 14), 32, 112, 32, 160);
}

/* draws the window of the box l (style l->bb), sets the font and writes text */
void layout_draw(void *self, Layout *l, const char *text, u32 alpha)
{
    switch (l->bb) {
    case 4:
        gfx_draw_window2(self, (u16)l->x, (u16)l->y, (u16)l->w, (u16)l->h, alpha);
        break;
    case 3:
    case 1:
        gfx_draw_subwindow(self, (u16)l->x, (u16)l->y, (u16)l->w, (u16)l->h, alpha);
        break;
    case 2:
    case 0:
        gfx_draw_window(self, (u16)l->x, (u16)l->y, (u16)l->w, (u16)l->h, alpha);
        break;
    }
    font_set_size(g_font, l->cw, l->b9);
    font_set_height2(g_font, (u8)l->lh);
    font_set_color(g_font, (s8)l->ba);
    if (text != NULL)
        font_draw_at(g_font, l->tx, l->ty, fmt_s, text);
}
