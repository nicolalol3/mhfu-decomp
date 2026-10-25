/* Result screens at the end of the quest (2): unlocked contents, size records, special
 * conditions; helpers of all the result screens. The object is g_result. (Guessed names.) */
#include "quest.h"
#include "result.h"
#undef menu_draw_sprite
#undef gfx_draw_window
#undef text_menu
#undef pouch_slot

extern void *D_09A44C5C;
extern const char D_089A8870[];    /* "%s" */
extern const u16 D_089B53A0[], D_089B53A2, D_089B53A6, D_089B53AA, D_089B53AE, D_089B53B0;
extern const u16 D_089B53B6, D_089B53EC, D_089B53EE, D_089B53F2, D_089B539C, D_089B539E;
extern const u16 D_089ACAF8[];     /* names of the unlocks */
extern const u16 D_089AD0BC[];     /* kinds and names of the special conditions */
extern const u16 D_089AC63A, D_089AC65C[];
extern const s16 D_089A88A0[7];
extern s16 D_089A8878, D_089A887C, D_089A8898;
extern const s16 D_089A8874, D_089A8876, D_089A887A, D_089A887E, D_089A8880, D_089A8882;
extern const s16 D_089A8884, D_089A8886, D_089A8888, D_089A888A, D_089A888C, D_089A888E;
extern const s16 D_089A8890, D_089A8892, D_089A8894, D_089A8896, D_089A889A, D_089A889C;
extern const s16 D_089A889E;
extern const u16 D_089A88C8[4], D_089A88D0[4], D_089A88D8[4], D_089A88E0[4], D_089A88E8[4];
extern const u8 D_089A88B0[24];
extern const s32 D_0899D1A8[];

const char *text_menu(void *g, u32 id);
void menu_begin_draw(void *menu);
void menu_draw_sprite(void *menu, u32 x, u32 y, u32 id);
void gfx_dim_screen(void *menu, int a);
void gfx_draw_window(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 alpha);
void gfx_draw_sprite_scaled(u8 *self, u32 x, u32 y, u32 w, u32 h, s32 id);
void emblem_draw(u8 *self, u32 x, u32 y, u32 u, u32 v);
void menu_draw_rating_icon(void *menu, u8 *em, s32 x, s32 y, s32 v);
void text_draw_lines_block(void *menu, int x, int y, const char *s, int w, int h);
u16 *pouch_slot(u8 *g, s32 i);
u8 *emblem_find(void *menu, u32 i);
f32 game_size_percent(void *g, u32 id, u32 v);
f32 game_size_max(u8 *g, u32 id, s32 mode);
f32 game_size_min(u8 *g, u32 id, s32 mode);
u16 game_best_record_max(u8 *g, u32 i, s32 a);
u16 game_best_record_min(u8 *g, u32 i, s32 a);
u32 game_group_count_total(u8 *g, u32 id);
u16 game_count_total(u8 *g, u32 i);
void game_add_count_a(u8 *g, u32 i, s32 n);
void game_add_count_b(u8 *g, u32 i, s32 n);
int game_has_progress(u8 *g, u32 f);
int game_test_flag_696e4(u8 *g);
int event_quest_any_done(u8 *g, u32 id);
s32 flag_id_at(u8 *g, s32 i);
int game_rank(u8 *g);
void record_add_best(u8 *g, u8 *log, s32 kind, s32 i, u32 v);
void result_size_records(u8 *self);
void result_init_sounds(u8 *self);
u8 *pouch_item_data(void *a, s32 i);
u32 hunter_add_item(u32 h, s32 id, s32 n, s32 c);
void func_08890114(void *a);
void font_set_layer(void *font, int a);
void font_set_pos(void *font, int x, int y);
void font_draw(void *font, const char *fmt, ...);
s32 font_text_width(void *font, const char *s);
void font_draw_centered(s32 x, s32 y, const char *s, s32 c);

