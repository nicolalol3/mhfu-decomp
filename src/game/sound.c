/* Sound system: the object g_sound (sound_*) and the manager of the sound banks
 * (sndbank_*, single object D_08A62D84). The functions 0x0889E8xx-0x0889EBxx are the
 * sound library (not decompiled yet). (Guessed names.) */
#include "menu.h"
#include "sound.h"

extern u8 *g_sound;
extern u8 *g_sys;
extern u8 *g_game;
extern void **g_loader;
extern u8 *D_08A62D84;            /* the sound bank manager */
extern u8 *D_08A62D94;            /* the music manager */
extern u32 D_08A62D88;
extern u8 D_09A0DB44;
extern u32 D_089B24D8, D_09A0DA8C;
extern const u32 D_089BAB84[], D_089BAB98[];
extern const u8 D_089B918C[];
extern const char D_089AC554[];   /* "sndPrxLoader" */
extern const u16 D_089AC564[];    /* music of each id */
extern const u8 D_089AA224[], D_089AA250[];

/* parameters of a sound to play (filled in part; flags 4 and 8 say which optional fields) */
typedef struct SndParam {
    u32 w[15];
} SndParam;

void cse_start(u8 *mem, int a, int b, int c);
void cse_core_start(void);
void stub_0889ea4c(u32 size);   /* -> bank_init */
void stub_0889ead4(void *cb);   /* -> setbl_set_cb */
void stub_0889e9d4(void);   /* -> cse_frame */
void cse_set_output(s32 mode);
void stub_0889ea54(s32 n);   /* -> bank_select */
s32 stub_0889ea5c(s32 i);   /* -> sound_map_offset */
void stub_0889ea64(s32 i, u32 a);   /* -> bank_set */
void stub_0889ea6c(s32 i, u32 b);   /* -> setbl_set */
u32 stub_0889ea74(u32 h);   /* -> bank_data_size */
s32 stub_0889ea7c(u32 a, u32 b, u32 c);   /* -> bank_merge */
void stub_0889ea84(u32 h, int a, s32 b);   /* -> se_chan_volume */
void stub_0889ea8c(void *p);   /* -> se_send_7 */
void stub_0889ea94(void);   /* -> se_stop_all */
void stub_0889ea9c(u32 h);   /* -> se_stop_chan */
void stub_0889eaa4(u32 h);   /* -> se_stop_id */
void stub_0889eaac(void);   /* -> se_release_all */
void stub_0889eab4(u32 h);   /* -> se_release_chan */
void stub_0889eabc(u32 h);   /* -> se_release_id */
void stub_0889eac4(u32 a, u32 b, u32 c);   /* -> se_release_match */
void stub_0889eacc(u32 a, u32 b, u32 c, SndParam *p);   /* -> se_change_match */
u32 cse_cmd16(u32 a, u32 b, SndParam *p);
u32 cse_voice_cmd(u32 a, u32 b, u32 c);
void stub_0889ee80(u32 a, u32 b, u32 c);   /* -> se_stop_match */
u32 func_088B826C(f32 f, u32 a, u32 b, SndParam *p);
void func_088B8320(f32 ang, SndParam *p);
void sas_render_pause(void);
void sas_render_resume(void);
u32 game_rand(void *a, int b);
void musplay_ctor(u8 *self);
void sound_mgr3_ctor(u8 *self);
void musctl_dtor(u8 *self, s32 a);
void musplay_dtor(u8 *self, s32 a);
void musplay_init(u8 *self);
void musplay_update(u8 *self);
void musplay_set_global(u8 *self, s32 a);
void sound_req_done(u8 *self);
void sound_load_step(u8 *self);
void sound_listener_reset(u8 *self);
void sound_select_bank(u8 *self, int a, int b);
void music_select_set(u8 *self, int a, int b);
int sceKernelCreateThread(const char *name, void *entry, int prio, int stack, u32 attr, void *opt);
int sceKernelStartThread(int thid, u32 arglen, void *argp);
int sceKernelDelayThreadCB(u32 delay);
int sceKernelExitDeleteThread(int status);
int sceUtilityLoadModule(int id);

typedef s32 (*LoaderBusy)(void *self);

