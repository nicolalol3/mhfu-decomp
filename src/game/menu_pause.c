/* Pause menu during a quest (start button): list of the pages, quest rewards,
 * options, quit. Same class as menu_box.c / hud.c. (Guessed names.) */
#include "menu.h"
#include "vfpu.h"
#undef menu_quest_data
#undef gfx_draw_window
#undef gfx_draw_item_icon_alpha
#undef layout_draw
#undef hud_draw_quest_info

extern u8 *g_result;            /* quest data */
extern void *g_sound;          /* sound */
extern void *g_net;          /* network */
extern const u16 D_0893B710[], D_0893B718[], D_0893B738[];
extern const s16 D_0893B728, D_0893B72A, D_0893B72C, D_0893B72E, D_0893B730, D_0893B732;
extern const u16 D_0893B734, D_0893B736;
extern const s16 D_0893B740, D_0893B742, D_0893B744, D_0893B746, D_0893B748, D_0893B74A;
extern const s16 D_0893B74C, D_0893B74E, D_0893B750, D_0893B752, D_0893B754;
extern const u16 D_089B53CA, D_089B5410, D_089B5412, D_089B5414, D_089B5416, D_089B5418,
    D_089B541A, D_089B541C, D_089B54F2, D_089B5554, D_089B5560, D_089B5594;
extern const u16 D_089B551C[];    /* option texts, 3 per option */
extern u8 D_089B54F8[];
extern const u8 D_0893AFC8[];
extern const char D_0893B650[], D_0893B758[];
extern const u16 D_0893B090, D_0893B094, D_0893B09E;
extern const u8 D_0899D198[];     /* item table (0x18 bytes per item) */

const char *func_09A63198(void *a, u16 id);
void sound_play(void *snd, int a, int b, int c, int d, int e, int f, int g, int h);
void font_set_layer(void *font, int v);
int font_char_count(void *font, const char *s);
s32 font_text_width(void *font, const char *s);
void menu_begin_draw(void *self);
void gfx_draw_window(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 alpha);
void gfx_draw_arrows(u8 *self, u32 x, u32 y, u32 dx, u32 r, u32 g, u32 b, u8 a);
void gfx_draw_arrows_blink(u8 *self, u32 x, u32 y, u32 dx, u8 a);
void gfx_draw_item_icon_alpha(u8 *self, u32 id, u32 x, u32 y, u32 w, s32 h_arg, u8 mode, u8 alpha);
void text_draw_lines_block(void *self, s32 x_arg, s32 y_arg, const char *s, s32 align, s32 lh_arg);
void font_draw_centered(int x, int y, const char *s, int size);
s32 hrp_scale(void *quest, s32 sum, int a);
void menu_cursor_two_columns(void *self, u8 *pos, u32 keys, u32 count_arg);
int menu_cursor_vertical(void *self, u8 *pos, u32 keys, u32 count);
void menu_cursor_yes_no(void *self, u32 a, u32 b);
void net_send(void *net, int a, int b);
void save_start(void *game);
s32 save_in_progress(void *game);
void save_end(void *game);
s32 text_center_x(void *game, const char *s, s32 x, int size);
void quest_fail(void *a);
void func_08880760(void *snd, int v);
void layout_draw(void *self, Layout *l, const char *text, u32 alpha);

int menu_open_item_list(void *self);
int menu_open_use_list(void *self);
int menu_open_info(void *self);
s32 menu_open_give(u8 *self);
s32 func_0889615C(void *self);
int menu_open_status(void *self);
int menu_open_equipment(void *self);
int menu_open_list_tabs(void *self);
u32 func_0881CB2C(void *self, u32 keys);
u32 func_0881DB5C(void *self, u32 keys);
u32 menu_info_input(void *self, u32 keys);
u32 menu_close(void *self, u32 keys);
u32 func_08896228(void *self, u32 keys);
u32 menu_status_input(void *self, u32 keys);
u32 menu_equipment_input(void *self, u32 keys);
u32 menu_list_tabs_input(void *self, u32 keys);

#define SND(n) sound_play(g_sound, 0, n, 0, 0, 0, 0, 0, 0)
#define TXT(id) func_09A63198(D_09A44C5C, id)
#define ONLINE ((OPT_FLAGS & 1) != 0)

