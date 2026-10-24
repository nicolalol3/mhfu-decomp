/* HUD in the game: icons on the area map, item bar, supply box grid.
 * Same class as menu_box.c / hud.c. (Guessed names.) */
#include "menu.h"
#include "vfpu.h"

extern u8 *D_08A62C40;            /* display list manager */
extern const u8 D_0893914C[];     /* map icon of each kind of object (0xFF = none) */
extern const u32 D_0893B614[];
extern const s16 D_0893B620[];
extern const u8 D_0893ABAC[];     /* color of each player (r, g, b, -) */
extern const f32 D_0893B640;
extern const u16 D_08939874[];
extern const u8 D_0893AA5C[];     /* item colors (r, g, b, -) */
extern const u8 D_0899D198[], D_0899D19D[], D_0899D19E[], D_0899D1A0[];
extern const s16 D_0893B644, D_0893B646, D_0893B648, D_0893B64A;
extern const char D_0893B64C[], D_0893B650[], D_0893B6F4[];
extern const u16 D_0893B654[], D_0893B660[], D_0893B668[], D_0893B66C[], D_0893B670[];
extern const u16 D_0893B674[], D_0893B67C[], D_0893B6E4[], D_0893B6EC[];
extern const u16 D_0893B6F2;
extern const s16 D_08939350, D_08939374, D_0893938C, D_08939398, D_089393B0;
extern const u16 D_089AC848, D_089AC854[], D_089AC85A, D_089AC85C;
extern const u8 D_0893B6FC[], D_0893B704;
extern void *D_09A1DAEC;

void func_0884424C(f32 x, f32 y, f32 scale, void *self, u32 color);
void gfx_draw_quad(u8 *self, const s16 *pt, u32 r, u32 g, u32 b, s32 id);
void gfx_draw_sprite_mode(u8 *self, u32 x, u32 y, s32 w_arg, s32 h_arg, u32 r, u32 g, u32 b, u8 a,
                          s32 id, u8 mode);
void gfx_draw_sprite_scaled(u8 *self, u32 x, u32 y, u32 w, u32 h, s32 id);
void gfx_draw_sprite_scaled_color(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 r, u32 g, u32 b, s32 id);
void gfx_draw_sprite_scaled_rgba(u8 *self, u32 x, u32 y, s32 w_arg, s32 h_arg, u32 r, u32 g, u32 b,
                                 u8 a, s32 id);
void gfx_draw_window(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 alpha);
void gfx_draw_item_icon(u8 *self, u32 id, u32 x, u32 y, u32 w, u32 h, s8 mode);
void gfx_fill_rect(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 r, u32 g, u32 b);
void gfx_set_filter(u8 *self, int on);
void menu_draw_sprite(u8 *self, u32 x, u32 y, s32 id);
int menu_slot_usable(void *self, u32 id_arg);
u16 item_icon_sprite(void *self, u32 i, u32 small);
void tex_select(u8 *self, u32 tex, u32 file);
void dl_link(void *dl, u32 *p, int n, u8 layer);
s32 func_09AD8358(void *obj);
s32 func_09D296A8(void *obj);
s32 func_09B31FB0(void *obj);
s32 func_09D1E6E0(void *obj);
s8 func_088D6AC8(void *hunter, int skill);
int func_08890E48(void *font, const char *s);
u8 *pouch_item_data(void *a, u16 id);
void func_08845EA8(u8 *self);
void hunter_add_item(void *hunter, u16 id, int a, int b);
void func_088AB6D0(void *net, int a, void *data);
void hud_box_take_anim(u8 *self, u32 id, u32 slot);
void hud_box_cursor_flash(u8 *self, u8 *h);
s32 pouch_room_or_full(void *game, u32 id);
void func_08825ADC(void *self, int a, u32 b, int c);
void layout_draw(void *self, Layout *l, const char *text, u32 alpha);

