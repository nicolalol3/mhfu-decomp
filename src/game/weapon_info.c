/* Weapon information in the equipment menu: element/status values, notes, sharpness,
 * slots and other lines of the weapon card. Same class as menu_box.c. (Guessed names.) */
#include "menu.h"

extern void *g_equip_data;          /* equipment data */
extern const u8 D_0893AD00[];      /* value lines: u8 n, u8 idx[3], u32 mask[4] (0x14 bytes) */
extern const u8 D_089786F8[];      /* values per weapon type (32 per type) */
extern const char D_0893855C[], D_089385E4[], D_089385EC[], D_089385F0[];
extern const u16 D_089AC96E, D_089AC970, D_089AC972, D_089AC974, D_089AC976;
extern const u16 D_089AC97C, D_089AC97E, D_089AC988, D_089AC98A, D_089AC98C, D_089AC98E, D_089AC990;
extern const u16 D_089AC8F0;
extern const u16 D_089AC998[], D_089AC9D4[], D_089AC954[], D_089AC9DC[];
extern const u8 D_089B7E18[];      /* note colors of the hunting horns (3 per weapon) */
extern const u8 D_0893ACE0[];      /* colors (r, g, b, -) */
extern const s8 D_08978E38[];      /* shells of the gunlances (4 per weapon) */
extern const u16 D_0893ACC4[6], D_0893ACB8[6];
extern const u8 D_08930A34[6][2];
extern const s8 D_0893AA88[];

u8 bowgun_byte13(void *a, void *eq);
s32 hunter_has_skill(void *hunter, int skill);
s16 func_088DA398(void *hunter, int v);
u8 equip_slots(void *a, void *eq);
s32 deco_free_slots(void *a, void *eq);
void deco_list(void *a, void *eq, u16 *out);
u8 *deco_data(void *a, u16 id);
void font_draw_at_color(void *font, int x, int y, int color, const char *fmt, ...);
void func_08891700(void *font, int x, int y, const char *s);
void gfx_draw_sprite_scaled_color(void *self, u16 x, u16 y, int w, int h, u8 r, u8 g, u8 b, int c);
char *strcpy(char *d, const char *s);
void equip_save(void *self, u8 *hunter);
void equip_restore(void *self, u8 *hunter);
void equip_try_weapon(void *self, u8 *hunter, u8 *eq);
int weapon_has_affinity(void *self, u8 *hunter);

/* line of values (up to 3) of the weapon: active ones in color, changed ones highlighted */
void weapon_draw_values(void *self, int x, int y, u8 *wp, u32 mask, u32 changed, u32 idx_arg, u32 bonus_arg)
{
    u8 idx = idx_arg;
    u8 bonus = bonus_arg;
    const u8 *e = D_0893AD00 + idx * 0x14;
    u8 val[4];
    char buf[32], tmp[32];
    s8 col;
    u8 c0 = 10, c2 = 10, c3 = 10;
    u8 d1 = 10, d2 = 10, d3 = 10;

    font_set_pos(g_font, x, y);
    val[0] = val[1] = val[2] = 0;
    col = 10;
    if (mask & *(const u32 *)(e + 0x10)) {
        col = (changed & *(const u32 *)(e + 0x10)) ? 3 : 0;
        c0 = 0;
    }
    if (mask & *(const u32 *)(e + 4)) {
        val[0] = bonus + D_089786F8[wp[0x10] * 32 + e[1]];
        if (bonus != 0)
            d1 = 3;
        else
            d1 = (changed & *(const u32 *)(e + 4)) ? 3 : 0;
    }
    if (mask & *(const u32 *)(e + 8)) {
        val[1] = bonus + D_089786F8[wp[0x10] * 32 + e[2]];
        if (bonus != 0)
            d2 = 3;
        else
            d2 = (changed & *(const u32 *)(e + 8)) ? 3 : 0;
        c2 = 0;
    }
    if (mask & *(const u32 *)(e + 12)) {
        val[2] = bonus + D_089786F8[wp[0x10] * 32 + e[3]];
        if (bonus != 0)
            d3 = 3;
        else
            d3 = (changed & *(const u32 *)(e + 12)) ? 3 : 0;
        c3 = 0;
    }
    if (c0 == d1)
        sprintf(buf, D_0893855C, val[0]);
    else
        sprintf(buf, text_menu(g_game, D_089AC970), d1, val[0]);
    if (e[0] >= 2) {
        if (d1 == c2)
            sprintf(tmp, text_menu(g_game, D_089AC976), buf);
        else
            sprintf(tmp, text_menu(g_game, D_089AC974), buf, c2);
        if (c2 == d2)
            sprintf(buf, D_089385E4, tmp, val[1]);
        else
            sprintf(buf, text_menu(g_game, D_089AC972), tmp, d2, val[1]);
        if (e[0] >= 3) {
            if (d2 == c3)
                sprintf(tmp, text_menu(g_game, D_089AC976), buf);
            else
                sprintf(tmp, text_menu(g_game, D_089AC974), buf, c3);
            if (c3 == d3)
                sprintf(buf, D_089385E4, tmp, val[2]);
            else
                sprintf(buf, text_menu(g_game, D_089AC972), tmp, d3, val[2]);
        }
    }
    font_set_color(g_font, col);
    font_draw(g_font, text_menu(g_game, D_089AC96E),
                  text_menu(g_game, D_089AC998[idx]), c0, buf);
}