/* sum of the values of the reward items of the quest */
static s32 reward_value(void)
{
    u8 *q = g_result;
    u16 *e = (u16 *)(q + 0x28);
    s32 i, sum = 0;

    for (i = 0; i < *(s32 *)(q + 0x24); i++, e += 2) {
        if (e[0] == 0)
            break;
        sum += (s16)e[1] * *(const s32 *)&D_0899D198[e[0] * 0x18 + 0x10];
    }
    return sum;
}

/* quest rewards page (10 items per page) and the money of the quest */
void hud_draw_quest_rewards(u8 *self)
{
    u8 *q;
    u8 *m;
    u16 *e;
    const char *s;
    s32 col, row, r;

    if ((U32(*(u8 **)(self + MENU_PLAYER), 0x614) & 0x200) == 0) {
        U32(self, 0x1194) &= ~0x33;
        return;
    }
    q = g_result + 0x10;
    m = g_menu;
    m[0x534] = 10;
    font_set_layer(g_font, 1);
    menu_begin_draw(m);
    font_set_size(g_font, 14, 14);
    font_set_color(g_font, 0);
    gfx_draw_window(g_menu, D_0893B718[0], D_0893B718[1], D_0893B718[2], D_0893B718[3], 0xFF);
    gfx_draw_window(g_menu, D_0893B738[0], D_0893B738[1], D_0893B738[2], D_0893B738[3], 0xFF);
    s = TXT(D_089B5410);
    font_char_count(g_font, s);
    text_draw_lines_block(g_menu, D_0893B750 + (D_0893B754 >> 1), D_0893B752, s, 14, 16);
    if (*(s32 *)(q + 0x14) >= 11)
        gfx_draw_arrows_blink(g_menu, D_0893B710[0], D_0893B710[1], D_0893B710[2], 0xF0);
    e = (u16 *)(q + *(s32 *)(q + 0x10) * 0x28 + 0x18);
    for (col = 0; col < 2; col++) {
        for (row = 0; row < 5; row++, e += 2) {
            if (e[0] == 0)
                break;
            gfx_draw_item_icon_alpha(g_menu, e[0], (D_0893B728 + D_0893B730 * col) & 0xFFFF,
                                     (D_0893B72A + D_0893B732 * row) & 0xFFFF, D_0893B734, D_0893B736, 1,
                                     0xFF);
            font_printf_at(g_font, (s16)(D_0893B72C + D_0893B730 * col),
                          (s16)(D_0893B72E + D_0893B732 * row), text_item_name(g_game, e[0]));
            font_printf_at(g_font, (s16)(D_0893B740 + D_0893B730 * col),
                          (s16)(D_0893B742 + D_0893B732 * row), TXT(D_089B5412),
                          *(const s32 *)&D_0899D198[e[0] * 0x18 + 0x10], (s16)e[1]);
        }
    }
    font_draw_centered(D_0893B74C, D_0893B74E, TXT(D_089B5414), 14);
    r = hrp_scale(g_result, reward_value(), 1);
    font_printf_at(g_font, D_0893B74C, D_0893B74E, TXT(D_089B5416), reward_value(), r);
    font_draw_centered(D_0893B744, D_0893B746, TXT(D_089B53CA), 14);
    font_printf_at(g_font, D_0893B744, D_0893B746, TXT(D_089B541A), *(s32 *)(g_game + 0x696F0));
    font_draw_centered(D_0893B748, D_0893B74A, TXT(D_089B5418), 14);
    font_printf_at(g_font, D_0893B748, D_0893B74A, TXT(D_089B541C), *(s32 *)(g_game + 0x696E8));
}

/* opens the pause menu */
void hud_pause_open(u8 *self)
{
    self[0x52D] = 1;
    self[0x57C] = 0;
    g_game[0x10] = 1;
    U32(self, MENU_NOTICE_FLAGS) |= 4;
    U32(self, MENU_HUD_FLAGS) &= ~0xB00;
    self[0x5FE] = 0;
    U16(self, MENU_HELP_TEXT) = self[0x57D];
    self[0x5FF] = 0xFF;
    if (ONLINE && self[0x57D] == 10)
        U16(self, MENU_HELP_TEXT) = 11;
    U32(self, MENU_HUD_FLAGS) &= ~4;
    U32(*(u8 **)(self + MENU_PLAYER), 0x614) &= ~1;
    U32(self, 0x1194) &= ~0x33;
}

