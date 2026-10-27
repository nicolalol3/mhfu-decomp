/* Result screens at the end of the quest (4): unlocked flags, notices, new hunter rank,
 * training points and their records. The object is g_result, the offsets are in result.h.
 * (Guessed names.) */
#include "quest.h"
#include "result.h"
#undef menu_draw_sprite
#undef gfx_draw_window
#undef pouch_slot

extern u8 *g_result;
extern u8 *g_sound;
extern u8 *g_net_players;
extern u8 *g_equip_data;
extern void *D_09A44C5C;
extern const char D_089A8870[], D_089A891C[], D_089A89F4[];
extern const u16 D_089B53A0, D_089B53AE, D_089B53C6, D_089B53C8, D_089B53E2, D_089B53EC;
extern const u16 D_089B53FC, D_089B5408, D_089B5418;
extern const u16 D_089B5422, D_089B5424, D_089B5426, D_089B5428, D_089B542A, D_089B542C;
extern const u16 D_089B542E, D_089B5430, D_089B5432, D_089B5434, D_089B5438, D_089B543A;
extern const u16 D_089B543C, D_089B543E;
extern const u32 D_089A894C[7];           /* training counters needed for flag 0x29 */
extern const u16 D_089A8968[17];          /* monster lists (0xFFFF-terminated) */
extern const u16 D_089A898C[44];
extern const u8 D_08930BDC[];             /* crowns: 14 bytes per monster (+3 id, +6 max, +8 min) */
extern const u16 D_089A872C[90];          /* points of each monster kind hunted (training) */
extern const u8 D_089A8708[30];           /* time limits (minutes) for 3 time bonuses */
extern const u8 D_089A8728[3];            /* the 3 time bonuses (percent) */
extern const s32 D_0899D1A8[];            /* item table, 6 words per item */
extern const s16 D_089A89E4[7], D_089A8A00[7];

u32 sound_play(void *snd, int a, int b, int c, int d, int e, int f, int g, int h);
void func_088854A0(u8 *snd, u32 h, int a);
u32 game_buttons(void *g);
int game_test_flag64(u8 *g, u32 i);
void game_set_flag64(u8 *g, u32 i);
int progress_a01(u8 *g);
int progress_a23(u8 *g);
int progress_a4(u8 *g);
int progress_a0_5(u8 *g);
int progress_b01(u8 *g);
int progress_b234(u8 *g);
int progress_b567(u8 *g);
int progress_a6(u8 *g);
int progress_a7(u8 *g);
int progress_a8(u8 *g);
int progress_b8(u8 *g);
int progress_b9(u8 *g);
int progress_b10(u8 *g);
int progress_all(u8 *g);
int progress_c(u8 *g);
int progress_55f1(u8 *g);
int event_quests_done_a(u8 *g);
int event_quests_done_b(u8 *g);
int event_quests_done_c(u8 *g);
u32 game_count_total(u8 *g, u32 i);
u32 game_count_b(u8 *g, u32 i);
u32 game_group_count_total(u8 *g, u32 id);
u32 game_best_record_max(u8 *g, u32 id, u8 *which);
u32 game_best_record_min(u8 *g, u32 id, u8 *which);
s32 quest_time(Quest *self, s32 mode);
int game_rank(u8 *g);
void game_add_points(u8 *g, s32 n);
int has_kitchen_skill(u8 *g, u32 k);
int quest_flag_10(void);
u32 equip_rarity(void *p, u32 k, u32 id);
u16 *func_08856884(void *game, int idx);
void func_088D893C(u32 h);
void menu_begin_draw(void *menu);
void menu_draw_sprite(void *menu, u32 x, u32 y, u32 id);
void gfx_dim_screen(u8 *self, s32 l);
void gfx_draw_window(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 alpha);
void text_draw_lines_block(void *self, s32 x, s32 y, const char *s, s32 align, s32 lh);
void font_set_layer(void *font, int a);
void font_draw(void *font, const char *fmt, ...);
void font_puts_at(void *font, int x, int y, const char *s);
s32 font_text_width(void *font, const char *s);
void result_init_sounds(u8 *self);
void result_text_center(u8 *self, s32 y, const char *s);
void result_draw_windows(u8 *self);
void *memcpy(void *d, const void *s, size_t n);

extern u32 g_pad_pressed;

