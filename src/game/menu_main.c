/* Menu: main update/draw loop, opening and closing of the menu, page indicator.
 * Same class as menu_box.c. (Guessed names.) */
#include "menu.h"
#undef menu_draw_sprite
#undef layout_build
#undef layout_draw_pages
#undef menu_draw_main_page


extern void *g_sound;          /* sound */
extern const char D_0893879C[];   /* "%2d/%2d" */
extern const char D_08938540[];   /* "%d/%d" */
extern const s16 D_08939854, D_08939856, D_08939434;

void sound_play(void *snd, int a, int b, int c, int d, int e, int f, int g, int h);
u16 game_buttons(void *game);
s32 func_0883E928(void *self, u32 key);
s32 func_08834B0C(void *self, u32 key);
void pouch_backup_deliveries(void *hunter);
void font_set_layer(void *font, int v);
void func_088392F4(void *self);
void hud_draw_item_get(void *self);

void hud_draw_choice_grid(void *self);
void hud_draw_player_page(void *self);
void hud_draw_quest_page(void *self);
void hud_draw_card_page(void *self);
void hud_draw_options(void *self);
void hud_notice_draw(void *self);
void hud_draw_player_page2(void *self);
void menu_draw_choice(void *self);
void menu_draw_trade(void *self);
void menu_draw_info(void *self);
void menu_draw_item_box(void *self);
void menu_draw_equip_box(void *self);
void menu_draw_main(void *self);
void menu_draw_entry_desc(void *self);
void menu_begin_draw(void *self);
void menu_update_counter(u8 *h);
void menu_draw_messages(void *self);
void menu_draw_typewriter(u8 *self);
void fx_draw_all(u8 *self);
void menu_draw_sprite(u8 *self, u32 x, u32 y, s32 id);
void gfx_draw_sprite_mode(u8 *self, u32 x, u32 y, s32 w_arg, s32 h_arg, u32 r, u32 g, u32 b, u8 a,
                          s32 id, u8 mode);
void gfx_draw_sprite_scaled_rgba(u8 *self, u32 x, u32 y, s32 w_arg, s32 h_arg, u32 r, u32 g, u32 b,
                                 u8 a, s32 id);
void menu_close_state(u8 *self);
void menu_close_all(u8 *self);
s32 menu_update_page(u8 *self, u32 key);
void menu_draw_layer0(u8 *self);
void menu_draw_layer1(u8 *self);
void menu_draw_layer2(u8 *self);
void menu_draw_page(u8 *self);
u32 menu_get_key(u8 *self);

/* page indicator "n/total" with the arrows; flags 1 = both arrows, 2 = with the
 * button icons, 4 = without padding */
void menu_draw_pager(u8 *self, s32 x_arg, s32 y, u32 cur_arg, u32 total_arg, u32 flags)
{
    s16 x = x_arg;
    u8 cur = cur_arg;
    u8 total = total_arg;
    char buf[16];
    s16 w;
    u8 arrows;

    font_set_size(g_font, 14, 14);
    if (flags & 4)
        sprintf(buf, D_08938540, cur + 1, total);
    else
        sprintf(buf, D_0893879C, cur + 1, total);
    font_char_count(g_font, buf);
    w = font_char_count(g_font, buf) * 7;
    x = x - (w >> 1);
    font_printf_at_color(g_font, x, y, 0, fmt_s, buf);
    if (flags & 1) {
        arrows = 3;
    } else if (total < 2) {
        arrows = 0;
    } else {
        arrows = cur != 0;
        if (cur != total - 1)
            arrows |= 2;
    }
    if (arrows == 0)
        return;
    if ((flags & 2) == 0) {
        if (arrows & 1)
            gfx_draw_sprite_mode(self, (u16)(x - 2 - D_08939854), (u16)(s16)((s16)y - 1), D_08939854,
                                 D_08939856, 0, 192, 64, 240, 142, 1);
        if (arrows & 2)
            gfx_draw_sprite_scaled_rgba(self, (u16)(x + 2 + w), (u16)(s16)((s16)y - 1), D_08939854,
                                        D_08939856, 0, 192, 64, 240, 142);
    } else {
        if (arrows & 1) {
            gfx_draw_sprite_mode(self, (u16)(x - 4 - D_08939854 - D_08939434), (u16)(s16)((s16)y - 1),
                                 D_08939854, D_08939856, 0, 192, 64, 240, 142, 1);
            menu_draw_sprite(self, (u16)(x - 2 - D_08939434), (u16)(s16)((s16)y - 2), 54);
        }
        if (arrows & 2) {
            gfx_draw_sprite_scaled_rgba(self, (u16)(D_08939434 + x + 4 + w), (u16)(s16)((s16)y - 1),
                                        D_08939854, D_08939856, 0, 192, 64, 240, 142);
            menu_draw_sprite(self, (u16)(x + 2 + w), (u16)(s16)((s16)y - 2), 55);
        }
    }
}