/* kind of the weapon (one of 3 texts) */
void weapon_draw_kind(void *self, void *eq, s32 x, s32 y)
{
    u8 k = bowgun_byte13(g_equip_data, eq);

    font_draw_at_color(g_font, (s16)((s16)x + 0x70), (s16)y, 0, fmt_s,
                  text_menu(g_game, D_089AC9D4[k % 3]));
}

/* the three notes of a hunting horn */
void weapon_draw_notes(void *self, void *eq, s32 x_arg, s32 y_arg)
{
    s16 x = x_arg;
    s16 y = y_arg;
    const u8 *n;
    int i;

    font_printf_at_color(g_font, (s16)(x + 0x62), y, 0, fmt_s, text_menu(g_game, D_089AC97C));
    x += 0x98;
    n = D_089B7E18 + bowgun_byte13(g_equip_data, eq) * 3;
    for (i = 3; i > 0; i--, n++) {
        const u8 *c = D_0893ACE0 + *n * 4;

        gfx_draw_sprite_scaled_color(self, x, y - 2, 12, 16, c[0], c[1], c[2], 0x177);
        x += 16;
    }
}

/* shell levels of a gunlance (3 or 4 lines, depending on the skills) */
void weapon_draw_shells(void *self, u8 *wd, u8 *hunter, u8 *eq, s32 x_arg, s32 y_arg)
{
    s16 x = x_arg;
    u16 y = y_arg;
    char num[8], txt[8];
    int n = 3;
    int i;

    font_printf_at_color(g_font, x, (s16)y, 5, fmt_s, text_menu(g_game, D_089AC97E));
    equip_save(self, hunter);
    equip_try_weapon(self, hunter, eq);
    if (wd[0xB] == 4)
        n++;
    else if (wd[0xB] == 2) {
        if ((u8)hunter_has_skill(hunter, 0xAA) == 0)
            n--;
    } else if ((u8)hunter_has_skill(hunter, 0xAA) == 1)
        n++;
    y += 16;
    for (i = 0; i < n; i++) {
        s8 c2;
        int color;
        const char *a;

        if (i == 3 && wd[0xB] != 4) {
            c2 = 3;
            color = 3;
        } else if (i == 2 && wd[0xB] == 2) {
            c2 = 3;
            color = 3;
        } else {
            color = 6;
            c2 = 0;
        }
        sprintf(num, D_089385EC, i + 1);
        strcpy(txt, num);
        a = text_menu(g_game, D_089AC954[i != 0 ? 23 : 22]);
        font_draw_at_color(g_font, x, (s16)y, color, a, txt, c2,
                      text_menu(g_game, D_089AC9DC[D_08978E38[wd[0x10] * 4 + i]]));
        y += 16;
    }
    equip_restore(self, hunter);
}