void hud_pause_close(u8 *self);
int hud_pause_select(u8 *self, u32 k);
u32 hud_pause_page_input(u8 *self, u32 k, u32 keys);

/* input of the pause menu: list of the pages, then the open page */
void hud_pause_input(u8 *self, u32 keys)
{
    if (self[0x57C] == 1) {
        u16 r = hud_pause_page_input(self, self[0x57D], keys);

        if (r & 8) {
            hud_pause_close(self);
            SND(0x14);
        } else if (r & 0x2000) {
            hud_pause_open(self);
            SND(0x14);
        }
        return;
    }
    if (self[0x57C] != 0)
        return;
    menu_cursor_two_columns(self, self + 0x57D, keys, 12);
    keys &= 0xFFFF;
    U16(self, MENU_HELP_TEXT) = self[0x57D];
    if (keys & 0x8000) {
        self[0x576] ^= 1;
        SND(9);
        hud_pause_close(self);
        return;
    }
    if (ONLINE && (keys & 0x1000)) {
        s16 *sig = (s16 *)(self + MENU_BEEP_TIMERS + G_MY_SLOT * 2);

        if (*sig < 0) {
            *sig = 0;
            net_send(g_net, 9, 0);
            hud_pause_close(self);
            return;
        }
        SND(0x15);
    }
    if (keys & 0x4000) {
        if (hud_pause_select(self, self[0x57D]) == 0) {
            self[0x57C]++;
            self[0x585] = 0;
            SND(0x13);
        } else {
            SND(0x15);
        }
    } else if (keys & 0x2008) {
        hud_pause_close(self);
        SND(0x14);
    }
}

/* closes the pause menu */
void hud_pause_close(u8 *self)
{
    self[0x52D] = 0;
    self[0x57C] = 0;
    g_game[0x10] = 0;
    U32(self, MENU_NOTICE_FLAGS) &= ~4;
    U32(self, MENU_HUD_FLAGS) &= ~0xB04;
}

int hud_save_open(u8 *self);
int hud_settings_open(u8 *self);
int hud_quit_open(u8 *self);
u32 hud_save_input(u8 *self, u32 keys);
u32 hud_settings_input(u8 *self, u32 keys);
u32 hud_quit_input(u8 *self, u32 keys);

/* opens the page k of the pause menu; 0 = opened */
int hud_pause_select(u8 *self, u32 k_arg)
{
    switch ((u8)k_arg) {
    case 0:
        return menu_open_item_list(self);
    case 1:
        return menu_open_use_list(self);
    case 2:
        return menu_open_info(self);
    case 3:
        return menu_open_give(self);
    case 4:
        return func_0889615C(self);
    case 5:
        return hud_save_open(self);
    case 6:
        return menu_open_status(self);
    case 7:
        return menu_open_equipment(self);
    case 8:
        return menu_open_list_tabs(self);
    case 9:
        return hud_settings_open(self);
    case 10:
        return hud_quit_open(self);
    case 11:
        return -1;
    }
    return 0;
}

/* input of the open page k of the pause menu */
u32 hud_pause_page_input(u8 *self, u32 k_arg, u32 keys)
{
    switch ((u8)k_arg) {
    case 0:
        return (u16)func_0881CB2C(self, keys);
    case 1:
        return (u16)func_0881DB5C(self, keys);
    case 2:
        return (u16)menu_info_input(self, keys);
    case 3:
        return (u16)menu_close(self, keys);
    case 4:
        return (u16)func_08896228(self, keys);
    case 5:
        return (u16)hud_save_input(self, keys);
    case 6:
        return (u16)menu_status_input(self, keys);
    case 7:
        return (u16)menu_equipment_input(self, keys);
    case 8:
        return (u16)menu_list_tabs_input(self, keys);
    case 9:
        return (u16)hud_settings_input(self, keys);
    case 10:
        return (u16)hud_quit_input(self, keys);
    }
    return 0;
}