void sndbank_clear_all(u8 *self);
void sndbank_select(u8 *self, s32 n, s8 reset);
void sndbank_set_mode(u8 *self, s32 mode);
void sndbank_fx_init(u8 *self);
void sndbank_set_params(u8 *self, u32 a, u32 b, u32 c, u32 d, u32 e, u32 f);
void sndbank_set_volume(u8 *self, u32 v);
void sndbank_set_global_a(u8 *self, u32 v);
void sndbank_set_global_b(u8 *self, u32 v);
void sndbank_rand_cb(u8 *voice, u8 *def);
void sndbank_clear_slot(u8 *self, s32 i);
void sndbank_clear_a(u8 *self, s32 i);
void sndbank_clear_b(u8 *self, s32 i);
void sndbank_clear_c(u8 *self, s32 i);
void sndbank_clear_abc(u8 *self, s32 i);
u16 sndbank_get_a(u8 *self, s32 i);
u16 sndbank_get_b(u8 *self, s32 i);
u16 sndbank_get_c(u8 *self, s32 i);
u8 sndbank_is_playing(u8 *self, s32 i);
u32 sndbank_slot_ready(u8 *self, s32 i);
void sndbank_stop_ch(u8 *self, u32 h, s32 now);
u8 *sndbank_ctor(u8 *self);
u8 *sndbank_dtor(u8 *self, s32 del);
void sndbank_init(u8 *self, u8 *mem, u32 size);
void sndbank_update(u8 *self);
s32 sound_prx_thread(u32 args, u8 **argp);
void sound_bgm_play(u8 *self, s32 i, u32 a, u32 b, u32 c, u32 d, u32 e);

#define SLOT(self, i) ((self) + (i) * 0x14)

/* ---- a small object with a position (vtables D_089BAB98 / D_089BAB84) ---- */

s32 vec3_nonzero(f32 *v);
void stub_nop(void *p);
extern const f32 D_089A8A10[3], D_089A8A1C[3], D_089A8A28[3];

u32 *posobj_ctor(u32 *self)
{
    f32 v[3];

    self[0] = (u32)D_089BAB98;
    v[0] = D_089A8A10[0];
    v[1] = D_089A8A10[1];
    v[2] = D_089A8A10[2];
    if (vec3_nonzero(v) != 0) {
        ((f32 *)self)[1] = v[0];
        ((f32 *)self)[2] = v[1];
        ((f32 *)self)[3] = v[2];
    }
    self[0] = (u32)D_089BAB84;
    self[7] = 0;
    return self;
}

u32 *posobj_dtor(u32 *self, s32 del)
{
    if (self != NULL) {
        self[0] = (u32)D_089BAB84;
        if (self != NULL)
            self[0] = (u32)D_089BAB98;
        if ((s16)del > 0)
            stub_nop(self);
    }
    return self;
}

s32 sys_loader_busy(u8 *sys);
void sys_overlay_ready(u8 *sys, int a, int b);
u8 sys_overlay_is(u8 *sys, int a, int b);
void sys_load_overlay(u8 *sys, int a, int b);
void fade_start(u8 *f, u32 n, u32 c, s32 out);
extern u8 *D_089CC01C;
extern u8 *D_09A44C60;
extern u8 *D_09C1B720;

static void posobj_set(u32 *self, const f32 *src)
{
    f32 v[3];

    v[0] = src[0];
    v[1] = src[1];
    v[2] = src[2];
    if (vec3_nonzero(v) != 0) {
        ((f32 *)self)[1] = v[0];
        ((f32 *)self)[2] = v[1];
        ((f32 *)self)[3] = v[2];
    }
}

/* loads the system files 0x3F and 0x4B, then puts the object at its start position */
void posobj_update(u32 *self)
{
    s32 s = (s32)self[7];

    if (s == 3) {
        if (sys_loader_busy(g_sys) != 0)
            return;
        sys_overlay_ready(g_sys, 0x4B, 2);
    } else if (s == 0) {
        fade_start(D_089CC01C, 1, 0, 1);
        if (sys_overlay_is(g_sys, 0x3F, 1) == 0) {
            sys_load_overlay(g_sys, 0x3F, 1);
            self[7]++;
            return;
        }
        self[7] = 2;
        return;
    } else if (s == 1 || s == 2) {
        if (s == 1) {
            if (sys_loader_busy(g_sys) != 0)
                return;
            sys_overlay_ready(g_sys, 0x3F, 1);
            self[7]++;
        }
        if (sys_overlay_is(g_sys, 0x4B, 2) == 0) {
            sys_load_overlay(g_sys, 0x4B, 2);
            self[7]++;
        } else {
            self[7] = -1;
        }
        return;
    }
    if (g_game[0x494] != 0) {
        if (D_09A44C60[0x9A2] == 0)
            U32(g_game, 0x498) = 0;
        else
            U32(g_game, 0x498) = 1;
    }
    g_game[0x494] = 0;
    self[7] = 0;
    if (*D_09C1B720 == 0)
        posobj_set(self, D_089A8A28);
    else
        posobj_set(self, D_089A8A1C);
}

