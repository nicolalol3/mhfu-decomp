/* Equipaggiamento: icone e punti abilità dei pezzi di armatura.
 * Stessa classe di menu_box.c. (Nomi ipotetici.) */
#include "menu.h"

extern void *D_08A62C54;          /* dati dell'equipaggiamento */

u32 func_0885C83C(void *a, u8 type, u16 id);
u32 func_08830354(void *self, u8 a, int b, int c);
u32 func_0883029C(void *self, void *p, int a);
void func_0882B624(void *self, u16 x, u16 y, u8 r, u8 g, u8 b, u32 icon, int c);
s32 func_088D6A1C(void *pad, int key);
s32 func_0885CF9C(void *a, u32 id, u8 skill);

/* icona di un pezzo di equipaggiamento (grigia se flags & 8) */
void equip_draw_icon(void *self, u8 *eq, u32 x_arg, u32 y_arg, u32 flags)
{
    u16 x = x_arg;
    u16 y = y_arg;
    u8 rarity = (u8)func_0885C83C(D_08A62C54, eq[1], *(u16 *)(eq + 2));
    u32 c;

    if ((flags & 8) == 0)
        c = func_08830354(self, rarity, 0xFF, 0);
    else
        c = 0xFF808080;
    func_0882B624(self, x, y, (u8)(c >> 16), (u8)(c >> 8), (u8)c, func_0883029C(self, eq, 0), 0);
}

/* cambia un valore 0..6 con i tasti (0x1D o 0xD1 = più, 0x1E = meno) */
int equip_adjust_level(void *self, void *pad, int v)
{
    if ((s8)func_088D6A1C(pad, 0x1D) == 1 || (s8)func_088D6A1C(pad, 0xD1) == 1) {
        if (v + 1 < 7)
            return v + 1;
        return 6;
    }
    if ((s8)func_088D6A1C(pad, 0x1E) == 1) {
        v--;
        if (v < 0)
            v = 0;
    }
    return v;
}

/* elenca (fino a 9) le abilità con punti non nulli del pezzo id: coppie (abilità, punti) */
void equip_list_skill_points(void *self, u32 id, u8 *out)
{
    int n = 0;

    if (id != 0) {
        u32 s;

        for (s = 1; (int)s < 100; s++) {
            s8 pts = (s8)func_0885CF9C(D_08A62C54, id, s & 0xFF);

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

/* confronta i punti dell'abilità a[0] (a[1]) con quelli nella lista b:
 * 1 se a è maggiore, -1 se minore, 0 se uguale */
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
