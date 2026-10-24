/* The current quest (object D_08A62C7C, 0x8A8 bytes): quest file, start area, counters.
 * (Guessed names.) */
#include "menu.h"
#include "vfpu.h"

extern u8 D_08A5C440[];            /* header of the loaded quest file */
extern s32 D_08A5C464;
extern u8 D_08A62C8C[15];
extern u8 D_08A62C80[10];
extern const u8 D_089A7EA4[10];
extern u8 *D_08A62CB4;
extern u8 *D_09C121B0;
extern void *D_09A1DAEC;
extern void *D_08A62D90;
extern void **D_09A4F060;
extern u16 **D_09CD3CF0[];         /* start areas for each map (3 weighted lists) */
extern u16 *D_09CE470C[][2];
extern const u8 D_089A609C[0x172][16];   /* points of the areas: area, x, y, z */

u16 game_map_id(u8 *g);
int has_kitchen_skill(u8 *g, u32 k);
int game_rank(u8 *g);
int quest_flag_10(void);
u32 func_088900C4(void *a, int b);
u16 func_088C722C(void *a, u32 b, u32 c);
void func_088AB514(void *a, int b, void *c);
void func_09A64E48(void *a, int b);
void func_08870D30(u8 *self);
void spots_init(u8 *self);
void func_0886E23C(u8 *self);
void func_08872BFC(u8 *self, u8 *p);
void func_0886E27C(u8 *self);
void quest_init_monsters(u8 *self);
void func_08895EA0(void *a);
void func_08872E48(u8 *self, s32 i);
void func_08873554(u8 *self, u8 *p);
s32 func_08873488(void *q);
void func_08873838(void *q);
void *memcpy(void *d, const void *s, size_t n);

int quest_is_flag_800000(u8 *self);
void quest_rand_seed(u8 *self);
u32 quest_rand(u8 *self);
void quest_clear_goals(u8 *self);
void quest_load_goals(u8 *self);
void quest_clear_waves(u8 *self);
void quest_pick_start_area(u8 *self, u32 who);
s32 quest_state(u8 *self, s32 mode);

/* counts the quests done for each kind of quest */
void quest_count_kind(u8 *self)
{
    s32 k;
    u8 *c;

    switch (*(s8 *)(U32(self, 0x68) + 0x1C)) {
    case 1: k = 0xC; break;
    case 2: case 0x10: k = 0; break;
    case 3: case 0x11: k = 2; break;
    case 4: case 0x12: k = 3; break;
    case 5: case 0x13: k = 4; break;
    case 6: case 0x14: k = 1; break;
    case 0xB: case 0x15: k = 5; break;
    case 0xC: k = 0xD; break;
    case 0xD: case 0xE: case 0xF: k = 6; break;
    case 0x1A: k = 8; break;
    case 0x1B: k = 10; break;
    case 0x1C: k = 0xB; break;
    case 0x1D: k = 9; break;
    case 0x1E: case 0x1F: k = 7; break;
    default: return;
    }
    c = D_089CC438 + 0x6A53C + k;
    if (*c < 0xFF)
        (*c)++;
}

/* 1 when the hunter is not in the map of the quest (and the quest does not allow it) */
int quest_outside_map(u8 *self)
{
    u16 cur = U16(D_089CC438, 0x6AF0E);

    if (game_map_id(D_089CC438) == cur)
        return 0;
    return (U32(self, 0x28) & 0x1000) == 0;
}

/* data of the object id of the current map */
u8 *quest_map_object(u8 *self, u32 id)
{
    u32 *t = (u32 *)U32(self, 0x60);
    u16 i = func_088C722C(D_09A4F060, U8(D_089CC438, 0x6AF10), id);

    if (i == 0xFFFF)
        return NULL;
    if (t[i] == 0)
        return NULL;
    return (u8 *)(t[i] + U32(self, 0x50));
}

/* moves pos to the last point of the current area within the radius in[3] */
void quest_snap_point(u8 *self, f32 *out, const f32 *in)
{
    f32 x = in[0], y = in[1], z = in[2];
    const u8 *e = D_089A609C[0];
    s32 n = 0;
    u32 i;

    for (i = 0; i < 0x172; i++, e += 16) {
        if (U16(e, 0) != U16(D_089CC438, 0x6AF0E))
            continue;
        if (vfpu_dist3_sq(in, (const f32 *)(e + 4)) <= in[3] * in[3]) {
            out[0] = F32(e, 4);
            out[1] = F32(e, 8);
            out[2] = F32(e, 0xC);
            n++;
        }
    }
    if (n == 0) {
        out[0] = x;
        out[1] = y;
        out[2] = z;
    }
}

u8 *quest_table_entry(u8 *self, s32 i)
{
    return (u8 *)(*(s32 *)(D_08A5C464 + U32(self, 0x50) + i * 4) + U32(self, 0x50));
}

/* list of rewards: 0 = main, 1 = secondary objective */
u8 *quest_reward_list(u8 *self, s32 which)
{
    u8 *t = (u8 *)U32(self, 0x71C);
    u32 off = 0;

    if (which == 0)
        off = U32(t, 8);
    else if (which == 1)
        off = U32(t, 0xC);
    if (U16(D_089CC438, 0x6AF12) == 0)
        return (u8 *)off;
    if (off == 0)
        return NULL;
    return (u8 *)(off + U32(self, 0x50));
}

