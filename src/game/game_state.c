/* General game state (D_089CC438): flags, money, points, counters of the items.
 * (Guessed names.) */
#include "menu.h"
#undef game_quest_locked

extern u8 *D_08A62C60;
extern u32 D_08A62C6C, D_08A62C74;   /* buttons pressed in this frame / held */
extern u8 *D_0899BF40[];
extern const u16 D_0899C0A8[];       /* ids of the progress flags (end: 0xEA60) */

u32 game_group_count_total(u8 *g, u32 id);

u8 *game_state_dtor(u8 *g, s32 flag)
{
    if (g != NULL) {
        D_089CC438 = NULL;
        if ((s16)flag > 0)
            func_0890BFA4(g);
    }
    return g;
}

/* 1 if the current quest state forbids the action (kind 0 / 2 for some states) */
int game_quest_locked(u8 *g, s32 kind)
{
    switch (g[0x2E]) {
    case 3:
        if (kind == 0)
            return 1;
        return 0;
    case 4:
    case 6:
    case 7:
    case 8:
        return 1;
    case 5:
        if (kind != 2)
            return 1;
        return 0;
    }
    return 0;
}

/* bit flags at g+0x508 (16 per halfword) */
void game_set_flag(u8 *g, s32 i)
{
    U16(g, 0x508 + (i / 16) * 2) |= (u16)(1 << (i & 0xF));
}

void game_clear_flag(u8 *g, s32 i)
{
    U16(g, 0x508 + (i / 16) * 2) &= ~(u16)(1 << (i & 0xF));
}

int game_test_flag(u8 *g, s32 i)
{
    return (U16(g, 0x508 + (i / 16) * 2) & (1 << (i & 0xF))) != 0;
}

/* buttons pressed for the menus (0 while the input is locked) */
u32 game_buttons(void *g)
{
    if (D_08A62C60[0x74] != 0)
        return 0;
    return ((D_08A62C74 & 0xF0) | D_08A62C6C) & 0xFFFF;
}

/* money (zenny), between 0 and 9999999 */
void game_add_money(u8 *g, s32 n)
{
    *(s32 *)(g + 0x696F0) += n;
    if (*(s32 *)(g + 0x696F0) >= 10000000)
        *(s32 *)(g + 0x696F0) = 9999999;
    if (*(s32 *)(g + 0x696F0) < 0)
        *(s32 *)(g + 0x696F0) = 0;
}

/* points (Pokke / guild points), between 0 and 9999999 */
void game_add_points(u8 *g, s32 n)
{
    *(s32 *)(g + 0x696E8) += n;
    if (*(s32 *)(g + 0x696E8) >= 10000000)
        *(s32 *)(g + 0x696E8) = 9999999;
    if (*(s32 *)(g + 0x696E8) < 0)
        *(s32 *)(g + 0x696E8) = 0;
}

u8 game_table_byte(void *g, u32 i)
{
    return *D_0899BF40[i & 0xFF];
}

u8 *game_table_ptr(void *g, u32 i)
{
    return D_0899BF40[i & 0xFF];
}

int game_has_progress(u8 *g, u32 id);

/* highest rank reached, from the progress flags */
int game_rank(u8 *g)
{
    if ((u8)game_has_progress(g, 0xC9C) == 1)
        return 8;
    if ((u8)game_has_progress(g, 0xC1F) == 1)
        return 7;
    if ((u8)game_has_progress(g, 0xBC1) == 1)
        return 6;
    if ((u8)game_has_progress(g, 0x899) == 1)
        return 5;
    if ((u8)game_has_progress(g, 0x848) == 1)
        return 4;
    if ((u8)game_has_progress(g, 0x4C2) == 1)
        return 3;
    if ((u8)game_has_progress(g, 0x4B1) == 1)
        return 2;
    return (u8)game_has_progress(g, 0x450) == 1;
}

