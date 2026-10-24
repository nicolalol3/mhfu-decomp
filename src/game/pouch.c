/* Item pouch of the hunter (24 slots at g+0x6AEA6), saving state, Felyne kitchen,
 * conversions of equipment kinds. (Guessed names.) */
#include "menu.h"

extern u8 *D_08A62C40;            /* display list manager */
extern void *D_08A62D8C;          /* sound */
extern u8 **D_09A4F060;
extern u16 *D_09CE470C[][2];
u32 game_rand(void *a, int b);
const u8 *kitchen_default(void *g, s32 i);
void felyne_create(u8 *g, u8 *e, s32 mode);
u32 *dl_cur(void *dl);
void dl_link(void *dl, u32 *p, int n, u8 layer);
void dl_set_cur(void *dl, u32 *end);
void func_08882F10(void *snd, int a);
void func_08885798(void *snd, int a, int b);
void func_08885644(void *snd, int a, int b);
void game_set_flag(u8 *g, s32 i);
int game_test_flag(u8 *g, s32 i);

#define DL_PUSH(dl, layer, w, nh, nc, bi) do {                                         \
        u32 cur_ = U32(dl, 0x80198);                                                \
        s32 used_ = (s32)(cur_ - (u32)((dl) + U32(dl, 4) * 0x40000 + 0x198)) / 4;   \
        if ((u32)(used_ + (nh) + (nc)) < 0x10000) {                                 \
            int i_;                                                                 \
            (w)[bi] = ((cur_ & 0xFF000000) >> 8) | 0x10000000;                      \
            (w)[(bi) + 1] = (U32(dl, 0x80198) & 0xFFFFFF) | 0x01000000;            \
            for (i_ = 0; i_ < (nh) + (nc); i_++) {                                  \
                **(u32 **)((dl) + 0x80198) = (w)[i_];                               \
                U32(dl, 0x80198) += 4;                                              \
            }                                                                       \
            dl_link(dl, (u32 *)(cur_ + (nh) * 4), nc, layer);                \
        }                                                                           \
    } while (0)

/* the 5 Felyne cooks: default state of their slots */
void kitchen_init(u8 *g)
{
    int i;

    for (i = 0; i < 5; i++) {
        u8 *e = g + i * 0x70 + 0x699B8;
        u8 *p;

        e[1] = i;
        e[2] = 0;
        e[3] = 1;
        e[6] = i;
        p = kitchen_default(D_089CC438, i);
        e[7] = p[0];
        e[0xA] = p[3];
        e[8] = p[1];
        e[0xB] = p[4];
        e[9] = p[2];
        e[0xC] = p[5];
        U16(e, 4) = 0;
        e[0] = 1;
        felyne_create(g, e, 1);
    }
}

/* id of the current area map */
u16 game_map_id(u8 *g)
{
    if (U16(g, 0x6AF0E) == 0x7C)
        return 0x54;
    if (U16(g, 0x6AF0E) == 0xC5)
        return 0x7F;
    return *D_09CE470C[*(s32 *)((u8 *)D_09A4F060 + 0x284)][0];
}

int save_in_progress(u8 *g);

/* black screen while saving */
void save_draw_black(u8 *g)
{
    u8 *dl;
    u32 *p;
    u32 v[14];

    if ((u8)save_in_progress(g) != 1)
        return;
    p = dl_cur(D_08A62C40);
    p[0] = 0x23000000;
    p[1] = 0xDE000000;
    p[2] = 0x21000001;
    p[3] = 0xDF000032;
    p[4] = 0x22000000;
    p[5] = 0xDBFF0003;
    p[6] = 0x10000000;
    p[7] = 0x08000000;
    dl_link(D_08A62C40, p, 8, 9);
    dl_set_cur(D_08A62C40, p + 8);
    dl = D_08A62C40;
    v[0] = 0xA0000000;
    v[1] = 0;
    v[2] = 0;
    v[3] = 0xA0000000;
    v[4] = 0x011001E0;
    v[5] = 0;
    v[6] = 0x1E000000;
    v[7] = 0x50000001;
    v[8] = 0x1280011C;
    v[11] = 0x04060002;
    v[12] = 0x10000000;
    v[13] = 0x08000000;
    DL_PUSH(dl, 9, v, 6, 8, 9);
}

int save_in_progress(u8 *g)
{
    return g[0x6AF1A] != 0;
}