/* entry id of the list idx (entries of 16 bytes, two offsets at +8 / +0xC) */
u8 *quest_find_entry(u8 *self, u32 id, s32 which, s32 idx)
{
    u32 t, *p;
    s32 off = 0;

    if (U16(D_089CC438, 0x6AF12) == 0)
        return NULL;
    t = *(u32 *)(U32(self, 0x718) + (s8)idx * 4);
    if (t == 0)
        return NULL;
    p = (u32 *)(t + U32(self, 0x50));
    for (;;) {
        if (p[0] == 0)
            return NULL;
        if (p[0] == (id & 0xFFFF))
            break;
        p += 4;
    }
    if (which == 0)
        off = p[2];
    else if (which == 1)
        off = p[3];
    if ((u32)(off + 1) < 2)
        return (u8 *)off;
    return (u8 *)(off + U32(self, 0x50));
}

u8 *quest_sub_data(u8 *self)
{
    if (U16(D_089CC438, 0x6AF12) == 0)
        return NULL;
    return (u8 *)(U32((u8 *)U32(self, 0x4C), 8) + U32(self, 0x50));
}

void quest_ctor(u8 *self)
{
    memset(self, 0, 0x8A8);
    func_08870D30(self);
}

int quest_is_flag_800000(u8 *self)
{
    return (*(u32 *)U32(self, 0x68) & 0x800000) != 0;
}

/* start of a quest: random seeds, counters and pointers into the quest file */
void quest_start(u8 *self)
{
    u8 *g;
    s32 i, v;
    s8 c;

    D_089CC438[0x2E] = 2;
    spots_init(self);
    func_0886E23C(self);
    quest_clear_goals(self);
    if ((*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) == 0) {
        for (i = 0; i < 15; i++)
            self[0x11D + i] = func_088900C4(D_08A631A0, 0);
    } else {
        for (i = 0; i < 15; i++)
            self[0x11D + i] = D_08A62C8C[i];
    }
    for (i = 0; i < 10; i++)
        D_08A62C80[i] = D_089A7EA4[i];
    quest_rand_seed(self);
    quest_rand(self);
    for (i = 0; i < 32; i++) {
        U16(self, 0x8C + i * 4) = 0;
        self[0x8E + i * 4] = 0;
    }
    for (i = 0; i < 90; i++) {
        U16(self, 0x1E8 + i * 2) = 0;
        U16(self, 0x134 + i * 2) = 0;
        U16(self, 0x29C + i * 2) = 0;
        U16(self, 0x350 + i * 2) = 0;
    }
    for (i = 0; i < 4; i++)
        self[0x11D + i] = quest_rand(self);
    U32(self, 0x110) = 0;
    U32(self, 0x10C) = 0;
    self[0x11C] = 0;
    self[0x117] = 0;
    self[9] = 0;
    for (i = 0; i < 8; i++)
        self[0x84 + i] = 0;
    U32(self, 0x5B0) = 0;
    U32(self, 0x5B4) = 0;
    self[0x5AC] = 0;
    U32(self, 0x5B8) = 0;
    U32(self, 0x5BC) = 0;
    v = game_rank(D_089CC438);
    self[D_089CC438[0x28] * 2 + 0x85] = v;
    D_089CC438[0x24] = 0;
    D_089CC438[0x25] = 0;
    D_089CC438[0x26] = 0;
    D_089CC438[0x27] = 0;
    for (i = 0; i < 5; i++) {
        U16(self, 0x6C + i * 4) = 0;
        self[0x6E + i * 4] = 0;
    }
    self[0x80] = 0;
    self[0x81] = 0;
    self[3] = 0;
    self[0] = 0;
    self[1] = 0;
    self[0x20] = 0;
    self[0x82] = 0;
    U32(self, 0x24) = 0;
    U32(self, 0x10) = 0;
    self[4] = 0;
    U16(self, 0x5C0) = 0;
    U32(self, 0x50) = (u32)D_08A5C440;
    U32(self, 0x4C) = (u32)D_08A5C440;
    U32(self, 0x68) = U32(D_08A5C440, 0) + U32(self, 0x50);
    D_089CC438[0x6AF10] = *(s8 *)(U32(self, 0x68) + 0x1C);
    quest_pick_start_area(self, 0xFF);
    v = func_0886A35C(self, 1);
    U32(self, 0x10) = v;
    c = (v - 1) / 9000;
    self[0x83] = c;
    if (c > 2)
        self[0x83] = 2;
    U32(self, 0x54) = U32(D_08A5C440, 0x10) + U32(self, 0x50);
    U16(self, 0x1C) = 0;
    U16(self, 0x1E) = 0;
    U32(self, 0x58) = U32((u8 *)U32(self, 0x68), 0x14) + U32(self, 0x50);
    U32(self, 0x718) = U32(D_08A5C440, 0x18) + U32(self, 0x50);
    U32(self, 0x71C) = U32(D_08A5C440, 0x14) + U32(self, 0x50);
    U32(self, 0x60) = U32(D_08A5C440, 0x1C) + U32(self, 0x50);
    U32(self, 0x5C) = U32(D_08A5C440, 0xC) + U32(self, 0x50);
    U32(self, 0x14) = U32((u8 *)U32(self, 0x68), 8);
    U32(self, 0x18) = U32((u8 *)U32(self, 0x68), 0xC);
    U32(self, 0x28) = U32((u8 *)U32(self, 0x68), 0);
    self[0x7C0] = D_08A5C440[0x4B];
    self[0x116] = D_08A5C440[0x49];
    func_08872BFC(self, self + 0x7C0);
    if ((u8)quest_is_flag_800000(self) == 1)
        D_089CC438[0x41A] = 1;
    else
        D_089CC438[0x41A] = 0;
    self[0x12D] = D_08A5C440[0x46];
    self[0x12E] = D_08A5C440[0x47];
    self[0x12F] = D_08A5C440[0x48];
    self[0x130] = 0;
    self[0x131] = (s32)U32(self, 0x10) / 9000;
    func_0886E27C(self);
    quest_load_goals(self);
    for (i = 0; i < 90; i++) {
        self[0x5C2 + i] = 0;
        self[0x61C + i] = 0;
    }
    quest_clear_waves(self);
    quest_init_monsters(self);
    func_08895EA0(D_089CBBEC);
    if (quest_flag_10() != 0) {
        g = D_089CC438;
        memcpy(g + 0x6ADFC, g + 0x43C8, 0x60);
        for (i = 0; i < 0x30; i++)
            U16(g, 0x43C8 + i * 2) = 0;
    }
    for (i = 0; i < 6; i++)
        func_08872E48(self, i);
    U32(D_08A62CB4, 0x104) = U32(D_089CC438, 0x696F0);
    U32(D_08A62CB4, 0x108) = U32(D_089CC438, 0x696E8);
    func_08873554(self, self + 0x404);
    self[0x767] = 1;
    if (func_08873488(D_08A62C7C) != 0)
        func_08873838(D_08A62C7C);
    self[0x67D] = 0;
    self[0x67E] = 0;
    self[0x67F] = 0;
    self[0x680] = 0;
    self[0x681] = 0;
    U16(self, 0x682) = 0;
    self[0x8A4] = 0;
    D_09C121B0[0x1278] = 0;
}