u32 result_tscore_save(u8 *self);
u8 crowns_large_all(u8 *self);
u8 crowns_small_all(u8 *self);
u8 hunted_list_a(u8 *self);
u8 hunted_list_b(u8 *self);
u32 hrp_scale(u8 *self, u32 v, s32 training);
s32 quest_elapsed_seconds(u8 *self);
void font_draw_centered(s32 x, s32 y, const char *s, s32 c);

#define SE(id) sound_play(g_sound, 0, id, 0, 0, 0, 0, 0, 0)
#define RTEXT(id) func_09A63198(D_09A44C5C, id)
#define G_TRAIN_N(i) U32(g_game, 0x67800 + (i) * 4)

/* the special flags of the save that the quest just unlocked; bit i of the result = flag i
 * (flags 0x10-0x17 and 0x28 are not checked here) */
u64 result_check_flags(u8 *self)
{
    u32 lo = 0, hi = 0;
    u32 i;
    u8 n;

#define CHECK(f, cond, dst, bit)                                                   \
    if ((u8)game_test_flag64(g_game, f) == 0 && (cond)) {                          \
        game_set_flag64(g_game, f);                                                \
        dst |= bit;                                                                \
    }
#define QUEST_IS(id) (quest_state(QUEST, 1) == 1 && (s16)G_QUEST_ID == (id))
#define HUNTED_TARGET (QUEST->kills[QUEST->record_target[0x1E8]] != 0)

    CHECK(0, (u8)progress_a01(g_game) == 1, lo, 1);
    CHECK(1, (u8)progress_a23(g_game) == 1, lo, 2);
    CHECK(2, (u8)progress_a4(g_game) == 1, lo, 4);
    CHECK(3, (u8)progress_a0_5(g_game) == 1, lo, 8);
    CHECK(4, (u16)game_count_total(g_game, 0x21) != 0, lo, 0x10);
    CHECK(5, (u16)game_count_total(g_game, 0x4C) != 0, lo, 0x20);
    CHECK(6, (u8)crowns_large_all(self) == 1, lo, 0x40);
    CHECK(7, (u8)crowns_small_all(self) == 1, lo, 0x80);
    CHECK(8, (u8)progress_b01(g_game) == 1, lo, 0x100);
    CHECK(9, (u8)progress_b234(g_game) == 1, lo, 0x200);
    CHECK(10, (u8)progress_b567(g_game) == 1, lo, 0x400);
    CHECK(11, event_quests_done_a(g_game) != 0, lo, 0x800);
    CHECK(12, event_quests_done_b(g_game) != 0, lo, 0x1000);
    CHECK(13, progress_55f1(g_game) != 0, lo, 0x2000);
    CHECK(14, (u8)hunted_list_a(self) == 1, lo, 0x4000);
    CHECK(15, (u8)hunted_list_b(self) == 1, lo, 0x8000);
    CHECK(0x18, (u8)progress_a6(g_game) == 1, lo, 0x1000000);
    CHECK(0x19, (u8)progress_a7(g_game) == 1, lo, 0x2000000);
    CHECK(0x1A, (u8)progress_a8(g_game) == 1, lo, 0x4000000);
    CHECK(0x1B, QUEST_IS(0x2BD9), lo, 0x8000000);
    CHECK(0x1C, (u8)progress_b8(g_game) == 1, lo, 0x10000000);
    CHECK(0x1D, (u8)progress_b9(g_game) == 1, lo, 0x20000000);
    CHECK(0x1E, (u8)progress_b10(g_game) == 1, lo, 0x40000000);
    CHECK(0x1F, (u8)progress_all(g_game) == 1, lo, 0x80000000);
    CHECK(0x20, QUEST_IS(0xC9E) && HUNTED_TARGET, hi, 1);
    CHECK(0x21, QUEST_IS(0xC9F) && HUNTED_TARGET, hi, 2);
    CHECK(0x22, QUEST_IS(0xCA0) && HUNTED_TARGET, hi, 4);
    CHECK(0x23, QUEST_IS(0xC8C), hi, 8);
    CHECK(0x24, (u16)game_count_total(g_game, 0x58) != 0, hi, 0x10);
    CHECK(0x25, (u8)progress_c(g_game) == 1, hi, 0x20);
    CHECK(0x26, (u8)event_quests_done_c(g_game) == 1, hi, 0x40);
    CHECK(0x27, g_game[0x6A548] >= 0x14 && g_game[0x6A549] >= 0x14, hi, 0x80);
    if ((u8)game_test_flag64(g_game, 0x29) == 0 && (s32)U32(g_game, 0x677FC) == 7 &&
        (s32)U32(g_game, 0x677F8) == -1) {
        u32 need[7];

        n = 0;
        need[0] = D_089A894C[0];
        need[1] = D_089A894C[1];
        need[2] = D_089A894C[2];
        need[3] = D_089A894C[3];
        need[4] = D_089A894C[4];
        need[5] = D_089A894C[5];
        need[6] = D_089A894C[6];
        i = 0;
        do {
            if (need[i] <= G_TRAIN_N(i))
                n++;
            i = (i + 1) & 0xFF;
        } while (i < 7);
        if (n == 7) {
            game_set_flag64(g_game, 0x29);
            hi |= 0x200;
        }
    }
#undef CHECK
#undef QUEST_IS
#undef HUNTED_TARGET
    return (u64)hi << 32 | lo;
}