/* index of the progress flag id in D_0899C0A8 (0xFFFF = not found) */
u32 game_progress_index(void *g, u32 id)
{
    const u16 *t = D_0899C0A8;
    u16 i = 0;

    for (; *t != 0xEA60; t++, i++) {
        if (*t == (u16)id)
            return i;
    }
    return 0xFFFF;
}

void game_set_progress(u8 *g, u32 id)
{
    u16 k = game_progress_index(g, id);

    if (k == 0xFFFF)
        return;
    U32(g, 0x4428 + (k >> 5) * 4) |= 1 << (k & 0x1F);
}

int game_has_progress(u8 *g, u32 id)
{
    u16 k = game_progress_index(g, id);

    if (k == 0xFFFF)
        return 0;
    return (U32(g, 0x4428 + (k >> 5) * 4) & (1 << (k & 0x1F))) != 0;
}

/* two counters for each monster (0..9999): g+0x46E8 and g+0x44CC */
void game_add_count_a(u8 *g, u32 i, s32 n)
{
    u16 *c = (u16 *)(g + 0x46E8 + (i & 0xFF) * 2);

    *c += n;
    if (*c >= 10000)
        *c = 9999;
}

u16 game_count_a(u8 *g, u32 i)
{
    return U16(g, 0x46E8 + (i & 0xFF) * 2);
}

u16 game_count_b(u8 *g, u32 i)
{
    return U16(g, 0x44CC + (i & 0xFF) * 2);
}

u16 game_count_total(u8 *g, u32 i)
{
    u16 n = U16(g, 0x46E8 + (i & 0xFF) * 2) + U16(g, 0x44CC + (i & 0xFF) * 2);

    if (n >= 10000)
        n = 9999;
    return n;
}

void game_add_count_b(u8 *g, u32 i, s32 n)
{
    u16 *c = (u16 *)(g + 0x44CC + (i & 0xFF) * 2);

    *c += n;
    if (*c >= 10000)
        *c = 9999;
}

/* records of each monster: largest size (g+0x4580) and smallest size (g+0x4634) */
u16 game_record_max(u8 *g, u32 i)
{
    return U16(g, 0x4580 + (i & 0xFFFF) * 2);
}

u16 game_record_min(u8 *g, u32 i)
{
    return U16(g, 0x4634 + (i & 0xFFFF) * 2);
}

void game_update_record_max(u8 *g, u32 i, u32 v)
{
    u16 *r = (u16 *)(g + 0x4580 + (i & 0xFFFF) * 2);

    if (*r < (u16)v)
        *r = v;
}

void game_update_record_min(u8 *g, u32 i, u32 v)
{
    u16 *r = (u16 *)(g + 0x4634 + (i & 0xFFFF) * 2);

    if (*r == 0 || (u16)v < *r)
        *r = v;
}

/* 1 if the counter a of the item list is at least n */
int game_has_at_least(void *g, u32 a, u32 n)
{
    return (u16)game_group_count_total(g, a) >= (u16)n;
}

extern const f32 D_0899C760[];      /* base size of each monster */
extern void *D_09CFB798;
void func_09C19080(void *a, int b);
s32 flag_id_index(void *g, u32 a);
s32 game_box_size(void *g);
int game_slot_equipped(u8 *g, u32 i);

u32 game_best_record_max(u8 *g, u32 id, u8 *which);
u32 game_best_record_min(u8 *g, u32 id, u8 *which);

/* largest size of the monster id (mode != 0: of its whole group), in percent of the base size */
f32 game_size_max(u8 *g, u32 id, s32 mode)
{
    u8 which;
    u16 v;

    if (mode == 0)
        v = game_record_max(g, id & 0xFF);
    else
        v = game_best_record_max(g, id, &which);
    return ((f32)v * D_0899C760[id & 0xFF]) / 100.0f;
}

f32 game_size_min(u8 *g, u32 id, s32 mode)
{
    u8 which;
    u16 v;

    if (mode == 0)
        v = game_record_min(g, id & 0xFF);
    else
        v = game_best_record_min(g, id, &which);
    return ((f32)v * D_0899C760[id & 0xFF]) / 100.0f;
}