/* page "save" (off-line only) */
int hud_save_open(u8 *self)
{
    if (ONLINE)
        return -1;
    if ((u8)game_quest_locked(g_game, 0) != 0)
        return -1;
    save_start(g_game);
    if ((u8)save_in_progress(g_game) == 1) {
        U32(self, MENU_NOTICE_FLAGS) &= ~4;
        self[0x11AB] = 1;
        return 0;
    }
    return -1;
}

u32 hud_save_input(u8 *self, u32 keys)
{
    if ((g_game[0x2A] & 0xF) != 0)
        return 0;
    if ((u16)keys & 0x6008) {
        save_end(g_game);
        self[0x11AB] = 0;
        return 8;
    }
    return 0;
}

/* slide of the "saving" window */
void hud_save_update(u8 *self)
{
    if (self[0x11AB] != 0 && (u8)save_in_progress(g_game) == 1) {
        if (self[0x11AC] < 5)
            self[0x11AC]++;
        return;
    }
    if (self[0x11AC] != 0)
        self[0x11AC]--;
}

void hud_draw_save(u8 *self)
{
    s16 y;
    const char *s;

    if (self[0x11AC] == 0)
        return;
    y = (self[0x11AC] - 1) * 34 - 7;
    gfx_draw_window(self, 0xB7, (y - 8) & 0xFFFF, 0x70, 0x1E, 0xFF);
    font_set_size(g_font, 14, 14);
    s = TXT(D_089B5594);
    font_printf_at_color(g_font, (s16)text_center_x(g_game, s, 0xF0, 14), y, 0, s);
}

/* page "options" */
int hud_settings_open(u8 *self)
{
    s8 v;

    self[MENU_SEL] = 0;
    U32(self, MENU_NOTICE_FLAGS) |= 4;
    self[0x5FE] = 3;
    U16(self, MENU_HELP_TEXT) = 0;
    v = g_game[0x6ADD9];
    if (v == 3)
        self[0x5E4] = 2;
    else if (v == 2)
        self[0x5E4] = 1;
    else if (v == 1)
        self[0x5E4] = 0;
    return 0;
}

u32 hud_settings_input(u8 *self, u32 keys)
{
    s8 sound = 0;
    u8 *v;
    u8 k;

    menu_cursor_vertical(self, self + MENU_SEL, keys, 9);
    keys &= 0xFFFF;
    U16(self, MENU_HELP_TEXT) = self[MENU_SEL];
    if (keys & 0xA0) {
        k = self[MENU_SEL];
        v = self + 0x5DC + k;
        switch (k) {
        case 1:
            if (self[0x5DC] != 0) {
                SND(0x15);
                break;
            }
            /* fall through */
        case 0:
        case 6:
        case 7:
            *v ^= 1;
            SND(0x17);
            break;
        case 8:
            sound = 1;
            /* fall through */
        case 2:
        case 3:
        case 4:
        case 5:
            if (keys & 0x80) {
                if (*v == 0)
                    *v = 2;
                else
                    *v = *v - 1;
            } else {
                *v = *v + 1;
                if (self[0x5DC + self[MENU_SEL]] >= 3)
                    self[0x5DC + self[MENU_SEL]] = 0;
            }
            SND(0x17);
            break;
        }
    }
    g_game[0x6ADDF] = self[0x5DF];
    g_game[0x6ADE0] = self[0x5E1];
    g_game[0x6ADE1] = self[MENU_OPTIONS];
    g_game[0x6ADE2] = self[0x5E0];
    g_game[0x6ADE4] = self[0x5E2];
    g_game[0x6ADE5] = self[0x5E3];
    if (sound != 0) {
        if (self[0x5E4] == 2)
            sound = 3;
        else if (self[0x5E4] == 1)
            sound = 2;
        else if (self[0x5E4] == 0)
            sound = 1;
        g_game[0x6ADD9] = sound;
        func_08880760(g_sound, (s8)g_game[0x6ADD9]);
    }
    return (u16)(keys & 0x2008);
}

