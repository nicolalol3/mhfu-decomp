/* In-game menu: detail screens (trade, star rating, time).
 * Same class as menu_box.c. (Guessed names.) */
#include "menu.h"

extern const u8 D_0893B0A4[], D_0893B0B8[];
extern const char D_08938580[], D_08938584[];
extern const char D_08938588[];          /* "%02d:%02d" */
extern const u16 D_089AC620[], D_089AC630[];
s32 combo_known(void *a, u32 b, int c, int d);
char *strcat(char *d, const char *s);

/* details of the selected trade (0x5A8): requested items and reward */
void menu_draw_trade_info(void *self)
{
    char title[32];
    u8 l1[0x28], l2[0x28];
    const char *t1, *t3, *t4, *t5, *t6;
    int c1, c3, c4, c5, c6;
    u8 *p;
    u16 *e;
    s16 y;
    int i;

    sprintf(title, D_0893854C, func_0885151C(D_089CC438, *(u16 *)0x089AC608));
    func_0882FA24(l1, D_0893B0A4, 1, 0, 0);
    if (U32(self, 0x5A8) == 0)
        return;
    func_0882F580(self, l1, (int)title, S8(self, 0x5C3), 0xFF, 0);
    func_0882FB38(self, l1, U8(self, 0x5C4), U8(self, 0x5C5), 0, 0);
    e = (u16 *)((u8 *)self + 0x5C8);
    y = S16(l1, 14);
    for (i = 0; i < 8; i++, e++) {
        u16 v = *e;
        const char *s;
        int col;

        if (v == 0)
            break;
        if (v & 0x4000) {
            col = 10;
            s = func_0885151C(D_089CC438, D_089AC834[3]);
        } else {
            col = (v & 0x8000) ? 2 : 0;
            s = func_08851528(D_089CC438, v & 0x3FFF);
        }
        func_088915B0(D_08A631A8, S16(l1, 12), y, col, D_08938478, s);
        y += S16(l1, 16);
    }
    func_0882FA24(l2, D_0893B0B8, 1, 0, 0);
    func_0882F580(self, l2, (int)D_08938580, -1, 0xFF, 0);
    p = (u8 *)U32(self, 0x5A8);
    func_088920A8(D_08A631A8, S16(l2, 0x24), S16(l2, 0x26),
                  func_0885151C(D_089CC438, D_089AC630[(p[7] & 1) ? 2 : 0]), U8(self, 0x5BB) + 1);
    if ((s8)combo_known(D_08A62C50, U32(self, 0x5A8), 1, 0) == 1) {
        p = (u8 *)U32(self, 0x5A8);
        c3 = c4 = c6 = c1 = 0;
        c5 = D_0893AF2C[(s8)p[4]];
        t1 = func_08851528(D_089CC438, U16(p, 2));
        t3 = func_08851528(D_089CC438, U16(self, 0x5AC));
        t4 = func_08851528(D_089CC438, U16(self, 0x5AE));
        t5 = func_0885151C(D_089CC438, D_089AC620[(s8)((u8 *)U32(self, 0x5A8))[4]]);
        t6 = func_08825104(self, U32(self, 0x5A8));
    } else {
        c6 = c5 = c4 = c3 = c1 = 10;
        t1 = func_0885151C(D_089CC438, D_089AC834[3]);
        if ((((u8 *)U32(self, 0x5A8))[7] & 4) == 0) {
            c3 = 0;
            t3 = func_08851528(D_089CC438, U16(self, 0x5AC));
        } else {
            t3 = t1;
        }
        t4 = t5 = t6 = t1;
        if ((((u8 *)U32(self, 0x5A8))[7] & 8) == 0) {
            c4 = 0;
            t4 = func_08851528(D_089CC438, U16(self, 0x5AE));
        }
    }
    func_088915B0(D_08A631A8, S16(l2, 12), (s16)(S16(l2, 14) + S16(l2, 16)), c1, D_08938478, t1);
    func_088915B0(D_08A631A8, S16(l2, 12), (s16)(S16(l2, 14) + S16(l2, 16) * 3), c3, D_08938478, t3);
    func_088915B0(D_08A631A8, S16(l2, 12), (s16)(S16(l2, 14) + S16(l2, 16) * 4), c4, D_08938478, t4);
    func_088915B0(D_08A631A8, (s16)(S16(l2, 12) + 0x46), (s16)(S16(l2, 14) + S16(l2, 16) * 5), c5, D_08938478, t5);
    func_088915B0(D_08A631A8, (s16)(S16(l2, 12) + 0x46), (s16)(S16(l2, 14) + S16(l2, 16) * 6), c6, D_08938478, t6);
}

/* rating icon next to a value (v) compared with the limits of the entry;
 * returns 0 (none), 1 or 2 */
s16 menu_draw_rating_icon(void *self, const u8 *ent, s32 x_arg, s32 y_arg, s32 v_arg)
{
    s16 x = x_arg;
    s16 y = y_arg;
    s16 v = v_arg;
    s16 lo = *(s16 *)(ent + 8);
    s16 r;

    if (v == 0 || (lo < v && v < *(s16 *)(ent + 4)))
        return 0;
    if (lo < v)
        r = !(v < *(s16 *)(ent + 6));
    else
        r = 1;
    if (r == 0) {
        func_0882A9DC(self, x, y, 0x1F);
        func_0882A9DC(self, x + 5, y + 4, 0x1A5);
    } else {
        func_0882A9DC(self, x, y, 0x20);
        func_0882A9DC(self, x + 5, y + 4, lo < v ? 0x1A5 : 0x1A4);
    }
    return r + 1;
}

/* number at coordinates x, y */
void menu_draw_number(void *self, int x, int y, u32 v)
{
    func_08891474(D_08A631A8, x, y, D_08938584, v);
}

/* time (in frames of 1/30 s) as minutes:seconds */
void menu_draw_time(void *self, int x, int y, s32 which_arg)
{
    s8 which = which_arg;
    int t;

    if ((*(u32 *)(D_089CC438 + 0x6AF14) & 1) && (u8)which == 1)
        t = U32(func_088298E4(self), 0x10);
    else
        t = func_0886A35C(D_08A62C7C, (u8)which);
    func_08891474(D_08A631A8, x, y, D_08938588, (t / 30) / 60, (t / 30) % 60);
}

/* row of stars (n); above 8 stars (except some quests) adds a prefix */
void menu_draw_stars(void *self, int n)
{
    char buf[64];
    char *p = buf;
    u16 q;

    buf[0] = 0;
    q = U16(func_088298E4(self), 0x18);
    if ((q < 10000 || q >= 12000) && n > 8) {
        strcat(p, func_0885151C(D_089CC438, *(u16 *)0x089AC57A));
        p = buf + 1;    /* (as in the original) */
        n -= 8;
    }
    for (; n > 0; n--) {
        p[0] = 0xE2;    /* "★" in UTF-8 */
        p[1] = 0x98;
        p[2] = 0x85;
        p += 3;
    }
    *p = 0;
    func_0889134C(D_08A631A8, D_08938478, buf);
}
