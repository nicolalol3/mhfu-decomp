/* Sound system (3): playing the sounds, volume and pan from the distance to the listener,
 * fades of the channels, music player wrappers. (Guessed names.) */
#include "menu.h"
#include "sound.h"
#include "vfpu.h"

extern u8 *g_game;
extern u8 *D_08A62D84;            /* the sound bank manager (sound.c) */
extern u8 *D_08A62D90;
extern u8 *D_08A62D94;            /* the music manager */
extern const u32 D_089AC454[];    /* sizes of the sound data: 16 pairs per bank */
extern const f32 D_089AA35C[][3]; /* for each kind of sound: full volume distance, step, max */
extern const f32 D_089AB880[][3];
extern const s32 *const D_089AB710[];   /* volume curves (step by step) */
extern const s32 *const D_089ABCE0[];
extern const s32 D_089AB7F8[], D_089AB83C[], D_08A62D40[];   /* pitch curves */
extern const u16 D_09BC8400[];    /* 3 halfwords per monster kind */

#define BANK D_08A62D84
#define VOL_A U32(self, SND_VOL_A)
#define VOL_B U32(self, SND_VOL_B)
#define VOL(i) ((i) == 0 ? VOL_B : VOL_A)

u32 sndbank_get_selected(u8 *self);
u32 sndbank_get_mode(u8 *self);
void sndbank_select(u8 *self, s32 n, s8 reset);
void sndbank_select_raw(u8 *self, s32 n);
void sndbank_clear_slot(u8 *self, s32 i);
void sndbank_set_slot(u8 *self, s32 i, u32 a, u32 b);
void sndbank_clear_a(u8 *self, s32 i);
void sndbank_clear_b(u8 *self, s32 i);
void sndbank_clear_c(u8 *self, s32 i);
u32 sndbank_play(u8 *self, s32 ch, u32 id, u32 a, s32 b, s32 c, u32 d, u32 e, u32 f, u32 g);
u32 sndbank_play_f(u8 *self, s32 ch, u32 id, u32 a, s32 b, s32 c, u32 d, u32 e, u32 f, f32 x);
void sndbank_play_at(u8 *self, u32 x, u32 a, u32 y, u32 z, u32 d, u32 e, u32 f, u32 g);
void sndbank_play_at_f(u8 *self, u32 x, u32 a, u32 y, u32 z, u32 d, u32 e, u32 f, f32 ang);
void sndbank_set_3(u8 *self, u32 a, u32 b, u32 c, s32 alt);
void sndbank_stop_voice(u8 *self, u32 h, s32 now);
void sndbank_stop_all(u8 *self, s32 now);
u8 sndbank_check(u8 *self, u32 a, u32 b, u32 c);
void sndbank_fade_in(u8 *self, u32 h, s32 n);
void sndbank_fade_out(u8 *self, u32 h, s32 n);
void sndbank_start_fade(u8 *self, u16 from, u16 to, u16 n);
s32 sound_scale7(u8 *self, s32 a, s32 v);
s32 sound_percent(u8 *self, s32 v);
void music_fade_to(u8 *self, s32 i, u32 t, s32 v);
void musplay_set_x(u8 *mus, s32 i, s32 v);
void musplay_set_volume(u8 *mus, s32 i, s32 v);
void musplay_fade(u8 *mus, s32 i, u32 a, s32 v);
void musplay_start(u8 *mus, s32 i, u32 a);
void musplay_stop(u8 *mus, s32 i);
s32 musplay_state(u8 *mus, s32 i);
u8 musctl_is_battle(u8 *p);
void musctl_reset(u8 *self);
void func_08883384(u8 *self);
s32 func_088840A8(u8 *self, s32 ch, u32 id, f32 *pos, s32 *kind, f32 *scale);
void func_088851C8(u8 *self, f32 *pos, s32 kind, s32 *vol, f32 *ang);
void func_08884E44(u8 *self, f32 *pos, s32 kind, s32 *vol, s32 *pan);
f32 func_08897D28(f32 x);

s32 sound_area_volume(u8 *self, s32 v, s32 ch, u32 id);
u32 sound_play_raw(u8 *self, s32 ch, s32 id, u32 a, s32 b, s32 c, s32 vol, s32 pan, s32 pitch,
                   s32 x);