/* status effects of the weapon (6 names, active ones in color) */
void weapon_draw_status(void *self, u8 *wd, s32 x_arg, s32 y_arg, u32 changed)
{
    s16 x = x_arg;
    s16 y = y_arg;
    const u16 *m = D_0893ACC4;
    const u16 *t = D_0893ACB8;
    const u8 *p = D_08930A34[0];
    int i;

    font_printf_at_color(g_font, x, y, 5, fmt_s, text_menu(g_game, D_089AC988));
    for (i = 0; i < 6; i++, m++, t++, p += 2) {
        s8 c = (*(u32 *)(wd + 0x18) & *m) ? 0 : 10;

        if (c == 10 && (changed & *m))
            c = 3;
        font_printf_at_color(g_font, (s16)(x + p[0]), (s16)(y + p[1]), c, fmt_s,
                      text_item_name(g_game, *t));
    }
}

/* affinity of the weapon, compared with that of the weapon other */
void weapon_draw_affinity(void *self, u8 *hunter, u8 *eq, s32 x_arg, s32 y_arg, u8 *other)
{
    s16 x = x_arg;
    s16 y = y_arg;
    char num[8], txt[16];
    int color, lcolor;
    s16 a, b;

    if (U8(eq, 1) != 6 && U8(eq, 1) != 5)
        return;
    equip_save(self, hunter);
    lcolor = 5;
    color = 0;
    equip_try_weapon(self, hunter, eq);
    a = 0;
    if ((u8)weapon_has_affinity(self, hunter) == 1) {
        a = func_088DA398(hunter, 8);
        if ((u8)hunter_has_skill(hunter, 0x20) == 1 || (u8)hunter_has_skill(hunter, 0x21) == 1
            || (u8)hunter_has_skill(hunter, 0x22) == 1 || (u8)hunter_has_skill(hunter, 0x23) == 1
            || (u8)hunter_has_skill(hunter, 0x24) == 1 || (u8)hunter_has_skill(hunter, 0x25) == 1) {
            color = 3;
            lcolor = 3;
        }
    }
    equip_restore(self, hunter);
    if (other != NULL) {
        equip_try_weapon(self, hunter, other);
        b = 0;
        if ((u8)weapon_has_affinity(self, hunter) == 1)
            b = func_088DA398(hunter, 8);
        equip_restore(self, hunter);
        if (b < a)
            color = 4;
        else if (a < b)
            color = 2;
    }
    font_set_color(g_font, lcolor);
    font_printf_at(g_font, x, y, text_menu(g_game, D_089AC98A));
    font_set_color(g_font, color);
    sprintf(num, D_089385F0, a);
    strcpy(txt, num);
    func_08891700(g_font, (s16)(x + 0x38), y, txt);
}

/* 1 if the weapon of the hunter can have an affinity value */
int weapon_has_affinity(void *self, u8 *hunter)
{
    if (U8(hunter, 0x4E9) == 6)
        return 1;
    return U8(hunter, 0x5F2) != 0;
}

