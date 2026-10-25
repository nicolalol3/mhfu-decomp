/* In-game menu (C++ class of 0x1220 bytes, global instance in g_menu):
 * creation/destruction and drawing of some windows (items, quantities, prices).
 * The structure is not yet rebuilt: fields are given by offset. (Names are guesses.) */
#include "menu.h"


/* costruttore */
void *menu_ctor(void *self)
{
    g_menu = self;
    func_0890BEC0((u8 *)self + 0xE28, (void *)0x08816B64, (void *)0x08816B24, 0x24, 0x14);
    memset(self, 0, 0x1220);
    U32(self, MENU_TRADE) = 0;
    memset((u8 *)self + 0xE28, 0, 0x2D0);
    U32(self, 0x1198) = 0;
    U16(self, 0x119E) = 0;
    U16(self, 0x119C) = 0;
    U32(self, MENU_HUD_FLAGS) = 0;
    U8(self, 0x608) = 0;
    return self;
}

/* destructors of the elements (free the memory if requested) */
void *menu_item_dtor(void *self, s32 del_arg)
{
    s16 del = del_arg;
    if (self != NULL && del > 0)
        op_delete(self);
    return self;
}

void *func_08816B24(void *self, s32 del_arg)
{
    s16 del = del_arg;
    if (self != NULL && del > 0)
        op_delete(self);
    return self;
}

/* destructor of the menu */
void *menu_dtor(void *self, s32 del_arg)
{
    s16 del = del_arg;
    if (self != NULL) {
        menu_free_data(self);
        func_0890BE54((u8 *)self + 0xE28, (void *)0x08816B24, 0x24, 0x14);
        g_menu = NULL;
        if (del > 0)
            op_delete(self);
    }
    return self;
}

