/* In-game menu: lists of items (8 per page) and the information panel.
 * Same class as menu_box.c. (Guessed names.) */
#include "menu.h"

extern const u16 D_089AC870[];
extern const s16 D_08938554[];
extern const char D_0893855C[];        /* "%d" (quantity) */
extern const u8 D_0899D19B[];
extern const s16 D_08938560[][3];      /* position (x, y) and flags of some texts */
extern const u8 D_08930BDC[];          /* table of 0xE bytes per entry */
extern const u16 D_089AC648[], D_089AC65C[];
extern const u8 D_0893B0CC[];

int func_08828EEC(void *self);
#define menu_combo_ready func_08828EEC
int func_0885B3A0(void *a, int id, int b, int c);
int menu_combo_has_slot(void *self, u16 idx);
u32 game_count_bits62(void *game);
u32 game_bits62_before(void *game, u8 idx);
s32 game_group_count_total(void *game, u8 id);
void font_set_height2(void *font, int a);
const char *text_table17(void *game, int idx);
int monster_is_large(void *a, u8 id);
s32 game_best_record_max(void *game, u8 id, u8 *out);
s32 game_best_record_min(void *game, u8 id, u8 *out);
f32 game_size_percent(void *game, u8 a, u16 b);
u64 __extendsfdf2(f32 x);    /* float -> double (game format) */
void menu_draw_rating_icon(void *self, const u8 *entry, int x, int y, int v);
void gfx_draw_sprite_scaled(void *self, int x, int y, int w, int h, int a);
void emblem_draw(void *self, int x, int y, u8 a, u8 b);

void menu_draw_list_hint(void *self, u32 idx);

/* list of 8 items (the page of the selected entry) with quantities and colors */
void menu_draw_items_page(void *self, s16 *list, int sel, u32 color, u32 flags, int d)
{
    char title[32];
    u8 layout[0x28];
    int page = sel / 8;
    u32 idx = page * 8;
    u16 *e;
    s16 y;
    int i;

    sprintf(title, D_0893854C, text_menu(g_game, D_089AC870[flags & 1]));
    layout_build(layout, D_0893B0E0, (int)title, 0, D_08938554[d]);
    if (color == 0) {
        sel = -1;
    } else {
        sel = sel - page * 8;
        U32(layout, 0x18) = color;
    }
    layout_draw_box(self, layout, (int)title, sel, 0xFF, 0);
    if (flags & 0x10)
        layout_draw_pages(self, layout, page & 0xFFFF, 3, 0, 0);
    e = (u16 *)(list + page * 16);
    y = S16(layout, 14);
    for (i = 8; i > 0; i--, e += 2, idx++) {
        int col = 0, full = 2;

        if (flags & 0xC) {
            if ((u8)menu_combo_ready(self) == 0) {
                if ((flags & 8) == 0) {
                    if ((u8)func_0885B3A0(D_08A62C50, (s16)e[0], 0, 0) == 0) {
                        col = 10;
                        full = 13;
                    } else if ((u8)menu_combo_has_slot(self, idx & 0xFFFF) == 1) {
                        col = 5;
                    }
                } else if ((u8)menu_combo_has_slot(self, idx & 0xFFFF) == 1) {
                    col = 5;
                } else if (combo_find(D_08A62C50, S16(self, 0x5AC), (s16)e[0], 0, 0, 0) == 0) {
                    col = 10;
                    full = 13;
                }
            } else {
                s16 r = (s16)menu_combo_room(self, e);

                if (r < 0) {
                    if ((u8)menu_combo_has_slot(self, idx & 0xFFFF) == 1) {
                        col = 5;
                    } else {
                        col = 10;
                        full = 13;
                    }
                } else if (r == 1) {
                    col = 6;
                } else if (r == 0) {
                    col = 2;
                }
            }
        }
        font_set_color(g_font, col);
        font_set_pos(g_font, S16(layout, 12), y);
        if (e[0] == 0) {
            font_printf(g_font, fmt_s, text_menu(g_game, *(u16 *)0x089AC876));
        } else {
            u8 max;

            font_printf(g_font, fmt_s, text_item_name(g_game, e[0]));
            max = D_0899D19B[e[0] * 0x18];
            if (max > 1) {
                font_set_pos(g_font, (s16)(S16(layout, 12) + 0x72), y);
                if (max == 0xFF) {
                    font_printf(g_font, fmt_s, text_menu(g_game, *(u16 *)0x089AC878));
                } else {
                    if ((s16)max <= (s16)e[1])
                        font_set_color(g_font, full);
                    font_printf(g_font, D_0893855C, (s16)e[1]);
                }
            }
        }
        y += S16(layout, 16);
    }
    if (flags & 0x40)
        menu_draw_list_hint(self, 1);
}