/* screen with a notice (D_089B53FC) */
void result_notice_init(u8 *self)
{
    U16(self, RES_STEP) = 0;
    U16(self, RES_XA6) = 0;
    U16(self, RES_TIMER) = 900;
    U16(self, RES_STEP_TIMER) = 10;
    result_init_sounds(self);
}

/* 1 when closed (like result_flags_update) */
static s32 wait_close(u8 *self)
{
    if (S16(self, RES_TIMER) > 0)
        S16(self, RES_TIMER)--;
    if (S16(self, RES_STEP) == 1) {
        if (S16(self, RES_TIMER) > 0) {
            if ((game_buttons(g_game) & 0x4000) == 0)
                return 0;
            if (S16(self, RES_TIMER) > 0x32A)
                return 0;
        }
        if (self[RES_SND1] != 0)
            return 0;
        S16(self, RES_STEP)++;
        SE(0x13);
        return 1;
    }
    if (S16(self, RES_STEP) != 0)
        return 0;
    if (--S16(self, RES_STEP_TIMER) <= 0)
        S16(self, RES_STEP)++;
    return 0;
}

static void draw_press_button(u8 *self, char *buf)
{
    if (self[RES_SND1] == 0 && S16(self, RES_TIMER) < 0x32B) {
        sprintf(buf, RTEXT(D_089B53AE));
        font_set_pos(g_font, 400, 0xC0);
        font_draw(g_font, D_089A8870, buf);
        menu_draw_sprite(g_menu, 0x17E, 0xBF, 0x33);
    }
    font_set_pos(g_font, 0x40, 0xC0);
    font_draw(g_font, RTEXT(D_089B53EC), S16(self, RES_TIMER) / 30);
}

s32 result_notice_update(u8 *self)
{
    return wait_close(self);
}

void result_notice_draw(u8 *self)
{
    char buf[0x100];

    U8(g_menu, 0x534) = 9;
    font_set_layer(g_font, 0);
    menu_begin_draw(g_menu);
    gfx_dim_screen(g_menu, 0);
    gfx_draw_window(g_menu, 0x18, 0x30, 0x1B0, 0xB0, 0xFF);
    font_set_color(g_font, 0);
    font_set_size(g_font, 0x10, 0x10);
    sprintf(buf, D_089A8870, RTEXT(D_089B53A0));
    result_text_center(self, 0x40, buf);
    text_draw_lines_block(g_menu, 0xF0, 0x60, RTEXT(D_089B53FC), 0x10, 0x10);
    draw_press_button(self, buf);
}

/* 1: offline, or we are the leader of the online room */
u8 net_is_leader(u8 *self)
{
    if ((OPT_FLAGS & 1) == 0)
        return 1;
    return g_net_players[0x12D6] == 1;
}

/* every large monster has a gold crown for the largest size */
u8 crowns_large_all(u8 *self)
{
    const u8 *e = D_08930BDC;
    s32 i;
    u8 which;

    for (i = 0; i < 0x3E; i++, e += 0xE) {
        if ((u8)monster_is_large(QUEST, e[3]) == 0)
            continue;
        if ((u16)game_group_count_total(g_game, e[3]) == 0)
            return 0;
        if ((s16)game_best_record_max(g_game, e[3], &which) < S16(e, 6))
            return 0;
    }
    return 1;
}