/* copies the nh + nc words of w at the end of the display list and links them */
#define DL_PUSH(dl, layer, w, nh, nc, bi) do {                                         \
        u32 cur_ = U32(dl, 0x80198);                                                \
        s32 used_ = (s32)(cur_ - (u32)((dl) + U32(dl, 4) * 0x40000 + 0x198)) / 4;   \
        if ((u32)(used_ + (nh) + (nc)) < 0x10000) {                                 \
            int i_;                                                                 \
            (w)[bi] = ((cur_ & 0xFF000000) >> 8) | 0x10000000;                      \
            (w)[(bi) + 1] = (U32(dl, 0x80198) & 0xFFFFFF) | 0x01000000;            \
            for (i_ = 0; i_ < (nh) + (nc); i_++) {                                  \
                **(u32 **)((dl) + 0x80198) = (w)[i_];                               \
                U32(dl, 0x80198) += 4;                                              \
            }                                                                       \
            dl_link(dl, (u32 *)(cur_ + (nh) * 4), nc, layer);                \
        }                                                                           \
    } while (0)

/* texture coordinate word (v << 16 | u), with the sign of u kept as the original */
#define UV(u, v) (((u32)(v) << 16) | (u32)(s32)(u))

/* angle of the game (0x10000 = full turn) in radians, as the map code computes it */
static f32 map_angle(u32 a)
{
    return ((((f32)a * 360.0f) / 65536.0f) / 360.0f) * 6.2831855f;
}

/* blinking value of the HUD: sin of the frame counter masked by mask, shifted by sh */
static f32 hud_wave(u32 mask, int sh)
{
    u32 t = (u16)((*(u32 *)(D_08A631A0 + 0xF7A554) & mask) << sh);

    return vfpu_sinf((f32)t * (3.14159265f / 32768.0f));
}

int hud_map_icon_hidden(u8 *self, u8 *obj);
u32 hud_map_icon_color(u8 *self, u8 *obj);