void sound_play_at(u8 *self, s32 x, u32 a, u32 y, u32 z, f32 *pos, s32 kind, s32 pitch, u32 p);
s32 sound_monster_channel(u8 *self, s32 id);
s32 sound_curve_volume(u8 *self, f32 *a, f32 *b, s32 kind, s32 mode);
void sound_curve_volume2(u8 *self, f32 *a, f32 *b, s32 kind, s32 *out, s32 mode);
void sound_set_slots(u8 *self, s8 all);
void sound_set_slots_range(u8 *self, s32 from, s32 to);
void music_play(u8 *self, s32 i, u32 a, s32 v, s32 b);

/* ---- music player wrappers ---- */

void music_play(u8 *self, s32 i, u32 a, s32 v, s32 b)
{
    musplay_set_x(D_08A62D94, i, b);
    musplay_set_volume(D_08A62D94, i, v);
    musplay_fade(D_08A62D94, i, 0, v);
    musplay_start(D_08A62D94, i, a);
}

void music_play_vol(u8 *self, s32 i, u32 a, s32 v)
{
    music_play(self, i, a, sound_scale7(self, v, VOL(i)), 0);
}

void music_play_fade(u8 *self, s32 i, u32 a, u32 t, s32 v)
{
    music_play(self, i, a, 0, 0);
    music_fade_to(self, i, t, v);
}

void music_set_both(u8 *self, s32 v)
{
    musplay_set_x(D_08A62D94, 0, v);
    musplay_set_x(D_08A62D94, 1, v);
}

void music_stop_player(u8 *self, s32 i)
{
    musplay_stop(D_08A62D94, i);
}

/* 1 while the music player is in the states 1-4 */
u32 music_busy(u8 *self, s32 i)
{
    s32 s = musplay_state(D_08A62D94, i);

    if (s < 1)
        return 0;
    return s < 5;
}

/* ---- sound banks ---- */

/* gives the memory of the sound data (after 0x549BD0) to the 16 slots of the bank */
void sound_set_slots(u8 *self, s8 all)
{
    u8 *dest = self + 0x549BD0;
    const u32 *t;
    s32 sel, i;
    u32 a;

    sel = sndbank_get_selected(BANK);
    if (sel == -1)
        return;
    t = D_089AC454 + sel * 32;
    for (i = 0; i < 16; i++, t += 2) {
        if (t[0] == 0)
            return;
        a = (u32)dest;
        dest += t[0] + t[1];
        if (all == 1)
            sndbank_set_slot(BANK, i, a, a + t[0]);
        else if (i > 0)
            sndbank_set_slot(BANK, i, a, a + t[0]);
    }
}

void sound_set_slots_range(u8 *self, s32 from, s32 to)
{
    u8 *dest;
    const u32 *t;
    s32 sel, i;
    u32 a;

    sel = sndbank_get_selected(BANK);
    if (sel == -1)
        return;
    dest = self + 0x549BD0;
    t = D_089AC454 + sel * 32;
    for (i = 0; i < from; i++)
        dest += t[i * 2] + t[i * 2 + 1];
    for (i = from; i <= to; i++) {
        sndbank_clear_slot(BANK, i);
        a = (u32)dest;
        dest += t[i * 2] + t[i * 2 + 1];
        sndbank_set_slot(BANK, i, a, a + t[i * 2]);
    }
}

void sound_clear_channel(u8 *self, s32 ch)
{
    sndbank_clear_a(BANK, ch);
    sndbank_clear_c(BANK, ch);
    sndbank_clear_b(BANK, ch);
}

void sound_select_bank(u8 *self, s32 n, s8 all)
{
    sndbank_select(BANK, n, all);
    sound_set_slots(self, all);
}

void sound_select_bank_range(u8 *self, s32 n, s32 from, s32 to)
{
    sndbank_select_raw(BANK, n);
    sound_set_slots_range(self, from, to);
}

void sound_set_data_size(u8 *self, s32 k, s32 i, u32 v)
{
    U32(U32(self, SND_DATA), k * 0x40 + 0x10 + i * 4) = v;
}

void sound_listener_reset(u8 *self)
{
    self[0x5623E0] = 0;
}

/* ---- playing ---- */

u32 sound_play_raw(u8 *self, s32 ch, s32 id, u32 a, s32 b, s32 c, s32 vol, s32 pan, s32 pitch,
                   s32 x)
{
    s32 p;

    if (id == -1)
        return 0;
    p = sound_percent(self, vol);
    return sndbank_play(BANK, ch, id, a, b, c, p, pan - 0x40, pitch - 0x2000, x * p / 100);
}