/* every large monster has a gold crown for the smallest size */
u8 crowns_small_all(u8 *self)
{
    const u8 *e = D_08930BDC;
    s32 i;
    u8 which;

    for (i = 0; i < 0x3E; i++, e += 0xE) {
        if ((u8)monster_is_large(QUEST, e[3]) == 0)
            continue;
        if ((u16)game_group_count_total(g_game, e[3]) == 0)
            return 0;
        if (S16(e, 8) < (s16)game_best_record_min(g_game, e[3], &which))
            return 0;
    }
    return 1;
}

/* every monster of the list was hunted at least once */
u8 hunted_list_a(u8 *self)
{
    u16 l[17];
    u16 *p;
    s32 i;

    for (i = 0; i < 17; i++)
        l[i] = D_089A8968[i];
    for (p = l; *p != 0xFFFF; p++)
        if ((u16)game_count_total(g_game, *p & 0xFF) == 0)
            return 0;
    return 1;
}

u8 hunted_list_b(u8 *self)
{
    u16 l[44];
    u16 *p;
    s32 i;

    for (i = 0; i < 44; i++)
        l[i] = D_089A898C[i];
    for (p = l; *p != 0xFFFF; p++)
        if ((u16)game_count_b(g_game, *p & 0xFF) == 0)
            return 0;
    return 1;
}

/* screen of the new hunter rank */
void result_rankup_init(u8 *self)
{
    U16(self, RES_STEP) = 0;
    U16(self, RES_XA6) = 0;
    U16(self, RES_TIMER) = 900;
    U16(self, RES_STEP_TIMER) = 10;
    g_game[0x44BC] = game_rank(g_game);
    result_init_sounds(self);
}

s32 result_rankup_update(u8 *self)
{
    return wait_close(self);
}

void result_rankup_draw(u8 *self)
{
    char buf[0x100];

    U8(g_menu, 0x534) = 9;
    font_set_layer(g_font, 0);
    menu_begin_draw(g_menu);
    gfx_dim_screen(g_menu, 0);
    gfx_draw_window(g_menu, 0x18, 0x30, 0x1B0, 0xB0, 0xFF);
    font_set_color(g_font, 0);
    font_set_size(g_font, 0x10, 0x10);
    sprintf(buf, D_089A8870, RTEXT(D_089B53A0));
    result_text_center(self, 0x40, buf);
    sprintf(buf, RTEXT(D_089B53E2), g_game[0x44BC] + 1);
    result_text_center(self, 0x70, buf);
    sprintf(buf, D_089A8870, RTEXT(D_089B5408));
    result_text_center(self, 0x60, buf);
    draw_press_button(self, buf);
}

/* text drawn so that its point at c/16 of the width (8: the middle) is at x */
void font_draw_centered(s32 x, s32 y, const char *s, s32 c)
{
    s32 w = font_text_width(g_font, s) * c;

    font_draw_at(g_font, (s16)(x - (w / 2)), (s16)y, s);
}

/* training: points of the quest */
void result_tscore_init(u8 *self)
{
    U16(self, RES_STEP) = 0;
    U16(self, RES_XA6) = 0;
    U16(self, RES_TIMER) = 3600;
    U16(self, RES_STEP_TIMER) = 10;
}

/* points: 0x10C quest, 0x110 monsters, 0x114 time bonus, 0x118 items left, 0x120 total */
void result_tscore_compute(u8 *self)
{
    Quest *q;
    u16 *it;
    u32 i, pct;
    s32 k, m, t1, t0;

    U32(self, 0x10C) = QUEST->file->hrp * 10;
    U32(self, 0x110) = 0;
    for (i = 0; i < 90; i++)
        U32(self, 0x110) += D_089A872C[i] * (u16)(QUEST->count_start[i] - QUEST->count_end[i]);
    q = QUEST;
    m = QUEST->info->time / 9000;
    k = 0;
    if (m > 0 && m < 11)
        k = m - 1;
    t1 = quest_time(QUEST, 1);
    t0 = quest_time(q, 0);
    k *= 3;
    m = (t1 - t0 - 1) / 1800;
    if (m < D_089A8708[k])
        pct = D_089A8728[0];
    else if (m < D_089A8708[k + 1])
        pct = D_089A8728[1];
    else if (m < D_089A8708[k + 2])
        pct = D_089A8728[2];
    else
        pct = 0;
    U32(self, 0x114) = U32(self, 0x10C) * pct / 100;
    U32(self, 0x118) = 0;
    it = func_08856884(g_game, 0);
    for (i = 0; i < 24; i++, it += 2)
        U32(self, 0x118) += (s16)it[1] * D_0899D1A8[it[0] * 6];
    U32(self, RES_POINTS_TOTAL) = U32(self, 0x11C) *
        (U32(self, 0x118) + U32(self, 0x114) + U32(self, 0x10C) + U32(self, 0x110)) / 100;
    if (quest_state(QUEST, 1) == 1)
        U32(self, 0x124) = result_tscore_save(self);
}

