/* Equipment set of the hunter: temporary changes (preview in the equipment menu),
 * weapon gauges and skill lists. Same class as menu_box.c. (Guessed names.)
 *
 * An equipment piece is 0xC bytes: u8 kind0, u8 type, u16 id, u16 extra[4].
 * In the hunter: 0x4E8 = weapon, 0x4F4 + type * 0xC = armor pieces (5). */
#include "menu.h"

extern void *g_equip_data;          /* equipment data */
extern u8 D_0893AC30[], D_0893AC38[], D_0893AC3C[], D_0893AC44[], D_0893AC4C[], D_0893AC50[];
extern u8 D_0893AC58[], D_0893AC60[], D_0893AC64[], D_0893AC6C[], D_0893AC74[], D_0893AC7C[];
extern u8 D_0893AC84[];
extern const u8 D_0893AC9C[];      /* sharpness colors (b, g, r, -) */
extern const u8 D_08978488[];
extern s16 *const D_0897923C[];     /* sharpness tables (7 s16 per weapon) */
extern s16 D_08939A40, D_08939A58;
extern const char D_089385D8[], D_089385DC[];
extern const u16 D_089AC9CC, D_089AC9D2, D_089AC8AA;
extern const s32 D_0893AEB8[5];

u8 *equip_data_of(void *a, void *eq);
u8 hunter_skill_value(void *pad, int key);
s32 hunter_has_skill(void *pad, int key);
void func_088D897C(void *hunter);
void func_088DAAA0(void *hunter);
u8 func_088DAB94(void *hunter, int v);
u32 deco_count(void *a, void *eq);
u8 equip_rarity(void *a, int type, u16 id);
u16 weapon_base_id(void *a, void *eq);
u8 item_rarity(void *a, u16 id);
u8 table_314fc_a(void *a, u8 v);
u8 table_314fc_b(void *a, u8 v);
u8 bowgun_byte13(void *a, void *eq);
u8 menu_armor_wearable(void *self, void *hunter, void *eq);
void gfx_draw_sprite(void *self, u16 x, u16 y, u8 r, u8 g, u8 b, u32 icon, int c);
void gfx_draw_sprite_scaled_color(void *self, u16 x, u16 y, int w, int h, u8 r, u8 g, u8 b, int c);
int equip_adjust_level(void *self, void *pad, int v);
void equip_list_skill_points(void *self, u32 id, u8 *out);
int equip_compare_skill(void *self, const s8 *a, const s8 *b);
void equip_save(void *self, u8 *hunter);
void equip_restore(void *self, u8 *hunter);
void equip_try_weapon(void *self, u8 *hunter, u8 *eq);
int equip_same(void *self, const u8 *a, const u8 *b);
void equip_check_armor(void *self, u8 *hunter);

/* copies an equipment piece field by field (as the original) */
#define EQ_COPY(d, s) do {                          \
        U8(d, 0) = U8(s, 0);   U8(d, 1) = U8(s, 1);    \
        U16(d, 2) = U16(s, 2); U16(d, 4) = U16(s, 4);  \
        { u16 t8 = U16(s, 8), t10 = U16(s, 10);         \
          U16(d, 6) = U16(s, 6); U16(d, 8) = t8; U16(d, 10) = t10; } \
    } while (0)

/* preview of the weapon eq on the hunter (self+0x530): two levels modified by the
 * skills (*sharp, *hits); returns the flags of what the skills changed */
u16 equip_preview_weapon(u8 *self, u8 *eq, u8 *sharp, u8 *hits)
{
    void *hunter = *(void **)(self + 0x530);
    u16 r = 0;
    u8 *w = equip_data_of(g_equip_data, eq);
    u8 k;

    equip_save(self, hunter);
    equip_try_weapon(self, hunter, eq);
    *sharp = w[0x13] + 3;
    k = hunter_skill_value(hunter, 0x14);
    if (k != 0) {
        switch (k) {
        case 0x2F: *sharp += 1; break;
        case 0x30: *sharp += 2; break;
        case 0x31: *sharp += 3; break;
        case 0x32: *sharp -= 1; break;
        case 0x33: *sharp -= 2; break;
        case 0x34: *sharp -= 3; break;
        }
        if (*sharp >= 10)
            *sharp = 9;
        r = 1;
    }
    if ((u8)hunter_has_skill(hunter, 0xD9) == 1) {
        *hits = 1;
        r |= 4;
    } else {
        *hits = w[0xB];
        if (U16(eq, 4) & 0x10)
            *hits += 1;
        if ((u8)hunter_has_skill(hunter, 0x36) == 1) {
            *hits += 4;
            if (*hits >= 7)
                *hits = 6;
            r |= 2;
        } else if ((u8)hunter_has_skill(hunter, 0x35) == 1) {
            *hits += 2;
            if (*hits >= 7)
                *hits = 6;
            r |= 2;
        }
    }
    equip_restore(self, hunter);
    return r;
}