/* help text next to the list (optional box) */
void menu_draw_list_hint(void *self, u32 idx_arg)
{
    u8 idx = idx_arg;
    const s16 *p = D_08938560[idx];

    if ((u16)p[2] & 1)
        gfx_draw_subwindow(self, p[0] - 0xC, p[1] - 8, 0x50, 0x20, 0xFF);
    menu_draw_sprite(self, p[0], p[1], 0x3A);
    font_set_size(g_font, 14, 14);
    font_printf_at_color(g_font, (s16)(p[0] + 0x16), (s16)(p[1] + 1), 0, fmt_s,
                  text_menu(g_game, *(u16 *)0x089ACA00));
}

/* panel with the information of an entry (0x5D9 = entry, <0 = none) */
void menu_draw_info_card(void *self)
{
    char title[32], b1[16], b2[16];
    Layout l;
    const u8 *ent;
    s8 n;

    sprintf(title, D_0893854C, text_menu(g_game, *(u16 *)0x089AC60A));
    layout_build(&l, D_0893B0CC, (int)title, 0, 0);
    layout_draw_box(self, &l, (int)title, -1, 0xFF, 0);
    n = S8(self, 0x5D9);
    if (n < 0) {
        font_printf_at_color(g_font, (s16)(l.x + (s16)((int)((u16)l.w - 0x70) >> 1)), (s16)(l.ty + 0x50), 0,
                      fmt_s, text_menu(g_game, *(u16 *)0x089AC638));
    } else {
        u16 cnt;
        s16 v;
        u8 kind;

        ent = D_08930BDC + n * 0xE;
        cnt = (u16)game_count_bits62(g_game);
        if (cnt > 1)
            layout_draw_pages(self, &l, ((u16)game_bits62_before(g_game, U8(self, 0x5D9)) & 0xFF) - 1, cnt, 0, 0);
        font_draw_at(g_font, l.tx, l.ty, text_menu(g_game, *(u16 *)0x089AC63A),
                      text_menu(g_game, D_089AC65C[ent[3]]));
        font_draw_at(g_font, l.tx, (s16)(l.ty + 0x10), text_menu(g_game, *(u16 *)0x089AC63C),
                      text_menu(g_game, D_089AC648[ent[0xC]]));
        v = (s16)game_group_count_total(g_game, ent[3]);
        font_draw_at(g_font, l.tx, (s16)(l.ty + 0x20), text_menu(g_game, *(u16 *)0x089AC63E), v);
        font_set_height2(g_font, 14);
        font_printf_at(g_font, l.tx, (s16)(l.ty + 0x50), fmt_s, text_table17(g_game, S8(self, 0x5D9)));
        if ((s8)monster_is_large(g_quest, ent[3]) == 1) {
            const char *s1, *s2;

            if (v == 0) {
                s1 = s2 = text_menu(g_game, *(u16 *)0x089AC644);
            } else {
                f32 f1, f2;
                s16 r;

                r = (s16)game_best_record_max(g_game, ent[3], &kind);
                f1 = game_size_percent(g_game, kind, r);
                menu_draw_rating_icon(self, ent, (s16)(l.tx + 0x4D), (s16)(l.ty + 0x30), r);
                r = (s16)game_best_record_min(g_game, ent[3], &kind);
                f2 = game_size_percent(g_game, kind, r);
                menu_draw_rating_icon(self, ent, (s16)(l.tx + 0x4D), (s16)(l.ty + 0x3E), r);
                sprintf(b1, text_menu(g_game, *(u16 *)0x089AC642), __extendsfdf2(f1));
                sprintf(b2, text_menu(g_game, *(u16 *)0x089AC642), __extendsfdf2(f2));
                s1 = b1;
                s2 = b2;
            }
            font_draw_at(g_font, l.tx, (s16)(l.ty + 0x30), text_menu(g_game, *(u16 *)0x089AC640));
            font_draw_at(g_font, (s16)(l.tx + 0x62), (s16)(l.ty + 0x30), s1);
            font_draw_at(g_font, (s16)(l.tx + 0x62), (s16)(l.ty + 0x3E), s2);
        }
        gfx_draw_sprite_scaled(self, l.x + 0x9F, l.y + 0x25, 0x58, 0x40, 0xAF);
        emblem_draw(self, l.x + 0xB3, l.y + 0x2C, ent[1], ent[2]);
    }
}
