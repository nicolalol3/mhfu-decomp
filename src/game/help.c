/* The help pages (tutorial notes) of the menu object: list of the pages of a chapter
 * (0x1111 chapter, 0x1112 page, 0x1114 pages), page shown (0x110E line, 0x1110 lines),
 * popups shown the first time something happens (game flags). (Guessed names.) */
#include "menu.h"

extern u8 *g_sound;
extern u8 *g_sys;
extern void *D_09A44C5C;
extern const u16 D_089B54E8, D_089B5614;
extern const char D_089B167C[];   /* "%s" */
extern const s16 D_089B12A0[], D_089B1338[], D_089B13C0[], D_089B13E8[], D_089B1390[];
extern const s16 D_089B13A8[], D_089B1358[];

u32 sound_play(void *snd, int a, int b, int c, int d, int e, int f, int g, int h);
u32 game_rand(void *a, int b);
int save_in_progress(u8 *g);
void save_start(void *game);
void save_end(void *game);
int game_test_flag(u8 *g, s32 i);
void game_set_flag(u8 *g, s32 i);
void menu_cursor_grid(void *self, u8 *g, u32 keys, s32 quiet);
void emblem_draw(u8 *self, u32 x, u32 y, u32 u, u32 v);
void font_set_height2(void *font, u8 h);
void font_draw_at_color(void *font, int x, int y, int color, const char *fmt, ...);
u8 help_pages(u8 *self, u32 chapter);
u8 help_page_known(u8 *self, u32 chapter, u32 page);
const char *help_page_title(u8 *self, u32 chapter, u32 page);
const char *help_page_line(u8 *self, u32 chapter, u32 page, u32 line);
u8 help_page_lines(u8 *self, u32 chapter, u32 page);
void help_list_box(u8 *self, s16 *r, const char *title, s8 a, u8 page, u8 pages, s32 d);
void help_page_box(u8 *self, s16 *r, const char *text, u32 line, u32 lines, s32 x);

u32 help_popup(u8 *self, u16 kind);
void help_page_draw(u8 *self, u32 x);
u32 help_check_list(u8 *self, const s16 *e);
u32 help_check_flag(u8 *self, u16 flag, u32 chapter, u8 page);
u32 help_check_first(u8 *self);

#define SE(id) sound_play(g_sound, 0, id, 0, 0, 0, 0, 0, 0)
#define RTEXT(id) func_09A63198(D_09A44C5C, id)
#define HELP_ON ((GAME_FLAGS & 0x100) != 0)

void help_init(u8 *self)
{
    self[0x10FF] = 0;
    self[0x10FE] = 0;
    self[0x10FD] = 0;
    self[0x10FC] = 0;
    self[0x110D] = 0;
    self[0x110C] = 0;
    self[0x1115] = 0;
    self[0x1117] = 0;
    if (!HELP_ON) {
        self[0x1111] = 1;
    } else {
        self[0x1111] = (u8)G_QUEST_ID - 0x20;
        help_popup(self, 0);
    }
    self[0x1114] = help_pages(self, self[0x1111]);
}

/* opens the popup asked by help_popup */
void help_open(u8 *self)
{
    if ((u8)save_in_progress(g_game) == 1 || self[0x1115] == 0)
        return;
    self[0x1115] = 0;
    self[0x1117]++;
    self[0x110E] = 0;
    self[0x1110] = help_page_lines(self, self[0x1111], self[0x1112]);
    save_start(g_game);
    sound_play(g_sound, 3, (u16)game_rand(g_sys, 1) % 3, 0, 0, 0, 0, 0, 0);
}

static void scroll_page(u8 *self, u32 keys)
{
    if ((keys & 0x80) != 0) {
        if (self[0x110E] != 0) {
            self[0x110E]--;
            SE(0x17);
        }
    } else if ((keys & 0x20) != 0 && self[0x110E] < self[0x1110] - 1) {
        self[0x110E]++;
        SE(0x17);
    }
}

