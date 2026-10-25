/* In-game menu: hunter status screen (stats, equipment, resistances, skills).
 * Same class as menu_box.c. (Guessed names.) */
#include "menu.h"
#include "vfpu.h"

extern const u8 D_0893B018[], D_0893B02C[];
extern s16 D_0893B038, D_0893B03A; extern s32 D_0893B03C;
extern const u16 D_089AC8A0[], D_089AC8AC[], D_089AC8C4[], D_089ACD38[], D_089ACEEC[];
extern u8 D_089AC888[];
extern const char D_08938594[], D_089385A8[], D_089385B0[];
extern const s32 D_0893ABBC[];
extern const s16 D_08938598[][2];
extern s16 D_08939704, D_08939706;
extern const s32 D_0893AEB8[];    /* armor pieces in display order */
extern const u8 D_0893AEB1[];
extern const u8 D_089385A0[];
extern void *D_08A62C54;

s32 menu_list_pages(void *self, int a);
void menu_draw_weapon_type(void *self, int x, int y, const u8 *eq);
void menu_quest_slot_name(void *self, int a, char *buf);
void func_088912BC(void *font, int x, int y, const void *name);
void game_hr_points(void *game, u8 *a, s32 *b, s32 *c);
u32 equip_rarity(void *a, u8 b, u16 c);
u32 func_088DADD8(void *p);
u32 skill_armor_has_slot(void *self, void *p);
void equip_draw_icon_rarity(void *self, u8 a, int b, s16 c, int d, s16 e, s16 f, int g, int h);
const char *equip_name(void *a, u8 b, u16 c);
s32 func_08827BF4(void *self, void *p, u8 idx);
void gfx_draw_sprite_mode(void *self, u16 x, u16 y, int w, int h, int r, int g, int b, int a, int c, int d);
s32 skill_applies_to_weapon(void *self, void *p, u32 id, const u8 *list);
void gfx_draw_sprite_scaled_color(void *self, u16 x, u16 y, int w, int h, u8 r, u8 g, u8 b, int c);
u32 equip_icon(void *self, void *p, int a);
void func_0889183C(void *font, int x, int y, int v, const char *fmt, ...);
void func_0882D8DC(void *self, u16 x, u16 y, int w, int h, int a, int r, int g, int b);
#define gfx_fill_rect_rgba func_0882D8DC

void menu_draw_skills(void *self, u8 *hunter, int x, int y);
void menu_draw_unique_list(void *self, const u8 *list, void *p, int x, int y, s32 step);
void menu_draw_resist_table(void *self, int x, int y, u32 page, s32 bar, int rows, u32 sel);