/* ---- sound bank manager ---- */

u8 *sndbank_ctor(u8 *self)
{
    D_08A62D84 = self;
    sndbank_clear_all(self);
    sndbank_fx_init(self);
    U32(self, SB_MEM) = 0;
    U32(self, SB_MEM_SIZE) = 0;
    U32(self, SB_SELECTED) = -1;
    U32(self, SB_MODE) = -1;
    self[SB_PAUSED] = 0;
    self[SB_PARAMS_SET] = 0;
    U16(self, SB_FADE_FROM) = 0;
    U16(self, SB_FADE_TO) = 0;
    U16(self, SB_FADE_LEN) = 0;
    U16(self, SB_FADE_LEFT) = 0;
    return self;
}

u8 *sndbank_dtor(u8 *self, s32 del)
{
    if (self != NULL) {
        if (self != NULL)
            D_08A62D84 = NULL;
        if ((s16)del > 0)
            op_delete(self);
    }
    return self;
}

void sndbank_init(u8 *self, u8 *mem, u32 size)
{
    cse_start(mem, 0x12, 0x11, 0);
    cse_core_start();
    stub_0889ea4c(size);
    U32(self, SB_MEM) = (u32)mem;
    U32(self, SB_MEM_SIZE) = size;
    sndbank_select(self, 0, 1);
    sndbank_set_mode(self, 1);
    stub_0889ead4(sndbank_rand_cb);
}

/* volume fade (0x174 from, 0x176 to, 0x178 length, 0x17A frames left) */
void sndbank_update(u8 *self)
{
    if (S16(self, SB_FADE_LEFT) > 0) {
        S16(self, SB_FADE_LEFT)--;
        sndbank_set_volume(self, S16(self, SB_FADE_FROM) + (S16(self, SB_FADE_TO) - S16(self, SB_FADE_FROM)) *
                                 (S16(self, SB_FADE_LEN) - S16(self, SB_FADE_LEFT)) / S16(self, SB_FADE_LEN));
    }
    stub_0889e9d4();
}

/* callback of the library for each new voice: random pitch / pan / volume */
void sndbank_rand_cb(u8 *voice, u8 *def)
{
    u16 r = U16(def, 0x1A);
    s16 d;
    s32 v;

    if (r != 0) {
        d = (s16)(((u16)game_rand(g_sys, 1) % r) << 3);
        if (((u16)game_rand(g_sys, 1) & 1) != 0)
            d = -d;
        S16(voice, 0xC) += d;
    }
    if (U16(def, 0x1C) != 0) {
        d = (s16)((u16)game_rand(g_sys, 1) % U16(def, 0x1C));
        if (((u16)game_rand(g_sys, 1) & 1) != 0)
            d = -d;
        S16(voice, 8) += d;
    }
    v = voice[0x11] + S16(def, 0x1E);
    if (v < 0)
        voice[0x11] = 0;
    else if (v < 0x100)
        voice[0x11] = v;
    else
        voice[0x11] = 0xFF;
    if ((OPT_FLAGS & 1) != 0 && (voice[6] & 0x20) != 0 && (voice[6] & 0xF) != G_MY_SLOT)
        voice[3] -= 3;
}

void sndbank_pause(u8 *self)
{
    if (self[SB_PAUSED] == 0) {
        sas_render_pause();
        self[SB_PAUSED] = 1;
    }
}

void sndbank_resume(u8 *self)
{
    if (self[SB_PAUSED] == 1) {
        sas_render_resume();
        self[SB_PAUSED] = 0;
    }
}

void sndbank_set_mode(u8 *self, s32 mode)
{
    s32 m = mode;

    D_09A0DB44 = 0;
    if (mode >= 3) {
        D_09A0DB44 = 3;
        m = 1;
        sndbank_set_global_a(D_08A62D84, 1);
        musplay_set_global(D_08A62D94, 1);
        sndbank_set_global_b(D_08A62D84, 1);
    }
    cse_set_output(m);
    U32(self, SB_MODE) = mode;
}

