/* Menu: pages of the item list (box at 0x5BC: u16 cursor at 0x5C0, u8 per-page count
 * at 0x5C2, page at 0x5C4, pages at 0x5C5, items on the last page at 0x5C6;
 * ids of the visible page at 0x5C8). Same class as menu_box.c. (Guessed names.) */
#include "menu.h"

extern void *g_sound;          /* sound */
extern u32 D_08A62C68;            /* buttons held */
extern u8 *g_net_players;            /* player data (overlay) */

void *combo_next(void *a, u8 *pos, int dir, u16 *x, u16 *y, int c);
s32 combo_known(void *a, void *b, int c, int d);
s32 game_count_bits62(void *game);
s32 menu_next_hunter(void *self, int a, int b);
void sound_play(void *snd, int a, int b, int c, int d, int e, int f, int g, int h);
void func_088911A0(void *font, void *p);
void menu_cursor_grid(void *self, u8 *g, u32 keys, s32 quiet_arg);
void menu_page_fill(u8 *self, u32 keys);

/* chooses the hunter of the list with the left/right buttons (0x80 / 0x20) */
void menu_select_hunter(u8 *self, u32 keys)
{
    U32(self, 0x5E8) &= ~4;
    if (game_count_bits62(g_game) == 0) {
        U8(self, 0x5D9) = 0xFF;
    } else if (keys & 0xA0) {
        s8 c = menu_next_hunter(self, S8(self, 0x5D9), (keys & 0x80) ? -1 : 1);

        if (S8(self, 0x5D9) != c) {
            U8(self, 0x5D9) = c;
            sound_play(g_sound, 0, 23, 0, 0, 0, 0, 0, 0);
        }
    }
}

/* moves the cursor of the item list and updates the description scroll */
void menu_page_input(u8 *self, u32 keys_arg)
{
    u16 keys = keys_arg;

    if (U32(self, 0x5A8) == 0) {
        U32(self, 0x5E8) &= ~4;
        return;
    }
    {
        u8 page = U8(self, 0x5C4);

        menu_page_fill(self, keys);
        if (page != U8(self, 0x5C4))
            U16(self, 0x1208) = keys & 0xA0;
    }
    U32(self, 0x5E8) |= 4;
    U32(self, 0x528) |= 0x800;
    U8(self, 0x5FE) = 1;
    if (U16(self, 0x1208) != 0) {
        if ((D_08A62C68 & U16(self, 0x1208)) == 0)
            U16(self, 0x1208) = 0;
        else
            S16(self, 0x5FC) = 24;
    }
    if (U16(self, 0x1208) != 0)
        return;
    if ((u8)combo_known(D_08A62C50, *(void **)(self + 0x5A8), 1, 0) == 1)
        S16(self, 0x5FC) = S16(*(u8 **)(self + 0x5A8), 2) + 24;
    else
        S16(self, 0x5FC) = 9;
}

/* stores the id of entry e in *slot, with the flags 0x8000 (marked) / 0x4000 (not usable) */
#define PAGE_STORE(slot, e) do {                                       \
        *(slot) = S16(e, 2);                                            \
        if ((u8)combo_known(D_08A62C50, e, 1, 0) == 1) {              \
            if ((U8(e, 7) & 1) != 0)                                    \
                *(slot) |= 0x8000;                                      \
        } else {                                                        \
            *(slot) |= 0x4000;                                          \
        }                                                               \
        (slot)++;                                                       \
    } while (0)

/* builds the list from the start: counts the entries and fills the current page;
 * returns 0 if the list is empty */
