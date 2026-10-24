/* Equipment: icons and skill points of the armor pieces.
 * Same class as menu_box.c. (Guessed names.) */
#include "menu.h"

extern void *D_08A62C54;          /* equipment data */

u32 func_0885C83C(void *a, u8 type, u16 id);
u32 equip_rarity_color(void *self, u8 a, int b, int c);
u32 equip_icon(void *self, void *p, int a);
void gfx_draw_sprite(void *self, u16 x, u16 y, u8 r, u8 g, u8 b, u32 icon, int c);
s32 hunter_has_skill(void *pad, int key);
s32 equip_skill_points(void *a, u32 id, u8 skill);

/* icon of an armor piece (grey if flags & 8) */
void equip_draw_icon(void *self, u8 *eq, u32 x_arg, u32 y_arg, u32 flags)
{
    u16 x = x_arg;
    u16 y = y_arg;
    u8 rarity = (u8)func_0885C83C(D_08A62C54, eq[1], *(u16 *)(eq + 2));
    u32 c;

    if ((flags & 8) == 0)
        c = equip_rarity_color(self, rarity, 0xFF, 0);
    else
        c = 0xFF808080;
    gfx_draw_sprite(self, x, y, (u8)(c >> 16), (u8)(c >> 8), (u8)c, equip_icon(self, eq, 0), 0);
}

/* changes a value 0..6 with the buttons (0x1D or 0xD1 = plus, 0x1E = minus) */
int equip_adjust_level(void *self, void *pad, int v)
{
    if ((s8)hunter_has_skill(pad, 0x1D) == 1 || (s8)hunter_has_skill(pad, 0xD1) == 1) {
        if (v + 1 < 7)
            return v + 1;
        return 6;
    }
    if ((s8)hunter_has_skill(pad, 0x1E) == 1) {
        v--;
        if (v < 0)
            v = 0;
    }
    return v;
}

/* lists (up to 9) the skills with non-zero points of piece id: pairs (skill, points) */
void equip_list_skill_points(void *self, u32 id, u8 *out)
{
    int n = 0;

    if (id != 0) {
        u32 s;

        for (s = 1; (int)s < 100; s++) {
            s8 pts = (s8)equip_skill_points(D_08A62C54, id, s & 0xFF);

            if (pts != 0) {
                out[0] = s;
                out[1] = pts;
                if (n > 7)
                    return;
                n++;
                out += 2;
            }
        }
    }
    for (; n < 9; n++) {
        out[0] = 0;
        out[1] = 0;
        out += 2;
    }
}

/* compares the points of skill a[0] (a[1]) with those in list b:
 * 1 if a is greater, -1 if smaller, 0 if equal */
int equip_compare_skill(void *self, const s8 *a, const s8 *b)
{
    int i;

    for (i = 9; i > 0; i--, b += 2) {
        if ((u8)b[0] == 0)
            break;
        if ((u8)a[0] == (u8)b[0]) {
            if (b[1] < a[1])
                return 1;
            return -(a[1] < b[1]);
        }
    }
    if (a[1] > 0)
        return 1;
    return -(a[1] < 0);
}