f32 game_size_percent(void *g, u32 id, u32 v)
{
    return ((f32)v * D_0899C760[id & 0xFF]) / 100.0f;
}

/* records of a group of monsters (list D_0899BF40[id] ending with 0xFF) */
u32 game_best_record_max(u8 *g, u32 id, u8 *which)
{
    const u8 *m = D_0899BF40[id & 0xFF];
    u16 best = 0;

    for (; *m != 0xFF; m++) {
        u16 v = game_record_max(g, *m);

        if (best < v) {
            best = v;
            if (which != NULL)
                *which = *m;
        }
    }
    return best;
}

u32 game_best_record_min(u8 *g, u32 id, u8 *which)
{
    const u8 *m = D_0899BF40[id & 0xFF];
    u16 best = 0;

    for (; *m != 0xFF; m++) {
        u16 v = game_record_min(g, *m);

        if (v != 0 && (best == 0 || v < best)) {
            best = v;
            if (which != NULL)
                *which = *m;
        }
    }
    return best;
}

/* counters of a group of monsters */
u32 game_group_count_a(u8 *g, u32 id)
{
    const u8 *m = D_0899BF40[id & 0xFF];
    u16 n = 0;

    for (; *m != 0xFF; m++)
        n += game_count_a(g, *m);
    if (n >= 10000)
        n = 9999;
    return n;
}

u32 game_group_count_b(u8 *g, u32 id)
{
    const u8 *m = D_0899BF40[id & 0xFF];
    u16 n = 0;

    for (; *m != 0xFF; m++)
        n += game_count_b(g, *m);
    if (n >= 10000)
        n = 9999;
    return n;
}

u32 game_group_count_total(u8 *g, u32 id)
{
    u16 n = (u16)game_group_count_a(g, id) + (u16)game_group_count_b(g, id);

    if (n >= 10000)
        n = 9999;
    return n;
}

void game_notify_overlay(void)
{
    func_09C19080(D_09CFB798, 1);
}

/* third money counter (0..9999999), returns the new value */
u32 game_add_money2(u8 *g, s32 n)
{
    u32 v = U32(g, 0x696EC) + n;

    if (v < 10000000)
        U32(g, 0x696EC) = v;
    else
        U32(g, 0x696EC) = 9999999;
    return U32(g, 0x696EC);
}

/* bit flags at g+0x6ADE8 */
void game_set_flag2(u8 *g, s32 i)
{
    U32(g, 0x6ADE8 + (i >> 5) * 4) |= 1 << (i & 0x1F);
}

int game_test_flag2(u8 *g, s32 i)
{
    return (U32(g, 0x6ADE8 + (i >> 5) * 4) & (1 << (i & 0x1F))) != 0;
}

void game_clear_flag3(u8 *g, u32 a)
{
    s32 i = flag_id_index(g, a);

    U32(g, 0x6ADF0 + (i >> 5) * 4) &= ~(1 << (i & 0x1F));
}

/* slots of 0xC bytes at g+0x548 */
u32 game_free_slot(u8 *g)
{
    s16 n = game_box_size(g);
    s16 i;

    for (i = 0; i < n; i++) {
        if (g[0x548 + i * 0xC] == 0)
            return (u16)i;
    }
    return 0xFFFF;
}

u32 game_alloc_slot(u8 *g, u32 kind_arg, u32 a, u32 b)
{
    u8 kind = kind_arg;
    s16 n, i;

    if (kind >= 7)
        return 0xFFFF;
    n = game_box_size(g);
    for (i = 0; i < n; i++) {
        u8 *s = g + 0x4A0 + i * 0xC;

        if (s[0xA8] == 0) {
            s[0xA8] = 1;
            s[0xA9] = kind;
            U16(s, 0xAA) = a;
            U16(s, 0xAC) = b;
            U16(s, 0xAE) = 0;
            U16(s, 0xB0) = 0;
            U16(s, 0xB2) = 0;
            return (u16)i;
        }
    }
    return 0xFFFF;
}

