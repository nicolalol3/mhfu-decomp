/* Menu: emblems, box with the name of an equipment piece, icons and colors of the
 * pieces by kind and rarity. Same class as menu_box.c. (Guessed names.) */
#include "menu.h"
#undef gfx_draw_window
#undef equip_draw_name_box
#undef equip_rarity_text_color
#undef menu_reset_messages

extern u8 *D_08A62C40;            /* display list manager */
extern void *g_equip_data;          /* equipment data */
extern const u8 D_08930BDC[62][14];   /* emblems: u8 -, u, v, id, ... */
extern const s16 D_08938758[][4];  /* sizes of the name boxes: w, h */
extern const u16 D_089AC8E4;
extern const u16 D_0893AE80[][11];
extern const u8 D_0893AEAC[][5];
extern const u32 D_0893AECC[];
extern const u8 D_0893AF0C[], D_0893AF20[];
extern const u16 D_0893AACC[][56];

void tex_select(void *self, u16 a, u16 b);
void dl_link(void *dl, u32 *p, int n, u8 layer);
void gfx_set_filter(void *self, int on);
void menu_draw_pager(void *self, s32 x, s32 y, u32 a, u32 b, u32 c);
void func_0881EE84(void *self, u8 *a, u32 x, u32 y, u32 flags, u8 *b, void *hunter, s32 idx);
u8 equip_rarity(void *a, int type, u16 id);
s32 deco_count(void *a, void *eq);
void heap_init(void *a, void *b, int n);
void gfx_draw_sprite_scaled(u8 *self, u32 x, u32 y, u32 w, u32 h, s32 id);
void gfx_draw_window(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 alpha);
void emblem_draw(u8 *self, u32 x, u32 y, u32 u, u32 v);
u32 equip_mark_kind(void *self, u8 *a, u32 flags, u8 *b);
u32 equip_mark_one(void *self, u8 *a, u32 flags, u8 *b);

/* copies the nh + nc words of w at the end of the display list and links them */
#define DL_PUSH(dl, layer, w, nh, nc, bi) do {                                         \
        u32 cur_ = U32(dl, 0x80198);                                                \
        s32 used_ = (s32)(cur_ - (u32)((dl) + U32(dl, 4) * 0x40000 + 0x198)) / 4;   \
        if ((u32)(used_ + (nh) + (nc)) < 0x10000) {                                 \
            int i_;                                                                 \
            (w)[bi] = ((cur_ & 0xFF000000) >> 8) | 0x10000000;                     \
            (w)[(bi) + 1] = (U32(dl, 0x80198) & 0xFFFFFF) | 0x01000000;            \
            for (i_ = 0; i_ < (nh) + (nc); i_++) {                                  \
                **(u32 **)((dl) + 0x80198) = (w)[i_];                               \
                U32(dl, 0x80198) += 4;                                              \
            }                                                                       \
            dl_link(dl, (u32 *)(cur_ + (nh) * 4), nc, layer);                \
        }                                                                           \
    } while (0)

/* emblem with id (NULL if none) */
const u8 *emblem_find(void *self, s32 id_arg)
{
    s8 id = id_arg;
    int i;

    for (i = 0; i < 62; i++)
        if ((s8)D_08930BDC[i][3] == id)
            return D_08930BDC[i];
    return NULL;
}

/* card with the emblem id */
void emblem_draw_card(u8 *self, u32 x, u32 y, s32 id)
{
    const u8 *e;

    gfx_draw_sprite_scaled(self, x, y, 88, 64, 175);
    e = emblem_find(self, id);
    if (e != NULL)
        emblem_draw(self, ((x & 0xFFFF) + 20) & 0xFFFF, ((y & 0xFFFF) + 7) & 0xFFFF, e[1], e[2]);
}

/* emblem image at (u, v) of the emblem texture, 51 x 51 on screen */
void emblem_draw(u8 *self, u32 x, u32 y, u32 u, u32 v)
{
    u8 *dl;
    u32 w[15];
    u8 layer;

    gfx_set_filter(self, 1);
    tex_select(self, 0x8002, 11);
    dl = D_08A62C40;
    w[2] = y & 0xFFFF;
    w[5] = ((y & 0xFFFF) + 51) & 0xFFFF;
    w[6] = 0x1E000001;
    w[8] = 0x50000001;
    w[9] = 0x12800116;
    w[12] = 0x04060002;
    w[13] = 0x10000000;
    w[14] = 0x08000000;
    w[7] = 0xC9000100;
    w[0] = (v << 16) | (u & 0xFFFF);
    w[3] = (((v & 0xFFFF) + 36) << 16) | ((u & 0xFFFF) + 36);
    w[1] = (x << 16) | 0xFFFF;
    w[4] = (((x & 0xFFFF) + 51) << 16) | 0xFFFF;
    layer = self[MENU_LAYER];
    if (layer < 20)
        DL_PUSH(dl, layer, w, 6, 9, 10);
    gfx_set_filter(self, 0);
}