/* slots of the piece: used (with the color of the decoration), free, missing */
void weapon_draw_slots(void *self, void *eq, s32 x_arg, s32 y_arg, u32 grey_arg)
{
    u16 x = x_arg;
    u8 grey = grey_arg;
    s16 y = y_arg;
    u16 deco[4];
    int total, used;
    int i;

    total = equip_slots(g_equip_data, eq);
    used = total - deco_free_slots(g_equip_data, eq);
    deco_list(g_equip_data, eq, deco);
    font_set_color(g_font, 9);
    x += (u16)((font_char_count(g_font, text_menu(g_game, D_089AC8F0)) + 1) * 7);
    for (i = 0; i < used; i++) {
        if (grey == 0)
            font_set_color(g_font, D_0893AA88[D_0899D198[deco[i] * 0x18 + 6]]);
        font_printf_at(g_font, (s16)x, y, text_menu(g_game, D_089AC98C));
        x += 14;
    }
    font_set_color(g_font, 9);
    for (; i < total; i++) {
        font_printf_at(g_font, (s16)x, y, text_menu(g_game, D_089AC98E));
        x += 14;
    }
    for (; i < 3; i++) {
        font_printf_at(g_font, (s16)x, y, text_menu(g_game, D_089AC990));
        x += 14;
    }
}

/* number of rows (two per row) of the decorations of piece id */
int weapon_deco_rows(void *self, u32 id_arg)
{
    u16 id = id_arg;
    u8 *p;
    int n;

    if (id == 0)
        return 0;
    p = deco_data(g_equip_data, id) + 8;
    for (n = 0; n < 4; n++, p += 2)
        if (*p == 0)
            break;
    return (n + 1) >> 1;
}

extern const u8 D_0893AC8C[];      /* deco layouts: low 2 bits = how many, high 4 = which */
extern const u16 D_089AC992, D_089AC994, D_089AC8AA;
extern const char D_08938548[], D_089385F8[];

/* decorations of the piece eq with their skill points (flags & 8 = greyed out) */
void weapon_draw_decorations(void *self, u8 *eq, s32 x_arg, u32 y_arg, u32 single_arg, u32 flags)
{
    s16 x = x_arg;
    u16 y = y_arg;
    u8 single = single_arg;
    u8 c0 = (flags & 8) ? 10 : 0;
    u8 *deco[2];
    const char *name[2];
    s8 col[2];
    char num[8], txt[8];
    int sum, mul, i, k;

    if ((u8)equip_slots(g_equip_data, eq) == 0) {
        font_printf_at_color(g_font, x, (s16)y, (s8)c0, fmt_s, text_menu(g_game, D_089AC992));
        return;
    }
    sum = 0;
    mul = 1;
    for (i = 0; i < 3; i++) {
        if (U16(eq, 6 + i * 2) != 0)
            sum += mul * weapon_deco_rows(self, U16(eq, 6 + i * 2));
        mul *= 3;
    }
    deco[1] = NULL;
    deco[0] = NULL;
    if (single == 0) {
        u8 m = D_0893AC8C[sum] & 3;

        if (m == 1 || m == 2) {
            if (m == 2) {
                deco[1] = deco_data(g_equip_data, U16(eq, 8));
                name[1] = text_item_name(g_game, U16(eq, 8));
                col[1] = D_0893AA88[D_0899D198[U16(eq, 8) * 0x18 + 6]];
            }
            deco[0] = deco_data(g_equip_data, U16(eq, 6));
            name[0] = text_item_name(g_game, U16(eq, 6));
            col[0] = D_0893AA88[D_0899D198[U16(eq, 6) * 0x18 + 6]];
        }
    } else if ((D_0893AC8C[sum] >> 4) != 0) {
        u16 id = U16(eq + (D_0893AC8C[sum] >> 4) * 2, 6);

        deco[0] = deco_data(g_equip_data, id);
        name[0] = text_item_name(g_game, id);
        col[0] = D_0893AA88[D_0899D198[id * 0x18 + 6]];
    }
    if (flags & 8) {
        col[1] = 10;
        col[0] = 10;
    }
    for (k = 0; k < 2; k++) {
        u8 *p;
        int j;

        if (deco[k] == NULL) {
            if (k == 0) {
                sprintf(num, D_08938548, deco_free_slots(g_equip_data, eq));
                strcpy(txt, num);
                font_printf_at_color(g_font, x, (s16)y, (s8)c0, text_menu(g_game, D_089AC994), txt);
            }
            return;
        }
        font_printf_at_color(g_font, x, (s16)y, col[k], fmt_s, text_menu(g_game, D_089AC98C));
        font_printf_at_color(g_font, (s16)(x + 14), (s16)y, (s8)c0, fmt_s, name[k]);
        y += 16;
        p = deco[k] + 8;
        for (j = 0; j < 4; j++, p += 2) {
            s16 px, py;

            if (*p == 0)
                break;
            px = x + (j & 1) * 120;
            py = y + (j >> 1) * 16;
            font_printf_at(g_font, px, py, fmt_s, text_menu(g_game, D_089ACC70[*p]));
            if (*p != 1) {
                font_set_pos(g_font, (s16)(px + 70), py);
                font_printf(g_font, D_089385F8, text_menu(g_game, D_089AC8AA), (s8)p[1]);
            }
        }
        y += (u16)(((j + 1) >> 1) << 4);
    }
}