#define RTEXT(id) func_09A63198(D_09A44C5C, id)

void result_text_center(u8 *self, s32 y, const char *s);
s32 result_rating(u8 *self, u8 *em, s32 v);
u32 result_special_check(u8 *self, u32 i);

static void begin_window(void)
{
    U8(g_menu, 0x534) = 9;
    font_set_layer(g_font, 0);
    menu_begin_draw(g_menu);
    gfx_dim_screen(g_menu, 0);
    gfx_draw_window(g_menu, 0x18, 0x30, 0x1B0, 0xB0, 0xFF);
}

static void draw_title(u8 *self, char *buf)
{
    sprintf(buf, D_089A8870, RTEXT(D_089B53A0[0]));
    result_text_center(self, 0x40, buf);
}

/* "press" button and the timer at the bottom of the window */
static void draw_bottom(u8 *self, char *buf, int show)
{
    if (show) {
        sprintf(buf, RTEXT(D_089B53AE));
        font_set_pos(g_font, 400, 0xC0);
        font_draw(g_font, D_089A8870, buf);
        menu_draw_sprite(g_menu, 0x17E, 0xBF, 0x33);
    }
    font_set_pos(g_font, 0x40, 0xC0);
    font_draw(g_font, RTEXT(D_089B53EC), S16(self, RES_XAC) / 30);
}

/* screen of the new unlocked contents */
void result_unlock_draw(u8 *self)
{
    char buf[0x100];
    s16 n = 0;
    s32 i, f;

    begin_window();
    font_set_color(g_font, 0);
    font_set_size(g_font, 0x10, 0x10);
    draw_title(self, buf);
    text_draw_lines_block(g_menu, 0xF0, 0x60, RTEXT(D_089B53A2), 0x10, 0x10);
    for (i = 0; i < 0x3D; i++) {
        if (quest_unlock_check((Quest *)self, i) == 0)
            continue;
        f = flag_id_at(g_game, (s8)i);
        if ((f < 0 || f >= 0xB) && f < 0x39)
            sprintf(buf, RTEXT(D_089B539E), text_menu(g_game, D_089ACAF8[i]));
        else
            sprintf(buf, RTEXT(D_089B539C), text_menu(g_game, D_089ACAF8[i]));
        result_text_center(self, (s16)((n + 4) * 0x10 + 0x40), buf);
        n++;
    }
    draw_bottom(self, buf, self[RES_SND1] == 0);
}

static void set_rating_color(u8 *self, u8 *em, s32 v)
{
    s16 r = result_rating(self, em, (s16)v);

    if (r == 2)
        font_set_color(g_font, 7);
    else if (r == 1)
        font_set_color(g_font, 0xB);
    else
        font_set_color(g_font, 0);
}

