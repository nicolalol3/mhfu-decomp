/* Music: the player of the music streams (2 players, object D_08A62D94) and the controller of
 * the music of the game (object D_08A62D90). The functions 0x088A1xxx-0x088A2xxx are the
 * stream library (not decompiled yet). (Guessed names.) */
#include "menu.h"

extern u8 *g_sound;
extern u8 *g_game;
extern u8 *D_08A62D90;
extern u8 *D_08A62D94;
extern u32 D_089B23E8;
extern const u16 D_0899D1A2[];    /* item table, 12 halfwords per item */

void func_088A1848(int n);
void func_088A2404(void *cb);
s32 func_088A1914(s32 a, s32 b, u8 *buf, u32 size);
s32 func_088A2338(u32 h);
void func_088A2068(u32 h, u32 addr, u32 size);
void func_088A2204(u32 h);
void func_088A232C(u32 h, s32 v);
void func_088A2378(u32 h, s32 v);
void func_09A63A98(u8 *self);

void music_play_full(u8 *self, s32 i, u32 a);
u32 music_state(u8 *self, s32 i);
void music_stop_player(u8 *self, s32 i);
void music_fade_to(u8 *self, s32 i, u32 t, s32 v);

void musctl_reset(u8 *self);
void musplay_clear_players(u8 *self);
void musplay_clear_entries(u8 *self);
void musplay_open(u8 *self, s32 i);
void musplay_set_volume(u8 *self, s32 i, s32 v);
s32 musplay_find(u8 *self, s16 id);

/* ---- music controller (D_08A62D90): 0x04 area kind, 0x08 music, 0x0A event music,
 *      0x10 volume, 0x14 flags ---- */

u8 *musctl_dtor(u8 *self, s32 del)
{
    if (self != NULL) {
        if (self != NULL)
            D_08A62D90 = NULL;
        if ((s16)del > 0)
            op_delete(self);
    }
    return self;
}

void musctl_reset(u8 *self)
{
    U32(self, 4) = 0;
    U32(self, 0x14) = 0;
    U16(self, 8) = 0xFFFF;
    U16(self, 0xC) = 0xFFFF;
    U16(self, 0xA) = 0xFFFF;
    self[0x18] = 0;
    self[0x19] = 0;
    self[0x1A] = 0;
    U16(self, 0x1C) = 0xFFFF;
    U16(self, 0x1E) = 0xFFFF;
    U16(self, 0x20) = 0xFFFF;
    U32(self, 0x24) = 0;
    U16(self, 0x2A) = 0;
}

/* short music on the player 1 (jingles) */
void musctl_jingle(u8 *self, u32 id)
{
    music_play_full(g_sound, 1, id);
    U32(self, 0x14) |= 2;
    S16(self, 0xA) = id;
    U32(self, 0x14) &= ~0x400;
}

void musctl_end_jingle(u8 *self)
{
    if (U16(self, 0xA) == 0x18E9)
        music_stop_player(g_sound, 1);
    if ((U32(self, 0x14) & 0x402) != 0) {
        U32(self, 0x14) &= ~0x402;
        if ((s32)U32(self, 4) < 4)
            music_fade_to(g_sound, 0, 0xF, U32(self, 0x10));
        else
            music_fade_to(g_sound, 0, 0xF, 0x7F);
    }
    U32(self, 0x14) &= ~0x100;
    U16(self, 0x2A) = 0;
}

void musctl_update(u8 *self)
{
    if ((s32)U32(self, 0) > 0)
        U32(self, 0)--;
    if ((U32(self, 0x14) & 0x102) == 2 && (s32)music_state(g_sound, 1) >= 5) {
        music_stop_player(g_sound, 1);
        U16(self, 0xA) = 0xFFFF;
        if ((s32)U32(self, 4) < 4)
            music_fade_to(g_sound, 0, 0xF, U32(self, 0x10));
        else
            music_fade_to(g_sound, 0, 0xF, 0x7F);
        U32(self, 0x14) &= ~2;
    }
    if ((GAME_FLAGS & 1) == 0)
        func_09A63A98(self);
}

void musctl_set_music(u8 *self, u16 id)
{
    U16(self, 8) = id;
}

void musctl_set_x18(u8 *self, u8 v)
{
    self[0x18] = v;
}

/* jingle of the item used (by its kind) */
void musctl_item_jingle(u8 *self, u32 item)
{
    s16 k = D_0899D1A2[(item & 0xFFFF) * 12];

    if (k == 3)
        musctl_jingle(self, 0x18F8);
    else if (k == 2)
        musctl_jingle(self, 0x18F7);
    else if (k == 0)
        musctl_jingle(self, 0x18F6);
}

u8 musctl_is_battle(u8 *self)
{
    return U32(self, 4) - 8 < 2;
}

/* lowers the music of the player 0 */
void musctl_lower(u8 *self, u32 t)
{
    if ((s32)U32(self, 4) >= 0xF) {
        music_fade_to(g_sound, 0, t, 0x46);
    } else if ((s32)U32(self, 4) >= 4 || U16(g_game, 0x6AF0E) == 0x5B ||
               (U16(g_game, 0x6AF0E) == 0x59 && U16(self, 8) == 0x19D4)) {
        music_fade_to(g_sound, 0, t, 0x32);
    } else {
        return;
    }
    U32(self, 0x14) |= 0x400;
    U32(self, 0x14) &= ~2;
}

/* ---- music player (D_08A62D94): 2 players of 0x18 bytes {handle, entry, state, volume,
 *      fade from/to/length/left}, then 21 entries of 0x10 bytes from 0x30
 *      {address, size, loaded size, id} and the buffers of the players from 0x180 ---- */