/* plays the sound id of the channel ch (force: also when the volume is 0) */
u32 sound_play(u8 *self, s32 ch, s32 id, u32 a, s32 b, s32 c, s32 x, s32 pitch, u8 force)
{
    s32 v;

    if (id == -1)
        return 0;
    v = sound_scale7(self, 0x7F, VOL_A);
    v = sound_area_volume(self, v, ch, id);
    if (force == 0 && v == 0)
        return 0;
    return sound_play_raw(self, ch, id, a, b, c, v, 0x40, pitch + 0x2000, x);
}

u32 sound_play_b(u8 *self, s32 ch, s32 id, u32 a, s32 b, s32 c, s32 x, s32 pitch)
{
    s32 v;

    if (id == -1)
        return 0;
    v = sound_scale7(self, 0x7F, VOL_B);
    if (v == 0)
        return 0;
    return sound_play_raw(self, ch, id, a, b, c, v, 0x40, pitch + 0x2000, x);
}

/* sound at the position pos (volume and pan from the distance to the listener) */
u32 sound_play_3d(u8 *self, s32 ch, s32 id, u32 a, s32 b, s32 c, f32 *pos, s32 kind, s32 x,
                  u32 pitch, u8 force)
{
    f32 scale = 1.0f;
    s32 alt = -1;
    s32 extra, v, p, pan;
    f32 ang;

    if (id == -1)
        return 0;
    extra = func_088840A8(self, ch, id, pos, &alt, &scale);
    if (alt != -1)
        kind = alt;
    U32(self, SND_PITCH) = pitch;
    if (sndbank_get_mode(BANK) == 2) {
        func_088851C8(self, pos, kind, &v, &ang);
        v = sound_scale7(self, v, VOL_A);
        v = sound_area_volume(self, v, ch, id);
        if (force == 0 && v <= 0)
            return -1;
        p = sound_percent(self, (s32)((f32)v * scale));
        return sndbank_play_f(BANK, ch, id, a, b, c, p, U32(self, SND_PITCH), p * (x + extra) / 100,
                              ang);
    }
    func_08884E44(self, pos, kind, &v, &pan);
    v = sound_scale7(self, (s32)((f32)v * scale), VOL_A);
    v = sound_area_volume(self, v, ch, id);
    if (force == 0 && v <= 0)
        return -1;
    return sound_play_raw(self, ch, id, a, b, c, v, pan, U32(self, SND_PITCH) + 0x2000, x + extra);
}

u32 sound_play_3d_center(u8 *self, s32 ch, s32 id, u32 a, s32 b, s32 c, f32 *pos, s32 kind,
                         s32 x, u32 pitch, u8 force)
{
    f32 scale = 1.0f;
    s32 alt = -1;
    s32 extra, v, pan;
    f32 ang;

    if (id == -1)
        return 0;
    extra = func_088840A8(self, ch, id, pos, &alt, &scale);
    if (alt != -1)
        kind = alt;
    U32(self, SND_PITCH) = pitch;
    if (sndbank_get_mode(BANK) == 2) {
        func_088851C8(self, pos, kind, &v, &ang);
        v = sound_scale7(self, v, VOL_A);
        if (force == 0 && v <= 0)
            return -1;
        return sound_play_raw(self, ch, id, a, b, c, v, 0x40, U32(self, SND_PITCH) + 0x2000,
                              x + extra);
    }
    func_08884E44(self, pos, kind, &v, &pan);
    v = sound_scale7(self, v, VOL_A);
    if (force == 0 && v <= 0)
        return -1;
    return sound_play_raw(self, ch, id, a, b, c, v, 0x40, U32(self, SND_PITCH) + 0x2000, x + extra);
}

/* volume of a sound at pos */
s32 sound_volume_at(u8 *self, f32 *pos, s32 kind)
{
    s32 v, pan;
    f32 ang;

    if (sndbank_get_mode(BANK) == 2) {
        func_088851C8(self, pos, kind, &v, &ang);
        return sound_scale7(self, v, VOL_A);
    }
    func_08884E44(self, pos, kind, &v, &pan);
    return sound_scale7(self, v, VOL_A);
}

s32 sound_curve_at(f32 d, u8 *self, f32 *a, f32 *b, s32 kind, s32 mode)
{
    s32 v;

    F32(self, SND_DIST) = d;
    sound_curve_volume2(self, a, b, kind, &v, mode + 1);
    return sound_scale7(self, v, VOL_A);
}