extern const u8 D_08938600[];
u8 *equip_data_of(void *a, void *eq);
u8 equip_rarity(void *a, int type, u16 id);

/* element/status of the weapon: index (*idx, -1 = none), base value (*base) and
 * value with the skills of the hunter (*val) */
void weapon_get_element(void *self, void *hunter, u8 *eq, s16 *val, s16 *idx, u16 *base)
{
    u8 *w;

    *idx = -1;
    if (eq[1] == 5) {
        w = equip_data_of(g_equip_data, eq);
        *base = w[0xC];
        if (w[0xE] != 0)
            *idx = w[0xE] - 1;
        else if (w[0x10] != 0)
            *idx = w[0x10] + 4;
    } else {
        w = equip_data_of(g_equip_data, eq);
        *base = w[0xA];
        if (equip_rarity(g_equip_data, eq[1], U16(eq, 2)) == 10 && w[0x11] != 0)
            *idx = w[0x11] - 1;
    }
    if (*idx >= 0)
        *val = func_088DA398(hunter, D_08938600[*idx]);
}

extern const s8 D_08938608[];
extern const u16 D_089AC966, D_089AC968;
extern const u16 D_089AC9BC[];
s32 bowgun_part_index(void *a, void *eq);
u8 *bowgun_part(void *a, s32 i);
s16 func_088DA740(void *hunter, u16 mask, s16 v);
u8 hunter_state_flag(void *hunter, int flag);
s32 hunter_felyne_skill(void *hunter, int v);
int weapon_element_boosted(void *self, void *hunter, s32 idx);

/* index of the lowest set bit (0..15), -1 if none */
static s16 lowest_bit(u16 m)
{
    u32 i;

    for (i = 0; (m & (1 << i)) == 0; )
        if (++i > 15)
            return -1;
    return i;
}