/* starts saving (off-line only) */
void save_start(u8 *g)
{
    if ((*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) != 0) {
        g[0x6AF1A] = 0;
        return;
    }
    g[0x6AF1A] = 1;
    func_08882F10(D_08A62D8C, 1);
    func_08885798(D_08A62D8C, 4, 0);
}

void save_end(u8 *g)
{
    g[0x6AF1A] = 4;
    func_08882F10(D_08A62D8C, 0);
    func_08885644(D_08A62D8C, 4, 0);
}

/* slot i of the pouch (item, number) */
u16 *pouch_slot(u8 *g, s32 i)
{
    if (i >= 0x19)
        i = 0;
    return (u16 *)(g + 0x6AEA6 + i * 4);
}

/* number of items id in the pouch */
s32 pouch_count(u8 *g, u32 id_arg)
{
    u16 id = id_arg;
    s32 i;

    for (i = 0; i < 0x18; i++) {
        if (pouch_slot(g, i)[0] == id)
            return (s16)pouch_slot(g, i)[1];
    }
    return 0;
}

/* how many more items id fit in their slot (0xFF = no limit) */
u32 pouch_room(u8 *g, u32 id_arg)
{
    u16 id = id_arg;
    s32 i;

    for (i = 0; i < 0x18; i++) {
        if (pouch_slot(g, i)[0] == id) {
            if (D_0899D19B[id * 0x18] == 0xFF)
                return 0xFF;
            return D_0899D19B[id * 0x18] - (s16)pouch_slot(g, i)[1];
        }
    }
    return D_0899D19B[id * 0x18];
}

/* same, -1 if the item is not in the pouch and the pouch is full */
s32 pouch_room_or_full(u8 *g, u32 id_arg)
{
    u16 id = id_arg;
    s16 i, empty = 0;

    for (i = 0; i < 0x18; i++) {
        if (pouch_slot(g, i)[0] == id) {
            if (D_0899D19B[id * 0x18] == 0xFF)
                return 0xFF;
            return (s16)(D_0899D19B[id * 0x18] - pouch_slot(g, i)[1]);
        }
        if (pouch_slot(D_089CC438, i)[0] == 0)
            empty++;
    }
    if (empty == 0)
        return -1;
    return D_0899D19B[id * 0x18];
}

/* daily ingredient of the Felyne kitchen */
void kitchen_roll_daily(u8 *g)
{
    if ((U32(g, 0x6ADF8) & 0x1000) == 0) {
        g[0x44BD] = ((u16)game_rand(D_08A631A0, 1) & 3) + 1;
    } else if ((u8)game_test_flag(g, 0x22) == 0) {
        game_set_flag(g, 0x22);
        g[0x44BD] = 0;
    } else {
        g[0x44BD] = (u16)game_rand(D_08A631A0, 1) % 5;
    }
}

void kitchen_reset_daily(u8 *g)
{
    if ((U32(g, 0x6ADF8) & 0x1000) == 0 || (u8)game_test_flag(g, 0x22) != 0) {
        g[0x44BD] = 1;
    } else {
        game_set_flag(g, 0x22);
        g[0x44BD] = 0;
    }
}

void kitchen_check_daily(u8 *g)
{
    if ((U32(g, 0x6ADF8) & 0x1000) != 0 && (u8)game_test_flag(g, 0x22) == 0) {
        game_set_flag(g, 0x22);
        g[0x44BD] = 0;
    }
}

/* conversions between the two numberings of the equipment kinds */
s32 equip_kind_to_slot(void *g, u32 k_arg)
{
    u8 k = k_arg;

    if (k == 4)
        return 5;
    if (k == 3)
        return 4;
    if (k == 2)
        return 3;
    if (k == 1)
        return 2;
    return 0;
}

s32 equip_slot_to_kind(void *g, u32 k_arg)
{
    u8 k = k_arg;

    if (k == 5)
        return 4;
    if (k == 4)
        return 3;
    if (k == 3)
        return 2;
    if (k == 2)
        return 1;
    if (k == 0)
        return 0;
    return 5;
}

/* 1 if the hunter has the skill / item k among the 5 at g+0x69942 */
int has_kitchen_skill(u8 *g, u32 k_arg)
{
    u8 k = k_arg;
    int i;

    for (i = 0; i < 5; i++) {
        if (g[0x69942 + i] == k)
            return 1;
    }
    return 0;
}

