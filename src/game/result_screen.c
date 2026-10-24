/* Result screens at the end of the quest: reward money, sold items (with the counting
 * animation). The object is D_08A62CB4. (Guessed names.) */
#include "quest.h"

extern u8 *D_08A62CB4;
extern u8 *D_08A62D8C;             /* sound */
extern u32 D_08A62C6C;             /* buttons pressed */
extern void *D_09A44C5C;           /* texts of the result screen (overlay) */
extern const u16 D_089B53B8, D_089B53BA, D_089B53BE, D_089B53C0, D_089B53C2, D_089B53C4;
extern const u16 D_089B53C6, D_089B53C8, D_089B53CA, D_089B53EA, D_089B5412, D_089B5414;
extern const u16 D_089B5416, D_089B5418, D_089B541A, D_089B541C, D_089B541E, D_089B5420;
extern const s16 D_089A87E0, D_089A87E2, D_089A87E4, D_089A87E6;
extern const s16 D_089A87E8[7], D_089A87F8[7], D_089A8850[7], D_089A8860[7];
extern const s16 D_089A8808, D_089A880A, D_089A880C;
extern const s16 D_089A8820, D_089A8822, D_089A8824, D_089A8826, D_089A8828, D_089A882A;
extern const u16 D_089A882C, D_089A882E;
extern const s16 D_089A8838, D_089A883A, D_089A8842, D_089A8846;
extern const s32 D_0899D1A8[];     /* item table (+0xC: price), 6 words per item */

u32 sound_play(void *snd, int a, int b, int c, int d, int e, int f, int g, int h);
void func_088854A0(u8 *snd, u32 h, int a);
u32 game_buttons(u8 *g);
void game_add_money(u8 *g, s32 n);
void game_add_points(u8 *g, s32 n);
void menu_begin_draw(void *menu);
void menu_draw_sprite(void *menu, u32 x, u32 y, u32 id);
void gfx_draw_item_icon_alpha(void *menu, u32 id, u32 x, u32 y, u32 w, u32 h, int a, int b);
void gfx_draw_arrows_blink(void *menu, u32 a, u32 b, u32 c, u32 d);
const char *text_item_name(void *g, u32 id);
u8 hunter_felyne_skill(u32 h, int a);
u8 func_0887CA08(u8 *self);
s32 func_0887E9AC(u8 *a, s32 b, s32 c);
void func_08890E08(void *font, int a);
void func_08890DFC(void *font, int x, int y);
void func_08892004(void *font, const char *fmt, ...);
void func_088912BC(void *font, int x, int y, const char *s);
void func_0887D02C(s32 x, s32 y, const char *s, s32 c);
void result_draw_windows(u8 *self);
void result_text_center(u8 *self, s32 y, const char *s);
void result_init_sounds(u8 *self);

/* sound (with the 8 empty arguments of the sound function) */
#define SE(id) sound_play(D_08A62D8C, 0, id, 0, 0, 0, 0, 0, 0)
#define RTEXT(id) func_09A63198(D_09A44C5C, id)

/* waits a step of 10 frames; 1 when the step ends */
static int step_wait(u8 *self)
{
    S16(self, 0xA8)--;
    if (S16(self, 0xA8) >= 0)
        return 0;
    S16(self, 0xA4)++;
    U16(self, 0xA8) = 10;
    return 1;
}