u32 sndbank_get_mode(u8 *self)
{
    return U32(self, SB_MODE);
}

void sndbank_clear_slot(u8 *self, s32 i)
{
    u8 *s = SLOT(self, i);

    U32(s, SB_CH_DATA) = 0;
    U32(s, SB_CH_SIZE) = 0;
    U32(s, SB_CH_ADDR) = 0;
    U16(s, SB_CH_A) = 0xFFFF;
    U16(s, SB_CH_B) = 0xFFFF;
    U16(s, SB_CH_C) = 0xFFFF;
    U16(s, SB_CH_FLAGS) = 0;
}

void sndbank_clear_all(u8 *self)
{
    s32 i;

    for (i = 0; i < 16; i++)
        sndbank_clear_slot(self, i);
}

void sndbank_set_slot(u8 *self, s32 i, u32 a, u32 b)
{
    u8 *s = SLOT(self, i);

    U32(s, SB_CH_DATA) = a;
    U32(s, SB_CH_SIZE) = b;
    U32(s + 0x18, 8) = U32(self, SB_MEM) + stub_0889ea5c(i);
    stub_0889ea64(i, U32(s + 0x18, 0));
    stub_0889ea6c(i, U32(s + 0x18, 4));
}

void sndbank_set_a(u8 *self, s32 i, u16 v) { U16(SLOT(self, i), 0x24) = v; }
void sndbank_set_b(u8 *self, s32 i, u16 v) { U16(SLOT(self, i), 0x26) = v; }
void sndbank_set_c(u8 *self, s32 i, u16 v) { U16(SLOT(self, i), 0x28) = v; }

void sndbank_clear_a(u8 *self, s32 i)
{
    u8 *s = SLOT(self, i);

    U16(s, SB_CH_A) = 0xFFFF;
    U16(s, SB_CH_FLAGS) &= 0xFFFE;
}

void sndbank_clear_b(u8 *self, s32 i) { U16(SLOT(self, i), 0x26) = 0xFFFF; }
void sndbank_clear_c(u8 *self, s32 i) { U16(SLOT(self, i), 0x28) = 0xFFFF; }

void sndbank_clear_abc(u8 *self, s32 i)
{
    sndbank_clear_a(self, i);
    sndbank_clear_b(self, i);
    sndbank_clear_c(self, i);
}

u32 sndbank_get_w0(u8 *self, s32 i) { return U32(SLOT(self, i), 0x18); }
u32 sndbank_get_w1(u8 *self, s32 i) { return U32(SLOT(self, i), 0x1C); }
u32 sndbank_get_w2(u8 *self, s32 i) { return U32(SLOT(self, i), 0x20); }
u16 sndbank_get_a(u8 *self, s32 i) { return U16(SLOT(self, i), 0x24); }
u16 sndbank_get_b(u8 *self, s32 i) { return U16(SLOT(self, i), 0x26); }
u16 sndbank_get_c(u8 *self, s32 i) { return U16(SLOT(self, i), 0x28); }

u8 sndbank_is_playing(u8 *self, s32 i)
{
    return (U16(SLOT(self, i), 0x2A) & 1) != 0;
}

/* reset != 1: clears the slots 1-15 only when the bank changes */
void sndbank_select(u8 *self, s32 n, s8 reset)
{
    s32 i = 1;

    stub_0889ea54(n);
    if (reset == 1) {
        sndbank_clear_all(self);
        U32(self, SB_SELECTED) = n;
        return;
    }
    if (U32(self, SB_SELECTED) != (u32)n) {
        do {
            sndbank_clear_slot(self, i);
            i++;
        } while (i < 16);
    }
    U32(self, SB_SELECTED) = n;
}

void sndbank_select_raw(u8 *self, s32 n)
{
    stub_0889ea54(n);
    U32(self, SB_SELECTED) = n;
}

u32 sndbank_get_selected(u8 *self)
{
    return U32(self, SB_SELECTED);
}

u32 sndbank_slot_ready(u8 *self, s32 i)
{
    u8 *s = SLOT(self, i);

    if (S16(s, SB_CH_A) == -1)
        return 0;
    if (S16(s, SB_CH_B) == -1 || S16(s, SB_CH_C) == -1)
        return 0;
    return 1;
}

