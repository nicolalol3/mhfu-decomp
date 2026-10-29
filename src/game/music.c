/* Music: the player of the music streams (2 players, object D_08A62D94) and the controller of
 * the music of the game (object D_08A62D90). The functions 0x088A1xxx-0x088A2xxx are the
 * stream library (not decompiled yet). (Guessed names.) */
#include "menu.h"
#include "sound.h"

extern u8 *g_sound;
extern u8 *g_game;
extern u8 *D_08A62D90;
extern u8 *D_08A62D94;
extern u32 D_089B23E8;
extern const u16 D_0899D1A2[];    /* item table, 12 halfwords per item */

void atrac_init(int n);
void atrac_set_error_cb(void *cb);
s32 atrac_open(s32 a, s32 b, u8 *buf, u32 size);
s32 atrac_state(u32 h);
void atrac_play(u32 h, u32 addr, u32 size);
void atrac_close(u32 h);
void atrac_set_pause(u32 h, s32 v);
void atrac_set_volume(u32 h, s32 v);
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
    U32(self, MC_AREA_KIND) = 0;
    U32(self, MC_FLAGS) = 0;
    U16(self, MC_MUSIC) = 0xFFFF;
    U16(self, MC_NEXT) = 0xFFFF;
    U16(self, MC_EVENT) = 0xFFFF;
    self[MC_X18] = 0;
    self[MC_X18 + 1] = 0;
    self[MC_X18 + 2] = 0;
    U16(self, MC_X1C) = 0xFFFF;
    U16(self, MC_X1C + 2) = 0xFFFF;
    U16(self, MC_X1C + 4) = 0xFFFF;
    U32(self, MC_X24) = 0;
    U16(self, MC_X2A) = 0;
}

/* short music on the player 1 (jingles) */
void musctl_jingle(u8 *self, u32 id)
{
    music_play_full(g_sound, 1, id);
    U32(self, MC_FLAGS) |= 2;
    S16(self, MC_EVENT) = id;
    U32(self, MC_FLAGS) &= ~0x400;
}

void musctl_end_jingle(u8 *self)
{
    if (U16(self, MC_EVENT) == 0x18E9)
        music_stop_player(g_sound, 1);
    if ((U32(self, MC_FLAGS) & 0x402) != 0) {
        U32(self, MC_FLAGS) &= ~0x402;
        if ((s32)U32(self, MC_AREA_KIND) < 4)
            music_fade_to(g_sound, 0, 0xF, U32(self, MC_VOLUME));
        else
            music_fade_to(g_sound, 0, 0xF, 0x7F);
    }
    U32(self, MC_FLAGS) &= ~0x100;
    U16(self, MC_X2A) = 0;
}

void musctl_update(u8 *self)
{
    if ((s32)U32(self, MC_TIMER) > 0)
        U32(self, MC_TIMER)--;
    if ((U32(self, MC_FLAGS) & 0x102) == 2 && (s32)music_state(g_sound, 1) >= 5) {
        music_stop_player(g_sound, 1);
        U16(self, MC_EVENT) = 0xFFFF;
        if ((s32)U32(self, MC_AREA_KIND) < 4)
            music_fade_to(g_sound, 0, 0xF, U32(self, MC_VOLUME));
        else
            music_fade_to(g_sound, 0, 0xF, 0x7F);
        U32(self, MC_FLAGS) &= ~2;
    }
    if ((GAME_FLAGS & 1) == 0)
        func_09A63A98(self);
}

void musctl_set_music(u8 *self, u16 id)
{
    U16(self, MC_MUSIC) = id;
}

void musctl_set_x18(u8 *self, u8 v)
{
    self[MC_X18] = v;
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
    return U32(self, MC_AREA_KIND) - 8 < 2;
}

/* lowers the music of the player 0 */
void musctl_lower(u8 *self, u32 t)
{
    if ((s32)U32(self, MC_AREA_KIND) >= 0xF) {
        music_fade_to(g_sound, 0, t, 0x46);
    } else if ((s32)U32(self, MC_AREA_KIND) >= 4 || G_AREA == 0x5B ||
               (G_AREA == 0x59 && U16(self, MC_MUSIC) == 0x19D4)) {
        music_fade_to(g_sound, 0, t, 0x32);
    } else {
        return;
    }
    U32(self, MC_FLAGS) |= 0x400;
    U32(self, MC_FLAGS) &= ~2;
}

/* ---- music player (D_08A62D94): 2 players of 0x18 bytes {handle, entry, state, volume,
 *      fade from/to/length/left}, then 21 entries of 0x10 bytes from 0x30
 *      {address, size, loaded size, id} and the buffers of the players from 0x180 ---- */

