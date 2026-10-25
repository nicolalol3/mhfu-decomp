/* In-game menu: window drawing (descriptions, lists, pages).
 * Same class as menu_box.c. (Guessed names.) */
#include "menu.h"


void menu_draw_page_list(void *self, s32 dim);

/* draws the main part of the menu according to the mode (0x57E/0x57F) */
void menu_draw_main(void *self)
{
    u8 layout[0x28];

    if (U8(self, MENU_MODE) == 1) {
        s8 m = U8(self, MENU_SEL);

        if (m == 2) {
            if (friend_marked(g_game) == 0) {
                font_set_color(g_font, 0);
                font_set_size(g_font, 14, 14);
                gfx_draw_frame(self, 0x10, 2, 0xE0, 0xBE, 0xFF, 0x1D5);
                gfx_draw_frame(self, 0x10, 0xC0, 0xE0, 0x4D, 0xFF, 0x1D5);
                font_printf_at(g_font, 0x22, 0x5C, text_menu(g_game, *(u16 *)0x089AD118));
            } else {
                func_08833290(self, 0x10, 2, friend_marked(g_game), 0);
            }
        } else if (m == 1) {
            menu_draw_info_card(self);
        } else if (m == 0) {
            menu_draw_trade_info(self);
        }
    } else if (U8(self, MENU_MODE) == 0) {
        menu_draw_page_list(self, 1);
        layout_build(layout, D_0893AFB4, 0, 0, 0);
        layout_draw_from(self, D_0893AFB4, 0, U8(self, MENU_SEL), 0xFF, 0);
    }
}

/* information window (pages 0..3) */
void menu_draw_info(void *self)
{
    char buf[64];
    Layout l;
    u32 page;
    u8 *p;

    sprintf(buf, D_0893854C, text_menu(g_game, D_089AC7C4[0]));
    if ((U32(self, MENU_HUD_FLAGS) & 0x40) == 0) {
        layout_draw_box(self, (void *)&D_0893B040, (int)buf, -1, 0xFF, 0);
        font_printf_at_color(g_font, 0xFC, 0x86, 0, fmt_s, text_menu(g_game, D_089AC7C4[1]));
        return;
    }
    l = D_0893B040;
    page = U8(self, MENU_SEL);
    if (GAME_FLAGS & 1)
        page = (page + 1) & 0xFF;
    if (page == 0)
        sprintf(buf, D_0893854C, text_menu(g_game, D_089AC7C4[2]));
    l.ptr = D_089AC7CC + page * 0x1A;
    layout_draw_box(self, &l, (int)buf, -1, 0xFF, 0);
    layout_draw_pages(self, &l, U8(self, MENU_SEL), U8(self, MENU_SEL_N), 0, 0);
    font_printf_at_color(g_font, l.tx, (s16)(l.ty + l.lh - 2), 0, fmt_s, quest_text(g_quest, 0));
    gfx_fill_rect(self, l.tx, l.ty + l.lh * 2 - 3, l.w - 0x18, 1, 0x80, 0x80, 0x80);
    if (page == 3) {
        font_printf_at(g_font, (s16)(l.tx + l.cw * 4), (s16)(l.ty + l.lh * 2), fmt_s,
                      quest_text(g_quest, 5));
        font_printf_at(g_font, l.tx, (s16)(l.ty + l.lh * 4), fmt_s, quest_text(g_quest, 3));
    } else if (page == 2) {
        font_set_pos(g_font, (s16)(l.tx + l.cw * 6), (s16)(l.ty + l.lh * 2));
        p = menu_quest_data(self);
        menu_draw_stars(self, (s8)p[0x1A]);
        font_printf_at(g_font, l.tx, (s16)(l.ty + l.lh * 5), fmt_s, quest_text(g_quest, 1));
        font_printf_at(g_font, l.tx, (s16)(l.ty + l.lh * 9), fmt_s, quest_text(g_quest, 2));
    } else if (page == 1) {
        int x = (s16)(l.tx + l.cw * 5);

        p = menu_quest_data(self);
        menu_draw_number(self, x, (s16)(l.ty + l.lh * 2), U32(p, 8));
        p = menu_quest_data(self);
        menu_draw_number(self, x, (s16)(l.ty + l.lh * 3), U32(p, 4));
        menu_draw_time(self, x, (s16)(l.ty + l.lh * 4), 1);
        p = menu_quest_data(self);
        font_printf_at(g_font, x, (s16)(l.ty + l.lh * 5), fmt_s,
                      text_menu(g_game, D_089ACC04[(s8)p[0x1C]]));
        font_printf_at(g_font, l.tx, (s16)(l.ty + l.lh * 7), fmt_s, quest_text(g_quest, 4));
        p = menu_quest_data(self);
        font_printf_at(g_font, l.tx, (s16)(l.ty + l.lh * 10), fmt_s,
                      text_menu(g_game, D_089ACC44[p[0x1D]]));
    } else if (page == 0) {
        hud_draw_quest_info(self);
    }
}