/* box with the name of piece a (compared with piece b), size idx */
int equip_draw_name_box(u8 *self, u8 *a, u32 x, u32 y, u32 flags, u8 *b, s32 idx)
{
    s8 col;
    u8 m;

    gfx_draw_window(self, x & 0xFFFF, y & 0xFFFF, (u16)D_08938758[idx][0], (u16)D_08938758[idx][1], 255);
    if (a != NULL && U16(a, 2) == 0)
        a = NULL;
    if (b != NULL && U16(b, 2) == 0)
        b = NULL;
    if ((flags & 8) == 0) {
        col = 0;
    } else {
        col = 10;
        b = NULL;
    }
    font_set_size(g_font, 14, 14);
    if (a != NULL && U16(a, 2) >= 1501) {
        font_set_color(g_font, col);
        font_printf_at(g_font, (s16)((s16)x + 42), (s16)((s16)y + 8), fmt_s,
                      text_menu(g_game, D_089AC8E4));
        return 0;
    }
    if ((flags & 0x80) == 0) {
        m = equip_mark_kind(self, a, flags, b);
        if (m > 0) {
            s16 mx = (s16)((s16)x + D_08938758[idx][0] - 44);
            u32 k = 1;

            if (flags & 0x40) {
                mx = (s16)(mx - 15);
                k = 7;
            }
            menu_draw_pager(self, mx, (s16)((s16)y + D_08938758[idx][1] - 20), flags & 7, m, k);
        }
    }
    func_0881EE84(self, a, x, y, flags, b, *(void **)(self + MENU_PLAYER), idx);
    return 0;
}

/* icon of piece eq (which = 0 normal, 1 alternative) */
u16 equip_icon(void *self, u8 *eq, u32 which_arg)
{
    u8 which = which_arg;

    if (U16(eq, 2) >= 1501)
        return which != 0 ? 225 : 113;
    if ((u32)(eq[1] - 5) < 2)
        return D_0893AE80[which][equip_rarity(g_equip_data, eq[1], U16(eq, 2))];
    return D_0893AEAC[which][eq[1]];
}

/* color of rarity r (+10 if alt) with alpha a */
u32 equip_rarity_color(void *self, u32 r_arg, u32 a_arg, u32 alt_arg)
{
    u8 r = r_arg;
    u8 a = a_arg;

    if ((u8)alt_arg != 0)
        r = r + 10;
    return ((u32)a << 24) | D_0893AECC[D_0893AF0C[r]];
}

/* text color of rarity v */
u8 equip_rarity_text_color(void *self, u32 v)
{
    return D_0893AF20[v & 0xFF];
}

/* sprite of item icon i (small or big) */
u16 item_icon_sprite(void *self, u32 i, u32 small)
{
    return D_0893AACC[small & 0xFF][i & 0xFFFF];
}

/* mark shown on the name box of a (compared with b): number of the mark,
 * +1 when the piece is new / better */
u32 equip_mark_kind(void *self, u8 *a, u32 flags, u8 *b)
{
    u32 ra, rb, ka, kb;

    if (a != NULL && S16(a, 2) == 0)
        a = NULL;
    if (b != NULL && S16(b, 2) == 0)
        b = NULL;
    ra = equip_mark_one(self, a, flags, b);
    if (a != NULL && (flags & 0x100) != 0)
        return ra & 0xFF;
    ka = ra & 7;
    if (b != NULL) {
        rb = equip_mark_one(self, b, flags, a);
        if (flags & 0x100)
            return rb & 0xFF;
        kb = rb & 7;
        if (kb <= ka)
            kb = ka;
        if ((ra & 0x8000) == 0 && (rb & 0x8000) == 0)
            return kb;
        return kb + 1;
    }
    if ((flags & 0x10) == 0) {
        if ((ra & 0x8000) == 0)
            return ka;
        if ((flags & 0x20) == 0)
            return ka;
    }
    return ka + 1;
}

/* mark of piece a; 0x8000 = with decorations */
u32 equip_mark_one(void *self, u8 *a, u32 flags, u8 *b)
{
    int deco = 0;
    u32 r;

    if (a == NULL)
        return 0;
    if (U16(a, 2) == 0)
        return 0;
    if (U16(a, 2) > 1500)
        return 1;
    if (flags & 0x100) {
        if (a[1] != 5)
            return a[1] == 6 ? 8 : 6;
        if (b == NULL)
            return 6;
        return b[1] == 6 ? 8 : 6;
    }
    if (deco_count(g_equip_data, a) != 0) {
        deco = 1;
    } else if (b == NULL) {
        goto kind;
    } else if (S16(b, 2) != 0 && deco_count(g_equip_data, b) != 0) {
        deco = 1;
    }
kind:
    if (a[1] == 5) {
        r = 2;
    } else if (a[1] == 6) {
        r = 4;
    } else {
        return deco ? 0x8005 : 0x8003;
    }
    if (deco)
        r = (r + 3) | 0x8000;
    return r;
}

/* clears the message queue and copies the hunter into the menu */
void menu_reset_messages(u8 *self)
{
    self[0x10F8] = 0;
    self[0x10F9] = 0;
    self[0x10FA] = 0;
    memset(self + 0xE28, 0, 0x2D0);
    heap_init(self + 0x500, self, 0x500);
}