#define PL(i) (self + (i) * 0x18)
#define EN(i) (self + 0x30 + (i) * 0x10)

u8 *musplay_ctor(u8 *self)
{
    D_08A62D94 = self;
    U32(self, MP_STREAM) = 0;
    U32(self, MP_X18) = 0;
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

    atrac_init(0x11);
    atrac_set_error_cb((void *)0x08886898);
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
        if (U32(p, MP_STREAM) == 0) {
            U32(p, MP_STATE) = -1;
            continue;
        }
        if (S16(p, MP_FADE_LEFT) > 0 && U32(p, MP_STATE) == 3) {
            n = S16(p, MP_FADE_LEFT) - 1;
            S16(p, MP_FADE_LEFT) = n;
            musplay_set_volume(self, i, S16(p, MP_FADE_FROM) + (S16(p, MP_FADE_TO) - S16(p, MP_FADE_FROM)) *
                                        (S16(p, MP_FADE_LEN) - n) / S16(p, MP_FADE_LEN));
        }
        U32(p, MP_STATE) = atrac_state(U32(p, MP_STREAM));
    }
}

void musplay_clear_player(u8 *self, s32 i)
{
    u8 *p = PL(i);

    U32(p, MP_ID) = -1;
    U32(p, MP_STATE) = -1;
    U16(p, MP_VOLUME) = 0;
    U16(p, MP_XE) = 0;
    U16(p, MP_FADE_FROM) = 0;
    U16(p, MP_FADE_TO) = 0;
    U16(p, MP_FADE_LEN) = 0;
    U16(p, MP_FADE_LEFT) = 0;
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

    U32(e, ME_ADDR) = 0;
    U32(e, ME_SIZE) = 0;
    U16(e, ME_ID) = 0xFFFF;
    U32(e, ME_LOADED) = 0;
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

    U32(e, ME_ADDR) = addr;
    U32(e, ME_SIZE) = size;
    U16(e, ME_ID) = 0xFFFF;
    U32(e, ME_LOADED) = 0;
}

void musplay_set_loaded(u8 *self, s32 i, u16 id, u32 size)
{
    u8 *e = self + i * 0x10;

    U16(e, ME_ID) = id;
    U32(e, ME_LOADED) = size;
}

void musplay_unload(u8 *self, s32 i)
{
    u8 *e = self + i * 0x10;

    U16(e, ME_ID) = 0xFFFF;
    U32(e, ME_LOADED) = 0;
}

u32 musplay_entry_addr(u8 *self, s32 i)
{
    return U32(self, i * 0x10 + 0x30);
}

void musplay_open(u8 *self, s32 i)
{
    u8 *p = PL(i);

    if (U32(p, MP_STREAM) == 0)
        U32(p, MP_STREAM) = atrac_open(i, i + 1, self + i * 0x4400 + 0x180, 0x4400);
}

s32 musplay_find(u8 *self, s16 id)
{
    s32 i;

    for (i = 0; i < 0x15; i++, self += 0x10)
        if (S16(self, ME_ID) == id)
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

    if (U32(p, MP_STREAM) == 0)
        return 0;
    if (S16(p, MP_FADE_LEFT) < 1 || U32(p, MP_STATE) != 3)
        return 0;
    return 1;
}

void musplay_start(u8 *self, s32 i, u32 id)
{
    u8 *p = PL(i);
    s32 k = musplay_find(self, id);

    U32(p, MP_ID) = k;
    if (k == -1)
        return;
    atrac_play(U32(p, MP_STREAM), U32(EN(U32(p, MP_ID)), 0), U32(EN(U32(p, MP_ID)), 8));
    U32(p, MP_STATE) = atrac_state(U32(p, MP_STREAM));
}

void musplay_stop(u8 *self, s32 i)
{
    u8 *p = PL(i);

    atrac_close(U32(p, MP_STREAM));
    U32(p, MP_STATE) = atrac_state(U32(p, MP_STREAM));
}

void musplay_set_x(u8 *self, s32 i, s32 v)
{
    atrac_set_pause(U32(PL(i), 0), v);
}

void musplay_set_volume(u8 *self, s32 i, s32 v)
{
    u8 *p = PL(i);

    S16(p, MP_VOLUME) = v;
    atrac_set_volume(U32(p, MP_STREAM), v);
}

void musplay_fade(u8 *self, s32 i, u16 n, u16 to)
{
    u8 *p = PL(i);

    U16(p, MP_FADE_FROM) = U16(p, MP_VOLUME);
    U16(p, MP_FADE_TO) = to;
    U16(p, MP_FADE_LEN) = n;
    U16(p, MP_FADE_LEFT) = n;
}

void musplay_set_global(u8 *self, u32 v)
{
    D_089B23E8 = v;
}