/* input of the popup: 1 while it is open */
u8 help_input(u8 *self, u32 keys)
{
    if (!HELP_ON)
        return 0;
    if (self[0x1117] == 1) {
        if ((keys & 0x2000) != 0) {
            self[0x1117] = 0;
            save_end(g_game);
            SE(0x14);
        } else {
            scroll_page(self, keys);
        }
    } else if (self[0x1117] == 0) {
        return self[0x1117];
    }
    return self[0x1117];
}

/* opens the list of the pages (0: opened) */
u32 help_list_open(u8 *self)
{
    if (!HELP_ON)
        return 1;
    if ((u8)game_quest_locked(g_game, 0) == 1)
        return 1;
    self[0x10FD] = 0;
    self[0x10FC] = 0;
    save_start(g_game);
    U16(self, 0x1104) = 0;
    self[0x1106] = 8;
    self[0x1107] = 0;
    self[0x1108] = 0;
    self[0x1109] = (self[0x1114] + 7) >> 3;
    self[0x110A] = self[0x1114] + (self[0x1109] - 1) * -8;
    U32(self, 0x5E8) &= ~4;
    return 0;
}

u32 help_list_input(u8 *self, u32 keys)
{
    u16 k = keys;

    if (self[0x10FC] == 1) {
        if ((k & 0x2000) != 0) {
            self[0x10FC] = 0;
            SE(0x14);
        } else {
            scroll_page(self, k);
        }
        return 0;
    }
    if (self[0x10FC] != 0)
        return 0;
    if ((k & 0x2008) != 0) {
        save_end(g_game);
        return keys;
    }
    if (self[0x1109] == 0)
        return 0;
    menu_cursor_grid(self, self + 0x1100, keys, 0);
    if ((k & 0x4000) == 0)
        return 0;
    if (help_page_known(self, self[0x1111], self[0x1104]) == 1) {
        self[0x110E] = 0;
        self[0x1112] = U16(self, 0x1104);
        self[0x1110] = help_page_lines(self, self[0x1111], self[0x1112]);
        self[0x10FC]++;
        SE(0x13);
    } else {
        SE(0x15);
    }
    return 0;
}

void help_draw(u8 *self)
{
    if (self[0x1117] != 0)
        help_page_draw(self, 0);
}

void help_list_draw(u8 *self)
{
    s16 r[10];
    const char *s;
    s32 i, color;
    u8 k;
    s16 y;

    if (self[0x10FC] == 1) {
        help_page_draw(self, 1);
        return;
    }
    if (self[0x10FC] != 0)
        return;
    help_list_box(self, r, RTEXT(D_089B54E8), (s8)self[0x1107], self[0x1108], self[0x1109], 0);
    y = r[7];
    k = self[0x1108] << 3;
    for (i = 0; i < 8; i++) {
        if (help_page_known(self, self[0x1111], k) == 1) {
            color = 0;
            s = help_page_title(self, self[0x1111], k);
        } else {
            color = 10;
            s = RTEXT(D_089B5614);
        }
        font_printf_at_color(g_font, r[6], y, color, D_089B167C, s);
        k++;
        if (k >= self[0x1114])
            return;
        y += r[8];
    }
}

void help_page_draw(u8 *self, u32 x)
{
    s16 r[10];

    help_page_box(self, r, help_page_title(self, self[0x1111], self[0x1112]), self[0x110E],
                  self[0x1110], x);
    font_set_height2(g_font, ((u8 *)r)[0x10]);
    font_draw_at_color(g_font, r[6], r[7], 0, D_089B167C,
                       help_page_line(self, self[0x1111], self[0x1112], self[0x110E]));
    gfx_draw_window(self, (u16)(r[0] - 0x40), (u16)r[1], 0x40, 0x40, 0xFF);
    emblem_draw(self, (u16)(r[0] - 0x3A), (u16)(r[1] + 7), 0x6C, 0xB4);
}