/* quest failed (state 7) */
void quest_fail(u8 *self)
{
    if (quest_state(self, 1) != 0)
        return;
    D_089CC438[0x2E] = 7;
    self[6] = 7;
    U16(self, 0x1C) = 0xFFFF;
    func_088AB514(D_09A1DAEC, 0xC, NULL);
}

/* start area of the quest (who = 0xFF: area of the game, else area of that hunter) */
void quest_pick_start_area(u8 *self, u32 who)
{
    u8 *g = D_089CC438;
    u16 **t = D_09CD3CF0[*(s8 *)(U32(self, 0x68) + 0x1C)];
    s8 c;
    u16 *l, *p;
    s32 sum, r, acc;
    u8 seed;

    c = g[0x480] == 0 ? *(s8 *)(U32(self, 0x4C) + 0x45) : 0;
    if (c == 0) {
        if ((who & 0xFFFF) != 0xFF)
            goto hunter;
        if (g[0x480] == 0)
            U16(g, 0x6AF0E) = *D_09CE470C[g[0x6AF10]][0];
        else
            U16(g, 0x6AF0E) = U16(g, 0x484);
        return;
    }
    c = g[0x480] == 0 ? *(s8 *)(U32(self, 0x4C) + 0x45) : 0;
    if (c == 1)
        l = has_kitchen_skill(g, 0x1F) != 0 ? t[1] : t[0];
    else
        l = t[2];
    /* weighted lists: {weight, area} ... 0xFF */
    sum = 0;
    for (p = l; *p != 0xFF; p += 2)
        sum += *p;
    who &= 0xFFFF;
    seed = who == 0xFF ? self[0x11D + D_089CC438[0x28]] : self[0x11D + who];
    r = (s32)seed % sum;
    acc = 0;
    p = l;
    while (*p != 0xFF) {
        acc += *p;
        if (r < acc) {
            p++;
            break;
        }
        p += 2;
    }
    if (who == 0xFF) {
        U16(D_089CC438, 0x6AF0E) = *p;
        return;
    }
hunter:
    {
        u16 a = U16(D_089CC438, 0x6AF0E);
        U16(func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]), 0x29A) = a;
    }
}

void quest_check_end(u8 *self)
{
    if (quest_state(self, 1) == 0)
        func_09A64E48(D_08A62D90, 0);
}

/* 1 while the quest is finishing (state 3-4), -1 when it ended (5-7, mode 1) */
s32 quest_state(u8 *self, s32 mode)
{
    u8 s = D_089CC438[0x2E];

    if ((u8)(s - 3) < 2)
        return 1;
    if (mode == 1 && (u8)(s - 5) < 3)
        return -1;
    return 0;
}

/* --- objectives, deliveries and end of the quest ----------------------------- */
typedef struct QEvent {         /* event sent to the network / replay (func_088AB514) */
    u8 type, b1;
    u16 a, b, c, d;
} QEvent;

extern const u16 D_089AC5A0[];     /* IDs of the quest messages */
extern const u16 D_089A7EB0[];
extern u8 *D_09A4F0D0;
extern u8 *D_08A62D8C;
extern void **D_089CC434;
extern void *D_09B23820;
extern const u8 *D_089A800C[];
extern const u16 D_09CE4808[], D_09CE5008[], D_09CE5220[];