/* page of the hunter status screen (page: 0 = stats, 1 = equipment, ...) */
void menu_draw_status(void *self, u8 *hunter, s32 page_arg)
{
    s8 page = page_arg;
    char buf[64];
    Layout l;
    s8 n = (s8)menu_list_pages(self, 9);
    int y2, x, i;
    s16 y;

    sprintf(buf, D_0893854C, text_menu(D_089CC438, *(u16 *)0x089AC884));
    layout_build(&l, D_0893B018, (int)buf, 0, 0);
    if (page == 0) {
        l.h = 0xE0;
        l.e = 0xB;
        l.ptr = D_089AC888;
    }
    func_0882F580(self, &l, (int)buf, -1, 0xFF, 0);
    layout_draw_pages(self, &l, (u8)page, (u8)(n + 2), 0, 0);
    layout_draw(self, D_0893B02C, text_menu(D_089CC438, D_089AC8C4[page != 0]), 0xFF);
    y2 = (s16)(D_0893B03A + D_0893B03C);
    if (page == 1) {
        menu_draw_equipment(self, hunter, 0);
        menu_draw_skills(self, hunter, D_0893B038, y2);
        return;
    }
    if (page != 0) {
        u32 v = skill_armor_has_slot(self, hunter);

        menu_draw_resist_table(self, l.tx, l.ty, page - 2, v, 9, 0xFF);
        menu_draw_skills(self, hunter, D_0893B038, y2);
        return;
    }

    /* page 0: labels */
    func_08890E38(D_08A631A8, 5);
    func_08890DE8(D_08A631A8, 14, 14);
    y = l.ty;
    for (i = 7; i != 0; i--) {
        func_08891474(D_08A631A8, (s16)(l.tx + 0x38), y, text_menu(D_089CC438, *(u16 *)0x089AC8AA));
        y += l.lh;
    }
    for (i = 4; i != 0; i--) {
        func_08891474(D_08A631A8, (s16)(l.tx + 0x3C), y, text_menu(D_089CC438, *(u16 *)0x089AC8AA));
        y += l.lh;
    }
    y = l.ty + l.lh * 7;
    for (i = 0; i < 5; i++) {
        func_08891474(D_08A631A8, (s16)(l.tx + 0x7C), y, text_menu(D_089CC438, D_089AC8A0[i]));
        func_08891474(D_08A631A8, (s16)(l.tx + 0xC2), y, text_menu(D_089CC438, *(u16 *)0x089AC8AA));
        y += l.lh;
    }
    func_08890E38(D_08A631A8, 0);

    /* values */
    x = (s16)(l.tx + 0x46);
    menu_draw_weapon_type(self, x, (s16)(l.ty + l.lh), hunter + 0x4E8);
    func_08890DFC(D_08A631A8, x, (s16)(l.ty + l.lh * 6));
    if ((*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) == 0) {
        func_0889134C(D_08A631A8, D_08938478, text_menu(D_089CC438, *(u16 *)0x089AD048));
    } else {
        menu_quest_slot_name(self, S8(self, 0x536), buf);
        func_0889134C(D_08A631A8, D_08938478, buf);
    }
    func_08890DE8(D_08A631A8, 16, 14);
    func_088912BC(D_08A631A8, x, l.ty, D_089CC438 + 0x4A0);
    {
        u8 *g = D_089CC438;
        u8 rank;
        s32 a, b;
        u8 w;
        u16 base;

        func_08891474(D_08A631A8, x, (s16)(l.ty + l.lh * 2), text_menu(g, *(u16 *)0x089AC8CA),
                      U32(g, 0x696F0));
        game_hr_points(D_089CC438, &rank, &a, &b);
        func_08891474(D_08A631A8, x, (s16)(l.ty + l.lh * 3), D_08938548, rank);
        func_08890DFC(D_08A631A8, x, (s16)(l.ty + l.lh * 4));
        if (b - a < 0)
            func_0889134C(D_08A631A8, D_08938548, a);
        else
            func_0889134C(D_08A631A8, text_menu(D_089CC438, *(u16 *)0x089AC8C8), a, b - a);
        g = D_089CC438;
        func_08891474(D_08A631A8, x, (s16)(l.ty + l.lh * 5), text_menu(g, *(u16 *)0x089AC8CC),
                      U32(g, 0x696E8));

        x = (s16)(l.tx + 0x4A);
        func_08891474(D_08A631A8, x, (s16)(l.ty + l.lh * 7), D_08938594, (int)S16(hunter, 0x41E));
        func_08891474(D_08A631A8, x, (s16)(l.ty + l.lh * 8), D_08938594, (int)S16(hunter, 0x62E) / 3);
        w = (u8)equip_rarity(D_08A62C54, U8(hunter, 0x4E9), U16(hunter, 0x4EA));
        base = (u16)func_088DADD8(hunter);
        func_08891474(D_08A631A8, x, (s16)(l.ty + l.lh * 9), D_08938594,
                      (int)(base * D_0893ABBC[w]) / 100 & 0xFFFF);
        func_08891474(D_08A631A8, x, (s16)(l.ty + l.lh * 10), D_08938594, U16(hunter, 0x422));
    }
    x = (s16)(l.tx + 0xC9);
    func_08891474(D_08A631A8, x, (s16)(l.ty + l.lh * 7), D_08938594, (int)(s16)(int)F32(hunter, 0x39C));
    func_08891474(D_08A631A8, x, (s16)(l.ty + l.lh * 8), D_08938594, (int)(s16)(int)F32(hunter, 0x3A0));
    func_08891474(D_08A631A8, x, (s16)(l.ty + l.lh * 9), D_08938594, (int)(s16)(int)F32(hunter, 0x3A4));
    func_08891474(D_08A631A8, x, (s16)(l.ty + l.lh * 10), D_08938594, (int)(s16)(int)F32(hunter, 0x3AC));
    func_08891474(D_08A631A8, x, (s16)(l.ty + l.lh * 11), D_08938594, (int)(s16)(int)F32(hunter, 0x3A8));
    func_08890DE8(D_08A631A8, 14, 14);
    menu_draw_unique_list(self, D_089CC438 + 0x69942, hunter + 0x4E8, D_0893B038, y2, D_0893B03C);
}