/* opening the menu: resets the state and prepares the data */
void menu_open(void *self)
{
    u8 *g;
    u32 f;

    U32(self, MENU_PLAYER) = player_get(g_players, G_MY_SLOT);
    U8(self, 0x52F) = 0;
    U8(self, 0x52E) = 0;
    U8(self, 0x52D) = 0;
    U8(self, 0x52C) = 0;
    U32(self, MENU_HUD_FLAGS) &= ~0x2400;
    U8(self, 0x57C) = 0;
    g_game[0x10] = 0;
    U8(self, 0x554) = 0;
    hud_notice_clear(self);
    U8(self, 0x604) = 0;
    U32(self, 0x600) = 0;
    U32(self, MENU_NOTICE_FLAGS) &= ~0x82;
    U16(self, 0x5FA) = 0;
    U8(self, 0x607) = 0;
    f = U32(self, MENU_NOTICE_FLAGS);
    U32(self, MENU_NOTICE_FLAGS) = (f & ~8) | 0x10;
    U8(self, 0x57D) = 0;
    U32(self, MENU_NOTICE_FLAGS) &= ~4;
    U32(self, MENU_HUD_FLAGS) &= ~4;
    U16(self, MENU_ITEM) = 0;
    U32(self, MENU_HUD_FLAGS) &= ~8;
    U32(self, 0x5EC) = 0;
    U32(self, 0x5F4) = 0xFFFFFFFF;
    U8(self, 0x5F0) = 0;
    g = g_game;
    if ((U32(g, 0x6AF14) & 1) == 0) {
        u8 i = U8(g, 0x6AF10);
        u32 v = D_08938A7C[i];
        u8 *p;

        U32(self, 0x568) = v;
        U32(self, 0x564) = v;
        F32(self, 0x564) = F32(self, 0x564) * (1.0f / D_0893887C[i][0]);
        F32(self, 0x568) = F32(self, 0x568) * (1.0f / D_0893887C[i][1]);
        U8(self, 0x576) = 0;
        U8(self, 0x577) = 0;
        U16(self, MENU_BEEP_TIMERS) = 0xFFFF;
        U16(self, 0x53C) = 0xFFFF;
        U16(self, 0x548) = 0xFFFF;
        U16(self, 0x56E) = 0xFFFF;
        U16(self, 0x53E) = 0xFFFF;
        U16(self, 0x54A) = 0xFFFF;
        U16(self, 0x570) = 0xFFFF;
        U16(self, 0x540) = 0xFFFF;
        U16(self, 0x54C) = 0xFFFF;
        U16(self, 0x572) = 0xFFFF;
        U16(self, 0x542) = 0xFFFF;
        U16(self, 0x54E) = 0xFFFF;
        U8(self, 0x579) = 4;
        U8(self, 0x57A) = 0;
        U32(self, MENU_HUD_FLAGS) &= ~1;
        p = (u8 *)U32(self, MENU_PLAYER);
        U16(self, 0x596) = U16(p, 0x648);
        U16(self, 0x59C) = U8(p, 0x686);
        U8(self, 0x5DC) = 0;
        U8(self, 0x5DD) = 0;
        U8(self, 0x5DF) = U8(g_game, 0x6ADDF);
        U8(self, 0x5E1) = U8(g_game, 0x6ADE0);
        U8(self, MENU_OPTIONS) = U8(g_game, 0x6ADE1);
        U8(self, 0x5E0) = U8(g_game, 0x6ADE2);
        U8(self, 0x5E2) = U8(g_game, 0x6ADE4);
        U8(self, 0x5E3) = U8(g_game, 0x6ADE5);
        U32(self, MENU_TIME_LEFT) = quest_time(g_quest, 0);
        U32(self, MENU_TIME_LIMIT) = quest_time(g_quest, 1);
        menu_get_mode(self);
        U32(self, MENU_HUD_FLAGS) &= ~0x20;
        U8(self, 0x11AE) = 0;
    } else {
        U8(self, 0x608) = U8(g, 0x6ADE3);
        U8(g, 0x422) = 0;
    }
    menu_reset_messages(self);
    U8(self, 0x112C) = 0;
    U8(self, 0x1138) = 0;
    U8(self, 0x1144) = 0;
    U8(self, 0x1150) = 0;
    U8(self, 0x115C) = 0;
    U8(self, 0x1168) = 0;
    U32(self, 0x538) = 0;
    U8(self, 0x11AC) = 0;
    U8(self, 0x11AB) = 0;
    U32(self, 0x1194) = 0;
}

/* text "%s" with the string id at the coordinates of the box */
static void draw_title_text(void *self)
{
    font_set_size(g_font, 14, 14);
    font_draw_at(g_font, (s16)(D_0893B164 + 98), (s16)(D_0893B166 + D_0893B168 * 2), fmt_s,
                  text_menu(g_game, D_089AC588[U8(self, MENU_YESNO)]));
}

