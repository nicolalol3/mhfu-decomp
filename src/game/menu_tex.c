/* Menu: selection of the texture used by the sprites (GE texture commands), lists of
 * texts and equipment icons. Same class as menu_box.c. (Guessed names.) */
#include "menu.h"

extern u8 *D_08A62C40;            /* display list manager */
extern void *D_089CBBE0;          /* texture bank of the menu */
extern void *D_09A0B540;
extern void *D_08A62C54;          /* equipment data */

/* texture description from the menu bank */
typedef struct BankTex {
    u32 a;
    u16 w, h;           /* 0x04 */
    u32 addr;           /* 0x08 */
    u16 fmt, f0e;       /* 0x0C */
    u32 clut;           /* 0x10 */
    u16 clutn, f16;     /* 0x14 */
} BankTex;

/* texture description from a loaded file */
typedef struct FileTex {
    u32 addr;
    u32 fmt;            /* 0x04 */
    u16 w, h;           /* 0x08 */
    u32 clut;           /* 0x0C */
    u32 clutn;          /* 0x10 */
} FileTex;

void vram_get_surface(void *bank, u32 i, BankTex *out);
s32 func_08898898(void *a, u32 b);
void tmh_get_image(void *dl, s32 file, u32 i, int a, int b, FileTex *out);
u32 pow2_log(void *dl, s32 v);
u32 *dl_cur(void *dl);
void dl_link(void *dl, u32 *p, int n, u8 layer);
void dl_set_cur(void *dl, u32 *end);
u8 func_0885C83C(void *a, u8 type, u16 id);
void gfx_draw_sprite_scaled_rgba(void *self, u32 x, u32 y, s32 w, s32 h, u8 r, u8 g, u8 b, u8 a, int mode);
u32 equip_rarity_color(void *self, u32 r_arg, u32 a_arg, u32 alt_arg);
u16 equip_icon(void *self, u8 *eq, u32 which_arg);

/* GE commands that select a texture (addr, buffer width, size, clut) */
#define TEX_CMDS(w, fmt, addr, bufw, lw, lh, clut, clutn) do {               \
        (w)[0] = 0xC2000001;                                                  \
        (w)[1] = (fmt) | 0xC3000000;                                          \
        (w)[2] = ((addr) & 0xFFFFFF) | 0xA0000000;                            \
        (w)[3] = (((addr) & 0xFF000000) >> 8) | 0xA8000000 | (bufw);          \
        (w)[4] = ((lh) << 8) | 0xB8000000 | (lw);                             \
        (w)[5] = (clutn) | 0xC500FF00;                                        \
        (w)[6] = ((clut) & 0xFFFFFF) | 0xB0000000;                            \
        (w)[7] = (((clut) & 0xFF000000) >> 8) | 0xB1000000;                   \
        (w)[8] = (((fmt) == 4 ? 16 : 256) + 7) >> 3 | 0xC4000000;              \
    } while (0)

/* selects texture tex (0x8000 = from the file file, otherwise from the bank) */
void tex_select(u8 *self, u32 tex, u32 file)
{
    u8 *dl;
    u32 w[12];
    u8 layer;

    w[11] = 0x08000000;
    w[9] = 0xCB000000;
    w[10] = 0x10000000;
    if (tex & 0x8000) {
        FileTex t;
        u32 lh;

        tmh_get_image(D_08A62C40, func_08898898(D_09A0B540, file), tex & 0xFFF, 0, 0, &t);
        lh = pow2_log(D_08A62C40, t.h);
        TEX_CMDS(w, t.fmt, t.addr, t.w, pow2_log(D_08A62C40, t.w), lh, t.clut, t.clutn);
    } else {
        BankTex t;
        u32 lh;

        vram_get_surface(D_089CBBE0, tex & 0xFF, &t);
        lh = pow2_log(D_08A62C40, t.h);
        TEX_CMDS(w, t.fmt, t.addr, t.w, pow2_log(D_08A62C40, t.w), lh, t.clut, t.clutn);
    }
    dl = D_08A62C40;
    layer = self[0x534];
    if (layer < 20) {
        u32 cur = U32(dl, 0x80198);
        s32 used = (s32)(cur - (u32)(dl + U32(dl, 4) * 0x40000 + 0x198)) / 4;

        if ((u32)(used + 12) < 0x10000) {
            int i;

            for (i = 0; i < 12; i++) {
                **(u32 **)(dl + 0x80198) = w[i];
                U32(dl, 0x80198) += 4;
            }
            dl_link(dl, (u32 *)cur, 12, layer);
        }
    }
}