const char *text_menu(void *g, u32 id);
u8 menu_msg_push_text(u8 *self, const char *s);
u8 menu_msg_push(u8 *self, u32 id, u32 x, u32 y);
void func_08883988(void *snd, int a, int b, int c, int d, int e, int f, int g, int h);
u8 *quest_find_monster(u8 *self, u8 *h);
u32 monster_is_large2(u8 *self, s32 id);
u8 func_0884FC74(u8 *g, u32 i);
u16 func_09ACBF40(u8 *h);
u8 func_08872CC0(u8 *self, s32 a, s32 b);
void func_09AC7668(u8 *h);
void func_09AB46A8(u8 *a, u32 b);
void func_088DE3D4(u8 *a, u32 b);
void func_09AB2B40(u8 *a, u32 b);
u32 monster_is_large(u8 *self, s32 id);
void func_08870A2C(u8 *self, u8 *e, u8 *h);
void func_08870A80(u8 *self, u8 *e, u8 *h);
void func_0886E5BC(u8 *self, u32 i);
s32 func_08870264(u8 *self, u32 i);
void func_0886E0D8(u8 *self, u32 i, u32 f);
void func_08873044(u8 *self, u32 i, s32 n, s32 m);
void func_0886E92C(u8 *self, s32 a);
void spots_update(u8 *self);
void func_0886E36C(u8 *self);
void func_0886E828(u8 *self, s32 a);
void func_08872B7C(u8 *self);
s16 func_0886E9F4(u8 *self, u32 h, s32 a);
s32 func_08872B40(u8 *self);
void func_088B87B0(void *a, int b);
u8 *func_09A8A4B0(void *a);

int quest_check_objective(u8 *self, u8 *h);
void quest_hunt_done(u8 *self, u8 *h);
void quest_set_target(u8 *self, u8 *h);
int quest_pick_target_pos(u8 *self, u8 *h);
u32 quest_check_targets_done(u8 *self, u32 id);
void quest_bonus_check(u8 *self);
void quest_check_deliveries(u8 *self);
void quest_goal_reached(u8 *self);
void quest_kill_count(u8 *self, s32 id);

/* time limit (mode 1) or time left */
s32 quest_time(u8 *self, s32 mode)
{
    s32 t;

    if (mode == 1)
        return U32((u8 *)U32(self, 0x68), 0x10);
    t = U32(self, 0x10);
    if (t < 0)
        t = 0;
    return t;
}

/* 1 when the monster h is a target still to be hunted */
int quest_is_target(u8 *self, u8 *h)
{
    u8 *e;

    if (quest_state(self, 0) != 0)
        return 0;
    e = quest_find_monster(self, h);
    if (e == NULL)
        return 0;
    if ((S16(e, 0x2E) & 1) != 0)
        return quest_check_objective(self, h) == 0;
    return *(s8 *)(e + 0xA) != 1;
}

void quest_clear_waves(u8 *self)
{
    s32 i, j;
    u8 *w;

    for (i = 0; i < 2; i++) {
        w = self + i * 0x2C;
        for (j = 0; j < 5; j++) {
            w[0x77C + j] = 0;
            w[0x788 + j] = 0;
            w[0x78D + j] = 0;
            U32(w, 0x768 + j * 4) = 0;
            w[0x781 + j] = 0;
        }
        w[0x786] = 0;
        w[0x787] = 0;
    }
}

/* adds a monster to the wave i */
void quest_wave_add(u8 *self, s32 i, u16 *m)
{
    u8 *w = self + i * 0x2C;

    U32(w, 0x768 + *(s8 *)(w + 0x786) * 4) = (u32)m;
    w[0x781 + *(s8 *)(w + 0x786)] = *(s8 *)(m + 2);
    w[0x77C + *(s8 *)(w + 0x786)] = (s8)m[0];
    w[0x786]++;
}

/* 1 when h is the last monster of its wave */
int quest_check_objective(u8 *self, u8 *h)
{
    s32 i;
    u8 *w = self;

    for (i = 0; i < 2; i++, w += 0x2C) {
        if (*(s8 *)(w + 0x77C + *(s8 *)(w + 0x787)) == h[0x1E8]) {
            w = self + i * 0x2C;
            return !(*(s8 *)(w + 0x787) < *(s8 *)(w + 0x786) - 1);
        }
    }
    return 0;
}

int quest_is_target2(u8 *self, u8 *h)
{
    u8 *e;

    if (quest_state(self, 0) != 0)
        return 0;
    e = quest_find_monster(self, h);
    if (e == NULL || (S16(e, 0x2E) & 1) != 0 || *(s8 *)(e + 0xA) == 1)
        return 0;
    return 1;
}

/* spawns the next monster of each wave when its slot is free */
void quest_spawn_waves(u8 *self)
{
    s32 i;
    u8 *w = self, *e;
    u32 used, a, b;

    for (i = 0; i < 2; i++, w += 0x2C) {
        used = (i >= 0 && i < 20) ? U32(D_09C121B0, 0x1220 + i * 4) : 0;
        if (used != 0)
            continue;
        if (*(s8 *)(w + 0x786) <= 0)
            continue;
        if (!(*(s8 *)(w + 0x787) < *(s8 *)(w + 0x786)))
            continue;
        e = w + *(s8 *)(w + 0x787);
        if (e[0x78D] != 0)
            continue;
        a = e[0x77C];
        b = e[0x77B];
        func_09AB46A8(D_09C121B0, b);
        func_088DE3D4(D_09A4F0D0, b);
        func_09AB2B40(D_09C121B0, a);
        w[*(s8 *)(w + 0x787) + 0x78D] = 1;
    }
}