/* element and status values of the weapon eq, compared with those of other */
void weapon_draw_element(u8 *self, u8 *eq, s32 x_arg, s32 y_arg, u8 *other)
{
    s16 x = x_arg;
    s16 y = y_arg;
    void *hunter = *(void **)(self + MENU_PLAYER);
    char buf1[40], buf2[40];
    s16 v[3], o[3];     /* value, index, base */
    s16 idx2, val2, oidx2, oval2;
    u8 c1, l1, c2, l2, c3, l3;
    s32 i;
    u8 *e;

    font_set_size(g_font, 14, 14);
    equip_save(self, hunter);
    equip_try_weapon(self, hunter, eq);
    weapon_get_element(self, hunter, eq, &v[0], &v[1], (u16 *)&v[2]);
    i = bowgun_part_index(g_equip_data, eq);
    if (i == -1) {
        idx2 = -1;
    } else {
        e = bowgun_part(g_equip_data, i);
        v[1] = D_08938608[lowest_bit(U16(e, 2))];
        idx2 = D_08938608[lowest_bit(U16(e, 6))];
        v[0] = func_088DA740(hunter, U16(e, 2), S16(e, 4));
        val2 = func_088DA740(hunter, U16(e, 6), S16(e, 8));
    }
    equip_restore(self, hunter);
    l2 = 0;
    c2 = 0;
    c3 = 0;
    l3 = 0;
    c1 = 0;
    l1 = 0;
    if ((u8)weapon_element_boosted(self, hunter, v[1]) == 1) {
        c1 = 3;
        l1 = 3;
    }
    if ((u8)weapon_element_boosted(self, hunter, idx2) == 1) {
        c2 = 3;
        l2 = 3;
    }
    if (other != NULL) {
        equip_try_weapon(self, hunter, other);
        weapon_get_element(self, hunter, other, &o[0], &o[1], (u16 *)&o[2]);
        i = bowgun_part_index(g_equip_data, other);
        if (i == -1) {
            oidx2 = -1;
        } else {
            e = bowgun_part(g_equip_data, i);
            o[1] = D_08938608[lowest_bit(U16(e, 2))];
            oidx2 = D_08938608[lowest_bit(U16(e, 6))];
            o[0] = func_088DA740(hunter, U16(e, 2), S16(e, 4));
            oval2 = func_088DA740(hunter, U16(e, 6), S16(e, 8));
        }
        equip_restore(self, hunter);
        if (v[1] >= 0) {
            if (v[1] == o[1]) {
                if (o[0] < v[0])
                    c1 = 4;
                else if (v[0] < o[0])
                    c1 = 2;
            } else if (v[1] == oidx2) {
                if (oval2 < v[0])
                    c1 = 4;
                else if (v[0] < oval2)
                    c1 = 2;
            } else {
                c1 = 4;
                l1 = 4;
            }
        }
        if (idx2 >= 0) {
            if (idx2 == o[1]) {
                if (o[0] < val2)
                    c2 = 4;
                else if (val2 < o[0])
                    c2 = 2;
            } else if (idx2 == oidx2) {
                if (oval2 < val2)
                    c2 = 4;
                else if (val2 < oval2)
                    c2 = 2;
            } else {
                c2 = 4;
                l2 = 4;
            }
        }
        c3 = 0;
        if (v[2] > 0) {
            if (o[2] < 1) {
                l3 = 4;
                c3 = 4;
            } else if (o[2] < v[2])
                c3 = 4;
            else if (v[2] < o[2])
                c3 = 2;
        }
    }
    if (v[1] >= 0) {
        sprintf(buf2, text_menu(g_game, D_089AC954[v[1] < 5 ? 11 : 12]), c1, v[0]);
        font_set_pos(g_font, x, y);
        font_draw(g_font, text_menu(g_game, D_089AC966), l1,
                      text_menu(g_game, D_089AC9BC[v[1]]), buf2);
        y += 15;
    }
    if (idx2 >= 0) {
        sprintf(buf1, text_menu(g_game, D_089AC954[idx2 < 5 ? 11 : 12]), c2, val2);
        font_set_pos(g_font, x, y);
        font_draw(g_font, text_menu(g_game, D_089AC966), l2,
                      text_menu(g_game, D_089AC9BC[idx2]), buf1);
        y += 15;
    }
    if (v[2] != 0) {
        font_set_pos(g_font, x, y);
        font_draw(g_font, text_menu(g_game, D_089AC968), l3, c3, v[2]);
    }
}

/* 1 if a skill of the hunter boosts the element (idx < 5) or the status (idx >= 5) */
int weapon_element_boosted(void *self, void *hunter, s32 idx_arg)
{
    s16 idx = idx_arg;

    if (idx < 0)
        return 0;
    if (idx < 5) {
        if (hunter_state_flag(hunter, 0x2000) == 1)
            return 1;
        return hunter_has_skill(hunter, 0x44);
    }
    if ((u8)hunter_has_skill(hunter, 0x43) == 1)
        return 1;
    return hunter_felyne_skill(hunter, 9);
}