/* name of the equipped weapon (type) */
void menu_draw_weapon_type(void *self, int x, int y, const u8 *eq)
{
    u8 t = (u8)equip_rarity(D_08A62C54, eq[1], *(u16 *)(eq + 2));

    func_08891474(D_08A631A8, x, y, D_08938478, text_menu(D_089CC438, D_089AC8AC[t]));
}

/* equipment list (weapon + 5 armor pieces) with icons */
void menu_draw_equipment(void *self, u8 *hunter, int col)
{
    s16 ix;
    int y, i;

    equip_draw_icon_rarity(self, U8(hunter, 0x4E9), S16(hunter, 0x4EA), D_08938598[col][0], 0x44, D_08939704, D_08939706,
                  0xFF, 0);
    func_088920A8(D_08A631A8, D_08938598[col][1], 0x4A, text_menu(D_089CC438, *(u16 *)0x089AC8E0),
                  equip_name(D_08A62C54, U8(hunter, 0x4E9), U16(hunter, 0x4EA)));
    ix = 0x5E;
    y = 100;
    for (i = 0; i < 5; i++) {
        u8 slot = (u8)D_0893AEB8[i];
        u8 *e = hunter + slot * 0xC;

        equip_draw_icon_rarity(self, slot, S16(e, 0x4F6), D_08938598[col][0], ix, D_08939704, D_08939706, 0xFF, 0);
        func_088920A8(D_08A631A8, D_08938598[col][1], y, text_menu(D_089CC438, *(u16 *)0x089AC8E2),
                      equip_name(D_08A62C54, U8(e, 0x4F5), U16(e, 0x4F6)));
        ix += 0x1A;
        y = (s16)(y + 0x1A);
    }
}

void menu_draw_skills_ex(void *self, u8 *hunter, int x, int y, const s8 *colors);

/* active skills (normal colors) */
void menu_draw_skills(void *self, u8 *hunter, int x, int y)
{
    s8 colors[6] = { 0, 0, 0, 0, 0, 0 };

    menu_draw_skills_ex(self, hunter, x, y, colors);
}

/* active skills (up to 6) with the given colors; "none" if there are none */
void menu_draw_skills_ex(void *self, u8 *hunter, int x, int y, const s8 *colors)
{
    u32 i;

    func_08890DE8(D_08A631A8, 14, 13);
    if (U8(hunter, 0x67A) == 0 && U8(hunter, 0x679) == 0 && U8(hunter, 0x678) == 0 && U8(hunter, 0x676) == 0 &&
        U8(hunter, 0x677) == 0) {
        func_088915B0(D_08A631A8, x, y, 0, D_08938478, text_menu(D_089CC438, D_089ACD38[0]));
        return;
    }
    for (i = 0; (int)i <= 5; i++) {
        u8 k = i & 0xFF;
        u8 skill = U8(hunter, k + 0x676);
        s8 c;

        if (skill == 0)
            continue;
        if ((u8)(s8)func_08827BF4(self, hunter, k) == 1)
            c = colors[i];
        else
            c = 9;
        func_088915B0(D_08A631A8, x, y, c, D_08938478, text_menu(D_089CC438, D_089ACD38[skill]));
        if (colors[i] == 0xE || colors[i] == 2)
            gfx_draw_sprite_mode(self, (s16)x - 8, (s16)y + 1, 6, 0xC, 0xD0, 0xD0, 0xD0, 0xC0, 0x8E, 0);
        y = (s16)(y + 0xE);
    }
}

/* list without duplicates of the (up to 5) entries of list; grey if not available */
void menu_draw_unique_list(void *self, const u8 *list, void *p, int x, int y, s32 step_arg)
{
    s16 step = step_arg;
    u8 seen[8];
    u32 n = 0;
    int i;

    for (i = 0; i < 5; i++) {
        u32 v = list[i];
        int j, fresh = 1;

        if (v == 0)
            continue;
        for (j = 0; j < (int)n; j++) {
            if (v == seen[j]) {
                fresh = 0;
                break;
            }
        }
        if (!fresh)
            continue;
        seen[n] = list[i];
        n = (n + 1) & 0xFF;
        func_088915B0(D_08A631A8, x, y, (u8)(s8)skill_applies_to_weapon(self, p, v, list) == 1 ? 0 : 10, D_08938478,
                      text_menu(D_089CC438, D_089ACEEC[v]));
        y = (s16)(y + step);
    }
    if (n == 0)
        func_08891474(D_08A631A8, x, y, D_08938478, text_menu(D_089CC438, D_089ACEEC[0]));
}