void quest_send_wave(u8 *self, u32 i)
{
    QEvent ev;

    ev.type = 0x15;
    ev.a = 0;
    ev.b = (u8)i;
    ev.c = *(s8 *)(self + (u8)i * 0x2C + 0x787);
    func_088AB514(D_09A1DAEC, 6, &ev);
}

/* a target monster h appears: position and area from the quest data */
s32 quest_place_target(u8 *self, u8 *h)
{
    u8 *e = quest_find_monster(self, h);
    u8 lim;

    if (e == NULL)
        return 0;
    if ((S16(e, 0x2E) & 1) != 0)
        return 0;
    U16(e, 0x2E) = 2;
    if ((u8)monster_is_large2(self, h[0x1E8]) == 1) {
        U16(h, 0x274) += 5;
        lim = func_0884FC74(D_089CC438, h[0x1E8]);
        if (D_089A7EB0[lim] < U16(h, 0x274))
            U16(h, 0x274) = D_089A7EB0[lim];
        U16(h, 0x5F4) = func_09ACBF40(h);
        if ((U32(self, 0x28) & 0x8000) == 0)
            h[0x4BC] = func_08872CC0(self, *(s8 *)(h + 0x4BC), 1);
    }
    U16(e, 0x32) = S16(h, 0x2E4);
    if (*(s8 *)(e + 0xB) != 1 || (u8)quest_pick_target_pos(self, h) != 1) {
        F32(h, 0x200) = F32(e, 0x20);
        F32(h, 0x204) = F32(e, 0x24);
        F32(h, 0x208) = F32(e, 0x28);
        U32(h, 0x1F4) = U32(e, 0x1C);
    }
    U16(h, 0x29A) = U16(e, 8);
    ((void (*)(u8 *))((void **)U32(h, 0))[0xF4 / 4])(h);
    func_09AC7668(h);
    return *(s8 *)(e + 4);
}

/* random start point of the target in its area (list of {area, n, offset}) */
int quest_pick_target_pos(u8 *self, u8 *h)
{
    u8 *base = (u8 *)U32(self, 0x50);
    u16 *l = (u16 *)(U32((u8 *)U32(self, 0x4C), 0x20) + (u32)base);
    u8 *st = (u8 *)D_09A4F060;
    u32 id, f, r;
    u32 *p;

    for (;; l += 4) {
        id = l[0];
        if (id == 0xFFFF)
            return 0;
        f = D_09CE4808[U32(st, 0x284)] != 0xFFFF;
        if (f != D_09CE5008[id] && D_09CE5220[id] != 0xFFFF)
            id = D_09CE5220[id];
        if (U16(h, 0x29A) == id)
            break;
    }
    r = func_088900C4(D_08A631A0, 0) & 0xFFFF;
    p = (u32 *)(U32(l, 4) + (u32)base + (s32)r % (s32)l[1] * 0x10);
    U32(h, 0x1F4) = p[0];
    U32(h, 0x200) = p[1];
    U32(h, 0x204) = p[2];
    U32(h, 0x208) = p[3];
    return 1;
}

/* records the best / worst times of the hunt of h */
static void record_time(u8 *self, u8 *h)
{
    u32 i;

    i = h[0x1E8];
    if (U16(self, 0x134 + i * 2) < U16(h, 0x274))
        U16(self, 0x134 + i * 2) = U16(h, 0x274);
    i = h[0x1E8];
    if (U16(self, 0x1E8 + i * 2) == 0 || U16(h, 0x274) < U16(self, 0x1E8 + i * 2))
        U16(self, 0x1E8 + i * 2) = U16(h, 0x274);
}

/* target h killed */
void quest_target_killed(u8 *self, u8 *h)
{
    u8 *e = quest_find_monster(self, h);
    u32 i;

    if (e == NULL)
        return;
    if ((S16(e, 0x2E) & 5) != 0) {
        quest_set_target(self, h);
        return;
    }
    e[4]--;
    quest_hunt_done(self, h);
    quest_kill_count(self, h[0x1E8]);
    i = h[0x1E8];
    U16(self, 0x29C + i * 2)++;
    i = h[0x1E8];
    if (U16(self, 0x29C + i * 2) >= 10000)
        U16(self, 0x29C + i * 2) = 9999;
    if ((u8)monster_is_large(self, h[0x1E8]) == 1)
        record_time(self, h);
    if (*(s8 *)(e + 4) > 0) {
        U16(e, 0x2E) = 4;
    } else {
        e[0x34] = 0xFF;
        U16(e, 0x2E) = 1;
    }
    quest_set_target(self, h);
    quest_check_targets_done(self, h[0x1E8]);
    if (h[0x6DB] != 0)
        func_08870A2C(self, e, h);
}

void quest_target_left(u8 *self, u8 *h)
{
    u8 *e = quest_find_monster(self, h);

    if (e != NULL && (S16(e, 0x2E) & 1) == 0) {
        U16(e, 0x32) = U16(h, 0x2E4);
        e[0x34] = 0xFF;
    }
}

