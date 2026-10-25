/* Village: start of the village scene, second item pouch (g+0x43C8), gathering lists.
 * (Guessed names.) */
#include "menu.h"
#undef pouch_count

extern u8 *D_08A62C60;
extern u8 *D_08A8B1E8;
extern void *g_players;
extern void *D_089CC01C;          /* screen fade */
extern u8 D_089BAB98[], D_089BAA94[];
extern const f32 D_0899D174[3], D_0899D18C[3];
extern const u8 D_089A47D0[];     /* 0xAC entries of 10 bytes */
int sceRtcGetCurrentClockLocalTime(void *t);
void func_08890080(void *a, u32 seed);
void game_reset_options(u8 *g, u32 apply);
void options_backup(u8 *g);
void osk_init(void *a);
void *heap_alloc(u32 *h, u32 size, u32 align);
u32 *func_08897290(u32 size, void *mem);
s32 vec3_nonzero(f32 *v);
void item_set_seen(void *a, u32 item);
s32 hunter_has_skill(void *hunter, int id);
s32 pouch_count(u8 *g, u32 id_arg);
s32 box_find(void *p, u32 id);

/* start of the village: options, random seed, screen fade object */
void village_start(u8 *o)
{
    u8 t[16];
    u8 *w;
    void *mem;
    f32 v[3];

    sceRtcGetCurrentClockLocalTime(t);
    func_08890080(g_sys, t[0xA]);
    game_reset_options(g_game, 1);
    options_backup(g_game);
    osk_init(D_08A62C60);
    w = D_08A8B1E8;
    mem = heap_alloc((u32 *)(w + 0x14010), 0x34, 0x10);
    if (mem == NULL) {
        U32(o, 0x1C) = 0;
    } else {
        u32 *f = func_08897290(0x34, mem);

        if (f == NULL) {
            U32(w, 8) = 0;
        } else {
            f[0] = (u32)D_089BAB98;
            v[0] = D_0899D174[0];
            v[1] = D_0899D174[1];
            v[2] = D_0899D174[2];
            if (vec3_nonzero(v) != 0) {
                ((f32 *)f)[1] = v[0];
                ((f32 *)f)[2] = v[1];
                ((f32 *)f)[3] = v[2];
            }
            D_089CC01C = f;
            f[0] = (u32)D_089BAA94;
            U32(w, 8) = (u32)f;
        }
        if (f != NULL) {
            u8 *q;

            ((u8 *)f)[0x10] = 2;
            U32((u8 *)U32(w, 8), 0x14) = 0xC;
            q = (u8 *)U32(w, 8);
            q[0x11] = 2;
            q[0x12] = 0;
        }
        U32(o, 0x1C) = 0;
    }
    v[0] = D_0899D18C[0];
    v[1] = D_0899D18C[1];
    v[2] = D_0899D18C[2];
    if (vec3_nonzero(v) != 0) {
        F32(o, 4) = v[0];
        F32(o, 8) = v[1];
        F32(o, 0xC) = v[2];
    }
}

void box_item_notify(void *a, u32 item)
{
    item_set_seen(a, item);
}

/* second pouch (24 slots at g+0x43C8) */
s32 pouch2_count(void *unused, u32 id_arg)
{
    u16 id = id_arg;
    u8 *p = g_game + 0x4A0;
    s16 i;

    for (i = 0; i < 0x18; i++) {
        if (U16(p, 0x3F28 + i * 4) == id)
            return S16(p, 0x3F2A + i * 4);
    }
    return 0;
}

s32 pouch2_room_or_full(void *unused, u32 id_arg)
{
    u16 id = id_arg;
    u8 *p = g_game + 0x4A0;
    s16 i, empty = 0;

    for (i = 0; i < 0x18; i++) {
        u16 v = U16(p, 0x3F28 + i * 4);

        if (v == id) {
            if (D_0899D19B[id * 0x18] == 0xFF)
                return 0xFF;
            return (s16)(D_0899D19B[id * 0x18] - S16(p, 0x3F2A + i * 4));
        }
        if (v == 0)
            empty++;
    }
    if (empty == 0)
        return -1;
    return D_0899D19B[id * 0x18];
}

int gather_has_tool(void *p, void *hunter, u8 *e, u32 flag);

/* number of the gathering points unlocked (kind 1: without tool, 3: with tool, else all) */
u32 gather_count(void *p, u32 kind)
{
    u8 *g = g_game + 0x4A0;
    void *h = (void *)player_get(g_players, G_MY_SLOT);
    const u8 *e = D_089A47D0;
    u16 n = 0;
    s32 i;

    if (kind == 0) {
        u8 r = gather_has_tool(p, h, NULL, 0);

        kind = (r == 1) ? 2 : r;
    }
    for (i = 0xAC; i != 0; i--, e += 10) {
        if ((g[0x3FCC + (e[6] >> 3)] & (1 << (e[6] & 7))) == 0)
            continue;
        if ((u8)kind == 1) {
            if ((e[7] & 1) == 0)
                n++;
        } else if ((u8)kind == 3) {
            if ((e[7] & 1) == 1)
                n++;
        } else {
            n++;
        }
    }
    return n;
}

/* 1 if the hunter can gather at e (no tool needed, skill 0xA0, or a tool 6 in the pouch) */
int gather_has_tool(void *p, void *hunter, u8 *e, u32 flag)
{
    if (e != NULL && (e[7] & 1) == 0)
        return 1;
    if ((u8)hunter_has_skill(hunter, 0xA0) == 1)
        return 1;
    if (pouch_count(g_game, 6) != 0)
        return 1;
    if ((u8)flag != 1)
        return 0;
    return (u16)box_find(p, 6) != 0xFFFF;
}

/* ids of the items that can be combined in the field */
u32 item_combinable(void *unused, u32 id_arg)
{
    static const u16 ids[] = {
        0x462, 0x469, 0x470, 0x473, 0x472, 0x471, 0x46F, 0x46E, 0x46D, 0x46C, 0x46B, 0x46A, 0x468, 0x467,
        0x466, 0x465, 0x464, 0x463, 0x461, 0x460, 0x45F, 0x45E, 0x45D, 0x45C, 0x36E, 0x371, 0x36F, 0x370,
        0x36A, 0x36D, 0x36C,
    };
    u32 id = id_arg & 0xFFFF;
    unsigned i;

    for (i = 0; i < sizeof(ids) / sizeof(ids[0]); i++) {
        if (id == ids[i])
            return id;
    }
    return 0xFFFFFFFF;
}