/* update and draw of the whole menu (one frame) */
void menu_frame(u8 *self)
{
    u16 key = menu_get_key(self);

    if (self[0x52C] == 1) {
        goto run;
    } else if (self[0x52C] == 0) {
        self[0x52C] = 1;
run:
        if ((*(u32 *)(g_sys + 0xF7A57C) & 1) == 1) {
            self[0x606] = 0;
            if ((s32)U32(self, 0x600) > 0)
                U32(self, 0x600) -= 1;
            else
                U32(self, 0x600) = 0;
        }
        if (S8(self, 0x5F0) == 0) {
            if (menu_update_page(self, key) == 0)
                func_08834B0C(self, key);
            fx_draw_all(self);
        }
    }
    menu_update_counter(self);
    menu_draw_layer0(self);
    menu_draw_layer1(self);
    menu_draw_layer2(self);
    menu_draw_messages(self);
}

/* opens the menu (pause screen) */
void menu_show(u8 *self)
{
    if (*(s32 *)D_09B22D70 == 12)
        return;
    g_game[0x10] = 1;
    *(s32 *)D_09B22D70 = 12;
    menu_close_state(self);
    self[0x111D] = 1;
    U16(self, 0x111E) = 0;
    sound_play(g_sound, 0, 17, 0, 0, 0, 0, 0, 0);
}

/* resets the state of the current page */
void menu_close_state(u8 *self)
{
    u32 f;

    self[0x111B] = 0;
    self[0x111A] = 0;
    self[0x1119] = 0;
    self[0x1118] = 0;
    U32(self, 0x5E8) |= 4;
    f = U32(self, 0x528);
    U32(self, 0x528) = f & ~0x800;
    U32(self, 0x528) = f & ~0xA00;
    U32(self, 0x528) = f & ~0xB00;
    self[0x5FE] = 4;
    U16(self, 0x5FC) = self[0x111C];
    self[0x5FF] = 0xFF;
    U32(self, 0x5E8) &= ~0x40;
}

/* 1 when the menu is closed (otherwise reads the key of the frame) */
int menu_check_closed(u8 *self)
{
    if (self[0x111D] == 0) {
        menu_close_all(self);
        return 1;
    }
    U16(self, 0x111E) = game_buttons(g_game);
    return 0;
}

/* closes the menu */
void menu_close_all(u8 *self)
{
    u32 f;

    self[0x111D] = 0;
    self[0x111B] = 0;
    self[0x111A] = 0;
    self[0x1119] = 0;
    self[0x1118] = 0;
    U32(self, 0x5E8) |= 4;
    f = U32(self, 0x528);
    U32(self, 0x528) = f & ~0x800;
    U32(self, 0x528) = f & ~0xA00;
    U32(self, 0x528) = f & ~0xB00;
    g_game[0x10] = 0;
    self[0x112C] = 0;
    self[0x1138] = 0;
    self[0x1144] = 0;
    self[0x1150] = 0;
    self[0x115C] = 0;
    self[0x1168] = 0;
}