/* first time help: kind of event -> page of the chapter (1 if a popup was asked) */
u32 help_popup(u8 *self, u16 kind)
{
    if ((OPT_FLAGS & 1) != 0)
        return 0;
    if (!HELP_ON)
        return 0;
    if ((u8)game_quest_locked(g_game, 0) == 1)
        return 0;
    self[0x1113] = 0xFF;
    switch (kind) {
    case 0: help_check_list(self, D_089B12A0); break;
    case 1: help_check_list(self, D_089B1338); break;
    case 2: help_check_first(self); break;
    case 3: help_check_flag(self, 0x67, 1, 3); break;
    case 4: help_check_flag(self, 0x68, 1, 4); break;
    case 5: help_check_list(self, D_089B13C0); break;
    case 6: help_check_flag(self, 0x77, 3, 3); break;
    case 7: help_check_list(self, D_089B13E8); break;
    case 8: help_check_flag(self, 0x79, 3, 5); break;
    case 9: help_check_flag(self, 0x7A, 3, 6); break;
    case 12: help_check_list(self, D_089B1390); break;
    case 13: help_check_flag(self, 0x6B, 1, 7); break;
    case 14: help_check_list(self, D_089B13A8); break;
    case 15: help_check_list(self, D_089B1358); break;
    case 16: help_check_flag(self, 0x84, 6, 2); break;
    case 17: help_check_flag(self, 0x90, 7, 10); break;
    case 18: help_check_flag(self, 0x91, 7, 11); break;
    default:
        return 0;
    }
    if ((s8)self[0x1113] < 0)
        return 0;
    self[0x1115] = 1;
    self[0x1112] = self[0x1113];
    return 1;
}

/* list of {flag, chapter, page, flags to set +100 x3}, flag -1 at the end */
u32 help_check_list(u8 *self, const s16 *e)
{
    const u8 *b;

    if (e[0] < 0)
        return 0;
    while ((s32)(u16)e[1] < (s8)self[0x1111]) {
        e += 4;
        if (e[0] < 0)
            return 0;
    }
    if ((u16)e[1] != (s8)self[0x1111])
        return 0;
    if ((u8)game_test_flag(g_game, e[0]) == 1)
        return 0;
    game_set_flag(g_game, e[0]);
    b = (const u8 *)e;
    self[0x1113] = b[4];
    if (b[5] != 0)
        game_set_flag(g_game, b[5] + 100);
    if (b[6] != 0)
        game_set_flag(g_game, b[6] + 100);
    if (b[7] != 0)
        game_set_flag(g_game, b[7] + 100);
    return 1;
}

u32 help_check_flag(u8 *self, u16 flag, u32 chapter, u8 page)
{
    if ((chapter & 0xFFFF) != (u32)(s8)self[0x1111])
        return 0;
    if ((u8)game_test_flag(g_game, flag) == 1)
        return 0;
    game_set_flag(g_game, flag);
    self[0x1113] = page;
    return 1;
}

u32 help_check_first(u8 *self)
{
    s32 i;

    if (self[0x1111] == 7) {
        if ((u8)game_test_flag(g_game, 0x87) == 1)
            return 0;
        for (i = 0x87; i < 0x90; i++)
            game_set_flag(g_game, i);
        self[0x1113] = 1;
        return 1;
    }
    if (self[0x1111] != 1)
        return 0;
    if ((u8)game_test_flag(g_game, 0x66) == 1)
        return 0;
    game_set_flag(g_game, 0x66);
    self[0x1113] = 2;
    return 1;
}

/* ---- the data of the help pages ---- */