/* 1 when closed */
s32 result_tscore_update(u8 *self)
{
    player_get(g_players, G_MY_SLOT);
    if (S16(self, RES_TIMER) > 0)
        S16(self, RES_TIMER)--;
    switch (U16(self, RES_STEP)) {
    case 0: case 1: case 2: case 3: case 4: case 5: case 6:
        if (--S16(self, RES_STEP_TIMER) < 0) {
            S16(self, RES_STEP)++;
            U16(self, RES_STEP_TIMER) = 10;
            if (S16(self, RES_STEP) < 7)
                SE(0x10);
            U32(g_result, RES_VALUE) = 0;
            U16(self, RES_SOUND_ON) = 0;
        }
        break;
    case 7:
        if (U32(self, RES_VALUE) == U32(self, RES_POINTS_TOTAL)) {
            func_088854A0(g_sound, U32(self, RES_SOUND), 0);
            S16(self, RES_STEP)++;
            SE(0x19);
        } else {
            if (S16(self, RES_SOUND_ON) == 0) {
                U32(self, RES_SOUND) = SE(8);
                S16(self, RES_SOUND_ON)++;
            }
            if ((g_pad_pressed & 0x4000) == 0)
                U32(self, RES_VALUE)++;
            else
                U32(self, RES_VALUE) = U32(self, RES_POINTS_TOTAL);
        }
        break;
    case 8:
        if (S16(self, RES_TIMER) < 1 || (game_buttons(g_game) & 0x4000) != 0) {
            SE(0x13);
            return 1;
        }
        break;
    default:
        return 0;
    }
    return 0;
}

static void draw_header(u8 *self, const s16 *src, u16 title, u16 label, u32 value)
{
    s16 dx[7];
    s32 i;
    s16 x;

    font_set_pos(g_font, 0xFC, 0xF2);
    font_draw(g_font, RTEXT(D_089B53C6), S16(self, RES_TIMER) / 30);
    for (i = 0; i < 7; i++)
        dx[i] = src[i];
    x = dx[LANG];
    font_draw_centered((s16)(x + 0xF4), 0xF, RTEXT(D_089B53C8), 0x10);
    font_puts_at(g_font, (s16)(x + 0xF4), 0xF, (const char *)g_game + 0x4A0);
    result_text_center(self, 0x30, RTEXT(title));
    font_draw_centered(0xF4, 0xB2, RTEXT(label), 0x10);
    font_printf_at(g_font, 0xF4, 0xB2, D_089A891C, value);
}

static void draw_record_mark(s32 y)
{
    font_set_color(g_font, 5);
    font_draw_at(g_font, 0x134, y, RTEXT(D_089B5434));
    font_set_color(g_font, 0);
}

static void draw_line(u16 label, s32 y, u32 v)
{
    font_draw_centered(0xF4, y, RTEXT(label), 0x10);
    font_draw_at(g_font, 0xF4, y, D_089A891C, v);
}