void menu_draw_item_box(void *self)
{
    menu_draw_status(self, (u8 *)U32(self, MENU_PLAYER), U8(self, MENU_SEL));
}

void menu_draw_equip_box(void *self)
{
    u8 buf[12];

    layout_draw_box(self, D_0893B068, (int)text_menu(g_game, *(u16 *)0x089AC860), U8(self, MENU_SEL), 0xFF, 0);
    menu_draw_equipment(self, (u8 *)U32(self, MENU_PLAYER), 1);
    equip_draw_name_box(self, (u32)menu_get_equip_slot(self, buf, U8(self, MENU_SEL), (u8 *)U32(self, MENU_PLAYER)), 8, 0x28,
                  U8(self, MENU_SEL2) | 0x100, 0, 1);
}

/* confirmation window with the name of the selected item */
void menu_draw_confirm(void *self)
{
    u8 layout[0x28];
    char buf[64];
    u32 lw[5];
    u8 *l = (u8 *)lw;
    u16 id;
    s8 m;

    if (U8(self, 0x52F) < 3)
        return;
    menu_draw_item_list(self, U8(self, MENU_LIST_SEL), D_0893AA94[U8(self, 0x52F)], 0x10, 0);
    if (U8(self, 0x592) == 0) {
        const char *s;
        int n, t, w;
        int i;

        for (i = 0; i < 20; i++)
            l[i] = D_0893B0F4[i];
        n = font_char_count(g_font, text_menu(g_game, *(u16 *)0x089AC84C));
        *(s16 *)(l + 0xC) = 0xF0 - ((n * l[8]) >> 2);
        t = (n * l[8]) >> 1;
        w = ((t + 0x20) / 16) * 16;
        *(s16 *)(l + 4) = w;
        *(s16 *)(l + 0) = 0xF0 - ((w & 0xFFFF) >> 1);
        s = text_menu(g_game, *(u16 *)0x089AC84C);
        layout_draw(self, l, s, 0xFF);
        U32((u8 *)U32(self, MENU_PLAYER), 0x614) &= ~1;
    } else {
        m = U8(self, 0x593);
        if (m < 2) {
            layout_draw(self, D_0893B108, text_item_name(g_game, U16(self, MENU_ITEM)), 0xFF);
        } else {
            const char *fmt = text_menu(g_game, *(u16 *)0x089AC84E);

            sprintf(buf, fmt, text_item_name(g_game, U16(self, MENU_ITEM)), (int)m);
            layout_draw(self, D_0893B11C, buf, 0xFF);
        }
    }
    id = U16(self, 0x58E);
    menu_draw_desc(self, 1, id, -1);
    m = U8(self, 0x52F);
    if (m == 5) {
        layout_draw_from(self, D_0893B130, 0, U8(self, MENU_YESNO), 0xFF, 0);
        return;
    }
    if (m != 3) {
        if (m != 4)
            return;
        if (U8(self, 0x5DA) == 0) {
            menu_draw_sprite(self, 0x10E, 0x50, 0x35);
            font_printf_at_color(g_font, 0x120, 0x52, 0, text_menu(g_game, *(u16 *)0x089AC9FE));
        }
    }
    layout_build(layout, D_0893B130, 0, 0, 0);
    U32(layout, 0x14) = (u32)D_089AC600;
    U32(layout, 0x18) = D_0893AAB0[U8(self, 0x52F)];
    layout_draw_box(self, layout, 0, U8(self, 0x58C), 0xFF, 0);
}