void hud_draw_settings(u8 *self)
{
    Layout l;
    char buf[32];
    const u16 *tab;
    s16 y;
    int i;

    layout_build(&l, D_0893AFC8, 1, 0, 0);
    l.ptr = D_089B54F8 + self[0x5DC] * 0x12;
    sprintf(buf, D_0893B758, TXT(D_089B54F2));
    layout_draw_box(self, &l, (int)buf, -1, 0xFF, 4);
    font_set_color(g_font, 5);
    y = l.ty;
    tab = D_089B551C;
    for (i = 0; i < 9; i++, tab += 3) {
        if (self[0x5DC] != 0 && i == 1) {
            s16 w = font_text_width(g_font, TXT(D_089B5554));

            font_draw_at(g_font, (s16)((u16)l.x + 0xAC - ((w * 7) >> 1)), y, D_0893B650,
                          TXT(D_089B5554));
        } else {
            s16 w = font_char_count(g_font, TXT(tab[self[0x5DC + i]]));

            font_printf_at(g_font, (s16)((u16)l.x + 0xAC - ((w * 7) >> 1)), y, D_0893B650,
                          TXT(tab[self[0x5DC + i]]));
        }
        y += l.lh;
    }
    if (self[0x5DC] != 0 && self[MENU_SEL] == 1) {
        gfx_draw_arrows(self, (u16)(l.tx + 0x7A), (u16)(l.ty + l.lh * self[MENU_SEL]), 0x46, 0x70, 0x70,
                        0x70, 0xF0);
    } else {
        f32 s = vfpu_sinf((f32)(u16)((*(u32 *)(g_sys + 0xF7A554) & 0x3F) << 10) *
                          (3.14159265f / 32768.0f));

        gfx_draw_arrows(self, (u16)(l.tx + 0x7A), (u16)(l.ty + l.lh * self[MENU_SEL]), 0x46, 0x20,
                        ((s8)(int)(48.0f * s) + 0xAF) & 0xFF, 0x20, 0xF0);
    }
}

/* page "quit the quest" */
int hud_quit_open(u8 *self)
{
    if ((u8)game_quest_locked(g_game, 0) == 1)
        return -1;
    self[MENU_MODE] = 0;
    self[MENU_YESNO] = 1;
    U32(self, MENU_NOTICE_FLAGS) &= ~4;
    return 0;
}

u32 hud_quit_input(u8 *self, u32 keys)
{
    if ((u8)game_quest_locked(g_game, 0) == 1)
        return 8;
    if (self[0x1115] == 1)
        return keys;
    if (self[MENU_MODE] != 0)
        return 0;
    menu_cursor_yes_no(self, keys, 0xA0);
    if (((u16)keys & 0x4000) == 0)
        return keys;
    if (self[MENU_YESNO] != 0)
        return 0x2000;
    quest_fail(g_quest);
    self[MENU_MODE]++;
    SND(0x13);
    return 0;
}

void hud_draw_quit(u8 *self)
{
    const char *s;

    layout_draw(self, (Layout *)&D_0893B090, TXT(D_089B5560), 0xFF);
    s = text_menu(g_game, D_089AC588[self[MENU_YESNO]]);
    font_draw_at(g_font,
                  (s16)text_center_x(g_game, s, (s16)(D_0893B090 + (D_0893B094 >> 1)), 14),
                  (s16)(D_0893B09E + 0x96), D_0893B650, s);
}

extern const s16 D_0893B760[7], D_0893B770[7];
extern const u16 D_089B5578[], D_089B5588, D_089B558A, D_089B558C, D_089B558E, D_089B5590, D_089B5592;
extern const u16 D_089AC710[];    /* monster names */
extern const char D_0893B64C[], D_0893B780[], D_0893B78C[], D_0893B794[];
void func_0881A8F8(void *self, s32 x, s32 y, int a);
#define menu_draw_time func_0881A8F8
void func_0881A8E0(void *self, s32 x, s32 y, u32 v);
#define menu_draw_number func_0881A8E0
void font_set_pos(void *font, int x, int y);
void font_printf(void *font, const char *fmt, ...);
s32 quest_goal_kind(void *q, int i);
s32 quest_goal_target(void *q, int i);
s32 quest_goal_amount(void *q, int i);
s32 quest_goal_left_i(void *q, int i);
s32 quest_hunter_item_get(void *q, u32 id, int a);
s32 quest_goal_count(void *q, u32 id, u32 i);
s32 quest_points(void *q);
u8 *menu_quest_data(void *self);

void hud_quest_count(u8 *self, u32 a, u32 b);