void sound_play_at(u8 *self, s32 x, u32 a, u32 y, u32 z, f32 *pos, s32 kind, s32 pitch, u32 p)
{
    s32 v, pan, q;
    f32 ang;

    U32(self, SND_PITCH) = p;
    if (sndbank_get_mode(BANK) == 2) {
        func_088851C8(self, pos, kind, &v, &ang);
        v = sound_scale7(self, v, VOL_A);
        if (v > 0) {
            q = sound_percent(self, v);
            sndbank_play_at_f(BANK, x, a, y, z, q, U32(self, SND_PITCH), pitch * q / 100, ang);
        }
    } else {
        func_08884E44(self, pos, kind, &v, &pan);
        v = sound_scale7(self, v, VOL_A);
        if (v > 0) {
            q = sound_percent(self, v);
            sndbank_play_at(BANK, x, a, y, z, q, pan, U32(self, SND_PITCH), pitch * q / 100);
        }
    }
}

void sound_play_monster_at(u8 *self, s32 id, u32 a, u32 y, u32 z, f32 *pos, s32 kind,
                           s32 pitch, u32 p)
{
    sound_play_at(self, sound_monster_channel(self, id), a, y, z, pos, kind, pitch, p);
}

/* volume at the distance of the listener, from the curve of the kind of sound */
s32 sound_dist_volume(u8 *self, f32 *pos, s32 kind)
{
    f32 step, maxd, d, n, fr;
    const s32 *t;
    const s32 *pt = NULL;
    s32 v, k;

    if (self[0x5623E0] == 0)
        return 0;
    step = D_089AA35C[kind][1];
    maxd = D_089AA35C[kind][2];
    if (kind == 0x11)
        pt = D_089AB7F8;
    else if (kind == 0x20)
        pt = D_089AB83C;
    else if (kind == 0x21)
        pt = D_08A62D40;
    func_08883384(self);
    t = D_089AB710[kind];
    if ((u8)musctl_is_battle(D_08A62D90) == 1)
        t += 0x11;
    d = vfpu_dist3(pos, &F32(self, SND_LISTENER));
    if (kind == 0x1F || F32(self, SND_RANGE) == 30.0f) {
        d = F32(self, SND_DIST);
        U32(self, SND_RANGE) = 0;
    }
    if (d >= maxd) {
        d = maxd;
        if (maxd != 19800.0f) {
            if (kind != 0x21 && kind != 0x20 && kind != 0x11)
                return 0;
            U32(self, SND_PITCH) = pt[0];
            return 0;
        }
    }
    if (d < D_089AA35C[kind][0])
        return 0x7F;
    n = func_08897D28(d / step);
    fr = (d - step * n) / step;
    k = (s32)n;
    v = t[k] - (s32)(fr * (f32)(t[k] - t[k + 1]));
    if ((kind == 0x21 || kind == 0x20 || kind == 0x11) && self[0x562464] != 0)
        U32(self, SND_PITCH) = pt[k] - (s32)(fr * (f32)(pt[k] - pt[k + 1]));
    return v;
}

s32 sound_curve_volume(u8 *self, f32 *a, f32 *b, s32 kind, s32 mode)
{
    f32 step = D_089AB880[kind][1];
    f32 maxd = D_089AB880[kind][2];
    const s32 *t = D_089ABCE0[kind];
    f32 d = 0.0f, n;
    s32 k;

    if (mode == 1) {
        d = vfpu_dist3(b, a);
        if (d >= maxd) {
            d = maxd;
            if (maxd != 19800.0f)
                return 0;
        }
    } else if (mode == 2) {
        d = F32(self, SND_DIST);
        if (d >= maxd) {
            d = maxd;
            if (maxd != 19800.0f)
                return 0;
        }
    }
    n = func_08897D28(d / step);
    k = (s32)n;
    return t[k] - (s32)(((d - step * n) / step) * (f32)(t[k] - t[k + 1]));
}

void sound_curve_volume2(u8 *self, f32 *a, f32 *b, s32 kind, s32 *out, s32 mode)
{
    f32 maxd = D_089AB880[kind][2];
    f32 d;

    *out = sound_curve_volume(self, a, b, kind, mode);
    if (sndbank_get_mode(BANK) == 2 || mode != 1)
        return;
    d = vfpu_dist3(b, a);
    if (d < 0.0f)
        d = 0.0f;
    if (d >= maxd) {
        if (maxd != 19800.0f) {
            *out = 0;
            return;
        }
        d = maxd;
    }
    if (d < D_089AB880[kind][0])
        *out = 0;
}