/* list of the entries of one page (6 per page); dim = draw in grey */
void menu_draw_page_list(void *self, s32 dim_arg)
{
    s8 dim = dim_arg;
    char title[32];
    u8 layout[0x28];
    char col[8];
    s16 page, y;
    const u16 *ids;
    int i;

    if (GAME_FLAGS & 1) {
        menu_draw_main_page(self, dim_arg);
        return;
    }
    page = (s16)(U8(self, 0x57D) / 6);
    sprintf(title, D_0893854C, text_menu(g_game, *(u16 *)0x089AC850));
    layout_build(layout, D_0893AF8C, (int)title, 0, 0);
    if (dim != 0)
        U32(layout, 0x18) = 0x80808080;
    layout_draw_box(self, layout, (int)title, U8(self, 0x57D) - (s16)(page * 6), 0xFF, 0);
    layout_draw_pages(self, layout, page, 2, 0, dim);
    for (i = 0; i < 6; i++)
        col[i] = 0;
    if (page == 0) {
        u8 *p = (u8 *)U32(self, MENU_PLAYER);

        if ((s8)menu_special_mode(self) == 0 || (s8)menu_input_ready(self, 0) == 0 || (s8)func_09A6E5A8(p) == 1)
            col[1] = 10;
        if (p[0x1E0] != 0)
            col[3] = 10;
        if ((GAME_FLAGS & 0x100) == 0)
            col[4] = 10;
        if ((s8)game_quest_locked(g_game, 0) == 1) {
            col[5] = 10;
            col[4] = 10;
        }
        if (OPT_FLAGS & 1)
            col[5] = 10;
    } else {
        if ((s8)game_quest_locked(g_game, 0) == 1)
            col[4] = 10;
        col[5] = 10;
    }
    ids = D_089B54E0 + (s16)(page * 6);
    y = S16(layout, 14);
    for (i = 0; i < 6; i++) {
        font_printf_at_color(g_font, S16(layout, 12), y, col[i], func_09A63198(D_09A44C5C, ids[i]));
        y += S16(layout, 16);
    }
}

/* help buttons at the bottom right */
void menu_draw_help_keys(void *self)
{
    int x = 0x168, w = 0x70;

    font_set_size(g_font, 14, 14);
    font_set_color(g_font, 0);
    font_draw_at(g_font, 0x170, 0xB1, func_09A63198(D_09A44C5C, D_089B5558[U8(self, 0x576) & 1]));
    if (OPT_FLAGS & 1) {
        x = 0x128;
        w = 0xB0;
        font_draw_at(g_font, 0x130, 0xB1, func_09A63198(D_09A44C5C, D_089B5558[2]));
    }
    gfx_draw_subwindow(self, x, 0xA8, w, 0x20, 0xFF);
}

/* description of the current entry */
void menu_draw_entry_desc(void *self)
{
    s16 id = S16(self, MENU_HELP_TEXT);

    if (U8(self, 0x5FE) == 3) {
        if (id == 8 && U8(self, 0x5E4) == 1)
            id = 0xC;
        else if (id == 10 && U8(self, 0x60A) == 1)
            id = 0xC;
        else if (id == 8 && U8(self, 0x5E4) == 2)
            id = 0xD;
        else if (id == 10 && U8(self, 0x60A) == 2)
            id = 0xD;
    }
    menu_draw_desc(self, U8(self, 0x5FE), (u16)id, S8(self, 0x5FF));
}

void menu_draw_item_stats(void *self, u32 id, s32 x, s32 y, u32 mode);

/* description box: kind = entry type, id = item/entry */
void menu_draw_desc(void *self, u32 kind_arg, u32 id_arg, int arg)
{
    u8 kind = kind_arg;
    u16 id = id_arg;
    const u8 *layout;
    const char *s;
    u32 f;

    if (kind > 7 || id == 0xFFFF)
        return;
    if (kind == 1 && id > 0x18) {
        s16 x = 0x48, y = 0x28;
        u16 it;
        const u8 *item;

        if (U32(self, MENU_HUD_FLAGS) & 0x200) {
            y = 0x88;
            x = 0x48;
        } else if (U32(self, MENU_HUD_FLAGS) & 0x800) {
            x = 0xD8;
            y = 200;
        }
        gfx_draw_window(self, x, y, 0x100, 0x40, 0xFF);
        it = id - 0x18;
        menu_draw_item_stats(self, it, x + 0x30, y + 9, arg);
        gfx_draw_item_icon_alpha(self, it, x + 10, y + 8, 0x20, 0x20, 1, 0xFF);
        item = D_0899D198 + it * 0x18;
        font_set_color(g_font, (s8)equip_rarity_text_color(self, item[2]));
        font_set_size(g_font, 12, 12);
        font_draw_at(g_font, x + 8, y + 0x2A, text_menu(g_game, *(u16 *)0x089AC864), item[2] + 1);
        font_set_color(g_font, 0);
        return;
    }
    f = U32(self, MENU_HUD_FLAGS);
    if (f & 0x200)
        layout = (f & 0x10) ? D_0893B158 : D_0893B144;
    else if (f & 0x100)
        layout = D_0893B158;
    else if (kind == 3 && S16(self, MENU_HELP_TEXT) == 8)
        layout = D_0893B1A8;
    else if (kind == 3 && id < 9)
        layout = D_0893B194;
    else if (kind == 3 && U16(self, MENU_HELP_TEXT) > 8)
        layout = D_0893B180;
    else
        layout = D_0893B16C;
    if (kind == 5 || kind == 4)
        s = func_09A7B790(D_09B22D70, D_0893AF34[kind][id]);
    else
        s = text_menu(g_game, D_0893AF34[kind][id]);
    layout_draw(self, layout, s, 0xFF);
}