/* "quest info" page: targets of the quest and their counters, time, reward */
void hud_draw_quest_info(u8 *self)
{
    s32 lang = LANG;
    s16 rows = 7;
    s16 y, y1, y2;
    char buf[16];
    s32 i;

    font_printf_at_color(g_font, 0xFC, 0xE0, 5, D_0893B650, TXT(D_089B558C));
    font_set_color(g_font, ((u8)game_quest_locked(g_game, 0) == 1) ? 10 : 0);
    menu_draw_time(self, (s16)(D_0893B760[lang] + 0x142), 0xE0, 0);
    font_printf_at_color(g_font, 0xFC, 0x60, 5, D_0893B650, TXT(D_089B5578[self[MENU_KIND]]));
    font_set_color(g_font, 0);
    switch (self[MENU_KIND]) {
    case 0:
        if (self[MENU_INFO_FLAGS] & 1) {
            u8 *q = g_quest;
            u16 m = (s8)q[0x77C];

            font_printf_at(g_font, 0xFC, 0x70, TXT(D_089B5592), text_menu(g_game, D_089AC710[m]));
            q = g_quest;
            if (S8(q, 0x7B2) == 0 || m == S8(q, 0x7A8)) {
                u8 *e;

                font_set_pos(g_font, 0x188, 0x70);
                q = g_quest;
                e = q + (*(u8 **)(q + 0x720))[0x1E8] * 2;
                hud_quest_count(self, (u16)(U16(e, 0x29C) + U16(e, 0x350)), 0);
            } else {
                s32 m2 = S8(q, 0x7A8);
                u8 a = q[0x781] - S8(*(u8 **)(q + 0x768), 4);

                font_set_pos(g_font, 0x188, 0x70);
                hud_quest_count(self, a, 0);
                font_printf_at(g_font, 0xFC, 0x80, TXT(D_089B5592),
                              text_menu(g_game, D_089AC710[m2]));
                q = g_quest;
                a = q[0x7AD] - S8(*(u8 **)(q + 0x794), 4);
                font_set_pos(g_font, 0x188, 0x80);
                hud_quest_count(self, a, 0);
            }
            break;
        }
        /* fall through */
    case 1:
        y = 0x70;
        if ((self[MENU_INFO_FLAGS] & 4) == 0) {
            for (i = 0; i < 2; i++) {
                u8 a, b;
                u16 id;

                if (self[MENU_INFO_FLAGS] & 2) {
                    if (quest_goal_kind(g_quest, i) != 3)
                        continue;
                    b = 1;
                    a = ((u8 *)g_quest)[quest_goal_target(g_quest, i) * 2 + 0x29C];
                } else {
                    if (quest_goal_kind(g_quest, i) != 2)
                        continue;
                    b = quest_goal_amount(g_quest, i);
                    a = b - quest_goal_left_i(g_quest, i);
                }
                id = quest_goal_target(g_quest, i);
                font_printf_at(g_font, 0xFC, y, TXT(D_089B5592), text_menu(g_game, D_089AC710[id]));
                font_set_pos(g_font, 0x188, y);
                hud_quest_count(self, a, b);
                y += 16;
            }
        } else {
            s32 n = S8(g_quest, 0x786);

            for (i = 0; i < n; i++) {
                u8 *q = g_quest;
                u8 got = q[0x781 + i];
                u8 a;
                u16 m;

                if (got == 0)
                    continue;
                a = got - S8(*(u8 **)(q + 0x768 + i * 4), 4);
                m = (s8)q[0x77C + i];
                font_printf_at(g_font, 0xFC, y, TXT(D_089B5592), text_menu(g_game, D_089AC710[m]));
                font_set_pos(g_font, 0x188, y);
                hud_quest_count(self, a, got);
                y += 16;
            }
            if (n >= 4)
                rows = 8;
        }
        break;
    case 2:
        y = 0x70;
        for (i = 0; i < 2; i++) {
            u16 id, a;

            if (quest_goal_kind(g_quest, i) != 1)
                continue;
            id = quest_goal_target(g_quest, i);
            font_printf_at(g_font, 0xFC, y, TXT(D_089B5592), text_item_name(g_game, id));
            font_set_pos(g_font, 0x181, y);
            a = quest_hunter_item_get(g_quest, id, 0);
            hud_quest_count(self, a, (u16)quest_goal_count(g_quest, id, (u16)i));
            y += 16;
        }
        break;
    case 3:
    case 4:
        sprintf(buf, D_0893B64C, (10 - S8(g_game, 0x38F)) * 10);
        font_printf_at(g_font, 0x150, 0x60, TXT(D_089B5590), buf);
        font_printf_at(g_font, 0xFC, 0x70, D_0893B650, TXT(D_089B558E));
        break;
    case 5: {
        s32 n = quest_points(g_quest);

        if (n >= 1000000)
            n = 999999;
        font_printf_at(g_font, 0x15E, 0x70, D_0893B780, n, 2000);
        break;
    }
    }
    font_set_color(g_font, 5);
    y1 = rows * 16 + 0x40;
    font_printf_at(g_font, 0xFC, y1, D_0893B650, TXT(D_089B5588));
    y2 = (rows + 1) * 16 + 0x40;
    font_printf_at(g_font, 0xFC, y2, D_0893B650, TXT(D_089B558A));
    font_set_color(g_font, 0);
    menu_draw_number(self, (s16)(D_0893B770[lang] + 0x142), y1, U32(menu_quest_data(self), 8));
    menu_draw_number(self, (s16)(D_0893B770[lang] + 0x142), y2, U32(g_quest, 0x14));
}