/* layout of an equipment entry, depending on the kind of the piece and on the flags */
u8 *equip_entry_layout(void *self, u8 *eq, u32 flags, u8 *other)
{
    u32 a;
    u32 kind;

    if (eq == NULL) {
        if (other == NULL)
            return D_0893AC84;
        if ((U8(other, 1) == 6 || U8(other, 1) == 5) && (flags & 0x100) == 0
            && deco_count(g_equip_data, other) == 0)
            return D_0893AC84;
        return D_0893AC7C;
    }
    a = deco_count(g_equip_data, eq);
    if (other == NULL) {
        flags &= 0x110;
        kind = 0xFFFFFFFF;
    } else {
        a |= deco_count(g_equip_data, other);
        kind = U8(other, 1);
        flags = (flags & 0x100) | a;
    }
    if (U8(eq, 1) == 6) {
        if (equip_rarity(g_equip_data, 6, U16(eq, 2)) == 10) {
            if (flags != 0)
                return D_0893AC58;
            return a == 0 ? D_0893AC60 : D_0893AC64;
        }
        if (flags != 0)
            return D_0893AC44;
        return a == 0 ? D_0893AC4C : D_0893AC50;
    }
    if (U8(eq, 1) == 5) {
        if (flags != 0)
            return D_0893AC30;
        return a == 0 ? D_0893AC38 : D_0893AC3C;
    }
    if (kind == 0xFFFFFFFF && flags == 0)
        return D_0893AC74;
    return D_0893AC6C;
}

/* sharpness gauge of a blade weapon (type 5), with the change after the new level */
void equip_draw_sharpness(void *self, u8 *hunter, u8 *eq, s32 x_arg, s32 y_arg)
{
    s16 x = x_arg;
    s16 y = y_arg;
    u8 *w;
    u32 lvl, nl;
    u32 c;
    const s16 *t;
    const u8 *col;
    s16 pos;
    int i;

    if (U8(eq, 1) != 5)
        return;
    w = equip_data_of(g_equip_data, eq);
    lvl = w[9];
    equip_save(self, hunter);
    equip_try_weapon(self, hunter, eq);
    nl = equip_adjust_level(self, hunter, lvl);
    equip_restore(self, hunter);
    if (nl == lvl) {
        c = 0xFF968A63;
    } else {
        int color, sign;

        if ((int)lvl < (int)nl) {
            c = 0xFF57FFFF;
            color = 4;
            sign = 1;
        } else {
            c = 0xFFFF435D;
            color = 2;
            sign = -1;
        }
        font_printf_at_color(g_font, (s16)(x + 100), (s16)(y + 1), color, D_089385D8, sign);
    }
    col = D_0893AC9C;
    t = D_0897923C[D_08978488[U16(w, 0)]] + w[8] * 7;
    pos = 0;
    for (i = 7; i > 0; i--, t++, col += 4) {
        s16 end = (s16)(int)((f32)*t * 0.2f);

        if (pos < end) {
            gfx_fill_rect(self, (s16)(x + 4) + pos, (s16)(y + 5), end - pos, 6, col[2], col[1], col[0]);
            pos = end;
        }
    }
    if ((int)nl < 6) {
        s16 p = (s16)(int)((f32)(int)(nl * 50 + 150) * 0.2f);

        gfx_fill_rect(self, (s16)(x + 4) + p, (s16)(y + 5), 90 - p, 6, 0, 0, 0);
    }
    gfx_draw_sprite(self, x, y, (u8)(c >> 16), (u8)(c >> 8), (u8)c, 0xB7, 0);
    gfx_draw_sprite_scaled_color(self, x + D_08939A40, y, (98 - D_08939A58) - D_08939A40, 16,
                  (u8)(c >> 16), (u8)(c >> 8), (u8)c, 0xB8);
    gfx_draw_sprite(self, (x + 98) - D_08939A58, y, (u8)(c >> 16), (u8)(c >> 8), (u8)c, 0xB9, 0);
}