s32 sndbank_slot_play(u8 *self, s32 i, u32 a, u32 b)
{
    u8 *s = SLOT(self, i);
    s32 r = -1;

    if (U32(s, SB_CH_DATA) != 0 && S16(s, SB_CH_A) == -1) {
        r = stub_0889ea7c(U32(s, SB_CH_DATA), a, b);
        if (r > 0)
            U16(s, SB_CH_FLAGS) |= 1;
    }
    return r;
}

u32 sndbank_data_size(u8 *self, u32 h)
{
    return stub_0889ea74(h);
}

u32 sndbank_play(u8 *self, s32 ch, u32 id, u32 a, s32 b, s32 c, u32 d, u32 e, u32 f, u32 g)
{
    SndParam p;

    if (sndbank_slot_ready(self, ch) == 0)
        return 0;
    p.w[14] = g;
    p.w[0] = 0x2B01;
    p.w[1] = a;
    p.w[12] = d;
    p.w[9] = e;
    p.w[10] = f;
    if (b != -1) {
        p.w[3] = b;
        p.w[0] |= 4;
    }
    if (c != -1) {
        p.w[4] = c;
        p.w[0] |= 8;
    }
    return cse_cmd16(ch, id, &p);
}

u32 sndbank_play_f(u8 *self, s32 ch, u32 id, u32 a, s32 b, s32 c, u32 d, u32 e, u32 f, f32 x)
{
    SndParam p;

    if (sndbank_slot_ready(self, ch) == 0)
        return 0;
    p.w[0] = 0x2A01;
    p.w[1] = a;
    p.w[14] = f;
    p.w[12] = d;
    p.w[10] = e;
    if (b != -1) {
        p.w[3] = b;
        p.w[0] |= 4;
    }
    if (c != -1) {
        p.w[4] = c;
        p.w[0] |= 8;
    }
    return func_088B826C(x, ch, id, &p);
}

void sndbank_play_at(u8 *self, u32 x, u32 a, u32 y, u32 z, u32 d, u32 e, u32 f, u32 g)
{
    SndParam p;

    p.w[1] = a;
    p.w[0] = 0x2B01;
    p.w[12] = d;
    p.w[9] = e;
    p.w[10] = f;
    p.w[14] = g;
    stub_0889eacc(x, y, z, &p);
}

void sndbank_play_at_f(u8 *self, u32 x, u32 a, u32 y, u32 z, u32 d, u32 e, u32 f, f32 ang)
{
    SndParam p;

    p.w[0] = 0x2A01;
    p.w[1] = a;
    p.w[12] = d;
    p.w[10] = e;
    p.w[14] = f;
    func_088B8320(ang, &p);
    stub_0889eacc(x, y, z, &p);
}

void sndbank_set_3(u8 *self, u32 a, u32 b, u32 c, s32 alt)
{
    if (alt == 0)
        stub_0889eac4(a, b, c);
    else
        stub_0889ee80(a, b, c);
}

void sndbank_stop_voice(u8 *self, u32 h, s32 now)
{
    if (now == 0)
        stub_0889eabc(h);
    else
        stub_0889eaa4(h);
}

void sndbank_stop_ch(u8 *self, u32 h, s32 now)
{
    if (now == 0)
        stub_0889eab4(h);
    else
        stub_0889ea9c(h);
}

void sndbank_stop_all(u8 *self, s32 now)
{
    if (now == 0)
        stub_0889eaac();
    else
        stub_0889ea94();
}

u8 sndbank_check(u8 *self, u32 a, u32 b, u32 c)
{
    return (u8)cse_voice_cmd(a, b, c) > 0;
}

void sndbank_set_params(u8 *self, u32 a, u32 b, u32 c, u32 d, u32 e, u32 f)
{
    self[SB_PARAMS_SET] = 1;
    U32(self, SB_PARAMS) = a;
    U32(self, SB_PARAM_B) = b;
    U32(self, SB_PARAM_C) = c;
    U32(self, SB_VOLUME) = d;
    U32(self, SB_PARAM_E) = e;
    U32(self, SB_PARAM_F) = f;
    stub_0889ea8c(self + 0x15C);
}

void sndbank_fx_init(u8 *self)
{
    D_08A62D88 = 0;
}