int game_free_slot_at(u8 *g, u32 i_arg)
{
    u16 i = i_arg;
    u8 *s;

    if ((u8)game_slot_equipped(g, i) != 0)
        return 0;
    s = g + 0x4A0 + i * 0xC;
    s[0xA8] = 0;
    s[0xA9] = 0;
    U16(s, 0xAA) = 0;
    U16(s, 0xAC) = 0;
    U16(s, 0xAE) = 0;
    U16(s, 0xB0) = 0;
    U16(s, 0xB2) = 0;
    return 1;
}

extern const u8 D_08963258[], D_08967678[], D_0896F850[], D_0896B818[], D_08973838[];  /* armor tables (0x28 bytes each) */
s32 item_seen(void *a, u32 id);

int game_slot_is(u8 *g, u32 i_arg, s32 k1, s32 k2);

/* copies the slot i of the box (0xC bytes) to the equipment at dst */
static void game_copy_slot(u8 *g, u8 *dst, u16 i)
{
    u8 *s = g + 0x4A0 + i * 0xC;

    dst[0] = s[0xA8];
    dst[1] = s[0xA9];
    U16(dst, 2) = U16(s, 0xAA);
    U16(dst, 4) = U16(s, 0xAC);
    U16(dst, 6) = U16(s, 0xAE);
    U16(dst, 8) = U16(s, 0xB0);
    U16(dst, 0xA) = U16(s, 0xB2);
}

void game_check_armor(u8 *g);

/* rebuilds the equipment of the hunter (g+0x4C0) from the indices at g+0x44C0 */
void game_update_equipment(u8 *g)
{
    if ((u8)game_slot_is(g, U16(g, 0x44C0), 5, 6) == 1)
        game_copy_slot(g, g + 0x4C0, U16(g, 0x44C0));
    game_check_armor(g);
    if ((u8)game_slot_is(g, U16(g, 0x44C2), 0, 0) == 1)
        game_copy_slot(g, g + 0x4CC, U16(g, 0x44C2));
    else
        memset(g + 0x4CC, 0, 0xC);
    if ((u8)game_slot_is(g, U16(g, 0x44C4), 1, 1) == 1)
        game_copy_slot(g, g + 0x4D8, U16(g, 0x44C4));
    else
        memset(g + 0x4D8, 0, 0xC);
    if ((u8)game_slot_is(g, U16(g, 0x44C6), 2, 2) == 1)
        game_copy_slot(g, g + 0x4E4, U16(g, 0x44C6));
    else
        memset(g + 0x4E4, 0, 0xC);
    if ((u8)game_slot_is(g, U16(g, 0x44C8), 3, 3) == 1)
        game_copy_slot(g, g + 0x4F0, U16(g, 0x44C8));
    else
        memset(g + 0x4F0, 0, 0xC);
    if ((u8)game_slot_is(g, U16(g, 0x44CA), 4, 4) == 1)
        game_copy_slot(g, g + 0x4FC, U16(g, 0x44CA));
    else
        memset(g + 0x4FC, 0, 0xC);
}

/* 1 if the slot i is used and of kind k1 or k2 */
int game_slot_is(u8 *g, u32 i_arg, s32 k1, s32 k2)
{
    u16 i = i_arg;
    u8 *s;

    if (i == 0xFFFF)
        return 0;
    s = g + 0x4A0 + i * 0xC;
    if (s[0xA8] == 0)
        return 0;
    if (s[0xA9] == (u8)k1)
        return 1;
    return s[0xA9] == (u8)k2;
}

/* 1 if the slot i is equipped */
int game_slot_equipped(u8 *g, u32 i)
{
    return U16(g, 0x44C0) == i || U16(g, 0x44C2) == i || U16(g, 0x44C4) == i || U16(g, 0x44C6) == i
        || U16(g, 0x44C8) == i || U16(g, 0x44CA) == i;
}