/* texels per 128 bytes for a texture format */
#define TEX_PER128(fmt) ((fmt) < 4 ? 8 : (fmt) == 4 ? 32 : (fmt) == 5 ? 16 : 4)

/* selects the part of the texture of sprite spr starting at its (u, v) */
void tex_select_sprite(u8 *self, const s16 *spr)
{
    u32 *p = dl_cur(D_08A62C40);
    u32 k, off, lh;

    if ((u16)spr[4] & 0x8000) {
        FileTex t;

        tmh_get_image(D_08A62C40, func_08898898(D_09A0B540, (u16)spr[5]), (u16)spr[4] & 0xFFF, 0, 0, &t);
        k = TEX_PER128(t.fmt);
        off = t.w * ((spr[1] / 8) * (128 / k)) + k * ((128 / k) * ((u32)(s32)spr[0] / k));
        lh = pow2_log(D_08A62C40, spr[3]);
        TEX_CMDS(p, t.fmt, t.addr + off, t.w, pow2_log(D_08A62C40, spr[2]), lh, t.clut, t.clutn);
    } else {
        BankTex t;

        vram_get_surface(D_089CBBE0, (u16)spr[4] & 0xFF, &t);
        k = TEX_PER128(t.fmt);
        off = t.w * ((spr[1] / 8) * (128 / k)) + k * ((128 / k) * ((u32)(s32)spr[0] / k));
        lh = pow2_log(D_08A62C40, spr[3]);
        TEX_CMDS(p, t.fmt, t.addr + off, t.w, pow2_log(D_08A62C40, spr[2]), lh, t.clut, t.clutn);
    }
    p[9] = 0xCB000000;
    p[10] = 0x10000000;
    p[11] = 0x08000000;
    dl_link(D_08A62C40, p, 12, self[0x534]);
    dl_set_cur(D_08A62C40, p + 12);
}

/* list of texts (ids until -1) one under the other, with optional colors */
void text_draw_list(void *self, s32 x, s32 y, const s16 *ids, s32 lh_arg, const s8 *col)
{
    s16 lh = lh_arg;

    while (*ids != -1) {
        if (col != NULL)
            func_08890E38(D_08A631A8, *col++);
        func_088920A8(D_08A631A8, x, y, D_08938478, text_menu(D_089CC438, *ids));
        ids++;
        y = (s16)(y + lh);
    }
}

/* icon of the equipment piece (type, id) with the color of its rarity */
void equip_draw_icon_rarity(void *self, u32 type, u32 id, u32 x, u32 y, u32 w_arg, s32 h_arg,
                            u32 alpha, u8 alt)
{
    u16 w = w_arg;
    s16 h = h_arg;
    u8 eq[4];
    u32 c;

    eq[1] = type;
    U16(eq, 2) = id;
    c = equip_rarity_color(self, func_0885C83C(D_08A62C54, type, id), alpha, alt);
    gfx_draw_sprite_scaled_rgba(self, x, y, (s16)w, h, (u8)(c >> 16), (u8)(c >> 8), (u8)c, c >> 24,
                  equip_icon(self, eq, w < 24));
}

/* 16 x 16 icon of the equipment piece (type, id) with the color rgba */
void equip_draw_icon_color(void *self, u32 type, u32 id, u32 x, u32 y, const u8 *rgba)
{
    u8 eq[4];

    eq[1] = type;
    U16(eq, 2) = id;
    gfx_draw_sprite_scaled_rgba(self, x, y, 16, 16, rgba[2], rgba[1], rgba[0], rgba[3], equip_icon(self, eq, 1));
}

#include "vfpu.h"
#undef menu_draw_sprite
#undef gfx_draw_subwindow
#undef menu_free_data
#undef gfx_draw_count_box