/* screen of the reward money: 1 when finished */
s32 result_money_update(u8 *self)
{
    u32 h;

    player_get(D_09A4F0C4, D_089CC438[0x28]);
    if (S16(self, 0xAA) > 0)
        S16(self, 0xAA)--;
    switch (U16(self, 0xA4)) {
    case 0:
        if (step_wait(self))
            SE(0x10);
        break;
    case 1:
    case 2:
        if (step_wait(self))
            SE(0x11);
        break;
    case 3:
        if (step_wait(self))
            SE(0x13);
        break;
    case 4:
        if (step_wait(self))
            U16(self, 0xAE) = 0;
        break;
    case 5:
        if (U32(D_08A62CB4, 0x14) == 0) {
            func_088854A0(D_08A62D8C, U32(self, 0xC8), 0);
            S16(self, 0xA4)++;
            SE(0x19);
            break;
        }
        if (S16(self, 0xAE) == 0) {
            h = SE(8);
            U32(self, 0xC8) = h;
            S16(self, 0xAE)++;
        }
        if ((D_08A62C6C & 0x4000) == 0) {
            game_add_money(D_089CC438, 1);
            U32(D_08A62CB4, 0x14)--;
        } else {
            game_add_money(D_089CC438, U32(D_08A62CB4, 0x14));
            U32(D_08A62CB4, 0x14) = 0;
        }
        break;
    case 6:
        if (S16(self, 0xAA) <= 0 || (game_buttons(D_089CC438) & 0x4000) != 0) {
            SE(0x13);
            return 1;
        }
        break;
    default:
        return 0;
    }
    return 0;
}

/* common bottom of the result screens: timer, hunter name, money */
static void draw_footer(u8 *self, const s16 *tbl, s32 lang)
{
    s16 t[7];
    s32 i;
    s16 x;

    func_08890DFC(D_08A631A8, 0xFC, 0xF2);
    func_08892004(D_08A631A8, RTEXT(D_089B53C6), S16(self, 0xAA) / 30);
    for (i = 0; i < 7; i++)
        t[i] = tbl[i];
    x = t[lang] + 0xF4;
    func_0887D02C(x, 0xF, RTEXT(D_089B53C8), 0x10);
    func_088912BC(D_08A631A8, x, 0xF, (const char *)(D_089CC438 + 0x4A0));
    func_0887D02C(0xF4, 0xB2, RTEXT(D_089B53CA), 0x10);
    func_08891474(D_08A631A8, 0xF4, 0xB2, RTEXT(D_089B541A), U32(D_089CC438, 0x696F0));
}

static s32 draw_header(u8 *self, const s16 *tbl, s16 *t)
{
    s32 i;

    U8(D_089CBBEC, 0x534) = 9;
    func_08890E08(D_08A631A8, 0);
    menu_begin_draw(D_089CBBEC);
    result_draw_windows(self);
    func_08890E38(D_08A631A8, 0);
    func_08890DE8(D_08A631A8, 0x10, 0x10);
    for (i = 0; i < 7; i++)
        t[i] = tbl[i];
    return LANG;
}

static void draw_press_button(s16 x)
{
    menu_draw_sprite(D_089CBBEC, (u16)(x + 0x1C), 0xF0, 0x33);
    func_08890DFC(D_08A631A8, (s16)(x + 0x30), 0xF2);
    func_08892004(D_08A631A8, RTEXT(D_089B53B8));
}

/* reward money screen (the lines appear one after the other) */
void result_money_draw(u8 *self)
{
    s16 t[7];
    s32 lang = draw_header(self, D_089A87E8, t);
    u32 v;
    char buf[0x100];

    switch (S16(self, 0xA4)) {
    default:
        draw_press_button(t[lang]);
        /* fall through */
    case 4:
    case 5:
        func_0887D02C(0xF4, D_089A87E6, RTEXT(D_089B53BA), 0x10);
        func_08891474(D_08A631A8, 0xF4, D_089A87E6, RTEXT(D_089B541A), U32(self, 0x14));
        /* fall through */
    case 3:
        v = 0;
        if (quest_state(QUEST, 1) == 1) {
            if ((OPT_FLAGS & 1) == 0 || (u8)func_0887CA08(self) == 1)
                v = QUEST->info->fee;
        }
        func_0887D02C(0xF4, D_089A87E4, RTEXT(D_089B53BE), 0x10);
        func_08891474(D_08A631A8, 0xF4, D_089A87E4, RTEXT(D_089B541E), v);
        /* fall through */
    case 2:
        v = 0;
        if (quest_state(QUEST, 1) == 1) {
            u32 h = player_get(D_09A4F0C4, D_089CC438[0x28]);

            if (hunter_felyne_skill(h, 0x31) == 0)
                v = QUEST->reward;
            else
                v = QUEST->reward << 1;
        }
        func_0887D02C(0xF4, D_089A87E2, RTEXT(D_089B53C0), 0x10);
        func_08891474(D_08A631A8, 0xF4, D_089A87E2, RTEXT(D_089B541A), v);
        /* fall through */
    case 1:
        if (quest_state(QUEST, 1) == 1)
            sprintf(buf, RTEXT(D_089B53C2));
        else
            sprintf(buf, RTEXT(D_089B53C4));
        result_text_center(self, 0x30, buf);
        result_text_center(self, D_089A87E0, (const char *)quest_text(QUEST, 0));
        /* fall through */
    case 0:
        draw_footer(self, D_089A87F8, lang);
        break;
    }
}