void result_tscore_draw(u8 *self)
{
    s32 t;

    U8(g_menu, 0x534) = 9;
    font_set_layer(g_font, 0);
    menu_begin_draw(g_menu);
    result_draw_windows(self);
    font_set_color(g_font, 0);
    font_set_size(g_font, 0x10, 0x10);
    switch (U16(self, RES_STEP)) {
    default:
        if ((U32(self, 0x124) & 2) != 0)
            draw_record_mark(0xB2);
    case 7:
        menu_draw_sprite(g_menu, 0x1C, 0xF0, 0x33);
        font_set_pos(g_font, 0x30, 0xF2);
        font_draw(g_font, RTEXT(D_089B5438));
    case 6:
        font_draw_centered(0xF4, 0x90, RTEXT(D_089B542E), 0x10);
        font_draw_at(g_font, 0x104, 0x90, RTEXT(D_089B5430), U32(self, 0x11C));
    case 5:
        draw_line(D_089B542C, 0x80, U32(self, 0x118));
    case 4:
        draw_line(D_089B542A, 0x70, U32(self, 0x114));
    case 3:
        draw_line(D_089B5428, 0x60, U32(self, 0x110));
    case 2:
        draw_line(D_089B5426, 0x50, U32(self, 0x10C));
    case 1:
        font_draw_centered(0xF4, 0x40, RTEXT(D_089B5424), 0x10);
        t = quest_elapsed_seconds(self);
        font_draw_at(g_font, 0xFC, 0x40, D_089A89F4, t / 60, t % 60);
        if ((U32(self, 0x124) & 1) != 0)
            draw_record_mark(0x40);
    case 0:
        draw_header(self, D_089A89E4, D_089B5422, D_089B5432, U32(g_result, RES_VALUE));
    }
}

/* training: the points are added to the hunter */
void result_tpoints_init(u8 *self)
{
    U16(self, RES_STEP) = 0;
    U16(self, RES_XA6) = 0;
    U16(self, RES_TIMER) = 3600;
    U16(self, RES_STEP_TIMER) = 10;
    U32(self, RES_VALUE) = hrp_scale(self, U32(self, RES_POINTS_TOTAL), 0);
}

s32 result_tpoints_update(u8 *self)
{
    s16 s;

    player_get(g_players, G_MY_SLOT);
    if (S16(self, RES_TIMER) > 0)
        S16(self, RES_TIMER)--;
    s = S16(self, RES_STEP);
    if (s == 4) {
        if (S16(self, RES_TIMER) < 1 || (game_buttons(g_game) & 0x4000) != 0) {
            SE(0x13);
            return 1;
        }
    } else if (s == 3) {
        if (U32(self, RES_VALUE) == 0) {
            func_088854A0(g_sound, U32(self, RES_SOUND), 0);
            S16(self, RES_STEP)++;
            SE(0x19);
        } else {
            if (S16(self, RES_SOUND_ON) == 0) {
                U32(self, RES_SOUND) = SE(8);
                S16(self, RES_SOUND_ON)++;
            }
            if ((g_pad_pressed & 0x4000) == 0) {
                game_add_points(g_game, 1);
                U32(g_result, RES_VALUE)--;
            } else {
                game_add_points(g_game, U32(g_result, RES_VALUE));
                U32(g_result, RES_VALUE) = 0;
            }
        }
    } else {
        if (s != 2 && s != 1 && s != 0)
            return 0;
        s = S16(self, RES_STEP_TIMER) - 1;
        S16(self, RES_STEP_TIMER) = s;
        if (s < 0) {
            S16(self, RES_STEP)++;
            U16(self, RES_STEP_TIMER) = 10;
            if (S16(self, RES_STEP) < 3)
                SE(0x10);
            U16(self, RES_SOUND_ON) = 0;
        }
    }
    return 0;
}

void result_tpoints_draw(u8 *self)
{
    s16 s;

    U8(g_menu, 0x534) = 9;
    font_set_layer(g_font, 0);
    menu_begin_draw(g_menu);
    result_draw_windows(self);
    font_set_color(g_font, 0);
    font_set_size(g_font, 0x10, 0x10);
    s = S16(self, RES_STEP);
    if (s != 0) {
        if (s != 1) {
            if (s != 2) {
                menu_draw_sprite(g_menu, 0x1C, 0xF0, 0x33);
                font_set_pos(g_font, 0x30, 0xF2);
                font_draw(g_font, RTEXT(D_089B5438));
            }
            draw_line(D_089B543C, 0x60, U32(self, RES_VALUE));
        }
        draw_line(D_089B543A, 0x50, U32(self, RES_POINTS_TOTAL));
    }
    draw_header(self, D_089A8A00, D_089B543E, D_089B5418, U32(g_game, 0x696E8));
}

/* record of the training: equipment (4 rarities in base 12) and time or points */
typedef struct TrainRecord {
    u32 equip : 15;
    u32 value : 17;
} TrainRecord;