/* removes the armor pieces that the hunter cannot wear (gender, blademaster / gunner) */
void game_check_armor(u8 *g)
{
    u8 k = (g[0x4B3] == 0) ? 1 : 2;

    k |= (g[0x4C1] == 6) ? 8 : 4;
    if ((k & D_08963258[U16(g, 0x4DA) * 0x28]) != k)
        U16(g, 0x44C4) = 0xFFFF;
    if ((k & D_08967678[U16(g, 0x4E6) * 0x28]) != k)
        U16(g, 0x44C6) = 0xFFFF;
    if ((k & D_0896F850[U16(g, 0x4FE) * 0x28]) != k)
        U16(g, 0x44CA) = 0xFFFF;
    if ((k & D_0896B818[U16(g, 0x4F2) * 0x28]) != k)
        U16(g, 0x44C8) = 0xFFFF;
    if ((k & D_08973838[U16(g, 0x4CE) * 0x28]) != k)
        U16(g, 0x44C2) = 0xFFFF;
}

/* number of pages of the item box (8, +1 for each of the two expansions) */
s32 game_box_pages(void *g)
{
    s16 n = 8;
    u16 id;

    for (id = 0x33D; id < 0x33F; id++) {
        if ((u8)item_seen(D_08A62C50, id) == 1)
            n++;
    }
    return n;
}

s32 game_box_size(void *g)
{
    return (s16)((s16)game_box_pages(g) * 100);
}

/* wraps a box position on the pages that exist */
s32 game_box_wrap(void *g, s32 v)
{
    s16 p = v;

    if (p > 0) {
        s16 m = (s16)game_box_pages(g) - 1;
        s16 q = p / 100;

        if (m >= q)
            m = q;
        return (s16)(p % 100 + m * 100);
    }
    if (p == 0)
        return v;
    do
        v = (s16)(v + 100);
    while (v < 0);
    return v;
}

/* play time: one second every 30 frames, up to 9999:59:59 */
void game_update_time(u8 *g)
{
    U16(g, 0x1C)++;
    if (U16(g, 0x1C) % 30 == 0) {
        u32 t = U32(g, 0x4468) + 1;

        U32(g, 0x4468) = t;
        if (t >= 36000000)
            U32(g, 0x4468) = 35999999;
    }
}

extern u8 *D_089CBBE4;            /* camera */
extern u8 *D_089CBFFC;            /* render lists */
void camera_init(void *cam);
void rl_begin_frame(u8 *self);
u16 *quest_sub_data(void *q);

/* start of a quest: resets the state, the camera and the render lists */
void game_quest_start(u8 *g)
{
    U16(g, 0x1C) = 0;
    g[0x28] = 0;
    g[0xC] = 0;
    g[0xD] = 0;
    U16(g, 0x6AF12) = 1;
    U32(g, 0x6AF14) &= ~1;
    camera_init(D_089CBBE4);
    rl_begin_frame(D_089CBFFC);
}

/* fills the supply box (32 items) from the quest data */
void game_fill_supply_box(u8 *g)
{
    u16 *s;
    int i;

    for (i = 0; i < 0x40; i++)
        U16(g, 0x390 + i * 2) = 0;
    s = quest_sub_data(D_08A62C7C);
    if (s[0] != 0) {
        for (i = 0;;) {
            U16(g, 0x390 + i * 4) = s[0];
            U16(g, 0x392 + i * 4) = s[1];
            i++;
            s += 2;
            if (i >= 0x20 || s[0] == 0)
                break;
        }
    }
    U32(g, 0x410) = 0;
    U32(g, 0x414) = 0;
}

int game_flag_10(u8 *g)
{
    return (U32(g, 0x6AF14) & 0x10) != 0;
}

extern u32 D_08A62C68;            /* buttons held */
extern const s16 D_0899CA50[];
extern const s8 D_0899BE20[14];   /* default options */
extern s32 D_09A1DB50;
extern void *D_08A62D8C;          /* sound */
extern const u32 D_0899C49C[], D_0899C514;
u32 game_rand(void *a, int b);
void func_088902A8(void *a);
void func_08880760(void *snd, int v);
void func_08880864(void *snd, int v);
void func_08880874(void *snd, int v);
s32 func_08890F70(void *font, const char *s);