int menu_page_build(u8 *self)
{
    u8 *box = self + 0x5BC;
    u16 *slot = (u16 *)(self + 0x5C8);
    u16 x, y;
    u8 pos;
    s32 i, n, first, last, page;
    s16 prev;

    for (i = 0; i < U8(box, 6); i++)
        U16(self, 0x5C8 + i * 2) = 0;
    U8(self, 0x5D8) = 0;
    if (combo_next(D_08A62C50, self + 0x5D8, -1, &x, &y, 1) == NULL) {
        U32(self, 0x5A8) = 0;
        return 0;
    }
    pos = U8(self, 0x5D8);
    prev = -1;
    page = U16(box, 4) / U8(box, 6);
    first = U8(box, 6) * page;
    last = first + U8(box, 6);
    for (i = 0, n = 0; i < 172; i++, n++) {
        u8 *e = combo_next(D_08A62C50, &pos, 1, &x, &y, 1);

        if (pos <= prev)
            break;
        prev = pos;
        if (n < first || n >= last)
            continue;
        if (U16(box, 4) == n) {
            U32(self, 0x5A8) = (u32)e;
            U8(self, 0x5BB) = pos;
            U16(self, 0x5AC) = x;
            U16(self, 0x5AE) = y;
        }
        PAGE_STORE(slot, e);
    }
    if (U16(box, 4) >= n)
        U16(box, 4) = n - 1;
    U8(box, 9) = (n + (U8(box, 6) - 1)) / U8(box, 6);
    U8(box, 10) = n - U8(box, 6) * (U8(box, 9) - 1);
    U8(box, 8) = page;
    U8(box, 7) = U16(box, 4) % U8(box, 6);
    return 1;
}

/* moves the cursor and fills the visible page */
void menu_page_fill(u8 *self, u32 keys)
{
    u16 *slot = (u16 *)(self + 0x5C8);
    u16 x, y;
    u8 pos;
    s32 i, n, first, last;

    menu_cursor_grid(self, self + 0x5BC, keys, 0);
    n = 0;
    for (i = 0; i < U8(self, 0x5C2); i++) {
        U16(self, 0x5C8 + i * 2) = 0;
        n = U8(self, 0x5C2);
    }
    first = n * U8(self, 0x5C4);
    if (U8(self, 0x5C5) - 1 <= U8(self, 0x5C4))
        n = U8(self, 0x5C6);
    last = first + n;
    pos = U8(self, 0x5D8);
    for (i = 0; i < last; i++) {
        u8 *e = combo_next(D_08A62C50, &pos, 1, &x, &y, 1);

        if (i < first || i >= last)
            continue;
        if (U16(self, 0x5C0) == i) {
            U32(self, 0x5A8) = (u32)e;
            U8(self, 0x5BB) = pos;
            U16(self, 0x5AC) = x;
            U16(self, 0x5AE) = y;
        }
        PAGE_STORE(slot, e);
    }
}

/* clears the selection of the list */
void menu_page_reset(u8 *self)
{
    U8(self, 0x5B8) = 0;
    U8(self, 0x586) = 0;
    U8(self, 0x5B9) = 0;
    U8(self, 0x5B1) = 0xFF;
    U8(self, 0x5B0) = 0xFF;
    U16(self, 0x5AE) = 0xFFFF;
    U16(self, 0x5AC) = 0xFFFF;
    U16(self, 0x5B4) = 0xFFFF;
    U32(self, 0x5A8) = 0;
    U32(self, 0x528) &= ~4;
}

/* sets the name of the hunter in the text writer */
void menu_set_hunter_name(void *self, u8 *hunter)
{
    void *name;

    if ((*(u32 *)(g_sys + 0xF7A57C) & 1) == 0)
        name = g_game + 0x4A0;
    else
        name = g_net_players + hunter[0x1E6] * 200 + 0xBC8;
    func_088911A0(g_font, name);
}

/* first value of the list of slot v */
s32 menu_slot_first(void *self, u32 v)
{
    return (s16)*pouch_slot(g_game, (u8)v);
}

u8 *pouch_item_data(void *a, u16 id);
u8 *equip_data_of(void *a, void *eq);
s32 skill_entry_fill(void *self, void *hunter, u8 *e, int c);
void skill_list_add(void *self, void *hunter, s32 c, void *other);
extern void *g_equip_data;

/* chooses the quantity of an item with the arrows (0x80 min, 0x20 max, 0x10 +1, 0x40 -1) */
void menu_select_count(u8 *self, u32 keys)
{
    u8 max = ((u8 *)pouch_slot(g_game, 0))[self[0x586] * 4 + 2];
    u8 c;
    u16 old;

    self[0x58B] = 0;
    c = self[0x58A];
    old = c;
    if (keys & 0x80) {
        if (c < 2)
            old = 0xFFFF;
        self[0x58A] = 1;
    } else if (keys & 0x20) {
        if (c >= max)
            old = 0xFFFF;
        self[0x58A] = max;
    } else if (keys & 0x10) {
        if (c < max) {
            self[0x58A] = c + 1;
        } else {
            self[0x58A] = max;
            old = 0xFFFF;
        }
    } else if (keys & 0x40) {
        if (c < 2)
            old = 0xFFFF;
        else
            self[0x58A] = c - 1;
    }
    if ((s16)old < 0)
        sound_play(g_sound, 0, 21, 0, 0, 0, 0, 0, 0);
    else if (old != self[0x58A])
        sound_play(g_sound, 0, 23, 0, 0, 0, 0, 0, 0);
    if (max <= self[0x58A])
        self[0x58B] = 1;
}