/* screen of a new size record (largest / smallest) of a monster */
void result_record_draw(u8 *self)
{
    u8 *e = self + RES_RECORDS + self[RES_RECORD_SHOWN] * 4;
    u8 *em = emblem_find(g_menu, e[0]);
    s16 t[7];
    char buf[0x100];
    const u16 *txt;
    s32 i, k;
    u16 best;
    f32 f;

    font_set_color(g_font, 0);
    font_set_size(g_font, 0x10, 0x10);
    for (i = 0; i < 7; i++)
        t[i] = D_089A88A0[i];
    D_089A8878 = t[LANG] + 0x13C;
    D_089A887C = D_089A8878;
    D_089A8898 = D_089A8878;
    begin_window();
    result_text_center(self, D_089A8874, RTEXT(D_089B53AA));
    sprintf(buf, text_menu(g_game, D_089AC63A), text_menu(g_game, D_089AC65C[e[0]]));
    result_text_center(self, D_089A8876, buf);
    set_rating_color(self, em, (s16)U16(e, 2));
    k = e[1] == 0 ? 6 : e[1] == 1 ? 9 : 10;
    txt = &D_089B53A0[k];
    font_draw_centered((u16)D_089A8878, (u16)D_089A887A, RTEXT(*txt), 0x10);
    f = game_size_percent(g_game, e[0], U16(e, 2)) + 0.05f;
    if (U16(e, 2) == 0)
        sprintf(buf, RTEXT(D_089B53B0));
    else
        sprintf(buf, RTEXT(D_089B53B6), (s32)f, (s32)(10.0f * f) % 10);
    font_printf_at(g_font, D_089A8878, D_089A887A, buf);
    best = e[1] == 2 ? game_best_record_min(g_game, e[0], 0) : game_best_record_max(g_game, e[0], 0);
    set_rating_color(self, em, (s16)best);
    font_draw_centered((u16)D_089A887C, (u16)D_089A887E, RTEXT(*txt), 0x10);
    f = (e[1] == 2 ? game_size_min(g_game, e[0], 1) : game_size_max(g_game, e[0], 1)) + 0.05f;
    font_printf_at(g_font, D_089A887C, D_089A887E, RTEXT(D_089B53B6), (s32)f,
                  (s32)(10.0f * f) % 10);
    font_set_color(g_font, 0);
    if (self[RES_SND1] == 0 && S16(self, RES_STEP_TIMER) <= 0) {
        font_printf_at(g_font, D_089A8884, D_089A8886, RTEXT(D_089B53AE));
        menu_draw_sprite(g_menu, (u16)(D_089A8884 - 0x12), (u16)(D_089A8886 - 1), 0x33);
    }
    font_draw_at(g_font, D_089A8880, D_089A8882, RTEXT(D_089B53EC), S16(self, RES_XAC) / 30);
    gfx_draw_sprite_scaled(g_menu, (u16)D_089A8888, (u16)D_089A888A, 0x50, 0x40, 0xAF);
    gfx_draw_sprite_scaled(g_menu, (u16)D_089A8898, (u16)D_089A889A, (u16)D_089A889C,
                           (u16)D_089A889E, 0x9C);
    emblem_draw(g_menu, (u16)D_089A888C, (u16)D_089A888E, em[1], em[2]);
    menu_draw_rating_icon(g_menu, em, D_089A8890, D_089A8892, (s16)U16(e, 2));
    best = e[1] == 2 ? game_best_record_min(g_game, e[0], 0) : game_best_record_max(g_game, e[0], 0);
    menu_draw_rating_icon(g_menu, em, D_089A8894, D_089A8896, (s16)best);
}