/* prices of the sold items (list at 0x28, count at 0x24) */
static s32 sold_total(u8 *self)
{
    u16 *p = &U16(self, 0x28);
    s32 i, sum = 0;

    for (i = 0; i < (s32)U32(self, 0x24); i++, p += 2) {
        if (p[0] == 0)
            break;
        sum += (s16)p[1] * D_0899D1A8[p[0] * 6];
    }
    return sum;
}

void result_sell_init(u8 *self)
{
    U16(self, 0xA4) = 0;
    U16(self, 0xA6) = 0;
    U16(self, 0xAA) = 3600;
    U32(D_08A62CB4, 0x14) = 0;
    D_08A62CB4[0x1C] = 0;
    U16(self, 0xA8) = 10;
    U32(self, 0x14) = func_0887E9AC(D_08A62CB4, sold_total(self), 1);
    U32(self, 0x10) = sold_total(self);
    D_08A62CB4[0x1D] = D_08A62CB4[0x1C];
    U32(self, 0x20) = 0;
}

/* screen of the sold items: pages with left / right, then counting of money and points */
s32 result_sell_update(u8 *self)
{
    s32 old, pages, h;

    player_get(D_09A4F0C4, D_089CC438[0x28]);
    if (S16(self, 0xAA) > 0)
        S16(self, 0xAA)--;
    if (S16(self, 0xA4) >= 2) {
        old = U32(self, 0x20);
        pages = ((s32)U32(self, 0x24) - 1) / 10;
        if ((game_buttons(D_089CC438) & 0x80) != 0) {
            if ((s32)--U32(self, 0x20) < 0)
                U32(self, 0x20) = pages;
        } else if ((game_buttons(D_089CC438) & 0x20) != 0) {
            if (pages < (s32)++U32(self, 0x20))
                U32(self, 0x20) = 0;
        }
        if (old != (s32)U32(self, 0x20))
            SE(0x11);
    }
    switch (U16(self, 0xA4)) {
    case 0:
        if (step_wait(self))
            SE(0x10);
        break;
    case 1:
        if (step_wait(self))
            SE(0x11);
        break;
    case 2:
        S16(self, 0xA8)--;
        if (S16(self, 0xA8) < 0) {
            if (--D_08A62CB4[0x1C] == 0) {
                U16(self, 0xA4) = 7;
                SE(0x13);
            } else {
                S16(self, 0xA4)++;
                SE(0x11);
            }
            U16(self, 0xA8) = 10;
        }
        break;
    case 3:
        if (step_wait(self))
            U16(self, 0xAE) = 0;
        break;
    case 4:
        if ((s32)U32(D_08A62CB4, 0x10) <= 0 && (s32)U32(D_08A62CB4, 0x14) <= 0) {
            func_088854A0(D_08A62D8C, U32(self, 0xC8), 0);
            S16(self, 0xA4)++;
            SE(0x19);
            return 0;
        }
        if (S16(self, 0xAE) == 0) {
            h = SE(8);
            U32(self, 0xC8) = h;
            S16(self, 0xAE)++;
        }
        if ((D_08A62C6C & 0x4000) == 0) {
            if ((s32)U32(D_08A62CB4, 0x10) > 0) {
                U32(D_08A62CB4, 0x10)--;
                game_add_money(D_089CC438, 1);
            }
            if ((s32)U32(D_08A62CB4, 0x14) > 0) {
                game_add_points(D_089CC438, 1);
                U32(D_08A62CB4, 0x14)--;
            }
        } else {
            game_add_money(D_089CC438, U32(D_08A62CB4, 0x10));
            game_add_points(D_089CC438, U32(D_08A62CB4, 0x14));
            U32(D_08A62CB4, 0x10) = 0;
            U32(D_08A62CB4, 0x14) = 0;
        }
        break;
    case 5:
        if (S16(self, 0xAA) <= 0 || (game_buttons(D_089CC438) & 0x4000) != 0) {
            SE(0x13);
            return 1;
        }
        break;
    default:
        return 0;
    }
    return 0;
}

