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

    sprintf(title, D_0893854C, text_menu(g_game, *(u16 *)0x089AC608));
    layout_build(l1, D_0893B0A4, 1, 0, 0);
    if (U32(self, MENU_TRADE) == 0)
        return;
    layout_draw_box(self, l1, (int)title, S8(self, 0x5C3), 0xFF, 0);
    layout_draw_pages(self, l1, U8(self, MENU_PAGE), U8(self, MENU_PAGES), 0, 0);
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
            s = text_menu(g_game, D_089AC834[3]);
        } else {
            col = (v & 0x8000) ? 2 : 0;
            s = text_item_name(g_game, v & 0x3FFF);
        }
        font_printf_at_color(g_font, S16(l1, 12), y, col, fmt_s, s);
        y += S16(l1, 16);
    }
    layout_build(l2, D_0893B0B8, 1, 0, 0);
    layout_draw_box(self, l2, (int)D_08938580, -1, 0xFF, 0);
    p = (u8 *)U32(self, MENU_TRADE);
    font_draw_at(g_font, S16(l2, 0x24), S16(l2, 0x26),
                  text_menu(g_game, D_089AC630[(p[7] & 1) ? 2 : 0]), U8(self, 0x5BB) + 1);
    if ((s8)combo_known(D_08A62C50, U32(self, MENU_TRADE), 1, 0) == 1) {
        p = (u8 *)U32(self, MENU_TRADE);
        c3 = c4 = c6 = c1 = 0;
        c5 = D_0893AF2C[(s8)p[4]];
        t1 = text_item_name(g_game, U16(p, 2));
        t3 = text_item_name(g_game, U16(self, 0x5AC));
        t4 = text_item_name(g_game, U16(self, 0x5AE));
        t5 = text_menu(g_game, D_089AC620[(s8)((u8 *)U32(self, MENU_TRADE))[4]]);
        t6 = menu_item_category(self, U32(self, MENU_TRADE));
    } else {
        c6 = c5 = c4 = c3 = c1 = 10;
        t1 = text_menu(g_game, D_089AC834[3]);
        if ((((u8 *)U32(self, MENU_TRADE))[7] & 4) == 0) {
            c3 = 0;
            t3 = text_item_name(g_game, U16(self, 0x5AC));
        } else {
            t3 = t1;
        }
        t4 = t5 = t6 = t1;
        if ((((u8 *)U32(self, MENU_TRADE))[7] & 8) == 0) {
            c4 = 0;
            t4 = text_item_name(g_game, U16(self, 0x5AE));
        }
    }
    font_printf_at_color(g_font, S16(l2, 12), (s16)(S16(l2, 14) + S16(l2, 16)), c1, fmt_s, t1);
    font_printf_at_color(g_font, S16(l2, 12), (s16)(S16(l2, 14) + S16(l2, 16) * 3), c3, fmt_s, t3);
    font_printf_at_color(g_font, S16(l2, 12), (s16)(S16(l2, 14) + S16(l2, 16) * 4), c4, fmt_s, t4);
    font_printf_at_color(g_font, (s16)(S16(l2, 12) + 0x46), (s16)(S16(l2, 14) + S16(l2, 16) * 5), c5, fmt_s, t5);
    font_printf_at_color(g_font, (s16)(S16(l2, 12) + 0x46), (s16)(S16(l2, 14) + S16(l2, 16) * 6), c6, fmt_s, t6);
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
        menu_draw_sprite(self, x, y, 0x1F);
        menu_draw_sprite(self, x + 5, y + 4, 0x1A5);
    } else {
        menu_draw_sprite(self, x, y, 0x20);
        menu_draw_sprite(self, x + 5, y + 4, lo < v ? 0x1A5 : 0x1A4);
    }
    return r + 1;
}

/* number at coordinates x, y */
void menu_draw_number(void *self, int x, int y, u32 v)
{
    font_printf_at(g_font, x, y, D_08938584, v);
}

/* time (in frames of 1/30 s) as minutes:seconds */
void menu_draw_time(void *self, int x, int y, s32 which_arg)
{
    s8 which = which_arg;
    int t;

    if ((*(u32 *)(g_game + 0x6AF14) & 1) && (u8)which == 1)
        t = U32(menu_quest_data(self), 0x10);
    else
        t = quest_time(g_quest, (u8)which);
    font_printf_at(g_font, x, y, D_08938588, (t / 30) / 60, (t / 30) % 60);
}

/* row of stars (n); above 8 stars (except some quests) adds a prefix */
void menu_draw_stars(void *self, int n)
{
    char buf[64];
    char *p = buf;
    u16 q;

    buf[0] = 0;
    q = U16(menu_quest_data(self), 0x18);
    if ((q < 10000 || q >= 12000) && n > 8) {
        strcat(p, text_menu(g_game, *(u16 *)0x089AC57A));
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
    font_printf(g_font, fmt_s, buf);
}