/* horizontal gradient: transparent at the edges, alpha a in the middle */
void gfx_fill_gradient(u8 *self, u32 x_arg, u32 y, u32 w, u32 h, u32 r, u32 g, u32 b, u8 a)
{
    u32 x = (u16)x_arg;
    u32 *p = dl_cur(D_08A62C40);
    u32 c0, c1, ya, yb, mid, xe, pa, pb;

    c0 = ((b & 0xFF) << 16) | (r & 0xFF) | ((g & 0xFF) << 8);
    ya = y << 16;
    yb = ((y & 0xFFFF) + (h & 0xFFFF)) << 16;
    p[0] = c0;
    p[1] = ya | x;
    p[2] = 0;
    p[3] = c0;
    p[4] = x | yb;
    mid = (u16)(x + ((w & 0xFFFF) >> 1));
    pa = ya | mid;
    pb = mid | yb;
    p[5] = 0;
    c1 = ((u32)a << 24) | c0;
    p[6] = c1;
    p[7] = pa;
    p[8] = 0;
    p[9] = c1;
    p[10] = pb;
    p[11] = 0;
    p[12] = (((u32)p & 0xFF000000) >> 8) | 0x10000000;
    p[13] = ((u32)p & 0xFFFFFF) | 0x01000000;
    p[14] = 0x1E000000;
    p[15] = 0x50000001;
    p[16] = 0x1280011C;
    p[17] = 0x04040004;
    p[18] = 0x10000000;
    p[19] = 0x08000000;
    dl_link(D_08A62C40, p + 12, 8, self[0x534]);
    dl_set_cur(D_08A62C40, p + 20);
    p = dl_cur(D_08A62C40);
    xe = (x + (w & 0xFFFF)) & 0xFFFF;
    p[0] = c1;
    p[1] = pa;
    p[2] = 0;
    p[3] = c1;
    p[4] = pb;
    p[5] = 0;
    p[6] = c0;
    p[7] = ya | xe;
    p[8] = 0;
    p[9] = c0;
    p[10] = yb | xe;
    p[11] = 0;
    p[12] = (((u32)p & 0xFF000000) >> 8) | 0x10000000;
    p[13] = ((u32)p & 0xFFFFFF) | 0x01000000;
    p[14] = 0x1E000000;
    p[15] = 0x50000001;
    p[16] = 0x1280011C;
    p[17] = 0x04040004;
    p[18] = 0x10000000;
    p[19] = 0x08000000;
    dl_link(D_08A62C40, p + 12, 8, self[0x534]);
    dl_set_cur(D_08A62C40, p + 20);
}

/* gradient with a pulsing alpha */
void gfx_fill_gradient_pulse(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 r, u32 g, u32 b)
{
    f32 s = vfpu_sinf((f32)(u16)((*(u32 *)(D_08A631A0 + 0xF7A554) & 0x3F) << 10) * (3.14159265f / 32768.0f));

    gfx_fill_gradient(self, x, y, w, h, r, g, b, (u8)((s8)(int)(48.0f * s) + 191));
}

extern void *D_08A62D90;
s32 hunter_add_item(void *hunter, u32 id, u32 n, int a);
s32 item_combinable(void *a, u32 id);
void func_088860A0(void *a, u32 id);
u8 menu_msg_push(u8 *self, u32 id, u32 x, u32 y);

/* gives n items id to the hunter; flags 1 = show the message, 2 = play the sound.
 * Returns the result of the pouch (5 = full: the item goes to the box if possible) */