/* inserts r in the sorted list (n entries); better(a, b): a goes before b */
static int record_insert(TrainRecord *l, s32 n, TrainRecord r, int lower)
{
    s32 i, j;

    for (i = 0; i < n; i++) {
        if (l[i].value == 0) {
            l[i] = r;
            return 1;
        }
        if (lower ? r.value < l[i].value : l[i].value < r.value) {
            for (j = n - 1; j >= i + 1; j--)
                l[j] = l[j - 1];
            l[i] = r;
            return 1;
        }
    }
    return 0;
}

/* the records of the training quest: bit 0 new best time, bit 1 new best points */
u32 result_tscore_save(u8 *self)
{
    TrainRecord r, rt, rp, *lt, *lp;
    u8 *e = g_game + 0x30;
    u32 d[4], k, id;
    s32 i, n, idx;
    u32 ret = 0;

    *(u32 *)&r = 0;
    for (i = 0; i < 4; i++, e += 0x8C) {
        if (e[0] == 0)
            k = 0xB;
        else
            k = (u8)equip_rarity(g_equip_data, e[0x31], U16(e, 0x32));
        d[0] = r.equip % 12;
        d[1] = r.equip / 12 % 12;
        d[2] = r.equip / 144 % 12;
        d[3] = r.equip / 1728 % 12;
        d[i] = k;
        r.equip = d[0] + d[1] * 12 + d[2] * 144 + d[3] * 1728;
    }
    rt = r;
    rt.value = quest_elapsed_seconds(self);
    rp = rt;
    rp.value = U32(self, RES_POINTS_TOTAL);
    id = G_QUEST_ID;
    if ((OPT_FLAGS & 1) == 0) {
        if ((s32)id < 0x5209 || (s32)id > 0x5220)
            return 0;
        lt = (TrainRecord *)(g_game + 0x679A8) + (id - 0x5209);
        lp = (TrainRecord *)(g_game + 0x67B94) + (id - 0x5209);
        n = 1;
    } else {
        if ((s32)id >= 0x55F1 && (s32)id < 0x55F8)
            idx = id - 0x55F1;
        else
            idx = id - 0xEE43;
        lt = (TrainRecord *)(g_game + 0x67A04) + idx * 5;
        lp = (TrainRecord *)(g_game + 0x67BF0) + idx * 5;
        n = 5;
    }
    if (record_insert(lt, n, rt, 1))
        ret = 1;
    if (record_insert(lp, n, rp, 0))
        ret |= 2;
    return ret;
}

/* hunter rank points of the quest; in the training (training != 0) they are not divided */
u32 hrp_scale(u8 *self, u32 v, s32 training)
{
    if (training == 0) {
        quest_flag_10();
        v /= 10;
        if (quest_flag_10() != 0 && (OPT_FLAGS & 1) != 0 && g_net_players[0xBBD] == 2)
            v >>= 1;
    }
    if (has_kitchen_skill(g_game, 0x20) != 0)
        v = v * 150 / 100;
    return v;
}

/* puts back the equipment of before the training quest */
void training_restore_all(void)
{
    u32 h = player_get(g_players, G_MY_SLOT);
    u8 *g = g_game;

    memcpy(g_game + 0x43C8, g_game + 0x6ADFC, 0x60);
    func_088D893C(h);
    memcpy(g + 0x4C0, g + 0x6AE5C, 0xC);
    memcpy(g + 0x4CC, g + 0x6AE68, 0x3C);
}

void training_restore_equip(void)
{
    u32 h = player_get(g_players, G_MY_SLOT);

    memcpy(g_game + 0x43C8, g_game + 0x6ADFC, 0x60);
    func_088D893C(h);
}

/* seconds since the start of the quest, rounded up */
s32 quest_elapsed_seconds(u8 *self)
{
    s32 t1 = quest_time(QUEST, 1);
    s32 t0 = quest_time(QUEST, 0);
    s32 d = t1 - t0;
    s32 s = d / 30;

    if (d % 30 != 0)
        s++;
    return s;
}

void record_add_best(u8 *g, u8 *log, s32 kind, s32 i, u32 v);
void log_push(u8 *g, u8 *e);
int game_test_flag(u8 *g, s32 i);
void game_set_flag(u8 *g, s32 i);
int progress_list_a(u8 *g, s32 i);
int progress_list_b(u8 *g, s32 i);

#define QLOG ((u8 *)g_quest + 0x684)
#define LOG(kind, i, v) record_add_best(g_game, QLOG, kind, i, v)