/* confirm/choice window */
void menu_draw_choice(void *self)
{
    u8 layout[0x28], layout2[0x24];
    u8 sel = U8(self, MENU_LIST_SEL);
    u32 flags = 0, color = 0;
    int v;

    layout_build(layout, D_0893AFDC, 0, 0, 0);
    if ((U32((u8 *)U32(self, MENU_PLAYER), 0x614) & 4) == 0) {
        v = 4;
    } else {
        v = (u8)menu_item_giftable(self) != 1;
        if ((u8)menu_item_for_quest(self, *pouch_slot(g_game, U8(self, MENU_LIST_SEL))) != 1)
            v += 2;
    }
    U32(layout, 20) = (u32)(D_089AC5D8 + v * 8);
    switch (U8(self, 0x585)) {
    case 3:
    case 2:
        color = 0xCC808080;
        switch (U8(self, 0x588)) {
        case 3:
            U32(layout, 24) = 0x80808080;
            layout_draw_box(self, layout, 0, 3, 0xFF, 0);
            sel = U8(self, 0x595);
            color = 0x8000C040;
            flags = 0x90;
            break;
        case 2:
            if (U8(self, 0x589) == 0)
                gfx_draw_count_box(self, 0xF0, 0x68, U8(self, MENU_COUNT), U8(self, MENU_COUNT) + 1 - U8(self, 0x58B), 0);
            else if (U8(self, 0x589) == 1)
                layout_draw_from(self, D_0893B004, 0, U8(self, MENU_YESNO), 0xFF, 0);
            break;
        case 1:
            menu_draw_give(self, 0);
            break;
        case 0:
            U32(layout, 24) = 0x80808080;
            layout_draw_box(self, layout, 0, 0, 0xFF, 0);
            layout_draw_from(self, D_0893AFF0, 0, U8(self, MENU_YESNO), 0xFF, 0);
            break;
        }
        break;
    case 1:
        layout_draw_box(self, layout, 0, U8(self, 0x588), 0xFF, 0);
        color = 0x80808080;
        break;
    case 0:
        if (U32(self, MENU_HUD_FLAGS) & 0x10) {
            flags = 0x20;
            color = 0;
            draw_title_text(self);
        } else {
            flags = (U32(self, MENU_HUD_FLAGS) & 0x20) ? 0x10 : 0x50;
            color = 0x8000C040;
        }
        break;
    }
    menu_draw_item_list(self, sel, color, flags, 0);
    if (flags & 0x80) {
        u32 row = U8(self, MENU_LIST_SEL) >> 3;

        if (row == (u32)(U8(self, 0x595) >> 3)) {
            layout_build(layout2, D_0893B0E0, 1, 0, 0);
            gfx_fill_rect_pulse(self, U16(layout2, 28),
                          (u16)(U16(layout2, 30) + U16(layout2, 16) * (U8(self, MENU_LIST_SEL) - row * 8)),
                          U16(layout2, 32), U16(layout2, 34), 0x400000);
        }
    }
}

/* item name (id) with color: 5 = highlighted, 10 = not available */
void menu_draw_item_name(void *self, int x, int y, s32 avail_arg, u32 id_arg, s32 hilite_arg)
{
    s8 avail = avail_arg;
    u16 id = id_arg;
    s8 hilite = hilite_arg;
    int color;

    if (id == 0xFFFF)
        return;
    if (hilite == 1)
        color = 5;
    else if (avail == -1)
        color = 10;
    else
        color = 0;
    font_printf_at_color(g_font, x, y, color, fmt_s, text_item_name(g_game, id_arg));   /* the original passes the received value */
}

/* searches the list "slots" for item id and sets the text color */
static void set_color_for_slot(void *self, u16 *slots, s16 id)
{
    int i;

    for (i = 0; i < 0x18; i++) {
        if (id == slots[i * 2]) {
            s16 r = (s16)menu_combo_room(self, slots + i * 2);

            if (r >= 0) {
                if (r == 1)
                    font_set_color(g_font, 6);
                else if (r == 0)
                    font_set_color(g_font, 2);
            }
            return;
        }
    }
}