s32 menu_give_items(u8 *self, u8 *hunter, u32 id, s32 key_arg, u32 flags_arg, u32 n)
{
    s16 key = key_arg;
    u8 flags = flags_arg;
    u16 r;

    if (self[0x52F] != 0)
        return -1;
    if (S16(self, 0x119E) > 0 && key != U16(self, 0x119C))
        return -1;
    r = hunter_add_item(hunter, id, n, 0);
    S16(self, 0x590) = 0;
    if (r == 5) {
        if (item_combinable(D_08A62C50, id) != -1) {
            if (flags & 1)
                menu_msg_push(self, 0, 12, 0);
            return 5;
        }
        S16(self, 0x590) = id;
        self[0x52F] = 1;
        S16(self, 0x58E) = id;
        self[0x593] = n;
        if ((flags & 2) && U16(hunter, 0x1E6) == D_089CC438[0x28])
            func_088860A0(D_08A62D90, id);
        self[0x58D] = 0xFF;
    } else if (r == 3 || r == 2) {
        if (flags & 1)
            menu_msg_push(self, 1, 3, (s16)id);
    } else if (r == 1 || r == 0) {
        if ((flags & 2) && U16(hunter, 0x1E6) == D_089CC438[0x28])
            func_088860A0(D_08A62D90, id);
        if (flags & 1)
            menu_msg_push(self, 1, 0, (s16)id);
    }
    return (s16)r;
}

extern const s16 D_08939980, D_08939982, D_08939DEA, D_08939DF4;
extern const char D_089386E8[], D_08938784[];
extern const u16 D_089AD048[];
void menu_draw_sprite(u8 *self, u32 x, u32 y, s32 id);
void gfx_draw_tex_rect(u8 *self, u32 x, u32 y, u32 w, u32 h, s32 id);
void gfx_draw_tex_rect_color(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 r, u32 g, u32 b, u8 a, s32 id);
void gfx_draw_sprite_rgba(u8 *self, u32 x, u32 y, u32 r, u32 g, u32 b, u32 a, s32 id);
void gfx_draw_sprite_scaled(u8 *self, u32 x, u32 y, u32 w, u32 h, s32 id);
void gfx_draw_sprite_scaled_color(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 r, u32 g, u32 b, s32 id);
void func_08832B98(void *self, int on);
void temp_free(void *a, int b);

/* window with the second frame style (sprites 167..173, edges 437, 438) */
void gfx_draw_window2(u8 *self, u32 x, u32 y, u32 w_arg, u32 h_arg, u32 alpha)
{
    u16 w = w_arg;
    u16 h = h_arg;
    u16 x16 = x, y16 = y;
    s32 tw = D_08939980, th;
    u32 xa = x16 + tw, xb = x16 + w - tw, ww = (u16)(w - tw * 2);
    u32 ya, yb, hh;

    th = D_08939982;
    yb = y16 + h - th;
    hh = (u16)(h - th * 2);
    ya = y16 + th;
    if ((alpha & 0xFF) == 0xFF) {
        menu_draw_sprite(self, x, y, 167);
        gfx_draw_tex_rect(self, xa & 0xFFFF, y16, ww, (u16)D_08939982, 437);
        menu_draw_sprite(self, xb & 0xFFFF, y16, 168);
        gfx_draw_tex_rect(self, x16, ya & 0xFFFF, (u16)D_08939980, hh, 169);
        gfx_draw_tex_rect(self, xa & 0xFFFF, ya & 0xFFFF, ww, hh, 173);
        gfx_draw_tex_rect(self, xb & 0xFFFF, ya & 0xFFFF, (u16)D_08939980, hh, 170);
        menu_draw_sprite(self, x16, yb & 0xFFFF, 171);
        gfx_draw_tex_rect(self, xa & 0xFFFF, yb & 0xFFFF, ww, (u16)D_08939982, 438);
        menu_draw_sprite(self, xb & 0xFFFF, yb & 0xFFFF, 172);
    } else {
        gfx_draw_sprite_rgba(self, x, y, 255, 255, 255, alpha, 167);
        gfx_draw_tex_rect_color(self, xa & 0xFFFF, y16, ww, (u16)D_08939982, 255, 255, 255, alpha, 437);
        gfx_draw_sprite_rgba(self, xb & 0xFFFF, y16, 255, 255, 255, alpha, 168);
        gfx_draw_tex_rect_color(self, x16, ya & 0xFFFF, (u16)D_08939980, hh, 255, 255, 255, alpha, 169);
        gfx_draw_tex_rect_color(self, xa & 0xFFFF, ya & 0xFFFF, ww, hh, 255, 255, 255, alpha, 173);
        gfx_draw_tex_rect_color(self, xb & 0xFFFF, ya & 0xFFFF, (u16)D_08939980, hh, 255, 255, 255, alpha, 170);
        gfx_draw_sprite_rgba(self, x16, yb & 0xFFFF, 255, 255, 255, alpha, 171);
        gfx_draw_tex_rect_color(self, xa & 0xFFFF, yb & 0xFFFF, ww, (u16)D_08939982, 255, 255, 255, alpha, 438);
        gfx_draw_sprite_rgba(self, xb & 0xFFFF, yb & 0xFFFF, 255, 255, 255, alpha, 172);
    }
}