/* fade in / out of a voice in n frames */
void sndbank_fade_in(u8 *self, u32 h, s32 n)
{
    stub_0889ea84(h, 0x7F, n == 0 ? 0x10000000 : 0x10000000 / n);
}

void sndbank_fade_out(u8 *self, u32 h, s32 n)
{
    stub_0889ea84(h, 0, n == 0 ? 0x10000000 : 0x10000000 / n);
}

void sndbank_start_fade(u8 *self, u16 from, u16 to, u16 n)
{
    U16(self, SB_FADE_FROM) = from;
    U16(self, SB_FADE_TO) = to;
    U16(self, SB_FADE_LEN) = n;
    U16(self, SB_FADE_LEFT) = n;
}

void sndbank_set_volume(u8 *self, u32 v)
{
    U32(self, SB_VOLUME) = v;
    stub_0889ea8c(self + 0x15C);
}

void sndbank_set_global_a(u8 *self, u32 v)
{
    D_089B24D8 = v;
}

void sndbank_set_global_b(u8 *self, u32 v)
{
    D_09A0DA8C = v;
}

/* ---- g_sound ---- */

u8 *sound_ctor(u8 *self)
{
    g_sound = self;
    musplay_ctor(self);
    sndbank_ctor(self + 0x8980);
    sound_mgr3_ctor(self + 0x8B00);
    sound_req_done(self);
    sound_listener_reset(self);
    U32(self, SND_VOL_A) = 7;
    U32(self, SND_VOL_B) = 7;
    U32(self, SND_MUSIC_SEL) = -1;
    self[0x562420] = 0;
    self[0x562421] = 0;
    return self;
}

u8 *sound_dtor(u8 *self, s32 del)
{
    if (self != NULL) {
        musctl_dtor(self + 0x8B00, -1);
        sndbank_dtor(self + 0x8980, -1);
        musplay_dtor(self, -1);
        if (self != NULL)
            g_sound = NULL;
        if ((s16)del > 0)
            op_delete(self);
    }
    return self;
}

/* loads the sound modules (sceSas, ...) in a thread */
void sound_load_prx(u8 *self)
{
    u8 *arg;

    if (self[0x562421] == 1)
        return;
    U32(self, SND_PRX_THREAD) = sceKernelCreateThread(D_089AC554, sound_prx_thread, 0x10, 0x1000, 0, NULL);
    arg = self;
    sceKernelStartThread(U32(self, SND_PRX_THREAD), 4, &arg);
}

s32 sound_prx_thread(u32 args, u8 **argp)
{
    u8 *self = *argp;

    U32(self, SND_PRX_MOD) = sceUtilityLoadModule(0x300);
    U32(self, SND_PRX_MOD + 4) = sceUtilityLoadModule(0x301);
    U32(self, SND_PRX_MOD + 8) = sceUtilityLoadModule(0x302);
    U32(self, SND_DATA) = (u32)D_089B918C;
    while (((LoaderBusy)((void **)g_loader[0])[0x38 / 4])(g_loader) != 0)
        sceKernelDelayThreadCB(10000);
    self[0x562421] = 1;
    sceKernelExitDeleteThread(0);
    return 0;
}

void sound_start(u8 *self)
{
    if (self[0x562421] == 0 || self[0x562420] == 1)
        return;
    sndbank_init(D_08A62D84, self + 0x308BD0, U32(self, SND_DATA));
    musplay_init(D_08A62D94);
    sound_select_bank(self, 0, 1);
    music_select_set(self, 0, 0);
    self[0x562420] = 1;
}

void sound_update(u8 *self)
{
    if (self[0x562420] == 0)
        return;
    sound_load_step(self);
    musplay_update(D_08A62D94);
    sndbank_update(D_08A62D84);
    sound_listener_reset(self);
}

void sound_set_mode(u8 *self, s32 mode)
{
    sndbank_set_mode(D_08A62D84, mode);
}

void sound_bgm_play(u8 *self, s32 i, u32 a, u32 b, u32 c, u32 d, u32 e)
{
    sndbank_set_params(D_08A62D84, D_089AC564[i], a, b, c, d, e);
}

/* music of the area id */
void sound_bgm_area(u8 *self, u32 id)
{
    sound_bgm_play(self, D_089AA224[D_089AA250[id & 0xFFFF] * 4], 0, 0, 0, 1, 1);
}