/* value of the table D_0899CA50 for (a, b), the column chosen by the d-pad (or at random) */
s32 game_dpad_value(void *g, u32 a, u32 b)
{
    u16 k;

    game_rand(D_08A631A0, 1);
    game_rand(D_08A631A0, 1);
    game_rand(D_08A631A0, 1);
    if ((D_08A62C68 & 0xF0) == 0)
        k = (u16)game_rand(D_08A631A0, 1) & 3;
    else if (D_08A62C68 & 0x10)
        k = 0;
    else if (D_08A62C68 & 0x40)
        k = 1;
    else if (D_08A62C68 & 0x80)
        k = 2;
    else
        k = 3;
    return D_0899CA50[k + (a & 0xFF) * 0x44 + (b & 0xFF) * 4];
}

int game_low_rank(u8 *g)
{
    return g[0x38D] < 2;
}

void game_clear_hunter(u8 *g)
{
    memset(g + 0x30, 0, 0x348);
}

/* default options (apply = 1: also the language) */
void game_reset_options(u8 *g, u32 apply)
{
    s8 lang = g[0x6ADD8];
    int i;

    for (i = 0; i < 14; i++)
        g[0x6ADD8 + i] = D_0899BE20[i];
    if ((u8)apply == 1) {
        s32 v = (s8)g[0x6ADD8];

        *(s32 *)(D_08A631A0 + 0xF7A574) = v;
        D_09A1DB50 = v;
        func_088902A8(D_08A631A0);
    } else {
        g[0x6ADD8] = lang;
    }
    func_08880760(D_08A62D8C, (s8)g[0x6ADD9]);
    func_08880864(D_08A62D8C, (s8)g[0x6ADDB]);
    func_08880874(D_08A62D8C, (s8)g[0x6ADDA]);
}

/* hunter rank, points and the points needed for the next rank */
void game_hr_points(u8 *g, u8 *rank, u32 *pts, u32 *next)
{
    const u32 *t = D_0899C49C;

    *pts = U32(g, 0x696EC);
    while (*t != D_0899C514 && U32(g, 0x696EC) >= *t)
        t++;
    *rank = (u8)game_rank(g) + 1;
    *next = *t;
}

/* 62 bit flags at g+0x44AC */
void game_set_bit62(u8 *g, u32 i_arg)
{
    u8 i = i_arg;

    if (i < 0x3E)
        g[0x44AC + (i >> 3)] |= 1 << (i & 7);
}

int game_test_bit62(u8 *g, u32 i_arg)
{
    u8 i = i_arg;

    if (i >= 0x3E)
        return 0;
    return (g[0x44AC + (i >> 3)] & (1 << (i & 7))) != 0;
}

s32 game_count_bits62(u8 *g)
{
    s32 n = 0;
    u32 i;

    for (i = 0; i < 0x3E; i++) {
        if (g[0x44AC + (i >> 3)] & (1 << (i & 7)))
            n++;
    }
    return n;
}

/* 1 + number of the flags set below n */
u8 game_bits62_before(u8 *g, u32 n_arg)
{
    u8 n = n_arg;
    s32 m = (n < 0x3F) ? n : 0x3E;
    u8 c = 1;
    s32 i;

    for (i = 0; i < m; i++) {
        if (g[0x44AC + (i >> 3)] & (1 << (i & 7)))
            c++;
    }
    return c;
}

/* x of a string centered on x (size = width of a character in quarters) */
s32 text_center_x(void *g, const char *s, s32 x_arg, s32 size_arg)
{
    s16 x = x_arg, size = size_arg;

    return (s16)(x - (size * func_08890F70(D_08A631A8, s)) / 4);
}

s32 text_center_x2(void *g, const char *s, s32 x_arg, s32 size_arg)
{
    s16 x = x_arg, size = size_arg;

    return (s16)(x - (size * func_08890F70(D_08A631A8, s)) / 2);
}