/* list of the skill points of piece id, colored by comparison with piece cmp */
void equip_draw_skill_points(void *self, u32 id, s32 x, s32 y, u32 cmp, u32 flags)
{
    s8 col[9];
    u8 pts[18];
    u8 other[18];
    s8 c0;
    int color;
    int i;
    s16 cx, ly, y0;
    u8 *p;

    if ((flags & 8) == 0) {
        c0 = 0;
        color = 5;
    } else {
        c0 = 10;
        color = 10;
        cmp = 0;
    }
    font_printf_at_color(g_font, x, y, color, fmt_s, text_menu(g_game, D_089AC9CC));
    for (i = 0; i < 9; i++)
        col[i] = c0;
    equip_list_skill_points(self, id, pts);
    if (cmp != 0 && id != cmp) {
        equip_list_skill_points(self, cmp, other);
        for (i = 0, p = pts; i < 9; i++, p += 2) {
            s16 d = equip_compare_skill(self, (s8 *)p, (s8 *)other);

            if (d > 0)
                col[i] = 4;
            else if (d < 0)
                col[i] = 2;
        }
    }
    cx = (s16)x;
    y0 = (s16)y + 16;
    ly = y0;
    for (i = 0, p = pts; i < 9; i++, p += 2) {
        font_set_color(g_font, c0);
        font_set_pos(g_font, cx, ly);
        if (p[0] == 0) {
            if (i == 0)
                font_printf(g_font, fmt_s, text_menu(g_game, D_089AC9D2));
            return;
        }
        font_printf(g_font, fmt_s, text_menu(g_game, D_089ACC70[p[0]]));
        if (p[0] != 1) {
            font_printf_at(g_font, (s16)(cx + 70), ly, fmt_s, text_menu(g_game, D_089AC8AA));
            font_set_color(g_font, col[i]);
            font_printf_at(g_font, (s16)(cx + 84), ly, D_089385DC, (s8)p[1]);
        }
        ly += 16;
        if (i == 4) {
            cx = (s16)(cx + 120);
            ly = y0;
        }
    }
}

/* saves the current equipment of the hunter in self (0x11BE) */
void equip_save(void *self, u8 *hunter)
{
    u8 *d = (u8 *)self + 0x11BE;
    u8 *s = hunter + 0x4E8;
    int i;

    EQ_COPY(d, s);
    for (i = 0; i < 5; i++)
        EQ_COPY((u8 *)self + 0x11CA + i * 0xC, hunter + 0x4F4 + i * 0xC);
    U8(self, 0x1206) = 0;
}

/* puts back the saved equipment, if it was changed */
void equip_restore(void *self, u8 *hunter)
{
    int i;

    if (U8(self, 0x1206) != 1)
        return;
    for (i = 0; i < 5; i++)
        EQ_COPY(hunter + 0x4F4 + i * 0xC, (u8 *)self + 0x11CA + i * 0xC);
    EQ_COPY(hunter + 0x4E8, (u8 *)self + 0x11BE);
    func_088D897C(hunter);
    func_088DAAA0(hunter);
    U8(hunter, 0x5F2) = func_088DAB94(hunter, S16(hunter, 0x60C));
    U8(self, 0x1206) = 0;
}

/* puts the weapon eq on the hunter (temporarily) */
void equip_try_weapon(void *self, u8 *hunter, u8 *eq)
{
    if (equip_same(self, hunter + 0x4E8, eq) != 0)
        return;
    EQ_COPY(hunter + 0x4E8, eq);
    if (U8(self, 0x11BF) != eq[1])
        equip_check_armor(self, hunter);
    func_088D897C(hunter);
    func_088DAAA0(hunter);
    U8(hunter, 0x5F2) = func_088DAB94(hunter, S16(hunter, 0x60C));
    U8(self, 0x1206) = 1;
}

/* puts the armor piece eq on the hunter (temporarily) */
void equip_try_armor(void *self, u8 *hunter, u8 *eq)
{
    u8 *d = hunter + eq[1] * 0xC;

    EQ_COPY(d + 0x4F4, eq);
    func_088D897C(hunter);
    U8(self, 0x1206) = 1;
}

/* 1 if the two pieces are the same */
int equip_same(void *self, const u8 *a, const u8 *b)
{
    return U8(a, 1) == U8(b, 1) && S16(a, 2) == S16(b, 2) && S16(a, 4) == S16(b, 4)
        && S16(a, 6) == S16(b, 6) && S16(a, 8) == S16(b, 8) && S16(a, 10) == S16(b, 10);
}

/* sets the weapon of the hunter and recomputes the derived values */
void equip_set_weapon(u8 *hunter, u8 *eq)
{
    EQ_COPY(hunter + 0x4E8, eq);
    U16(hunter, 0x534) = weapon_base_id(g_equip_data, hunter + 0x4E8);
    U8(hunter, 0x1E8) = item_rarity(g_equip_data, U16(hunter, 0x534));
    U8(hunter, 0x5F3) = table_314fc_a(g_equip_data, U8(hunter, 0x1E8));
    U8(hunter, 0x5F4) = table_314fc_b(g_equip_data, U8(hunter, 0x1E8));
    U8(hunter, 0x5F5) = bowgun_byte13(g_equip_data, hunter + 0x4E8);
    func_088DAAA0(hunter);
    U8(hunter, 0x5F2) = func_088DAB94(hunter, S16(hunter, 0x60C));
}

/* removes the armor pieces that cannot be worn with the current weapon */
void equip_check_armor(void *self, u8 *hunter)
{
    const s32 *t = D_0893AEB8;
    int i;

    for (i = 0; i < 5; i++, t++) {
        u8 *eq = hunter + *t * 0xC;

        if (S16(eq, 0x4F6) != 0 && menu_armor_wearable(self, hunter, eq + 0x4F4) == 0)
            S16(eq, 0x4F6) = 0;
    }
}