/* trade window: two items and details */
void menu_draw_trade(void *self)
{
    s16 xoff[7], pos[7][2], a[7], b[7];
    u8 layout[0x28];
    u16 *slots;
    int lang, i, x2;
    u32 flags = 5, color = 0;
    u8 st, c;

    for (i = 0; i < 7; i++)
        xoff[i] = D_089384F4[i];
    lang = LANG;
    if ((GAME_FLAGS & 0x200) == 0)
        slots = (u16 *)(g_game + 0x43C8);
    else
        slots = pouch_slot(g_game, 0);

    st = U8(self, 0x5B8);
    if (st < 3 && (U32(self, MENU_HUD_FLAGS) & 0x10) == 0) {
        layout_draw(self, st == 2 ? D_089384CC : D_0893847C, text_menu(g_game, D_089AC834[0]), 0xFF);
        c = U8(self, 0x5B8);
        if (c == 0) {
            u8 s;

            font_printf_at_color(g_font, (s16)(D_08938488 + D_0893848C * 3 + xoff[lang]), D_0893848A, 3,
                          fmt_s, text_menu(g_game, D_089AC834[9]));
            s = U8(self, MENU_LIST_SEL);
            if ((u8)menu_slot_has_item(self, s, slots[s * 2]) != 1)
                menu_draw_item_name(self, D_08938488, (s16)(D_0893848A + D_0893848C), -1, slots[U8(self, MENU_LIST_SEL) * 2], 0);
            else
                menu_draw_item_name(self, D_08938488, (s16)(D_0893848A + D_0893848C), U8(self, MENU_LIST_SEL),
                                    slots[U8(self, MENU_LIST_SEL) * 2], 0);
        } else if (c == 2) {
            menu_draw_item_name(self, D_089384D8, (s16)(D_089384DA + D_0893848C), U8(self, 0x5B0), U16(self, 0x5AC), 1);
        } else {
            menu_draw_item_name(self, D_08938488, (s16)(D_0893848A + D_0893848C), U8(self, 0x5B0), U16(self, 0x5AC), 1);
        }
        c = U8(self, 0x5B8);
        if (c != 0) {
            layout_draw(self, c == 2 ? D_089384E0 : D_08938490, text_menu(g_game, D_089AC834[1]), 0xFF);
            c = U8(self, 0x5B8);
            if (c == 1) {
                u8 s;

                font_printf_at_color(g_font, (s16)(D_0893849C + D_089384A0 * 3 + xoff[lang]), D_0893849E, 3,
                              fmt_s, text_menu(g_game, D_089AC834[9]));
                s = U8(self, MENU_LIST_SEL);
                if (combo_find(D_08A62C50, S16(self, 0x5AC), (s16)slots[s * 2], 0, 0, 0) == 0)
                    menu_draw_item_name(self, D_0893849C, (s16)(D_0893849E + D_089384A0), -1,
                                        slots[U8(self, MENU_LIST_SEL) * 2], 0);
                else
                    menu_draw_item_name(self, D_0893849C, (s16)(D_0893849E + D_089384A0), U8(self, MENU_LIST_SEL),
                                        slots[U8(self, MENU_LIST_SEL) * 2], 0);
            } else if (c == 2) {
                /* the original does not set the last argument here (random value in t1) */
                menu_draw_item_name(self, D_089384EC, (s16)(D_0893849E + D_089384F0), U8(self, 0x5B1), U16(self, 0x5AE), 0);
            } else {
                menu_draw_item_name(self, D_0893849C, (s16)(D_0893849E + D_089384A0), U8(self, 0x5B1), U16(self, 0x5AE), 0);
            }
        }
        st = U8(self, 0x5B8);
        if (st < 2)
            color = 0x8000C040;
        flags = (st == 0 ? 4 : 8) | 0x11;
    }

    for (i = 0; i < 7; i++) {
        pos[i][0] = D_08938504[i][0];
        pos[i][1] = D_08938504[i][1];
    }
    D_089384A4 = pos[lang][0] + 0xC;
    D_089384B0 = pos[lang][0] + 0x14;
    D_089384A8 = pos[lang][1] + 0x9C;
    x2 = (s16)(D_089384B0 + D_089384AC * 5);

    switch (U8(self, 0x5B8)) {
    case 0:
    case 1:
        if (U32(self, MENU_HUD_FLAGS) & 0x10) {
            flags |= 0x21;
            color = 0;
            draw_title_text(self);
            break;
        }
        flags |= (U32(self, MENU_HUD_FLAGS) & 0x20) ? 0x10 : 0x50;
        color = 0x8000C040;
        break;
    case 2:
        for (i = 0; i < 7; i++)
            a[i] = D_08938520[i];
        for (i = 0; i < 7; i++)
            b[i] = D_08938530[i];
        D_089384B8 = 0xF8 - a[LANG];
        D_089384C0 = b[LANG] + 9;
        layout_build(layout, &D_089384B8, 0, 0, 0);
        layout_draw_box(self, layout, 0, U8(self, 0x5B9), 0xFF, 0);
        font_printf_at_color(g_font, S16(layout, 12), S16(layout, 14), U8(self, 0x5BA) ? 10 : 0, fmt_s,
                      text_menu(g_game, D_089AC834[6]));
        /* prosegue */
    case 3:
    case 4:
        layout_draw(self, &D_089384A4, text_menu(g_game, D_089AC834[2]), 0xFF);
        if (S16(self, 0x5B4) < 1) {
            const char *s1, *s2;

            font_set_color(g_font, 10);
            s1 = text_menu(g_game, D_089AC834[3]);
            font_printf_at(g_font, D_089384B0, (s16)(D_089384B2 + D_089384B4), fmt_s, s1);
            s2 = text_menu(g_game, D_089AC834[5]);
            font_printf_at(g_font, x2, (s16)(D_089384B2 + D_089384B4 * 2), fmt_s, s2);
            font_printf_at(g_font, x2, (s16)(D_089384B2 + D_089384B4 * 3), fmt_s, s1);
            font_printf_at(g_font, x2, (s16)(D_089384B2 + D_089384B4 * 4), fmt_s, s1);
        } else {
            s8 rank;
            u16 have;

            font_set_color(g_font, 0);
            font_printf_at(g_font, D_089384B0, (s16)(D_089384B2 + D_089384B4), fmt_s,
                          text_item_name(g_game, U16((u8 *)U32(self, MENU_TRADE), 2)));
            rank = (s8)combo_rate_of(D_08A62C50, U32(self, MENU_TRADE), 0);
            font_set_color(g_font, D_0893AF2C[(u8)combo_rate_class(rank)]);
            font_printf_at(g_font, x2, (s16)(D_089384B2 + D_089384B4 * 2),
                          text_menu(g_game, D_089AC834[4]), (int)rank);
            font_set_color(g_font, 0);
            font_printf_at(g_font, x2, (s16)(D_089384B2 + D_089384B4 * 3), fmt_s,
                          menu_item_category(self, U32(self, MENU_TRADE)));
            set_color_for_slot(self, slots, S16(self, 0x5B4));
            have = (u16)pouch_count(g_game, S16(self, 0x5B4));
            font_printf_at(g_font, x2, (s16)(D_089384B2 + D_089384B4 * 4), D_08938540, have,
                          D_0899D19B[S16(self, 0x5B4) * 0x18]);
            font_set_color(g_font, 0);
        }
        break;
    case 5:
    case 6: {
        int t;
        u8 n, max;
        u16 have;

        if (S16(self, 0x5B6) == 0x9B) {
            t = 8;
            n = 1;
        } else {
            n = U8(self, 0x5E5);
            t = 7;
        }
        layout_draw(self, &D_089384A4, text_menu(g_game, D_089AC834[t]), 0xFF);
        have = (u16)pouch_count(g_game, S16(self, 0x5B6));
        max = D_0899D19B[S16(self, 0x5B6) * 0x18];
        font_set_color(g_font, 0);
        font_printf_at(g_font, D_089384B0, (s16)(D_089384B2 + D_089384B4), fmt_s,
                      text_item_name(g_game, U16(self, 0x5B6)));
        font_printf_at(g_font, x2, (s16)(D_089384B2 + D_089384B4 * 3), D_08938548, n);
        set_color_for_slot(self, slots, S16(self, 0x5B4));
        font_printf_at(g_font, x2, (s16)(D_089384B2 + D_089384B4 * 4), D_08938540, have, max);
        font_set_color(g_font, 0);
        break;
    }
    }
    menu_draw_item_list(self, U8(self, MENU_LIST_SEL), color, flags, 0);
}