void sound_set_vol_a(u8 *self, u32 v)
{
    U32(self, SND_VOL_A) = v;
}

void sound_set_vol_b(u8 *self, u32 v)
{
    U32(self, SND_VOL_B) = v;
}

/* volume v (0-7 scale) applied to a */
s32 sound_scale7(u8 *self, s32 a, s32 v)
{
    return (s32)((f32)a * (f32)v / 7.0f);
}

/* 0-127 -> percent */
s32 sound_percent(u8 *self, s32 v)
{
    return (s32)((f32)v / 127.0f * 100.0f);
}

/* asks to change the sounds a, b, c of the channel ch (0xFFFF: unchanged) */
u8 sound_request_change(u8 *self, s32 ch, u32 a, u32 c, u32 b)
{
    u8 busy, ca;

    if (sndbank_is_playing(D_08A62D84, ch) == 1) {
        busy = 1;
    } else {
        ca = a != sndbank_get_a(D_08A62D84, ch);
        if (!ca)
            a = 0xFFFF;
        busy = c != sndbank_get_c(D_08A62D84, ch);
        if (!busy)
            c = 0xFFFF;
        busy = busy || ca;
        if (b == sndbank_get_b(D_08A62D84, ch))
            b = 0xFFFF;
        else
            busy = 1;
    }
    if (busy) {
        sndbank_stop_ch(D_08A62D84, ch, 0);
        if (a != 0xFFFF)
            sndbank_clear_a(D_08A62D84, ch);
        if (c != 0xFFFF)
            sndbank_clear_c(D_08A62D84, ch);
        if (b != 0xFFFF)
            sndbank_clear_b(D_08A62D84, ch);
    }
    if (U32(self, SND_REQ_KIND) != 0)
        return 0;
    S16(self, SND_RQ_CH) = ch;
    S16(self, SND_RQ_ARG) = a;
    S16(self, SND_RQ_C) = c;
    S16(self, SND_RQ_B) = b;
    U32(self, SND_REQ_KIND) = 2;
    return 1;
}

/* asks to load n pairs {a[i], b[i]} on the channel ch */
u32 sound_request_load(u8 *self, s32 ch, u16 *a, u16 *b, u16 k, s32 n)
{
    s32 change, i;

    if (sndbank_is_playing(D_08A62D84, ch) == 1) {
        change = 1;
    } else {
        s16 x = sndbank_get_a(D_08A62D84, ch);
        s16 z = sndbank_get_c(D_08A62D84, ch);

        change = z != -1 || x != -1;
        if ((s16)sndbank_get_b(D_08A62D84, ch) != -1)
            change = 1;
    }
    if (change) {
        sndbank_stop_ch(D_08A62D84, ch, 0);
        sndbank_clear_abc(D_08A62D84, ch);
    }
    if (U32(self, SND_REQ_KIND) != 0)
        return 0;
    for (i = 0; i < n; i++) {
        U16(self, SND_RQ_ARG + i * 2) = a[i];
        U16(self, SND_RQ_LOAD_B + i * 2) = b[i];
    }
    S16(self, SND_RQ_LOAD_CH) = ch;
    U16(self, SND_RQ_LOAD_K) = k;
    U32(self, SND_RQ_LOAD_N) = n;
    U32(self, SND_REQ_KIND) = 3;
    return 1;
}

void sound_slot_clear(u8 *self, s32 i)
{
    u16 *p = &U16(self, SND_CH_A + i * 2);

    p[0] = 0xFFFF;
    p[4] = 0xFFFF;
    p[8] = 0xFFFF;
    p[12] = 0;
    self[SND_CH_EXTRA + i] = 0;
    if (i == 0) {
        U32(self, SND_MON_A) = -1;
        U32(self, SND_MON_B) = -1;
    }
    p = &U16(self, SND_LOADED_A + i * 2);
    p[0] = 0xFFFF;
    p[4] = 0xFFFF;
    p[8] = 0xFFFF;
    p[12] = 0;
    self[SND_LOADED_EXTRA + i] = 0;
}

void sound_slot_set(u8 *self, u16 d, u16 a, u16 b, u16 c, u8 e, s32 i)
{
    u16 *p = &U16(self, SND_LOADED_A + i * 2);

    p[0] = a;
    p[4] = b;
    p[8] = c;
    p[12] = d;
    self[SND_LOADED_EXTRA + i] = e;
}