void sound_set3(u8 *self, u32 a, u32 b, u32 c, s32 alt)
{
    sndbank_set_3(BANK, a, b, c, alt);
}

void sound_stop(u8 *self, u32 h, s32 now)
{
    if (h != 0)
        sndbank_stop_voice(BANK, h, now);
}

void sound_stop_all(u8 *self, s32 now)
{
    sndbank_stop_all(BANK, now);
}

u8 sound_check(u8 *self, u32 a, u32 b, u32 c)
{
    return sndbank_check(BANK, a, b, c);
}

/* channel of the sounds of the monster kind id (same group of D_09BC8400) */
s32 sound_monster_channel(u8 *self, s32 id)
{
    s32 i;

    for (i = 0; i < 4; i++)
        if (D_09BC8400[id * 3] == D_09BC8400[self[SND_LOADED_EXTRA + i] * 3])
            return S16(self, SND_LOADED_CH + i * 2);
    return 0;
}

/* fades of the channels 1-10 (the channel 3 only when the player 1 is a Felyne) */
void sound_fade_in_all(u8 *self, s32 n, s32 keep)
{
    u8 *p = (u8 *)player_get(g_players, 1);
    s32 i;

    for (i = 1; i < 11; i++) {
        if (p != NULL && p[0x1E8] == 0xC)
            sndbank_fade_in(BANK, i, n);
        else if (i != 3)
            sndbank_fade_in(BANK, i, n);
    }
    if (keep == 0)
        sndbank_start_fade(BANK, 0, 0x1000, (s16)n);
}

void sound_fade_in_all2(u8 *self, s32 n, s32 keep)
{
    s32 i;

    for (i = 1; i < 11; i++)
        sndbank_fade_in(BANK, i, n);
    if (keep == 0)
        sndbank_start_fade(BANK, 0, 0x1000, (s16)n);
}

void sound_fade_out_all(u8 *self, s32 n, s32 keep)
{
    u8 *p = (u8 *)player_get(g_players, 1);
    s32 i;

    for (i = 1; i < 11; i++) {
        if (p != NULL && p[0x1E8] == 0xC)
            sndbank_fade_out(BANK, i, n);
        else if (i != 3)
            sndbank_fade_out(BANK, i, n);
    }
    if (keep == 0)
        sndbank_start_fade(BANK, 0x1000, 0, (s16)n);
}

void sound_fade_out_all2(u8 *self, s32 n, s32 keep)
{
    s32 i;

    for (i = 1; i < 11; i++)
        sndbank_fade_out(BANK, i, n);
    if (keep == 0)
        sndbank_start_fade(BANK, 0x1000, 0, (s16)n);
}

/* lower volume of some sounds in the area 13 of the arena (by the state 0x562466) */
s32 sound_area_volume(u8 *self, s32 v, s32 ch, u32 id)
{
    u8 st;
    int hit = 0;

    if ((GAME_FLAGS & 1) == 0 || S16(g_game, 0x6AF0E) != 0xD)
        return v;
    st = self[0x562466];
    if (st == 0) {
        if (ch == 7 && id == 0x85)
            v = (s32)((f32)v * 0.8f);
        return v;
    }
    if (ch == 3) {
        switch (id) {
        case 0x37: case 0x34: case 0x33: case 0x2B: case 0x4F: case 0x29: case 0x4E: case 0x28:
        case 0x4D: case 0x26: case 0x4C: case 0x25: case 0x4B: case 0x22: case 0x4A: case 0x21:
        case 0x20: case 0x1F: case 0x49: case 0x1E: case 0x48: case 0x1D: case 0x47: case 0xB:
        case 0x46: case 0xA: case 5: case 4: case 3: case 2: case 1: case 0:
            hit = 1;
        }
    } else if (ch == 7) {
        if ((id >= 0x28 && id <= 0x56) || (id > 0x81 && id < 0x85))
            hit = 1;
    }
    if (hit) {
        if (st == 3)
            v = (s32)((f32)v * 0.3f);
        else if ((st & 1) != 0)
            v = (s32)((f32)v * 0.5f);
    }
    return v;
}

u8 *sound_mgr3_ctor(u8 *self)
{
    D_08A62D90 = self;
    musctl_reset(self);
    return self;
}