/* same, on-line: any of the 6 hunters of the room */
int party_has_kitchen_skill(u8 *p, u32 k_arg)
{
    u8 k = k_arg;
    int i, j;

    if ((*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) == 0)
        return has_kitchen_skill(p, k_arg);
    for (i = 0; i < 6; i++, p += 0x8C) {
        if (p[0x30] == 0)
            continue;
        for (j = 0; j < 5; j++) {
            if (p[0xA9 + j] == k)
                return 1;
        }
    }
    return 0;
}

extern const s32 D_0899CC08[0x3D];
int game_has_progress(u8 *g, u32 id);
int memcmp(const void *a, const void *b, size_t n);

/* 1 if the quest is still going (not ended, failed or abandoned) */
int quest_running(void)
{
    u32 f = U32(D_089CC438, 0x6AF14);

    if (f & 1)
        return 0;
    return (f & 0x20) == 0 && (f & 8) == 0;
}

int event_quest_done(u8 *g, s32 id, s32 i);

/* all the 5 levels of the event quests 0x5209..0x5212 done */
int event_quests_done_a(u8 *g)
{
    s32 id, i;

    for (id = 0x5209; id < 0x5213; id++)
        for (i = 0; i < 5; i++)
            if (event_quest_done(g, id, i) == 0)
                return 0;
    return 1;
}

int event_quests_done_b(u8 *g)
{
    s32 id, i;

    for (id = 0x5213; id < 0x5218; id++)
        for (i = 0; i < 5; i++)
            if (event_quest_done(g, id, i) == 0)
                return 0;
    return 1;
}

int event_quests_done_c(u8 *g)
{
    s32 id, i;

    for (id = 0x5218; id < 0x5220; id++)
        for (i = 0; i < 5; i++)
            if (event_quest_done(g, id, i) == 0)
                return 0;
    return 1;
}

/* progress flags 0x55F1..0x55F6 */
int progress_55f1(u8 *g)
{
    u16 i;

    for (i = 0; i < 6; i++) {
        if (game_has_progress(g, (u16)(i + 0x55F1)) == 0)
            return 0;
    }
    return 1;
}

/* bit flags of the event quests (5 per quest) at g+0x696D4 */
int event_quest_done(u8 *g, s32 id, s32 i)
{
    s32 b;

    if (id < 0x5209 || id >= 0x5220)
        return 0;
    b = i + (id - 0x5209) * 5;
    return (U32(g, 0x696D4 + (b / 32) * 4) & (1 << (b % 32))) != 0;
}

int event_quest_any_done(u8 *g, s32 id)
{
    s32 i;

    for (i = 0; i < 5; i++) {
        if (event_quest_done(g, id, i) != 0)
            return 1;
    }
    return 0;
}

void event_quest_set_done(u8 *g, s32 id, s32 i)
{
    s32 b;

    if (id < 0x5209 || id >= 0x5220)
        return;
    b = i + (id - 0x5209) * 5;
    U32(g, 0x696D4 + (b / 32) * 4) |= 1 << (b % 32);
}

int game_test_flag_696e4(u8 *g, u32 i)
{
    return (U32(g, 0x696E4) & (1 << (i & 0x1F))) != 0;
}

/* forgets the selected friend if it is the hunter e */
void friend_unselect(u8 *g, u8 *e)
{
    if (g[0x6992C] != 2 || S8(e, 0) == 0 || S8(g, 0x6992F) == -1)
        return;
    if (memcmp(g + g[0x6992F] * 0x70 + 0x6975C, e + 0x60, 0x10) == 0)
        g[0x6992F] = 0xFF;
}

s32 flag_id_at(void *g, s32 i)
{
    return D_0899CC08[(s8)i];
}

u32 flag_id_index(void *g, s32 v)
{
    u32 i;

    for (i = 0; i < 0x3D; i++) {
        if (D_0899CC08[i] == v)
            return i;
    }
    return 0;
}

/* backup of the options (restored when leaving the options menu without saving) */
void options_backup(u8 *g)
{
    int i;

    for (i = 0; i < 14; i++)
        g[0x6AF1C + i] = g[0x6ADD8 + i];
    U32(g, 0x6AF2C) = U32(g, 0x6ADE8);
    U32(g, 0x6AF30) = U32(g, 0x6ADEC);
    U32(g, 0x6AF34) = U32(g, 0x6ADF0);
    U32(g, 0x6AF38) = U32(g, 0x6ADF4);
    U32(g, 0x6AF3C) = U32(g, 0x6ADF8);
}

extern void *D_09CFB798;
extern const u8 D_0899C8C8[];
void game_clear_flag(u8 *g, s32 i);
s32 func_09C19080(void *a, int b);