/* update of the current page; when it returns != 0 the pouch of the hunter is refreshed */
s32 menu_update_page(u8 *self, u32 key)
{
    s32 r = func_0883E928(self, key);

    if (r != 0)
        pouch_backup_deliveries(*(void **)(self + 0x530));
    return r;
}

/* layer 9: background */
void menu_draw_layer0(u8 *self)
{
    self[0x534] = 9;
    font_set_layer(g_font, 0);
    menu_begin_draw(self);
    if (*(s16 *)(g_game + 0x6AF0E) == 13)
        func_088392F4(self);
}

/* layer 10: pages */
void menu_draw_layer1(u8 *self)
{
    self[0x534] = 10;
    font_set_layer(g_font, 1);
    menu_begin_draw(self);
    if (self[0x52F] != 0)
        hud_draw_item_get(self);
    else if (g_game[0x10] != 0)
        menu_draw_page(self);
}

/* layer 11: typewriter text */
void menu_draw_layer2(u8 *self)
{
    self[0x534] = 11;
    font_set_layer(g_font, 2);
    menu_begin_draw(self);
    if (self[0x5F0] != 0) {
        menu_draw_typewriter(self);
        self[0x5F0] = 0;
    }
}

/* draws the current page of the menu */
void menu_draw_page(u8 *self)
{
    if (self[0x1118] == 1) {
        switch (self[0x111C]) {
        case 0: menu_draw_choice(self); break;
        case 1: menu_draw_trade(self); break;
        case 2: menu_draw_info(self); break;
        case 3: hud_draw_choice_grid(self); break;
        case 4: hud_draw_player_page(self); break;
        case 5: hud_draw_quest_page(self); break;
        case 6: hud_draw_card_page(self); break;
        case 7: menu_draw_item_box(self); break;
        case 8: menu_draw_equip_box(self); break;
        case 9: menu_draw_main(self); break;
        case 10: hud_draw_options(self); break;
        case 11: hud_notice_draw(self); break;
        case 12: hud_draw_player_page2(self); break;
        }
    } else if (self[0x1118] == 0) {
        func_0883488C(self, 0);
    }
    if (U32(self, 0x5E8) & 4)
        menu_draw_entry_desc(self);
}

/* key of the frame (the saved one while the menu is open) */
u32 menu_get_key(u8 *self)
{
    if (self[0x111D] == 0)
        return game_buttons(g_game);
    return U16(self, 0x111E);
}

extern const u16 D_089AC850;
extern const char D_0893B218[], D_0893B220[];
extern const u8 D_0893AFA0[];
const char *func_09A7B790(void *a, u16 id);
s32 hud_event_state(void *self, u8 a, u8 b);
void layout_draw_box(void *self, void *layout, int a, int b, int alpha, int c);
void layout_build(Layout *out, const u8 *src, int title, s32 dx_arg, s32 dy_arg);
void layout_draw_pages(void *self, Layout *l, u32 cur, u32 total_arg, s32 color, s32 alt);
void menu_cursor_two_columns(void *self, u8 *pos, u32 keys, u32 count_arg);
s32 menu_select_entry(u8 *self, u32 i);
u16 menu_page_input_dispatch(u8 *self, u32 page, u32 keys);