/* special conditions reached in this quest (shown at the end) */
u32 result_special_check(u8 *self, u32 i)
{
    u16 q = G_QUEST_ID;

    if (game_test_flag_696e4(g_game) != 0)
        return 0;
    switch (i) {
    case 0: return q == 0x277E;
    case 1: return q == 0x277F;
    case 2: return q == 0x27DB;
    case 3: return q == 0x27E2;
    case 4: return q == 0x283E;
    case 5: return q == 0x2840;
    case 6: return q == 0x28A1;
    case 7: return q == 0x28A6;
    case 8: return q == 0x28AB;
    case 9: return q == 0x28A2;
    case 10:
        return (s16)game_group_count_total(g_game, 8) != 0 &&
               event_quest_any_done(g_game, 0x5209) != 0 && event_quest_any_done(g_game, 0x520A) != 0;
    case 11:
        return (s16)game_group_count_total(g_game, 0x15) != 0 &&
               event_quest_any_done(g_game, 0x520B) != 0 && event_quest_any_done(g_game, 0x520C) != 0;
    case 12:
        return (s16)game_group_count_total(g_game, 0x43) != 0 &&
               event_quest_any_done(g_game, 0x520D) != 0 && event_quest_any_done(g_game, 0x520E) != 0;
    case 13:
        return (s16)game_group_count_total(g_game, 0xB) != 0 &&
               event_quest_any_done(g_game, 0x520F) != 0 && event_quest_any_done(g_game, 0x5210) != 0;
    case 14:
        return (s16)game_group_count_total(g_game, 0x35) != 0 &&
               event_quest_any_done(g_game, 0x5211) != 0 && event_quest_any_done(g_game, 0x5212) != 0;
    case 15:
        return game_has_progress(g_game, 0x3F7) != 0 && game_count_total(g_game, 8) > 4;
    case 16:
        return game_has_progress(g_game, 0x4B9) != 0 && (u16)game_group_count_total(g_game, 1) > 9;
    case 17:
        return game_has_progress(g_game, 0x845) != 0 && (u16)game_group_count_total(g_game, 0x43) > 4;
    case 18:
        return game_has_progress(g_game, 0xC82) != 0 && game_count_total(g_game, 0x4B) > 9;
    case 19: return game_has_progress(g_game, 0xBC1) != 0;
    case 20: return game_has_progress(g_game, 0xBCC) != 0;
    case 21: return game_has_progress(g_game, 0xBC4) != 0;
    case 22: return game_has_progress(g_game, 0xC1F) != 0;
    case 23: return game_has_progress(g_game, 0xC31) != 0;
    case 24: return game_has_progress(g_game, 0xC22) != 0;
    case 25: return game_has_progress(g_game, 0xC26) != 0;
    case 26: return game_has_progress(g_game, 0xC8C) != 0;
    }
    return 0;
}

u32 result_any_special(u8 *self)
{
    s32 i;

    for (i = 0; i < 0x1B; i++)
        if (result_special_check(self, i) != 0)
            return 1;
    return 0;
}

void result_unlock_init_timers(u8 *self)
{
    U16(self, RES_STEP_TIMER) = 0x5A;
    U16(self, RES_TIMER) = 1;
    U16(self, RES_XAC) = 900;
    result_init_sounds(self);
}

/* screen of the special conditions (at most 2) */
void result_special_draw(u8 *self)
{
    char buf[0x100];
    s16 y = 0x70;
    s32 i, n = 0, k;

    begin_window();
    font_set_color(g_font, 0);
    font_set_size(g_font, 0x10, 0x10);
    draw_title(self, buf);
    result_text_center(self, 0x60, RTEXT(D_089B53A6));
    for (i = 0; i < 0x1B; i++) {
        if (result_special_check(self, i) != 0) {
            k = i < 10 ? 0 : i < 15 ? 1 : i < 19 ? 2 : 3;
            result_text_center(self, y, text_menu(g_game, D_089AD0BC[k]));
            result_text_center(self, (s16)(y + 0x10), text_menu(g_game, D_089AD0BC[i + 4]));
            y += 0x20;
            n++;
        }
        if (n >= 2)
            break;
    }
    draw_bottom(self, buf, self[RES_SND1] == 0 && S16(self, RES_XAC) < 0x32B);
    font_set_color(g_font, 5);
    sprintf(buf, D_089A8870, RTEXT(D_089B53EE));
    result_text_center(self, y, buf);
}

void result_special_init_timers(u8 *self)
{
    U16(self, RES_STEP_TIMER) = 0x5A;
    U16(self, RES_TIMER) = 1;
    U16(self, RES_XAC) = 900;
    result_init_sounds(self);
}

/* screen "the comrade / bonus" with a block of text */
void result_text_draw(u8 *self)
{
    char buf[0x100];

    begin_window();
    font_set_color(g_font, 0);
    font_set_size(g_font, 0x10, 0x10);
    draw_title(self, buf);
    text_draw_lines_block(g_menu, 0xF0, 0x60, RTEXT(D_089B53F2), 0x10, 0x10);
    draw_bottom(self, buf, self[RES_SND1] == 0 && S16(self, RES_XAC) < 0x32B);
}