/* index of the slot of the item id in the pouch (0xFF = none) */
s32 pouch_find(u8 *g, u32 id_arg)
{
    u16 id = id_arg;
    s32 i;

    for (i = 0; i < 0x18; i++) {
        if (pouch_slot(g, i)[0] == id)
            return (s16)i;
    }
    return 0xFF;
}

int village_flags_any(u8 *g);
int village_min_rank(u8 *g, u32 n_arg);
void village_random_visitor(u8 *g);
int village_rare_visitor(u8 *g);

/* daily choice of the village visitors (flags 7, 0x5D, 0x30, 0x31) */
void village_update_visitors(u8 *g)
{
    if ((u8)game_test_flag(g, 0x25) != 1) {
        game_set_flag(g, 7);
        game_clear_flag(g, 0x5D);
        game_clear_flag(g, 0x30);
        return;
    }
    if ((u8)village_flags_any(g) != 1 || (u8)func_09C19080(D_09CFB798, 2) != 1) {
        village_random_visitor(g);
        return;
    }
    if ((u8)game_test_flag(g, 0x31) == 0) {
        game_set_flag(g, 0x31);
        game_set_flag(g, 0x30);
        game_clear_flag(g, 7);
        game_clear_flag(g, 0x5D);
    } else if ((u8)village_rare_visitor(g) == 0) {
        village_random_visitor(g);
    }
}

void village_first_visitor(u8 *g)
{
    if ((u8)village_flags_any(g) == 1 && (u8)game_test_flag(g, 0x25) == 1 && (u8)village_min_rank(g, 2) == 1
        && (u8)game_test_flag(g, 0x31) == 0) {
        game_set_flag(g, 0x31);
        game_set_flag(g, 0x30);
        game_clear_flag(g, 7);
        game_clear_flag(g, 0x5D);
        game_set_flag(g, 0x32);
    }
}

void village_reset_visitors(u8 *g)
{
    if ((u8)game_test_flag(g, 0x25) == 1) {
        game_clear_flag(g, 7);
        game_set_flag(g, 0x5D);
    } else {
        game_set_flag(g, 7);
        game_clear_flag(g, 0x5D);
    }
    game_clear_flag(g, 0x30);
    game_set_flag(g, 0x26);
    game_set_flag(g, 0x5E);
    game_set_flag(g, 0xCD);
}

void village_random_visitor(u8 *g)
{
    if (((u16)game_rand(D_08A631A0, 1) & 1) != 0) {
        game_clear_flag(g, 7);
        game_set_flag(g, 0x5D);
    } else {
        game_set_flag(g, 7);
        game_clear_flag(g, 0x5D);
    }
    game_clear_flag(g, 0x30);
}

/* 1 in 8: the rare visitor comes */
int village_rare_visitor(u8 *g)
{
    if (((u16)game_rand(D_08A631A0, 1) & 7) == 0) {
        game_set_flag(g, 0x30);
        game_clear_flag(g, 7);
        game_clear_flag(g, 0x5D);
        return 1;
    }
    game_clear_flag(g, 0x30);
    return 0;
}

int village_flags_any(u8 *g)
{
    u32 f = U32(g, 0x6ADF8);

    return (f & 1) || (f & 2) || (f & 4);
}

/* 1 if the village rank (g+0x4B8) is at least n (n >= 15: always) */
int village_min_rank(u8 *g, u32 n_arg)
{
    u16 n = n_arg;

    if (n == 0 || n >= 15)
        return 1;
    return U16(g, 0x4B8) >= n;
}

/* first of the 3 friends with the flag 2 */
u8 *friend_marked(u8 *g)
{
    int i;

    for (i = 0; i < 3; i++) {
        if (S8(g, 0x69BEC + i * 0x70) & 2)
            return g + i * 0x70 + 0x69BEC;
    }
    return NULL;
}

/* 0x4ED bit flags at g+0x6A7B4 */
int game_test_flag_6a7b4(u8 *g, u32 i_arg)
{
    u16 i = i_arg;

    if (i == 0xFFFF || i >= 0x4ED)
        return 0;
    return (U32(g, 0x6A7B4 + (i >> 5) * 4) & (1 << (i & 0x1F))) != 0;
}

void game_set_flag_6a7b4(u8 *g, u32 i_arg)
{
    u16 i = i_arg;

    U32(g, 0x6A7B4 + (i >> 5) * 4) |= 1 << (i & 0x1F);
}

const u8 *kitchen_default(void *g, s32 i)
{
    return D_0899C8C8 + i * 6;
}