/* target h captured */
void quest_target_captured(u8 *self, u8 *h)
{
    u8 *e = quest_find_monster(self, h);
    u32 i;

    if (e == NULL)
        return;
    if ((S16(e, 0x2E) & 8) != 0) {
        quest_set_target(self, h);
        return;
    }
    i = h[0x1E8];
    U16(self, 0x350 + i * 2)++;
    i = h[0x1E8];
    if (U16(self, 0x350 + i * 2) >= 10000)
        U16(self, 0x350 + i * 2) = 9999;
    if ((u8)monster_is_large(self, h[0x1E8]) == 1)
        record_time(self, h);
    quest_set_target(self, h);
    func_0886E5BC(self, h[0x1E8]);
    if ((S16(e, 0x2E) & 5) == 0) {
        func_08870A80(self, e, h);
        e[4]--;
        quest_hunt_done(self, h);
        if (*(s8 *)(e + 4) > 0) {
            U16(e, 0x2E) = 4;
        } else {
            e[0x34] = 0xFF;
            U16(e, 0x2E) = 1;
        }
    }
    U16(e, 0x2E) |= 8;
}

void quest_set_target(u8 *self, u8 *h)
{
    if (func_08870264(self, h[0x1E8]) != 0)
        U32(self, 0x24) = (u32)h;
}

/* goals "hunt N monsters of kind id": 1 when one of them is now complete */
u32 quest_check_targets_done(u8 *self, u32 id)
{
    u32 *g = (u32 *)(self + 0x728);
    u32 r = 0, f;
    s32 i;

    for (i = 0; i < 2; i++, g += 2) {
        f = g[0];
        if ((f & 0x60) != 0)
            continue;
        if ((f & 0x11) == 0x11 && U16(g, 4) == (id & 0xFFFF)) {
            if ((f & 0x100) != 0) {
                g[0] = f | 0x40;
                return 0;
            }
            S16(g, 6)--;
            if (S16(g, 6) <= 0) {
                func_0886E0D8(self, i & 0xFFFF, 0x200);
                r = 1;
            } else {
                func_08873044(self, id & 0xFF, S16(g, 6), S16(self, 0x73E + i * 8));
            }
        }
        if ((g[0] & 0x8014) == 0x8014 && U16(g, 4) == (id & 0xFFFF)) {
            func_0886E0D8(self, i & 0xFFFF, 0x8000);
            r = 1;
        }
    }
    return r;
}

/* time passes: 1 when the time is over, 2 when there is no time limit */
s32 quest_tick(u8 *self)
{
    s32 t;

    if ((s32)U32(self, 0x14) <= 0)
        return 2;
    t = U32(self, 0x14) - U32(self, 0x18);
    U32(self, 0x14) = t;
    if (t > 0)
        return 0;
    if (quest_state(self, 1) == 0)
        func_0886E92C(self, 0);
    if ((s32)U32(self, 0x14) <= 0)
        U32(self, 0x14) = 0;
    return 1;
}

/* marks in 0x110 the players that are not present (bit shift + i) */
static void mark_absent(u8 *self, s32 shift)
{
    s32 i;

    for (i = 0; i < 4; i++)
        if (D_089CC438[(u8)i * 0x8C + 0x30] == 0)
            U32(self, 0x110) |= 1 << (i + shift);
}

static void push_msg(u32 i)
{
    menu_msg_push_text(D_089CBBEC, text_menu(D_089CC438, D_089AC5A0[i]));
}