void result_sell_draw(u8 *self)
{
    s16 t[7];
    s32 lang = draw_header(self, D_089A8850, t);
    s32 c, r;
    u16 *p;
    char buf[0x100];

    switch (S16(self, 0xA4)) {
    default:
        draw_press_button(t[lang]);
        /* fall through */
    case 3:
    case 4:
        func_0887D02C(0xF4, D_089A8846, RTEXT(D_089B5414), 0x10);
        func_08891474(D_08A631A8, 0xF4, D_089A8846, RTEXT(D_089B5416), U32(self, 0x10),
                      U32(self, 0x14));
        /* fall through */
    case 2:
        func_08890DE8(D_08A631A8, 0xE, 0xE);
        p = &U16(self, 0x28 + U32(self, 0x20) * 0x28);
        for (c = 0; c < 2; c++) {
            for (r = 0; r < 5; r++, p += 2) {
                if (p[0] == 0)
                    continue;
                gfx_draw_item_icon_alpha(D_089CBBEC, p[0], (u16)(D_089A8820 + D_089A8828 * c),
                                         (u16)(D_089A8822 + D_089A882A * r), D_089A882C,
                                         D_089A882E, 1, 0xFF);
                func_08891474(D_08A631A8, (s16)(D_089A8824 + D_089A8828 * c),
                              (s16)(D_089A8826 + D_089A882A * r),
                              text_item_name(D_089CC438, p[0]));
                func_08891474(D_08A631A8, (s16)(D_089A8838 + D_089A8828 * c),
                              (s16)(D_089A883A + D_089A882A * r), RTEXT(D_089B5412),
                              D_0899D1A8[p[0] * 6 + 1], (s16)p[1]);
            }
        }
        func_08890DE8(D_08A631A8, 0x10, 0x10);
        if ((s32)U32(self, 0x24) > 10) {
            gfx_draw_arrows_blink(D_089CBBEC, (u16)D_089A8808, (u16)D_089A880A,
                                  (u16)D_089A880C, 0xF0);
            func_08891474(D_08A631A8, 0x192, 0x30, RTEXT(D_089B5420), U32(self, 0x20) + 1,
                          ((s32)U32(self, 0x24) - 1) / 10 + 1);
        }
        /* fall through */
    case 1:
        sprintf(buf, RTEXT(D_089B53EA));
        result_text_center(self, 0x30, buf);
        /* fall through */
    case 0:
        draw_footer(self, D_089A8860, lang);
        func_0887D02C(0xF4, D_089A8842, RTEXT(D_089B5418), 0x10);
        func_08891474(D_08A631A8, 0xF4, D_089A8842, RTEXT(D_089B541C), U32(D_089CC438, 0x696E8));
        break;
    }
}

void result_sell_init_timers(u8 *self)
{
    U16(self, 0xA8) = 0x5A;
    U16(self, 0xAA) = 1;
    U16(self, 0xAC) = 900;
    result_init_sounds(self);
}