/* repeat timer of a held button: 1 when the action must be repeated */
int menu_repeat_tick(u8 *self, s16 *last)
{
    if (self[0x52F] != 0)
        return 0;
    if (S16(self, 0x119E) < 1) {
        s16 v = S16(self, 0x119C) + 1;

        S16(self, 0x119C) = v;
        *last = v;
        S16(self, 0x119E) = 4;
        return 1;
    }
    if (*last == S16(self, 0x119C)) {
        S16(self, 0x119E) = 4;
        return 1;
    }
    return 0;
}

/* 1 if slot id contains a usable item */
int menu_slot_usable(void *self, u32 id_arg)
{
    u16 id = id_arg;

    if (S16(pouch_slot(g_game, id), 2) < 1)
        return 0;
    if (*(s16 *)pouch_slot(g_game, id) == 0)
        return 0;
    if (pouch_item_data(D_08A62C50, id)[1] != 1)
        return 0;
    return 1;
}

/* 1 if the armor piece eq can be worn by the hunter (blademaster/gunner, gender) */
int menu_armor_wearable(void *self, u8 *hunter, void *eq)
{
    u8 *game = g_game;
    u8 *a = equip_data_of(g_equip_data, eq);
    u8 m = game[0x4B3] == 0 ? 1 : 2;

    if (hunter[0x4E9] == 5)
        m |= 4;
    else
        m |= 8;
    return m == (m & a[4]);
}

/* number of pages of the list at 0x684 (n per page) */
u32 menu_list_pages(u8 *self, s32 per)
{
    u8 *p = self + 0x684;
    s32 i;

    for (i = 0; i < 54; i++, p += 0x12)
        if (p[2] == 0)
            break;
    if (i == 0)
        return 1;
    return ((i - 1) / per + 1) & 0xFF;
}

/* builds the list of the wearable pieces (self+0xA50) and of all the pieces (self+0x684) */
void menu_build_equip_lists(u8 *self, u8 *hunter, u8 *other)
{
    u8 *e;
    s32 n, i;
    s8 c;

    U8(self, 0xE1C) = U8(hunter, 0x4E8);
    U8(self, 0xE1D) = U8(hunter, 0x4E9);
    U16(self, 0xE1E) = U16(hunter, 0x4EA);
    U16(self, 0xE20) = U16(hunter, 0x4EC);
    {
        u16 a = U16(hunter, 0x4F0), b = U16(hunter, 0x4F2);

        U16(self, 0xE22) = U16(hunter, 0x4EE);
        U16(self, 0xE24) = a;
        U16(self, 0xE26) = b;
    }
    if (other != NULL) {
        e = self + 0xA50;
        n = 54;
        for (c = 1; c < 100; c++) {
            if (c == 1)
                continue;
            e[3] = c;
            if ((u8)skill_entry_fill(self, other, e, 0) == 1) {
                n--;
                e += 0x12;
                if (n < 1)
                    break;
            }
        }
        for (; n > 0; n--, e += 0x12)
            e[2] = 0;
        if (S16(self, 0xE1E) == 0) {
            u16 a, b;

            U8(self, 0xE1C) = U8(other, 0x4E8);
            U8(self, 0xE1D) = U8(other, 0x4E9);
            U16(self, 0xE1E) = U16(other, 0x4EA);
            U16(self, 0xE20) = U16(other, 0x4EC);
            a = U16(other, 0x4F0);
            b = U16(other, 0x4F2);
            U16(self, 0xE22) = U16(other, 0x4EE);
            U16(self, 0xE24) = a;
            U16(self, 0xE26) = b;
        }
    }
    for (i = 0; i < 54; i++)
        U8(self, 0x686 + i * 0x12) = 0;
    for (c = 1; c < 100; c++)
        skill_list_add(self, hunter, c, other);
}