/* layer number (9..14) of the menu layer l, other values unchanged */
#define MENU_LAYER(self, l) do {                                    \
        switch (l) {                                                \
        case 0: (self)[0x534] = 9; break;                           \
        case 1: (self)[0x534] = 10; break;                          \
        case 2: (self)[0x534] = 11; break;                          \
        case 3: (self)[0x534] = 12; break;                          \
        case 4: (self)[0x534] = 13; break;                          \
        case 5: (self)[0x534] = 14; break;                          \
        }                                                           \
    } while (0)

/* darkens the whole screen on layer l */
void gfx_dim_screen(u8 *self, s32 l)
{
    u8 old;

    func_08832B98(self, 1);
    old = self[0x534];
    MENU_LAYER(self, l);
    gfx_draw_sprite_scaled(self, 0, 0, 480, 272, 264);
    MENU_LAYER(self, old);
    func_08832B98(self, 0);
}

/* frame made of 5 colored pieces (top, bottom, sides, center) */
void gfx_draw_panel(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 c)
{
    u32 x16 = x & 0xFFFF, y16 = y & 0xFFFF, w16 = w & 0xFFFF, h16 = h & 0xFFFF;

    gfx_draw_sprite_scaled_color(self, x, y, w, (u16)D_08939DEA, c, c, c, 261);
    gfx_draw_sprite_scaled_color(self, x, (y16 + h16 - D_08939DEA) & 0xFFFF, w, (u16)D_08939DEA, c, c, c, 261);
    gfx_draw_sprite_scaled_color(self, x, (y16 + D_08939DEA) & 0xFFFF, (u16)D_08939DF4,
                                 (h16 - D_08939DEA * 2) & 0xFFFF, c, c, c, 262);
    gfx_draw_sprite_scaled_color(self, (x16 + w16 - D_08939DF4) & 0xFFFF, (y16 + D_08939DEA) & 0xFFFF,
                                 (u16)D_08939DF4, (h16 - D_08939DEA * 2) & 0xFFFF, c, c, c, 262);
    gfx_draw_sprite_scaled_color(self, (x16 + D_08939DF4) & 0xFFFF, (y16 + D_08939DEA) & 0xFFFF,
                                 (w16 - D_08939DF4 * 2) & 0xFFFF, (h16 - D_08939DEA * 2) & 0xFFFF,
                                 c, c, c, 263);
}

/* frees the menu data loaded by menu_load_data */
void menu_free_data(u8 *self)
{
    if (U32(self, 0x528) & 0x1000) {
        temp_free(D_09A0B540, 11);
        U32(self, 0x528) &= ~0x1000;
    }
}

/* name of quest slot n ("<kind> <n+1>") */
void menu_quest_slot_name(void *self, s32 n, char *out)
{
    char num[8];
    u32 f = *(u32 *)(D_089CC438 + 0x6AF14);
    u16 k = ((f & 8) != 0 || (f & 4) != 0) ? 2 : 1;

    sprintf(num, D_089386E8, n + 1);
    sprintf(out, D_08938784, text_menu(D_089CC438, D_089AD048[k]), num);
}

extern const s16 D_08939E18, D_08939E1A, D_08939908, D_0893990A;
extern const u16 D_0893878C[][4];
extern const u16 D_089ACAD8, D_089ACA08;
void gfx_draw_sprite_mode(u8 *self, u32 x, u32 y, s32 w_arg, s32 h_arg, u32 r, u32 g, u32 b, u8 a,
                          s32 id, u8 mode);
void gfx_draw_subwindow(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 alpha);