extern const u8 D_089B152C[];     /* chapters: {u16 first flag, u8 pages, pad} */
extern const u8 *const D_089B1630[]; /* pages of each chapter: {text, lines} */
extern const u16 D_089B0F70[];    /* titles */
extern const u16 *const D_089B1408[];   /* lines */
extern const u8 D_089B1680[];
extern const u16 D_089B1694[20];  /* layout of the page */
extern const char D_089B16BC[];
extern const u16 D_089AD0B8, D_089AD0BA;
const char *func_09A63188(void *a, u16 id);
void menu_draw_pager(void *self, s32 x, s32 y, u32 a, u32 b, u32 c);
void font_draw(void *font, const char *fmt, ...);

u8 help_pages(u8 *self, u32 chapter)
{
    return D_089B152C[(chapter & 0xFF) * 4 + 2];
}

/* the page was unlocked */
u8 help_page_known(u8 *self, u32 chapter, u32 page)
{
    return game_test_flag(g_game, (page & 0xFF) + U16(D_089B152C, (chapter & 0xFF) * 4));
}

const char *help_page_title(u8 *self, u32 chapter, u32 page)
{
    const u8 *t = D_089B1630[chapter & 0xFF];

    return func_09A63188(D_09A44C5C, D_089B0F70[t[(page & 0xFF) * 2]]);
}

const char *help_page_line(u8 *self, u32 chapter, u32 page, u32 line)
{
    const u8 *t = D_089B1630[chapter & 0xFF];

    return func_09A63188(D_09A44C5C, D_089B1408[t[(page & 0xFF) * 2]][line & 0xFF]);
}

u8 help_page_lines(u8 *self, u32 chapter, u32 page)
{
    return D_089B1630[chapter & 0xFF][(page & 0xFF) * 2 + 1];
}

/* box of the list of the pages */
void help_list_box(u8 *self, s16 *r, const char *title, s8 a, u8 page, u8 pages, s32 d)
{
    s16 l[20];
    s32 w;

    if (r == NULL)
        r = l;
    layout_build(r, D_089B1680, 1, 0, 0);
    if (d != 0)
        ((u8 *)r)[0xB] = 2;
    w = font_char_count(g_font, title);
    r[0x12] = r[0] + ((s32)((u16)r[2] - w * 7) >> 1);
    layout_draw_box(self, r, (int)title, a, 0xFF, 0);
    if (pages >= 2)
        layout_draw_pages(self, r, page, pages, 0, 0);
    menu_draw_sprite(self, (u16)(r[6] + 0x108), (u16)(r[7] + r[8] * 8 - 1), 0x33);
    font_printf_at_color(g_font, (s16)(r[6] + 0x118), (s16)(r[7] + r[8] * 8 + 1), 0, D_089B167C,
                         text_menu(g_game, D_089AD0B8));
}

/* box of a page */
void help_page_box(u8 *self, s16 *r, const char *text, u32 line, u32 lines, s32 x)
{
    s16 l[20];
    s32 i, w;
    s16 tx;

    if (r == NULL)
        r = l;
    for (i = 0; i < 20; i++)
        ((u16 *)r)[i] = D_089B1694[i];
    layout_draw_box(self, r, (int)D_089B16BC, -1, 0xFF, 0);
    w = font_char_count(g_font, text);
    tx = r[0] + ((s32)((u16)r[2] - w * 7) >> 1);
    r[0x12] = tx;
    font_printf_at_color(g_font, tx, (s16)(r[0x13] - 8), 5, D_089B167C, text);
    gfx_fill_rect(self, r[0] + 8, r[7] + ((u8 *)r)[9] + r[8] * 7, r[2] - 0x10, 1, 0x80, 0x80, 0x80);
    menu_draw_pager(self, (s16)(r[0] + ((u16)r[2] >> 1)), (s16)(r[7] + r[8] * 8 + 2), line, lines,
                    0);
    if (x == 0) {
        menu_draw_sprite(self, (u16)(r[6] + 0x108), (u16)(r[7] + r[8] * 8), 0x32);
        font_set_pos(g_font, (s16)(r[6] + 0x118), (s16)(r[7] + r[8] * 8 + 2));
        font_draw(g_font, text_menu(g_game, D_089AD0BA));
    }
}