/* counter "a/b" of a quest target (only "a" for the hunt-a-number quests) */
void hud_quest_count(u8 *self, u32 a, u32 b)
{
    if (self[MENU_INFO_FLAGS] & 1)
        font_printf(g_font, D_0893B794, (u16)a);
    else
        font_printf(g_font, D_0893B78C, (u16)a, (u16)b);
}

extern const f32 D_0893B1CC[];
extern const u16 D_0893B1F4[];
u32 game_rand(void *a);
void func_08882D20(void *snd, int a, u32 id);

/* animation of the quest banner (start / clear / fail) */
void hud_update_banner(u8 *self)
{
    if (S8(self, 0x11AE) == 0)
        return;
    if (++S16(self, 0x11BC) >= 300) {
        self[0x11AE] = 0;
        return;
    }
    switch (self[MENU_BANNER_KIND]) {
    case 2:
        if (++S16(self, 0x11BA) >= 0x71) {
            S16(self, 0x11BA) = 0x71;
            self[0x11B0]++;
        }
        break;
    case 1:
        if (++S16(self, 0x11BA) >= 100) {
            S16(self, 0x11BA) = 100;
            self[0x11B0]++;
        }
        break;
    case 0:
        if (self[0x11B0] == 0 && ++S16(self, 0x11BA) >= 101) {
            S16(self, 0x11BA) = 100;
            self[0x11B0]++;
        }
        break;
    }
    switch (self[0x11B1]) {
    case 3:
        self[0x11B1] = 4;
        break;
    case 2:
        self[0x11B1] = 3;
        func_08882D20(g_sound, 1, D_0893B1F4[self[MENU_BANNER_KIND]]);
        break;
    case 1:
        if (S16(self, 0x11BA) >= 60) {
            self[0x11B1] = 2;
            S16(self, 0x11B8) = (int)(D_0893B1CC[self[MENU_BANNER_KIND]] *
                                      (-1.0f + 1.5258789e-05f * (2.0f * (f32)game_rand(g_sys))));
        }
        break;
    case 0:
        if (S16(self, 0x11BA) >= 15)
            self[0x11B1] = 1;
        break;
    }
}

extern const f32 D_0893B1D8[];
extern const u8 D_0893B1EC[];
extern const u32 D_0893B1FC[];
extern const s16 D_0893B798, D_0893B79A, D_0893A370, D_0893A372, D_0893B79C, D_0893B79E, D_0893B7A0,
    D_0893B7A2;
void gfx_set_filter(u8 *self, int on);
void gfx_draw_sprite_rotated(void *self, u32 x, u32 y, u32 w, u32 h, s32 ang, u32 a, s32 id);
void gfx_draw_sprite_rotated_pivot(void *self, u32 x, u32 y, s32 px, s32 py, u32 w, u32 h, s32 ang,
                                   u8 a, s32 id);