/* table of resistances/skill points (rows of 0x12 bytes at self+0x684) */
void menu_draw_resist_table(void *self, int x, int y, u32 page_arg, s32 bar_arg, int rows, u32 sel_arg)
{
    u8 page = page_arg;
    s8 bar = bar_arg;
    u8 sel = sel_arg;
    s16 ix, sy = (s16)y;
    s16 dim = 0, normal = 0, ry;
    u8 *e;
    int i, j;

    func_08890DE8(D_08A631A8, 12, 14);
    func_08890E38(D_08A631A8, 5);
    func_08891474(D_08A631A8, x, y, D_08938478, text_menu(D_089CC438, *(u16 *)0x089AC8D8));
    ix = (s16)x + 0x45;
    if (sel < 7) {
        f32 s = vfpu_sinf((f32)(int)(((*(u32 *)(D_08A631A0 + 0xF7A554) & 0x1F) << 11) & 0xFFFF) *
                          9.58738e-05f);
        u8 c = (s8)(int)(s * 31.0f) + 223;

        gfx_draw_sprite_scaled_color(self, (s16)(ix - 3 + D_089385A0[sel] * 24), (s16)(sy - 2) + 1, 23, 17, c, c,
                      (s8)(int)(s * 32.0f) + 32, 0x1A6);
    }
    menu_draw_sprite(self, ix, sy - 2, equip_icon(self, (u8 *)self + 0xE1C, 1));
    for (i = 0; i < 5; i++) {
        ix += 0x18;
        menu_draw_sprite(self, ix, sy - 2, D_0893AEB1[D_0893AEB8[i]]);
    }
    ry = sy + 0x10;
    e = (u8 *)self + page * rows * 0x12 + 0x684;
    {
        s16 cy = ry;

        for (i = 0; i < rows; i++, e += 0x12, cy += 0x10) {
            if (e[2] == 0)
                break;
            if ((e[2] & 2) == 0) {
                int cx;

                normal++;
                func_08890DE8(D_08A631A8, 12, 14);
                func_088915B0(D_08A631A8, x, cy, (e[2] & 1) ? 3 : 0, D_08938478,
                              text_menu(D_089CC438, D_089ACC70[e[3]]));
                func_08890DE8(D_08A631A8, 14, 14);
                cx = (s16)((s16)x + 0x42);
                for (j = 0; j < 6; j++) {
                    if (e[j + 4] != 0)
                        func_0889183C(D_08A631A8, cx, cy, (s8)e[j + 0xB], D_089385A8, (s8)e[j + 4]);
                    cx = (s16)(cx + 0x18);
                }
                func_0889183C(D_08A631A8, cx, cy, (s8)e[0x11], D_089385B0, (s8)e[10]);
            } else {
                s16 cx;

                dim++;
                func_08890E38(D_08A631A8, -1);
                func_08890DE8(D_08A631A8, 12, 14);
                func_088920A8(D_08A631A8, x, cy, D_08938478, text_menu(D_089CC438, D_089ACC70[e[3]]));
                func_08890DE8(D_08A631A8, 14, 14);
                cx = (s16)x + 0x42;
                for (j = 0; j < 6; j++) {
                    if (e[j + 4] != 0)
                        func_08891474(D_08A631A8, cx, cy, D_08938478,
                                      text_menu(D_089CC438, *(u16 *)0x089AC8DC));
                    cx += 0x18;
                }
            }
        }
    }
    if ((int)(u16)normal > 0) {
        int total = (u16)normal + dim;

        for (; total > 0; total--) {
            s16 cx = (s16)x + 0x40;

            for (j = 7; j > 0; j--) {
                gfx_fill_rect(self, cx, ry, 1, 0xB, 0x50, 0x50, 0x50);
                cx += 0x18;
            }
            ry += 0x10;
        }
        if (bar == 1)
            gfx_fill_rect_rgba(self, (s16)x + 0x71, sy + 0xE, 0x17, ((u16)normal & 0xFFF) << 4, 0, 0x3E, 0x74, 0xB2);
    }
}