/* the notable events of the quest for the hunter's log (diary) */
void result_log_events(u8 *self)
{
    u32 n, t, f;
    s32 i, s;
    u8 *h;
    s16 kind;

    if (S16(g_quest, 0x688) != 0) {
        if (S8(g_quest, 0x67D) >= 4)
            LOG(0xB, 0, 0);
        else if (S8(g_quest, 0x680) >= 13)
            LOG(0xC, 0, 0);
        else if (S8(g_quest, 0x681) >= 8)
            LOG(0xD, 0, 0);
        else if (S8(g_quest, 0x67E) >= 5)
            LOG(0xE, 0, 0);
        else if (S8(g_quest, 0x67F) >= 10)
            LOG(0xF, 0, 0);
        if (S16(g_quest, 0x682) != 0)
            LOG(7, S16(g_quest, 0x682), 0);
    }
    n = g_game[0x6ADD0];
    if ((n + 1) * 100 <= U32(g_game, 0x4468) / 3600 && n != 0xFF) {
        g_game[0x6ADD0] = n + 1;
        LOG(0x14, g_game[0x6ADD0], 0);
    }
    kind = S16(g_quest, 0x688);
    if (kind == 1) {
    } else if (kind == 0) {
        if (g_game[0x6A5EA] % 10 == 0)
            LOG(0x1C, g_game[0x6A5EA], 0);
    } else if (kind == 4) {
        if (quest_elapsed_seconds(self) / 60 < 5)
            LOG(5, 0, 0);
        if (quest_time(QUEST, 0) / 30 / 60 == 0)
            LOG(6, 0, 0);
        t = U16(g_game, 0x67778) + U16(g_game, 0x6777A) + U16(g_game, 0x6777C) +
            U16(g_game, 0x6777E) + U16(g_game, 0x67780) + U16(g_game, 0x67782) +
            U16(g_game, 0x67784);
        if (t != 0 && t % 100 == 0)
            LOG(0x16, t / 100, 0);
        h = (u8 *)player_get(g_players, G_MY_SLOT);
        if (S16(h, 0x4F6) == 0 && S16(h, 0x526) == 0 && S16(h, 0x51A) == 0 &&
            S16(h, 0x502) == 0 && S16(h, 0x50E) == 0)
            LOG(4, 0, 0);
        f = 0x1D6;
        i = 0;
        do {
            if ((u8)game_test_flag(g_game, f) == 0 && (u8)progress_list_a(g_game, i) == 1) {
                LOG(0x1A, i, 0);
                game_set_flag(g_game, f);
            }
            f++;
            i++;
        } while (f < 0x1DF);
        for (i = 0; f < 0x1EA; f++, i++) {
            if ((u8)game_test_flag(g_game, f) == 0 && (u8)progress_list_b(g_game, i) == 1) {
                LOG(0x1D, i, 0);
                game_set_flag(g_game, f);
            }
        }
    }
    if (S16(g_quest, 0x688) != 0) {
        h = (u8 *)player_get(g_players, G_MY_SLOT);
        f = h[0x113E];
        if (f != 0) {
            if (f == 2)
                LOG(3, 0, 2);
            else if (f == 1)
                LOG(1, 0, 1);
            else
                LOG(0x12, 0, f);
            U32(g_game, 0x6AD78) += f;
            if (U32(g_game, 0x6AD78) >= 1000000000)
                U32(g_game, 0x6AD78) = 999999999;
            if (f + (U32(g_game, 0x6AD78) - f) % 100 >= 100)
                LOG(0x17, U32(g_game, 0x6AD78) / 100, 0);
        }
        n = 0;
        for (s = 0; s < 4; s++) {
            h = (u8 *)player_get(g_players, s);
            if (h == NULL || U16(h, 0x1E6) == G_MY_SLOT || h[0x1E8] == 0xC)
                continue;
            memcpy((u8 *)g_quest + n * 0x12 + 0x6A8,
                   g_net_players + (U16(h, 0x1E6) & 0xFF) * 200 + 0xBC8, 0x12);
            f = h[0x113E];
            n = (n + 1) & 0xFF;
            if (f == 0)
                continue;
            if (f == 2)
                LOG(3, n, f);
            else if (f == 1)
                LOG(1, n, f);
            else
                LOG(0x12, n, f);
        }
    }
    sprintf((char *)g_quest + 0x6DE, (const char *)quest_text(QUEST, 0));
    log_push(g_game, QLOG);
}