#define PL(i) (self + (i) * 0x18)
#define EN(i) (self + 0x30 + (i) * 0x10)

u8 *musplay_ctor(u8 *self)
{
    D_08A62D94 = self;
    U32(self, 0) = 0;
    U32(self, 0x18) = 0;
    musplay_clear_players(self);
    musplay_clear_entries(self);
    return self;
}

u8 *musplay_dtor(u8 *self, s32 del)
{
    if (self != NULL) {
        if (self != NULL)
            D_08A62D94 = NULL;
        if ((s16)del > 0)
            op_delete(self);
    }
    return self;
}

void musplay_init(u8 *self)
{
    s32 i;

    func_088A1848(0x11);
    func_088A2404((void *)0x08886898);
    for (i = 0; i < 2; i++)
        musplay_open(self, i);
}

/* fades and state of the 2 players */
void musplay_update(u8 *self)
{
    u8 *p = self;
    s32 i;
    s16 n;

    for (i = 0; i < 2; i++, p += 0x18) {
        if (U32(p, 0) == 0) {
            U32(p, 8) = -1;
            continue;
        }
        if (S16(p, 0x16) > 0 && U32(p, 8) == 3) {
            n = S16(p, 0x16) - 1;
            S16(p, 0x16) = n;
            musplay_set_volume(self, i, S16(p, 0x10) + (S16(p, 0x12) - S16(p, 0x10)) *
                                        (S16(p, 0x14) - n) / S16(p, 0x14));
        }
        U32(p, 8) = func_088A2338(U32(p, 0));
    }
}

void musplay_clear_player(u8 *self, s32 i)
{
    u8 *p = PL(i);

    U32(p, 4) = -1;
    U32(p, 8) = -1;
    U16(p, 0xC) = 0;
    U16(p, 0xE) = 0;
    U16(p, 0x10) = 0;
    U16(p, 0x12) = 0;
    U16(p, 0x14) = 0;
    U16(p, 0x16) = 0;
}

void musplay_clear_players(u8 *self)
{
    s32 i;

    for (i = 0; i < 2; i++)
        musplay_clear_player(self, i);
}

void musplay_clear_entry(u8 *self, s32 i)
{
    u8 *e = self + i * 0x10;

    U32(e, 0x30) = 0;
    U32(e, 0x34) = 0;
    U16(e, 0x3C) = 0xFFFF;
    U32(e, 0x38) = 0;
}

void musplay_clear_entries(u8 *self)
{
    s32 i;

    for (i = 0; i < 0x15; i++)
        musplay_clear_entry(self, i);
}

void musplay_set_entry(u8 *self, s32 i, u32 addr, u32 size)
{
    u8 *e = self + i * 0x10;

    U32(e, 0x30) = addr;
    U32(e, 0x34) = size;
    U16(e, 0x3C) = 0xFFFF;
    U32(e, 0x38) = 0;
}

void musplay_set_loaded(u8 *self, s32 i, u16 id, u32 size)
{
    u8 *e = self + i * 0x10;

    U16(e, 0x3C) = id;
    U32(e, 0x38) = size;
}

void musplay_unload(u8 *self, s32 i)
{
    u8 *e = self + i * 0x10;

    U16(e, 0x3C) = 0xFFFF;
    U32(e, 0x38) = 0;
}

u32 musplay_entry_addr(u8 *self, s32 i)
{
    return U32(self, i * 0x10 + 0x30);
}

void musplay_open(u8 *self, s32 i)
{
    u8 *p = PL(i);

    if (U32(p, 0) == 0)
        U32(p, 0) = func_088A1914(i, i + 1, self + i * 0x4400 + 0x180, 0x4400);
}

s32 musplay_find(u8 *self, s16 id)
{
    s32 i;

    for (i = 0; i < 0x15; i++, self += 0x10)
        if (S16(self, 0x3C) == id)
            return i;
    return -1;
}

u16 musplay_entry_id(u8 *self, s32 i)
{
    return U16(self, i * 0x10 + 0x3C);
}

u32 musplay_state(u8 *self, s32 i)
{
    return U32(PL(i), 8);
}

u32 musplay_fading(u8 *self, s32 i)
{
    u8 *p = PL(i);

    if (U32(p, 0) == 0)
        return 0;
    if (S16(p, 0x16) < 1 || U32(p, 8) != 3)
        return 0;
    return 1;
}

void musplay_start(u8 *self, s32 i, u32 id)
{
    u8 *p = PL(i);
    s32 k = musplay_find(self, id);

    U32(p, 4) = k;
    if (k == -1)
        return;
    func_088A2068(U32(p, 0), U32(EN(U32(p, 4)), 0), U32(EN(U32(p, 4)), 8));
    U32(p, 8) = func_088A2338(U32(p, 0));
}

void musplay_stop(u8 *self, s32 i)
{
    u8 *p = PL(i);

    func_088A2204(U32(p, 0));
    U32(p, 8) = func_088A2338(U32(p, 0));
}

void musplay_set_x(u8 *self, s32 i, s32 v)
{
    func_088A232C(U32(PL(i), 0), v);
}

void musplay_set_volume(u8 *self, s32 i, s32 v)
{
    u8 *p = PL(i);

    S16(p, 0xC) = v;
    func_088A2378(U32(p, 0), v);
}

void musplay_fade(u8 *self, s32 i, u16 n, u16 to)
{
    u8 *p = PL(i);

    U16(p, 0x10) = U16(p, 0xC);
    U16(p, 0x12) = to;
    U16(p, 0x14) = n;
    U16(p, 0x16) = n;
}

void musplay_set_global(u8 *self, u32 v)
{
    D_089B23E8 = v;
}