/* main page of the menu: 14 entries on two pages of 7 (entries not available in grey) */
void menu_draw_main_page(u8 *self, int dim)
{
    u8 sel = self[0x111C];
    u8 page = sel / 7;
    char title[32];
    s8 col[8];
    Layout l;
    const u16 *ids;
    int i;

    sprintf(title, D_0893B218, text_menu(g_game, D_089AC850));
    layout_build(&l, D_0893AFA0, (int)title, 0, 0);
    if ((s8)dim != 0)
        l.color = 0x80808080;
    layout_draw_box(self, &l, (int)title, (u8)(sel % 7), 255, 1);
    layout_draw_pages(self, &l, page, 2, 0, dim);
    col[6] = 0;
    col[5] = 0;
    col[4] = 0;
    col[3] = 0;
    col[2] = 0;
    col[1] = 0;
    col[0] = 0;
    if (page == 0) {
        ids = (const u16 *)0x09B051A0;
        if ((*(u32 *)(g_sys + 0xF7A57C) & 1) == 0) {
            col[3] = 10;
            col[4] = 10;
            col[6] = 10;
        }
        if (U32(*(u8 **)(self + 0x530), 0x614) & 8)
            col[1] = 10;
    } else {
        ids = (const u16 *)0x09B051AE;
        if ((*(u32 *)(g_sys + 0xF7A57C) & 1) == 0) {
            col[4] = 10;
            col[5] = 10;
        } else {
            u8 *h = *(u8 **)(self + 0x530);

            if ((u8)hud_event_state(self, h[0x298], h[0x299]) == 1)
                col[5] = 10;
            if (U32(*(u8 **)(self + 0x530), 0x614) & 8)
                col[5] = 10;
        }
        col[6] = 10;
    }
    for (i = 0; i < 7; i++)
        font_printf_at_color(g_font, l.tx, (s16)(l.ty + (s16)i * l.lh), col[i], D_0893B220,
                      func_09A7B790(D_09B22D70, ids[i]));
}

/* input of the main page and of the open sub-page; 1 when the menu must close */
s32 menu_input(u8 *self, u32 keys)
{
    u8 *st = self + 0x1118;

    if (self[0x111D] == 0)
        return 1;
    U16(self, 0x111E) = 0;
    if (st[0] == 1) {
        u16 r = menu_page_input_dispatch(self, self[0x111C], keys);

        if (U32(self, 0x121C) == 0) {
            if (r & 8) {
                self[0x111D] = 0;
                sound_play(g_sound, 0, 20, 0, 0, 0, 0, 0, 0);
                return 1;
            }
            if (r & 0x2000) {
                menu_close_state(self);
                sound_play(g_sound, 0, 20, 0, 0, 0, 0, 0, 0);
            }
        }
    } else if (st[0] == 0) {
        menu_cursor_two_columns(self, self + 0x111C, keys, 14);
        U16(self, 0x5FC) = self[0x111C];
        if (keys & 0x4000) {
            if (menu_select_entry(self, self[0x111C]) == 0) {
                st[0] += 1;
                U32(self, 0x121C) = 0;
                sound_play(g_sound, 0, 19, 0, 0, 0, 0, 0, 0);
            } else {
                sound_play(g_sound, 0, 21, 0, 0, 0, 0, 0, 0);
            }
        } else if (keys & 0x2008) {
            self[0x111D] = 0;
            sound_play(g_sound, 0, 20, 0, 0, 0, 0, 0, 0);
            return 1;
        }
    } else {
        return 0;
    }
    return 0;
}

s32 menu_open_item_list(void *self);
s32 menu_open_use_list(void *self);
s32 menu_open_info(void *self);
s32 menu_open_status(void *self);
s32 menu_open_equipment(void *self);
s32 menu_open_list_tabs(void *self);
void func_088D897C(void *hunter);
void skill_update_hunter(void *self, void *hunter);