/* icon of the object obj on the map at (x, y), rotated as the object */
void hud_map_draw_icon(f32 x, f32 y, f32 scale, u8 *self, u8 *obj)
{
    u8 k = D_0893914C[obj[0x1E8]];
    s16 pt[8];
    u32 idx, ang, c;
    s16 spr;
    f32 s, co, d, d2, z;
    s16 i;

    if (k == 0xFF)
        return;
    if (k & 0x80) {
        if ((k & 0x40) == 0) {
            scale = 2.0f;
            c = D_0893B614[k & 0xF];
        } else {
            scale = 4.0f * scale;
            c = hud_map_icon_color(self, obj);
        }
        func_0884424C(x, y, scale, self, c);
        return;
    }
    if ((u8)hud_map_icon_hidden(self, obj) == 1)
        return;
    if (self[0x5E7] < 2) {
        scale = scale * 0.8f;
        idx = 7;
        ang = 0;
    } else {
        idx = k;
        if (obj[0x280] != 2)
            idx = (k | 8) & 0xFF;
        ang = (0x18000 - *(s32 *)(obj + 0x1F4)) & 0xFFFF;
    }
    spr = D_0893B620[idx & 0xFF];
    {
        f32 r = map_angle(ang);

        s = vfpu_sinf(r);
        co = vfpu_cosf(r);
    }
    d = 6.0f * scale;
    d2 = 2.0f * d;
    z = -d;
    for (i = 0; i < 4; i++) {
        f32 vx = z + d2 * (f32)(i / 2);
        f32 vy = z + d2 * (f32)(i & 1);

        pt[i * 2] = (int)(0.5f + ((co * vx + s * vy) + x));
        pt[i * 2 + 1] = (int)(0.5f + ((-s * vx + co * vy) + y));
    }
    c = hud_map_icon_color(self, obj);
    gfx_draw_quad(self, pt, (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, spr);
}

/* blinking color of a map icon */
u32 hud_map_icon_color(u8 *self, u8 *obj)
{
    u8 b;

    if (obj[0x298] == 5 || (u8)(*(s32 (**)(void *))(*(u8 **)obj + 0x44))(obj) == 1) {
        f32 s = vfpu_sinf((f32)(u16)(*(u8 *)(D_08A631A0 + 0xF7A554) << 8) * (3.14159265f / 32768.0f));
        u32 v = ((s8)(int)(48.0f * s) + 0x80) & 0xFF;

        return (v << 16) | 0xD0000000 | (v << 8) | v;
    }
    if ((u8)func_09AD8358(obj) == 1) {
        if ((u8)func_088D6AC8(*(void **)(self + 0x530), 0x5A) != 0)
            return ((s8)(int)(48.0f * hud_wave(0x3F, 10)) + 0x70) | 0xFFF0F000;
    }
    b = self[0x5E7];
    if (b < 2) {
        f32 s = hud_wave(0x3F, 10);

        return ((((s8)(int)(54.0f * s) + 0x53) << 8) | 0xFFFA0000) | ((s8)(int)(6.0f * s) + 0xDD);
    }
    if (obj[0x4B9] == 1)
        return (((s8)(int)(64.0f * hud_wave(0xF, 12)) + 0x48) << 8) | 0xFFF00000;
    return (((s8)(int)(48.0f * hud_wave(0x3F, 10)) + 0x80) << 8) | 0xFF1000E0;
}

/* 1 if the icon of obj must not be drawn (depends on the kind of object) */
int hud_map_icon_hidden(u8 *self, u8 *obj)
{
    u8 k = obj[0x1E8];

    if (k == 0x3B)
        return 1;
    if (self[0x5E7] >= 4)
        return 0;
    if (obj[0x613] != 0)
        return 1;
    switch (k) {
    case 0x53:
        return func_09D296A8(obj);
    case 0x3D:
    case 0x54:
    case 0x33:
        return func_09B31FB0(obj);
    case 0x2B:
    case 0x0E:
    case 0x2C:
    case 0x1A:
        return obj[0x280] == 4;
    case 0x16:
        return func_09D1E6E0(obj);
    case 0x2D:
    case 0x0F:
        return S8(obj, 0x529) != 0;
    }
    return 0;
}

void hud_map_draw_ping(u8 *self, s32 x, s32 y, s32 n_arg, u32 color);

/* arrow of the player p on the map, at (x, y), rotated by ang */
void hud_map_draw_hunter(f32 x, f32 y, f32 scale, u8 *self, u32 ang_arg, u32 p)
{
    u32 ang = (0x18000 - (ang_arg & 0xFFFF)) & 0xFFFF;
    s16 pt[8];
    const u8 *col;
    f32 s, c, r;
    s32 i, id;
    s16 n;

    r = map_angle(ang);
    s = vfpu_sinf(r);
    c = vfpu_cosf(r);
    for (i = 0; i < 4; i++) {
        f32 vx = -(5.0f * scale) + 5.0f * scale * 2.0f * (f32)(i / 2);
        f32 vy = -(6.0f * scale) + 6.0f * scale * 2.0f * (f32)(i & 1);

        pt[i * 2] = (int)(x + (c * vx + s * vy));
        pt[i * 2 + 1] = (int)(y + (-s * vx + c * vy));
    }
    id = (p == D_089CC438[0x28]) ? 0x92 : 0x97;
    col = &D_0893ABAC[(p & 3) * 4];
    gfx_draw_quad(self, pt, col[0], col[1], col[2], id);
    n = S16(self, 0x56C + p * 2);
    if (n >= 0)
        hud_map_draw_ping(self, (int)x, (int)y, n, *(const u32 *)col);
}

void hud_map_draw_ring(f32 scale, f32 v, u8 *self, s32 x_arg, s32 y_arg, u32 color);

/* expanding rings around a player who sent a signal (n = frame of the animation) */
void hud_map_draw_ping(u8 *self, s32 x, s32 y, s32 n_arg, u32 color)
{
    s16 n = n_arg;

    if (n < 0x14) {
        hud_map_draw_ring(D_0893B640, (f32)(25 - n), self, x, y, color);
    } else if (n < 0x55) {
        s16 r = n % 20;
        s32 v;

        hud_map_draw_ring(D_0893B640, (f32)(25 - r), self, x, y, color);
        v = 5 - r;
        if (v >= 0)
            hud_map_draw_ring(D_0893B640, (f32)v, self, x, y, color);
    } else {
        hud_map_draw_ring(D_0893B640, (f32)(0x69 - n), self, x, y, color);
    }
}

/* one ring (4 mirrored quarters of the sprite 0x7E) */
void hud_map_draw_ring(f32 scale, f32 v, u8 *self, s32 x_arg, s32 y_arg, u32 color)
{
    s16 x = x_arg;
    s16 y = y_arg;
    u8 a = (u32)(vfpu_sqrtf(v) * 51.0f) & 0xFF;
    s16 r = (int)(scale * (0.291667f + 0.1180555f * vfpu_sqrtf(25.0f - v)));
    u32 cr = color & 0xFF, cg = (color >> 8) & 0xFF, cb = (color >> 16) & 0xFF;

    gfx_draw_sprite_mode(self, (u16)(x - r), (u16)(y - r), r, r, cr, cg, cb, a, 0x7E, 5);
    gfx_draw_sprite_mode(self, (u16)(x - r), (u16)y, r, r, cr, cg, cb, a, 0x7E, 1);
    gfx_draw_sprite_mode(self, (u16)x, (u16)(y - r), r, r, cr, cg, cb, a, 0x7E, 4);
    gfx_draw_sprite_mode(self, (u16)x, (u16)y, r, r, cr, cg, cb, a, 0x7E, 0);
}

/* the map texture (u0, v0)-(u1, v1) at (x, y), size w x h; alpha 0xFF = opaque */
void hud_map_draw_texture(u8 *self, s32 x_arg, u32 y_arg, s32 w_arg, s32 h_arg, s32 u0_arg,
                          s32 v0_arg, s32 u1_arg, s32 v1_arg, u8 alpha)
{
    s16 x = x_arg, y = y_arg, w = w_arg, h = h_arg;
    s16 u0 = u0_arg, v0 = v0_arg, u1 = u1_arg, v1 = v1_arg;
    s16 x1, y1;
    u8 *dl;
    u8 layer;

    tex_select(self, D_08939874[4], D_08939874[5]);
    x1 = x + w;
    y1 = y + h;
    if (self[0x578] != 0) {
        u0 = u0 >> 1;
        u1 = u1 >> 1;
        if (self[0x578] == 2) {
            u0 = u0 + 0x4B;
            u1 = u1 + 0x4B;
        }
    }
    dl = D_08A62C40;
    layer = self[0x534];
    if (alpha == 0xFF) {
        u32 v[15];

        v[0] = UV(u0, v0);
        v[1] = ((u32)x << 16) | 0xFFFF;
        v[2] = (u16)y;
        v[3] = UV(u1, v1);
        v[4] = ((u32)x1 << 16) | 0xFFFF;
        v[5] = (u16)y1;
        v[6] = 0x1E000001;
        v[7] = 0xC9000100;
        v[8] = 0x50000001;
        v[9] = 0x12800116;
        v[12] = 0x04060002;
        v[13] = 0x10000000;
        v[14] = 0x08000000;
        if (layer < 20)
            DL_PUSH(dl, layer, v, 6, 9, 10);
    } else {
        u32 v[17];
        u32 c = ((u32)alpha << 24) | 0xFFFFFF;

        v[0] = UV(u0, v0);
        v[1] = c;
        v[2] = ((u32)y << 16) | (u16)x;
        v[3] = 0;
        v[4] = UV(u1, v1);
        v[5] = c;
        v[6] = ((u32)y1 << 16) | (u16)x1;
        v[7] = 0;
        v[8] = 0x1E000001;
        v[9] = 0xC9000100;
        v[10] = 0x50000001;
        v[11] = 0x1280011E;
        v[14] = 0x04060002;
        v[15] = 0x10000000;
        v[16] = 0x08000000;
        if (layer < 20)
            DL_PUSH(dl, layer, v, 8, 9, 12);
    }
}

void hud_draw_item_bar_single(u8 *self);

/* item bar at the bottom right (name and number of the selected item) */
void hud_draw_item_bar(u8 *self)
{
    u8 *h;

    if (U32(self, 0x1194) & 8)
        return;
    h = *(u8 **)(self + 0x530);
    if ((U32(h, 4) & 1) == 0 || self[0x5DB] != 0)
        return;
    if ((U32(h, 0x614) & 1) == 0)
        hud_draw_item_bar_single(self);
    else
        func_08845EA8(self);
    gfx_set_filter(self, 0);
    if ((u8)menu_slot_usable(self, U16(h, 0x648)) == 1) {
        u16 *pouch;
        u16 item;
        u16 x;

        func_08890DE8(D_08A631A8, 14, 14);
        func_08890E38(D_08A631A8, 0);
        pouch = func_08856884(D_089CC438, 0);
        item = pouch[U16(h, 0x648) * 2];
        if ((s16)pouch[U16(h, 0x648) * 2 + 1] > 0 && D_0899D19B[item * 0x18] >= 2) {
            if ((s16)pouch[U16(h, 0x648) * 2 + 1] >= D_0899D19B[item * 0x18])
                func_08890E38(D_08A631A8, 2);
            func_08891474(D_08A631A8, D_0893B648, D_0893B64A, D_0893B64C,
                          (s16)pouch[U16(h, 0x648) * 2 + 1]);
        }
        x = D_0893B644 - ((func_08890E48(D_08A631A8, func_08851528(D_089CC438, item)) * 14) >> 2);
        func_08890E38(D_08A631A8, 0);
        func_08891474(D_08A631A8, (s16)x, D_0893B646, D_0893B650, func_08851528(D_089CC438, item));
    }
    if (S8(self, 0x598) > 0)
        S8(self, 0x598)--;
    if (S8(self, 0x59A) > 0)
        S8(self, 0x59A)--;
}

void hud_draw_bar_frame(u8 *self, u32 x, u32 y, u32 w, u32 h);
void hud_draw_item_slot(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 slot);
int hud_draw_item_name(u8 *self, u32 slot, s32 y);

/* item bar with a single item (the normal one) */
void hud_draw_item_bar_single(u8 *self)
{
    u8 *h = *(u8 **)(self + 0x530);
    int other = h[0x5F3] != 1;
    s16 dy;

    gfx_draw_window(self, D_0893B654[0], D_0893B654[1], D_0893B654[2], D_0893B654[3], 0xFF);
    hud_draw_bar_frame(self, D_0893B660[0], D_0893B660[1], D_0893B660[2], D_0893B660[3]);
    menu_draw_sprite(self, D_0893B668[0], D_0893B668[1], 0x24);
    menu_draw_sprite(self, 0x192, 0xE0, 0x1A1);
    gfx_draw_sprite_mode(self, 0x166, 0xE0, 0x18, 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0x1A1, 1);
    gfx_set_filter(self, 1);
    dy = 0;
    if (h[0x674] & 1)
        dy = 2;
    menu_draw_sprite(self, D_0893B66C[0], (u16)(D_0893B66C[1] + dy), 0x35);
    menu_draw_sprite(self, D_0893B670[0], D_0893B670[1], 0x36);
    if ((u8)menu_slot_usable(self, U16(h, 0x648)) == 1) {
        u16 *e = func_08856884(D_089CC438, U16(h, 0x648));

        gfx_draw_item_icon(self, e[0], D_0893B674[0], D_0893B674[1], D_0893B674[2], D_0893B674[3], 0);
    }
    if (!other) {
        s8 sel = -1;
        u16 y = D_0893B67C[1] + 2;

        if (S16(h, 0x646) != 0) {
            func_08890DE8(D_08A631A8, 14, 14);
            sel = S8(h, 0x686);
            hud_draw_item_name(self, (u8)sel, (s16)y);
        }
        hud_draw_item_slot(self, D_0893B67C[0], D_0893B67C[1], D_0893B67C[2], D_0893B67C[3], (u8)sel);
    }
}

void hud_draw_bar_frame2(u8 *self, u32 x, u32 y, u32 w, u32 h);

/* arrows of the item bar */
void hud_draw_item_bar_arrows(u8 *self)
{
    u16 y;

    if ((U32(self, 0x528) & 2) == 0)
        return;
    hud_draw_bar_frame2(self, D_0893B6E4[0], D_0893B6E4[1], D_0893B6E4[2], D_0893B6E4[3]);
    y = D_0893B6EC[1];
    if ((*(u8 **)(self + 0x530))[0x674] & 0x10)
        y += 2;
    menu_draw_sprite(self, D_0893B6EC[0], y, 0x34);
    y = D_0893B6EC[2];
    if ((*(u8 **)(self + 0x530))[0x674] & 8)
        y += 2;
    menu_draw_sprite(self, D_0893B6EC[0], y, 0x33);
}

/* number of the items of the pouch that can be used now */
int hud_count_usable_items(void)
{
    s16 i, n = 0;

    for (i = 0; i < 0x18; i++) {
        if ((s16)func_08856884(D_089CC438, i)[1] > 0 && func_08856884(D_089CC438, i)[0] != 0
            && pouch_item_data(D_08A62C50, i)[1] == 1)
            n++;
    }
    return n;
}

/* frame of a bar: left end, middle stretched, right end */
void hud_draw_bar_frame(u8 *self, u32 x, u32 y, u32 w, u32 h)
{
    menu_draw_sprite(self, x, y, 0x21);
    gfx_draw_sprite_scaled(self, ((x & 0xFFFF) + D_08939350) & 0xFFFF, y,
                           ((w & 0xFFFF) - D_08939350 * 2) & 0xFFFF, h, 0x22);
    menu_draw_sprite(self, ((x & 0xFFFF) + (w & 0xFFFF) - D_08939350) & 0xFFFF, y, 0x23);
}

/* slot of the item bar, colored as the item of the pouch slot (0xFF = empty) */
void hud_draw_item_slot(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 slot_arg)
{
    u8 slot = slot_arg;
    u32 xx = x & 0xFFFF, ww = w & 0xFFFF;
    u32 c;
    u8 *cp = (u8 *)&c;
    u8 *hu;

    if (slot == 0xFF) {
        c = 0xFF808080;
    } else {
        u16 *e = func_08856884(D_089CC438, slot);

        c = *(const u32 *)&D_0893AA5C[D_0899D19E[e[0] * 0x18] * 4];
    }
    gfx_draw_sprite_scaled(self, (xx + D_08939374) & 0xFFFF, y, (ww - D_08939374 - D_0893938C) & 0xFFFF,
                           h, 0x27);
    gfx_draw_sprite_scaled(self, (xx + ww - D_0893938C) & 0xFFFF, y, (u16)D_0893938C, h, 0x28);
    hu = *(u8 **)(self + 0x530);
    if (hu[0x1E8] != 10) {
        gfx_draw_sprite_scaled_color(self, x, y, (u16)D_08939374, h, cp[0], cp[1], cp[2], 0x26);
        return;
    }
    gfx_draw_sprite_scaled(self, x, y, (u16)D_08939374, h, 0x25);
    if (slot == 0xFF || func_08856884(D_089CC438, slot)[0] == 0x80)
        return;
    gfx_draw_sprite_scaled_color(self, (xx + 1) & 0xFFFF, y, 0x10, h, cp[0], cp[1], cp[2], 0x19C);
    if (slot == (*(u8 **)(self + 0x530))[0x5F6]) {
        menu_draw_sprite(self, (xx + 3) & 0xFFFF, y, 0x17F);
        menu_draw_sprite(self, (xx + 0xC) & 0xFFFF, y, 0x180);
    }
}

/* second kind of bar frame */
void hud_draw_bar_frame2(u8 *self, u32 x, u32 y, u32 w, u32 h)
{
    menu_draw_sprite(self, x, y, 0x29);
    gfx_draw_sprite_scaled(self, ((x & 0xFFFF) + D_08939398) & 0xFFFF, y,
                           ((w & 0xFFFF) - D_08939398 - D_089393B0) & 0xFFFF, h, 0x2A);
    menu_draw_sprite(self, ((x & 0xFFFF) + (w & 0xFFFF) - D_089393B0) & 0xFFFF, y, 0x2B);
}

/* name (and number) of the item of the pouch slot, right aligned at line y; 1 if written */
int hud_draw_item_name(u8 *self, u32 slot_arg, s32 y)
{
    u8 slot = slot_arg;
    u8 *h = *(u8 **)(self + 0x530);
    char buf[32];
    const u8 *it;
    u16 item;
    u8 n, sh = 0;
    s8 color;

    if (slot == 0xFF)
        return 0;
    item = func_08856884(D_089CC438, slot)[0];
    it = &D_0899D198[item * 0x18];
    if ((it[1] & 6) == 0)
        return 0;
    n = ((u8 *)func_08856884(D_089CC438, slot))[2];
    if (n == 0)
        return 0;
    if (h[0x1E8] == 10)
        color = (slot == h[0x5F6]) ? 5 : 0;
    else
        color = (it[8] == h[0x689]) ? 0 : 5;
    func_08890E38(D_08A631A8, color);
    if (item == 0x80) {
        sprintf(buf, D_0893B650, func_08851528(D_089CC438, item));
        sh = 4;
    } else if (it[3] == 0xFF) {
        sprintf(buf, func_0885151C(D_089CC438, D_089AC848), func_08851528(D_089CC438, item));
    } else {
        sprintf(buf, D_0893B6F4, func_08851528(D_089CC438, item), n);
    }
    func_08891474(D_08A631A8,
                  (s16)(D_0893B6F2 - sh - ((func_08890E48(D_08A631A8, buf) * 14) >> 2)), y,
                  D_0893B650, buf);
    return 1;
}

/* takes the item selected in the supply box (on-line: asks the host) */
void hud_take_box_item(u8 *self, u8 *h)
{
    u8 *game = D_089CC438;

    h[0x55E] = 30;
    hud_box_cursor_flash(self, h);
    if ((*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) == 0) {
        u8 slot;

        S16(h, 0x5EC) = 0;
        U32(h, 0x614) &= ~0x10000;
        hunter_add_item(h, U16(game, 0x390 + h[0x55D] * 4), S16(game, 0x392 + h[0x55D] * 4), 0);
        slot = h[0x55D];
        U32(game, 0x410 + (slot >> 5) * 4) |= 1 << (slot & 0x1F);
        hud_box_take_anim(self, U16(game, 0x390 + h[0x55D] * 4), h[0x55D]);
    } else {
        u8 msg[4];

        S16(h, 0x5EC) = 0x708;
        U32(h, 0x614) |= 0x10000;
        msg[1] = h[0x55D];
        func_088AB6D0(D_09A1DAEC, 1, msg);
        h[0x5EE] = D_089CC438[0x420];
    }
}

#define BOX_X(i) (((i) & 7) * 28 + 0xE8)
#define BOX_Y(i) (((i) >> 3) * 24 + 0x38)

/* supply box: grid of 4 x 8 items, cursor, name of the selected item */
void hud_draw_supply_box(u8 *self)
{
    u8 *game = D_089CC438;
    u8 *h = *(u8 **)(self + 0x530);
    char buf[48];
    const char *str;
    u32 taken;
    u16 item;
    s16 i;
    u8 slot;

    if ((U32(h, 0x614) & 2) == 0)
        return;
    if (S8(D_08A62C7C, 0x12D) != 0)
        taken = 0xFFFFFFFF;
    else
        taken = U32(game, 0x410);
    layout_draw(self, (Layout *)D_0893B6FC, func_0885151C(game, D_089AC854[0]), 0xFF);
    for (i = 0; i < 32; i++)
        gfx_fill_rect(self, BOX_X(i) & 0xFFFF, BOX_Y(i) & 0xFFFF, 22, 22, 0, 0, 0);
    gfx_set_filter(self, 1);
    for (i = 0; i < 32; i++) {
        if ((taken & (1 << (i & 0x1F))) == 0) {
            u16 id = U16(game, 0x390 + i * 4);

            if (id != 0 && D_0899D19D[id * 0x18] != 0xFF) {
                const u8 *col = &D_0893AA5C[D_0899D19E[id * 0x18] * 4];

                gfx_draw_sprite_scaled_color(self, BOX_X(i) & 0xFFFF, BOX_Y(i) & 0xFFFF, 22, 22, col[0],
                                             col[1], col[2],
                                             item_icon_sprite(self, D_0899D19D[id * 0x18], 1));
            }
        }
    }
    if (self[0x5A1] != 0) {
        u8 n = self[0x5A1];
        f32 t = (f32)n / (f32)self[0x5A2];
        u8 d = self[0x5A3];
        s16 px = (int)(232.0f * (1.0f - t) + t * (f32)BOX_X(d));
        s16 py = (int)(150.0f * (1.0f - t) + t * (f32)BOX_Y(d));
        f32 a = 57.0f * vfpu_sqrtf((f32)n);
        u16 id = U16(self, 0x5A4);
        const u8 *col;
        u8 al;

        if (a > 255.0f)
            a = 255.0f;
        col = &D_0893AA5C[D_0899D19E[id * 0x18] * 4];
        al = (u32)a & 0xFF;
        gfx_draw_sprite_scaled_rgba(self, (u16)px, (u16)py, 22, 22, col[0], col[1], col[2], al,
                                    item_icon_sprite(self, D_0899D19D[id * 0x18], 1));
    }
    slot = h[0x55D];
    gfx_draw_sprite_scaled(self, (BOX_X(slot) - 2) & 0xFFFF, (BOX_Y(slot) - 2) & 0xFFFF, 0x1A, 0x1A, 0x49);
    menu_draw_sprite(self, 0xEB, 0x99, 0x1C);
    if (S16(self, 0x59E) > 0) {
        f32 f = (f32)(S16(self, 0x59E) & 0x1F);
        u8 a = (u32)(45.0f * vfpu_sqrtf(f)) & 0xFF;
        f32 size = 32.0f * (1.0f + 0.17960416f * vfpu_sqrtf(31.0f - f));
        f32 off = 2.0f + (0.5f * size - 16.0f);

        slot = h[0x55D];
        gfx_draw_sprite_scaled_rgba(self, (u32)((f32)(BOX_X(slot) - 2) - off),
                                    (u32)((f32)(BOX_Y(slot) - 2) - off), (int)size, (int)size,
                                    0xFF, 0xFF, 0xFF, a, 0x49);
    }
    slot = h[0x55D];
    if (taken & (1 << (slot & 0x1F)))
        item = 0;
    else
        item = U16(D_089CC438, slot * 4 + 0x390);
    if (item == 0) {
        str = func_08851528(D_089CC438, 0);
    } else {
        s16 n = S16(game, slot * 4 + 0x392);
        s16 have = pouch_room_or_full(D_089CC438, item);
        s16 k = -1;

        if (have < 0)
            k = 1;
        else if (have < n)
            k = 2;
        else if (have == 0xFF && func_088568AC(D_089CC438, item) != 0)
            k = 2;
        if (k >= 0)
            func_088915B0(D_08A631A8, 0xE8, 0xB2, 2, D_0893B650, func_0885151C(D_089CC438, D_089AC854[k]));
        if (D_0899D19B[item * 0x18] == 0xFF) {
            sprintf(buf, func_0885151C(D_089CC438, D_089AC85A), func_08851528(D_089CC438, item));
        } else if (D_0899D19B[item * 0x18] == 1) {
            sprintf(buf, func_08851528(D_089CC438, item));
        } else {
            sprintf(buf, func_0885151C(D_089CC438, D_089AC85C), func_08851528(D_089CC438, item), n);
        }
        str = buf;
    }
    func_088915B0(D_08A631A8, (s16)(0x1C2 - (D_0893B704 >> 1) * func_08890E48(D_08A631A8, str)), 0x9C, 0,
                  D_0893B650, str);
    gfx_set_filter(self, 0);
    func_08825ADC(self, 1, (u16)(item + 0x18), -1);
}

/* starts the animation of the item taken from the supply box */
void hud_box_take_anim(u8 *self, u32 id, u32 slot)
{
    f32 dx, dy;
    u8 n;

    U16(self, 0x5A4) = id;
    self[0x5A3] = slot;
    dx = (f32)((self[0x5A3] & 7) * 28);
    dy = (f32)((self[0x5A3] >> 3) * 24 - 0x5E);
    n = (u32)(0.1f * (vfpu_sqrtf(dx * dx + dy * dy) * 1.7f));
    self[0x5A1] = n;
    self[0x5A2] = n;
}

/* starts the flash of the cursor of the supply box */
void hud_box_cursor_flash(u8 *self, u8 *h)
{
    S16(self, 0x59E) = 0x1F;
    self[0x5A0] = h[0x55D];
}

extern u8 *D_08A62CB4;
extern void *D_08A62D8C;
u32 game_buttons(void *game);
void sound_play(void *snd, int a, int b, int c, int d, int e, int f, int g, int h);

/* pages of the quest info (left / right), button 0x2000 closes it */
void hud_info_pages_input(u8 *self)
{
    u8 *p;
    s32 old, last;

    U32(self, 0x1194) |= 0x33;
    p = D_08A62CB4;
    old = *(s32 *)(p + 0x20);
    last = (*(s32 *)(p + 0x24) - 1) / 10 + 1;
    if ((u16)game_buttons(D_089CC438) & 0x80) {
        if (--*(s32 *)(p + 0x20) < 0)
            *(s32 *)(p + 0x20) = last - 1;
    } else if ((u16)game_buttons(D_089CC438) & 0x20) {
        if (last - 1 < ++*(s32 *)(p + 0x20))
            *(s32 *)(p + 0x20) = 0;
    }
    if (old != *(s32 *)(p + 0x20))
        sound_play(D_08A62D8C, 0, 0x11, 0, 0, 0, 0, 0, 0);
    if ((u16)game_buttons(D_089CC438) & 0x2000) {
        sound_play(D_08A62D8C, 0, 0x14, 0, 0, 0, 0, 0, 0);
        U32(*(u8 **)(self + 0x530), 0x614) ^= 0x200;
        U32(self, 0x1194) &= ~0x33;
    }
}