/* per-frame update of the quest: 0, 1 (finished), -1, -2 */
s32 quest_update(u8 *self)
{
    u32 h = func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]);
    QEvent ev;
    u32 v;

    mark_absent(self, 8);
    spots_update(self);
    if (U16(D_089CC438, 0x6AF12) == 0)
        return 0;
    if (*(s8 *)(self + 6) == 7)
        return -2;
    if (*(s8 *)(self + 6) == 8)
        return -2;
    if (*(s8 *)(self + 0x5AC) != 0) {
        ev.type = 0xF;
        ev.a = 0;
        v = U32(self, 0x5AC + *(s8 *)(self + 0x5AC) * 4);
        ev.c = v;
        ev.b = v >> 16;
        func_088AB514(D_09A1DAEC, 6, &ev);
        self[0x5AC] = 0;
        ev.type = 0x10;
        ev.a = 0;
        ev.b = U32(self, 0x5B8);
        ev.c = U32(self, 0x5B8) >> 16;
        ev.d = U32(self, 0x5BC);
        func_088AB514(D_09A1DAEC, 6, &ev);
    }
    switch (*(s8 *)(self + 0)) {
    case 3:
        return 0;
    case 2:
        switch (*(s8 *)(self + 1)) {
        case 3:
            return 0;
        case 2:
            if ((s32)U32(self, 0x118) > 0)
                U32(self, 0x118)--;
            mark_absent(self, 4);
            if (self[0x110] != 0xFF && (s32)U32(self, 0x118) > 0)
                return 0;
            self[1]++;
            D_089CC438[0x2E] = 4;
            U32(self, 0x14) = (s32)U32(self, 0x14) / (s32)self[0x70C];
            func_08873554(self, self + 0x4B8);
            return 0;
        case 1:
            if ((s32)U32(self, 0x118) > 0)
                U32(self, 0x118)--;
            mark_absent(self, 0);
            if ((U32(self, 0x110) & 0xF) != 0xF && (s32)U32(self, 0x118) > 0)
                return 0;
            self[1]++;
            ev.type = 0xD;
            ev.a = 0;
            ev.c = U32(self, 0x10C);
            ev.b = U32(self, 0x10C) >> 16;
            func_088AB514(D_09A1DAEC, 6, &ev);
            U32(self, 0x110) |= 1 << (D_089CC438[0x28] + 4);
            U32(self, 0x118) = 1800;
            return 0;
        case 0:
            self[1] = 1;
            self[6] = 4;
            self[D_089CC438[0x28] * 2 + 0x84] = game_rank(D_089CC438);
            ev.type = 0xC;
            ev.a = 0;
            ev.b = self[D_089CC438[0x28] * 2 + 0x84];
            ev.c = 0;
            func_088AB514(D_09A1DAEC, 6, &ev);
            U32(self, 0x110) |= 1 << D_089CC438[0x28];
            U32(self, 0x118) = 5400;
            return 0;
        }
        return 0;
    case 1:
        switch (*(s8 *)(self + 1)) {
        case 2:
            if ((s32)--U32(self, 0x710) > 0)
                return 0;
            self[0]++;
            self[1] = 0;
            push_msg(2);
            if ((U32(D_089CC438, 0x6AF14) & 4) == 0)
                menu_msg_push(D_089CBBEC, 0, 0xF, 0);
            self[0x767] = 0;
            return 0;
        case 1:
            if ((s32)--U32(self, 0x710) > 0)
                return 0;
            self[1]++;
            if ((U32(self, 0x28) & 0x80000) == 0 ||
                ((U32(self, 0x28) & 0x200000) != 0 && func_08872B40(self) == 0)) {
                U32(self, 0x710) = 1800;
                push_msg(1);
            } else {
                U32(self, 0x710) = 600;
                push_msg(0);
                if ((U32(self, 0x28) & 0x10) != 0)
                    push_msg(10);
            }
            return 0;
        case 0:
            push_msg((U32(self, 0x28) & 0x10) == 0 ? 0xC : 0x1A);
            self[1]++;
            U32(self, 0x710) = 150;
            return 0;
        }
        return 0;
    case 0:
        if (*(s8 *)(self + 6) == 4)
            return 1;
        if (*(s8 *)(self + 6) == 6)
            return -1;
        if (S16(self, 0x1C) == -1)
            return 1;
        func_0886E36C(self);
        func_0886E828(self, 1);
        func_08872B7C(self);
        U16(self, 0x1C) = func_0886E9F4(self, h, S16(self, 0x1C));
        quest_bonus_check(self);
        quest_check_deliveries(self);
        quest_spawn_waves(self);
        return 0;
    }
    return 0;
}

/* seed of the quest random generator, from the 15 seed bytes */
void quest_rand_seed(u8 *self)
{
    s32 i;

    U16(self, 0x132) = 0;
    for (i = 0; i < 7; i++)
        U16(self, 0x132) += (u16)(self[0x11D + i * 2] + (u16)(self[0x11E + i * 2] << 8));
    if (U16(self, 0x132) == 0)
        U16(self, 0x132) = 0x1C3;
}

u32 quest_rand(u8 *self)
{
    u32 v = U16(self, 0x132);

    if (v == 0)
        v = 1;
    v = (v * 0xB0) % 0xFF53;
    U16(self, 0x132) = v;
    return v & 0xFFFF;
}

static void bonus_failed(void)
{
    push_msg(9);
    func_08883988(D_08A62D8C, 1, 0x77, 0, 0, 0, 0, 0, 0);
}

/* time bonus (kind 1): lost after the time or by chance */
void quest_bonus_check(u8 *self)
{
    const u8 *t = D_089A800C[*(s8 *)(self + 0x131)];
    const u8 *p;
    QEvent ev;

    if (*(s8 *)(self + 0x12D) == 0 || *(s8 *)(self + 0x12D) != 1)
        return;
    p = t + *(s8 *)(self + 0x130) * 2;
    if ((s32)U32(self, 0x10) > p[0] * 1800)
        return;
    self[0x130]++;
    if ((func_088900C4(D_08A631A0, 0) & 0x7F) >= p[1])
        return;
    self[0x12D] = 0;
    bonus_failed();
    ev.type = 0xE;
    ev.a = 0;
    ev.b = 0;
    ev.c = 0;
    func_088AB514(D_09A1DAEC, 6, &ev);
}

/* hunting bonus (kind 2): lost after hunting the wrong monsters */
void quest_hunt_done(u8 *self, u8 *h)
{
    int bad;

    if (*(s8 *)(self + 0x12D) == 0 || *(s8 *)(self + 0x12D) != 2)
        return;
    if (*(s8 *)(self + 0x12E) != 0) {
        if (*(s8 *)(self + 0x12E) != h[0x1E8])
            return;
        if (--*(s8 *)(self + 0x12F) <= 0) {
            self[0x12D] = 0;
            bonus_failed();
        }
        return;
    }
    bad = 0;
    if (*(s8 *)(self + 0x786) != 0 && *(s8 *)(self + 0x7B2) == 0) {
        bad = 1;
    } else if (*(s8 *)(self + 0x786) != 0) {
        if (*(s8 *)(self + 0x7B2) != 0 &&
            *(s8 *)(self + 0x781) != *(s8 *)(U32(self, 0x768) + 4) &&
            *(s8 *)(self + 0x7AD) != *(s8 *)(U32(self, 0x794) + 4))
            bad = 1;
    }
    if (*(s8 *)(self + 0x12F) > 0)
        (*(s8 *)(self + 0x12F))--;
    if (*(s8 *)(self + 0x12F) <= 0 && bad) {
        self[0x12D] = 0;
        bonus_failed();
    }
}