/* total value of the 32 rewards */
s32 result_rewards_value(void)
{
    s32 i, sum = 0;
    u16 *p = &U16(g_game, 0x390);

    for (i = 0; i < 0x20; i++, p += 2)
        if (p[0] != 0)
            sum += (s16)p[1] * D_0899D1A8[p[0] * 6];
    return sum;
}

s32 result_rewards_count(void)
{
    s32 i, n = 0;
    s16 *p = &S16(g_game, 0x390);

    for (i = 0; i < 0x20; i++, p += 2)
        if (p[0] != 0)
            n += p[1];
    return n;
}

/* end of the quest: items used up, hunting counters, records, rank */
void result_finish(u8 *self)
{
    u32 h = player_get(g_players, G_MY_SLOT);
    Quest *q;
    u8 t[24];
    s32 i, j;
    u32 n, step, c, tot;
    u16 *p, *s;

    func_08890114(g_sys);
    U32(g_game, 0x47C) = 0;
    for (i = 0; i < 0x18; i++) {
        if ((pouch_item_data(D_08A62C50, i)[4] & 4) == 0)
            continue;
        p = pouch_slot(g_game, i);
        s = pouch_slot(g_game, i);
        hunter_add_item(h, p[0], (s16)-(s16)s[1], 0);
    }
    for (i = 0; i < 0x5A; i++) {
        q = QUEST;
        n = q->kills[i] + q->captures[i];
        if (n != 0 && i != 0x1D) {
            for (j = 0; j < 24; j++)
                t[j] = D_089A88B0[j];
            step = 10;
            for (j = 0; t[j] != 0; j++) {
                if (t[j] == (u32)i) {
                    step = 0x32;
                    break;
                }
            }
            c = game_group_count_total(g_game, i & 0xFF) & 0xFFFF;
            tot = c + n;
            if (tot < 9999 && n + c % step >= step)
                record_add_best(g_game, QUEST->log, 0x15, i, (step / 10) * (tot / step));
        }
        q = QUEST;
        if (q->kills[i] != 0)
            game_add_count_a(g_game, i & 0xFF, q->kills[i]);
        q = QUEST;
        if (q->captures[i] != 0)
            game_add_count_b(g_game, i & 0xFF, q->captures[i]);
    }
    result_size_records(self);
    self[RES_RANK] = game_rank(g_game);
    self[RES_SND1] = 0;
}

/* text centered on the screen */
void result_text_center(u8 *self, s32 y, const char *s)
{
    s32 w = font_text_width(g_font, s);

    font_draw_at(g_font, (s16)(0xF0 - ((w << 3) >> 1)), y, D_089A8870, s);
}

/* the 5 windows of the result screens */
void result_draw_windows(u8 *self)
{
    gfx_dim_screen(g_menu, 0);
    gfx_draw_window(g_menu, D_089A88D0[0], D_089A88D0[1], D_089A88D0[2], D_089A88D0[3], 0xFF);
    gfx_draw_window(g_menu, D_089A88C8[0], D_089A88C8[1], D_089A88C8[2], D_089A88C8[3], 0xFF);
    gfx_draw_window(g_menu, D_089A88D8[0], D_089A88D8[1], D_089A88D8[2], D_089A88D8[3], 0xFF);
    gfx_draw_window(g_menu, D_089A88E0[0], D_089A88E0[1], D_089A88E0[2], D_089A88E0[3], 0xFF);
    gfx_draw_window(g_menu, D_089A88E8[0], D_089A88E8[1], D_089A88E8[2], D_089A88E8[3], 0xFF);
}

/* crown of a size: 2 = gold (smallest / largest), 1 = silver, 0 = none */
s32 result_rating(u8 *self, u8 *em, s32 v)
{
    v = (s16)v;
    if (v != 0) {
        if (!(S16(em, 8) < v))
            return 2;
        if (!(v < S16(em, 6)))
            return 2;
    }
    return v >= S16(em, 4);
}