void gfx_draw_sprite_mode(u8 *self, u32 x, u32 y, s32 w_arg, s32 h_arg, u32 r, u32 g, u32 b, u8 a,
                          s32 id, u8 mode);

void hud_draw_banner_stripes(u8 *self, s32 t_arg);
void hud_draw_banner_start(f32 scale, u8 *self, u32 x, u32 y, s32 ang, u32 a);
void hud_draw_banner_clear(f32 scale, u8 *self, u32 x, u32 y, s32 ang, u32 a);
void hud_draw_banner_fail(f32 scale, u8 *self, u32 x, u32 y, s32 ang_arg, u32 a);

/* quest banner: stripes, then the text zooming in (kind self[MENU_BANNER_KIND]) */
void hud_draw_banner(u8 *self)
{
    u8 k;
    f32 s;
    u8 a;

    if (S8(self, 0x11AE) == 0)
        return;
    hud_draw_banner_stripes(self, S16(self, 0x11BC));
    k = self[0x11B1];
    if (k != 4 && k != 3 && k != 2)
        return;
    s = 0.607f * D_0893B1D8[k];
    a = D_0893B1EC[k];
    gfx_set_filter(self, 1);
    switch (self[MENU_BANNER_KIND]) {
    case 2:
        hud_draw_banner_fail(2.2f * s, self, 0xF0, 0x88, S16(self, 0x11B8), a);
        break;
    case 1:
        hud_draw_banner_clear(2.2f * s, self, 0xF0, 0x86, S16(self, 0x11B8), a);
        break;
    case 0:
        hud_draw_banner_start(2.0f * s, self, 0xF0, 0x88, S16(self, 0x11B8), a);
        break;
    }
    gfx_set_filter(self, 0);
}

void hud_draw_banner_start(f32 scale, u8 *self, u32 x, u32 y, s32 ang, u32 a)
{
    gfx_draw_sprite_rotated(self, x & 0xFFFF, y & 0xFFFF, (u32)((f32)D_0893B798 * scale),
                            (u32)((f32)D_0893B79A * scale), ang, a, 0x17A);
}

void hud_draw_banner_clear(f32 scale, u8 *self, u32 x, u32 y, s32 ang, u32 a)
{
    gfx_draw_sprite_rotated(self, x & 0xFFFF, y & 0xFFFF, (u32)((f32)D_0893A370 * scale),
                            (u32)((f32)D_0893A372 * scale), ang, a, 0x17B);
}

/* "quest failed": two halves rotated around the center */
void hud_draw_banner_fail(f32 scale, u8 *self, u32 x, u32 y, s32 ang_arg, u32 a)
{
    s16 ang = ang_arg;
    s16 p = (int)(80.0f * scale);

    gfx_draw_sprite_rotated_pivot(self, x & 0xFFFF, y & 0xFFFF, p, p, (u32)((f32)D_0893B79C * scale),
                                  (u32)((f32)D_0893B79E * scale), (s16)(ang + 0x4000), a, 0x17C);
    gfx_draw_sprite_rotated_pivot(self, x & 0xFFFF, y & 0xFFFF, p, p, (u32)((f32)D_0893B7A0 * scale),
                                  (u32)((f32)D_0893B7A2 * scale), (s16)(ang - 0x4000), a, 0x17D);
}

/* stripes of the banner sliding in from the top and the bottom */
void hud_draw_banner_stripes(u8 *self, s32 t_arg)
{
    s16 t = t_arg;
    s16 x = 0, i = 0;
    s32 id;

    for (;;) {
        s16 d = t - i * 2;
        s16 y, lim;

        if (d < 0)
            d = 0;
        y = d * 3 - 0x17;
        lim = (i & 1) ? -4 : 0;
        if (lim < y)
            y = lim;
        id = D_0893B1FC[self[MENU_BANNER_KIND]];
        gfx_draw_sprite_mode(self, (u16)x, (u16)y, 0x14, 0x17, 0xFF, 0xFF, 0xFF, 0xE6, id, 0);
        gfx_draw_sprite_mode(self, (u16)(0x1CC - x), (u16)(0xF9 - y), 0x14, 0x17, 0xFF, 0xFF, 0xFF, 0xE6, id,
                             4);
        x += 0x14;
        if (x >= 0x1E0)
            break;
        i++;
    }
}