/* the quest goal is reached: event and "goal" flags of the monsters cleared */
void quest_goal_reached(u8 *self)
{
    QEvent ev;
    u8 *m = D_09A4F0D0;
    s32 i;

    ev.a = 0;
    ev.type = 6;
    ev.b = *(s8 *)(self + 0x20);
    ev.c = 0;
    func_088AB514(D_09A1DAEC, 6, &ev);
    U32(m, 0x210) = U32(m, 0x204);
    U32(m, 0x20C) = U32(m, 0x208);
    for (i = 0x15; i <= 0x2A; i++)
        U16(m, i * 0xC) &= 0xFFFD;
    func_088B87B0(D_089CC434, 1);
}

/* delivery goals (kind 2): enough items in the pouch */
void quest_check_deliveries(u8 *self)
{
    u8 *g = self + 0x74C;
    s32 i;

    for (i = 0; i < 2; i++, g += 8) {
        if (U32(g, 0) != 2)
            continue;
        if ((s32)func_088568AC(D_089CC438, U16(g, 4)) < *(s8 *)(g + 6))
            continue;
        if (*(s8 *)(self + 0x20) < *(s8 *)(g + 7)) {
            self[0x20] = g[7];
            quest_goal_reached(self);
        }
        U32(g, 0) = 0;
    }
}

/* hunting goals (kind 1): monster id hunted */
void quest_kill_count(u8 *self, s32 id)
{
    u8 *g = self + 0x74C;
    s32 i;

    for (i = 0; i < 2; i++, g += 8) {
        if (U32(g, 0) != 1 || S16(g, 4) != (s16)id)
            continue;
        if (--*(s8 *)(g + 6) > 0)
            continue;
        if (*(s8 *)(self + 0x20) < *(s8 *)(g + 7)) {
            self[0x20] = g[7];
            quest_goal_reached(self);
        }
        U32(g, 0) = 0;
    }
}

void quest_clear_goals(u8 *self)
{
    U32(self, 0x74C) = 0;
    U16(self, 0x750) = 0;
    self[0x752] = 0;
    U32(self, 0x754) = 0;
    U16(self, 0x758) = 0;
    self[0x75A] = 0;
}

void quest_load_goals(u8 *self)
{
    u8 *d = self + 0x74C;
    u8 *s = (u8 *)U32(self, 0x4C) + 0x28;
    s32 i;

    for (i = 0; i < 2; i++, d += 8, s += 8) {
        if (U32(s, 0) == 0)
            continue;
        U32(d, 0) = U32(s, 0);
        U16(d, 4) = U16(s, 4);
        d[6] = s[6];
        d[7] = i + 1;
    }
}

/* text i of the quest file in the current language */
u8 *quest_text(u8 *self, s32 i)
{
    if ((U32(D_089CC438, 0x6AF14) & 1) != 0)
        return func_09A8A4B0(D_09B23820);
    return (u8 *)(*(s32 *)(*(u32 *)(U32(self, 0x58) + LANG * 4) + U32(self, 0x50) + i * 4) +
                  U32(self, 0x50));
}

/* copies the reward list (ids until -1), returns the count */
s32 quest_rewards(u8 *self, u8 *out)
{
    s32 n = 0;
    s32 *a = (s32 *)quest_reward_list(self, 0);
    u8 *b = quest_reward_list(self, 1);

    if (a == NULL || b == NULL)
        return 0;
    while (*a != -1) {
        *out++ = *a++;
        n++;
    }
    return n;
}

/* the 5 slots of items given by the quest (id, count) */
s32 quest_item_count(u8 *self, u32 id)
{
    s16 i;

    for (i = 0; i < 5; i++)
        if (U16(self, 0x6C + i * 4) == (id & 0xFFFF))
            return *(s8 *)(self + 0x6E + i * 4);
    return 0;
}

s32 quest_item_add(u8 *self, u32 id, s32 n)
{
    s16 i;
    s8 max, v;
    u8 *e;

    if ((s16)quest_item_count(self, id) == 0) {
        for (i = 0; i < 5; i++) {
            if (U16(self, 0x6C + i * 4) == 0 && (s16)n > 0) {
                e = self + i * 4;
                U16(e, 0x6C) = id;
                e[0x6E] = n;
                return 0;
            }
        }
        e = self + *(s8 *)(self + 0x80) * 4;
        U16(e, 0x6C) = id;
        self[*(s8 *)(self + 0x80) * 4 + 0x6E] = n;
        self[0x80]++;
        if (*(s8 *)(self + 0x80) >= 5)
            self[0x80] = 0;
        return 5;
    }
    for (i = 0; i < 5; i++) {
        if (U16(self, 0x6C + i * 4) != (id & 0xFFFF))
            continue;
        max = D_0899D19B[(id & 0xFFFF) * 0x18];
        e = self + i * 4;
        if ((s16)n > 0 && *(s8 *)(e + 0x6E) >= max) {
            e[0x6E] = max;
            return 3;
        }
        v = *(s8 *)(e + 0x6E) + (s8)n;
        e[0x6E] = v;
        if (v <= 0) {
            U16(e, 0x6C) = 0;
            e[0x6E] = 0;
            return 4;
        }
        if (max < v) {
            e[0x6E] = max;
            return 2;
        }
        return 1;
    }
    return 0;
}