/* four corner marks around the point (x, y), color 0xAABBGGRR */
void gfx_draw_corners(u8 *self, u32 x, u32 y, u32 c)
{
    u8 r = c >> 16, g = c >> 8, b = c, a = c >> 24;

    gfx_draw_sprite_mode(self, x & 0xFFFF, y & 0xFFFF, D_08939E18, D_08939E1A, r, g, b, a, 265, 1);
    gfx_draw_sprite_mode(self, ((s16)x - D_08939E18 + 1) & 0xFFFF, y & 0xFFFF, D_08939E18, D_08939E1A,
                         r, g, b, a, 265, 0);
    gfx_draw_sprite_mode(self, ((s16)x - 1) & 0xFFFF, ((s16)y - D_08939E18 + 1) & 0xFFFF, D_08939E1A,
                         D_08939E18, r, g, b, a, 265, 3);
    gfx_draw_sprite_mode(self, ((s16)x - 1) & 0xFFFF, y & 0xFFFF, D_08939E1A, D_08939E18, r, g, b, a, 265, 2);
}

/* stencil/clip state of the next sprites (0 normal, 1 masked) */
void gfx_set_mask(u8 *self, int on)
{
    u32 *p = dl_cur(D_08A62C40);

    if (on == 0) {
        p[0] = 0x21000001;
        p[1] = 0xDF000032;
        p[2] = 0x10000000;
        p[3] = 0x08000000;
        dl_link(D_08A62C40, p, 4, self[0x534]);
        dl_set_cur(D_08A62C40, p + 4);
    } else {
        p[0] = 0x21000001;
        p[1] = 0xDF0000A2;
        p[2] = 0xE1FFFFFF;
        p[3] = 0x10000000;
        p[4] = 0x08000000;
        dl_link(D_08A62C40, p, 5, self[0x534]);
        dl_set_cur(D_08A62C40, p + 5);
    }
}

/* texture filtering of the next sprites (0 nearest, 1 linear) */
void gfx_set_filter(u8 *self, int on)
{
    u32 *p = dl_cur(D_08A62C40);

    p[0] = on == 0 ? 0xC6000000 : 0xC6000101;
    p[1] = 0x10000000;
    p[2] = 0x08000000;
    dl_link(D_08A62C40, p, 3, self[0x534]);
    dl_set_cur(D_08A62C40, p + 3);
}

/* button hint i (icon and text) */
void gfx_draw_button_hint(u8 *self, u32 i)
{
    const u16 *e = D_0893878C[i & 0xFF];

    menu_draw_sprite(self, e[0], e[1], 56);
    func_08890DE8(D_08A631A8, 14, 14);
    func_088915B0(D_08A631A8, (s16)e[2], (s16)e[3], 0, text_menu(D_089CC438, D_089ACAD8));
}

/* number n with the up/down arrows (n < max: up, n != 1: down) */
void gfx_draw_counter(u8 *self, s32 x_arg, s32 y_arg, u32 n, u32 max, u32 flags)
{
    s16 x = x_arg;
    s16 y = y_arg;
    char buf[16];

    if ((flags & 1) == 0) {
        if (n < max)
            gfx_draw_sprite_mode(self, (u16)(x + 6), (u16)(y - 16), D_08939908, D_0893990A, 0, 192, 64,
                                 255, 157, 0);
        if (n != 1)
            gfx_draw_sprite_mode(self, (u16)(x + 6), (u16)(y + 16), D_08939908, D_0893990A, 0, 192, 64,
                                 255, 157, 4);
    }
    sprintf(buf, D_089386E8, n);
    func_08890DE8(D_08A631A8, 14, 14);
    func_088915B0(D_08A631A8, (s16)(x + 5), y_arg, 0, buf);
}

/* box with a quantity selector */
void gfx_draw_count_box(u8 *self, u32 x, u32 y, u32 n, u32 max, u32 flags)
{
    gfx_draw_subwindow(self, x & 0xFFFF, y & 0xFFFF, 88, 48, 255);
    func_08890DE8(D_08A631A8, 14, 14);
    func_088915B0(D_08A631A8, (s16)((s16)x + 8), (s16)((s16)y + 8), 5, text_menu(D_089CC438, D_089ACA08));
    gfx_draw_counter(self, (s16)((s16)x + 52), (s16)((s16)y + 18), n, max, flags);
}