/* opens entry i of the main page; 0 if it was opened */
s32 menu_select_entry(u8 *self, u32 i_arg)
{
    u8 i = i_arg;
    u8 *h = (u8 *)player_get(g_players, g_game[0x28]);
    int online = (*(u32 *)(g_sys + 0xF7A57C) & 1) != 0;
    s32 r = 1;

    switch (i) {
    case 0:
        r = menu_open_item_list(self);
        break;
    case 1:
        func_088D897C(h);
        r = menu_open_use_list(self);
        break;
    case 2:
        r = menu_open_info(self);
        break;
    case 3:
        if (online) {
            self[0x57E] = 0;
            self[0x57F] = 0;
            self[0x580] = 0;
            self[0x5FE] = 0;
            r = 0;
            U16(self, 0x5FC) = 12;
        }
        break;
    case 4:
        if (online) {
            self[0x57E] = 0;
            self[0x57F] = 0;
            self[0x580] = 0;
            r = 0;
        }
        break;
    case 5:
        r = 0;
        U32(self, 0x5E8) |= 4;
        U16(self, 0x5FC) = 14;
        self[0x57E] = 0;
        self[0x57F] = 0;
        break;
    case 6: {
        u8 *g;
        u32 k;
        s8 n = 0;

        if (!online)
            return 1;
        U32(self, 0x5E8) |= 4;
        self[0x57E] = 0;
        self[0x57F] = 0;
        U32(self, 0x118C) = (u32)(g_game + 0x69BEC);
        g = g_game;
        for (k = 0; k < 13; k++, g += 0x70)
            if (g[0x69BEC] != 0)
                n++;
        if (n == 0)
            U16(self, 0x5FC) = 21;
        else
            U16(self, 0x5FC) = self[0x57F] + 18;
        r = 0;
        break;
    }
    case 7:
        skill_update_hunter(self, h);
        r = menu_open_status(self);
        break;
    case 8:
        func_088D897C(h);
        r = menu_open_equipment(self);
        break;
    case 9:
        func_088D897C(h);
        r = menu_open_list_tabs(self);
        break;
    case 10: {
        s8 m;

        self[0x609] = g_game[0x6ADDE];
        m = g_game[0x6ADD9];
        if (m == 3)
            self[0x60A] = 2;
        else if (m == 2)
            self[0x60A] = 1;
        else if (m == 1)
            self[0x60A] = 0;
        U32(self, 0x5E8) |= 4;
        self[0x5FE] = 3;
        U16(self, 0x5FC) = 9;
        self[0x57F] = 0;
        r = 0;
        break;
    }
    case 11:
        if (online)
            r = 0;
        break;
    case 12:
        if (online && (u8)hud_event_state(self, h[0x298], h[0x299]) == 0
            && (U32(*(u8 **)(self + 0x530), 0x614) & 8) == 0) {
            self[0x57E] = 0;
            self[0x57F] = 0;
            self[0x580] = 0;
            r = 0;
        }
        break;
    }
    return r;
}

u32 func_0881CB2C(void *self, u32 keys);
u32 func_0881DB5C(void *self, u32 keys);
u32 menu_info_input(void *self, u32 keys);
u32 hud_grid_input(void *self, u32 keys);
u32 hud_player_page_input(void *self, u32 keys);
u32 hud_card_input(void *self, u32 keys);
u32 func_0883BB10(void *self, u32 keys);
u32 menu_status_input(void *self, u32 keys);
u32 menu_equipment_input(void *self, u32 keys);
u32 menu_list_tabs_input(void *self, u32 keys);
u32 hud_options_input(void *self, u32 keys);
u32 hud_invite_input(void *self, u32 keys);

/* input of the open sub-page */
u16 menu_page_input_dispatch(u8 *self, u32 page, u32 keys)
{
    u32 r = keys & 0xFFFF;

    switch ((u8)page) {
    case 0:
        r = func_0881CB2C(self, keys);
        pouch_backup_deliveries((void *)player_get(g_players, g_game[0x28]));
        return r;
    case 1:
        r = func_0881DB5C(self, keys);
        pouch_backup_deliveries((void *)player_get(g_players, g_game[0x28]));
        break;
    case 2: r = menu_info_input(self, keys); break;
    case 3: r = hud_grid_input(self, keys); break;
    case 4: r = hud_player_page_input(self, keys); break;
    case 5: r = hud_card_input(self, keys); break;
    case 6: r = func_0883BB10(self, keys); break;
    case 7: r = menu_status_input(self, keys); break;
    case 8: r = menu_equipment_input(self, keys); break;
    case 9: r = menu_list_tabs_input(self, keys); break;
    case 10: r = hud_options_input(self, keys); break;
    case 11: break;
    case 12: r = hud_invite_input(self, keys); break;
    }
    return r;
}