/* item statistics (attack, elements, ...) in the description box */
void menu_draw_item_stats(void *self, u32 id_arg, s32 x_arg, s32 y_arg, u32 mode_arg)
{
    u16 id = id_arg;
    s16 x = x_arg;
    s16 y = y_arg;
    u8 mode = mode_arg;
    const u8 *item = D_0899D198 + id * 0x18;
    u8 kind;

    font_set_size(g_font, 14, 14);
    if (*(s16 *)(item + 0x16) == 0 || (s8)mode < 0) {
        kind = 0;
    } else {
        kind = mode & 3;
        font_set_color(g_font, 0);
        font_set_pos(g_font, (s16)(x + 0x8C), (s16)(y + 0x1C));
        font_draw(g_font, fmt_s,
                      text_menu(g_game, (mode & 3) == 0 ? *(u16 *)0x089AC880 : *(u16 *)0x089AC882));
    }
    font_set_color(g_font, 0);
    if (kind == 1) {
        u16 n = *(u16 *)(item + 0x16);
        const u8 *e = D_089779DC + n * 0x10;
        int i;

        for (i = 0; i < 4; i++, e += 2) {
            font_set_pos(g_font, (s16)(x + D_0893AF54[i][0]), (s16)(y + D_0893AF54[i][1]));
            if (e[0] == 0) {
                if (i == 0)
                    font_printf(g_font, fmt_s, text_menu(g_game, *(u16 *)0x089AC9D2));
                break;
            }
            font_set_size(g_font, 12, 14);
            font_printf(g_font, fmt_s, text_menu(g_game, D_089ACC70[e[0]]));
            if (e[0] != 1) {
                font_set_size(g_font, 14, 14);
                font_set_pos(g_font, (s16)(x + D_0893AF54[i][0] + 0x3C), (s16)(y + D_0893AF54[i][1]));
                font_printf(g_font, text_menu(g_game, *(u16 *)0x089AC87E), (s8)e[1]);
            }
        }
        font_printf_at_color(g_font, x_arg, (s16)(y + 0x1C), 5, text_menu(g_game, *(u16 *)0x089AC87C),
                      D_089779D4[n * 0x10]);
    } else if (kind == 0) {
        font_draw_at(g_font, x_arg, y_arg, fmt_s, text_item_info(g_game, id_arg)); /* values received, not reduced */
    }
}

/* item list (24 slots): flag 0x20 = compact and sort the list */
void menu_draw_item_list(void *self, int sel, u32 color, u32 flags, int d)
{
    s16 list[48];
    s16 *src;
    int i, n, used;

    if ((flags & 2) == 0)
        src = (s16 *)pouch_slot(g_game, 0);
    else
        src = (s16 *)(g_game + 0x43C8);
    if (flags & 0x20) {
        s16 *dst = list;

        n = 0;
        for (i = 0x18; i != 0; i--, src += 2) {
            if (src[0] != 0) {
                n++;
                dst[0] = src[0];
                dst[1] = src[1];
                dst += 2;
            }
        }
        used = n;
        for (n = 0x18 - n; n != 0; n--, dst += 2) {
            dst[1] = 0;
            dst[0] = 0;
        }
        item_pairs_sort(list, used);   /* sorts the used entries */
        src = list;
        color = 0;
        flags = (flags & 1) ? 0x21 : 0x20;
        sel = sel % 8;
    }
    menu_draw_items_page(self, src, sel, color, flags, d);
}
