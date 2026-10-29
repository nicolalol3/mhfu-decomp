/* The Capcom Sound Engine (2): the driver thread (commands of the queue), the update of the
 * playing voices (volume, pan, pitch to the sceSas voice), the search of the voices by their
 * conditions, the channel volumes with their fades and the LFOs. (Guessed names.)
 * Voice (0x48): 0 state (1 starting, 2 playing, 3 to stop, 5 stopping, 7 ended), 4 age,
 * 8 length, 0x10 sceSas voice, 0x14 id/conditions, 0x1B priority, 0x1C channel, 0x20/0x21
 * pan, 0x22/0x24 volumes, 0x26/0x28/0x2C pitch, 0x34 rate, 0x38/0x40 LFOs. */
#include "common.h"
#include "cse.h"

#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define S16(p, off) (*(s16 *)((u8 *)(p) + (off)))
#define U16(p, off) (*(u16 *)((u8 *)(p) + (off)))

extern u32 D_09A0D138;
extern u32 D_09A0D13C;            /* 1: mono */
extern u8 D_09A0D140[16 * 0x10];  /* the channels */
extern u8 D_09A0D240[0x10];       /* the master */
extern const s16 D_089B2170[];    /* quarter of a sine wave (256 steps) */
extern s32 D_09A0B564, D_09A0B568;
extern const char D_089B2374[], D_089B237C[], D_089B2384[], D_089B239C[];

int printf(const char *fmt, ...);
int sceKernelCpuSuspendIntr(void);
void sceKernelCpuResumeIntr(int s);
f32 powf(f32 x, f32 y);
s32 cse_queue_get(s32 q, void *cmd);
u8 *cse_used_first(void);
u8 *cse_used_next(void);
u32 cse_voice_give(u32 v);
void stub_088a0bec(void);   /* -> cse_reset */
void stub_088a0bf4(void *cmd);   /* -> cse_reset */
s16 func_088A0BE4(s32 pan);
s32 sas_frame(void);
u32 sas_ended(void);
void sasv_key_off(u8 *sas);
void sasv_reset(u8 *sas);
void func_088A0DB0(void *cmd);
void cse_cmd_play(void *cmd);
void func_088A10E0(void *cmd);
void func_088A10E8(void *cmd);
void func_088A10F0(void *cmd);
void cse_cmd_lfo(void *cmd);
void cse_cmd_priority(void *cmd);

void cse_chans_update(void);
u32 cse_voices_update(void);
u32 cse_voice_apply(u8 *v);
u32 cse_voice_stop(u8 *v);
u32 cse_voices_release(void);
void cse_lfo_step(u8 *l);
s32 cse_lfo_value(u8 *l);
s32 cse_sin(u32 x);
void cse_chan_fade_step(u8 *c);
void cse_chan_reset(u8 *c);
void cse_set_mono(u8 *cmd);
void cse_chan_volume(u8 *cmd);
void cse_chan_fade(u8 *cmd);
void cse_chan_stop_fade(u8 *cmd);

u32 cse_set_data(u32 a)
{
    D_09A0D138 = a;
    stub_088a0bec();
    return 0;
}

/* the driver thread: one frame of the voices, then up to 64 commands */
u32 cse_driver(void)
{
    u32 cmd[16];
    u32 n;
    s32 r;

    cse_chans_update();
    cse_voices_update();
    for (n = 0; n < 0x40; n++) {
        if (D_09A0B564++ == 0)
            D_09A0B568 = sceKernelCpuSuspendIntr();
        r = cse_queue_get(0, cmd);
        if (D_09A0B564 != 0 && --D_09A0B564 == 0)
            sceKernelCpuResumeIntr(D_09A0B568);
        if (r < 0)
            return 0;
        switch (cmd[0]) {
        case 0x10000000: cse_cmd_play(cmd); break;
        case 0x10000001: func_088A10E0(cmd); break;
        case 0x10000002: func_088A10E8(cmd); break;
        case 0x10000003: func_088A10F0(cmd); break;
        case 0x10000004: cse_cmd_lfo(cmd); break;
        case 0x10000006: cse_cmd_priority(cmd); break;
        case 0x10000007: func_088A0DB0(cmd); break;
        case 0x10000008: break;
        case 0x10000009: cse_set_mono((u8 *)cmd); break;
        case 0x1000000A: cse_chan_volume((u8 *)cmd); break;
        case 0x1000000B: cse_chan_fade((u8 *)cmd); break;
        case 0x1000000C: cse_chan_stop_fade((u8 *)cmd); break;
        case 0x1000000D: stub_088a0bf4(cmd); break;
        default:
            printf(D_089B2374);
            printf(D_089B237C);
            printf(D_089B2384, cmd[0]);
            break;
        }
    }
    return 0;
}

u32 cse_voices_update(void)
{
    u8 *v, *sas;
    s32 st;
    u32 len;

    for (v = cse_used_first(); v != NULL; v = cse_used_next()) {
        U32(v, CV_AGE)++;
        v[CV_KEY] = v[CV_KEY] < 0x11 ? 0 : v[CV_KEY] - 0x10;
        cse_lfo_step(v + CV_LFO1);
        cse_lfo_step(v + CV_LFO2);
        st = U32(v, CV_STATE);
        sas = (u8 *)U32(v, CV_SAS);
        if (st == 1 && S16(sas, SV_STATE) == 0) {
            if ((s32)U32(sas, SV_FRAME) != sas_frame()) {
                U32(v, CV_STATE) = 2;
                U16(sas, SV_STATE) = 1;
            }
            continue;
        }
        if (st != 0 && st != 7)
            cse_voice_apply(v);
        len = U32(v, CV_LENGTH);
        if (len != 0 && len < U32(v, CV_AGE))
            U32(v, CV_STATE) = 3;
        if (U32(v, CV_STATE) == 4)
            U32(v, CV_STATE) = 3;
        if (U32(v, CV_STATE) == 3) {
            sasv_key_off((u8 *)U32(v, CV_SAS));
            U32(v, CV_STATE) = 5;
        }
        if ((U32(U32(v, CV_SAS), 8) & sas_ended()) != 0) {
            cse_voice_stop(v);
            U32(v, CV_STATE) = 7;
        }
    }
    cse_voices_release();
    return 0;
}

/* volume, pan and pitch of the voice to its sceSas voice */
u32 cse_voice_apply(u8 *v)
{
    s16 ch = S16(D_09A0D140, (v[CV_CHANNEL] & 0xF) * 0x10);
    u8 *sas;
    s32 vol, pan, l, r, lfo;
    u32 rate;
    f32 f;
    u8 b;

    lfo = cse_lfo_value(v + CV_LFO2);
    vol = ((s32)(ch * U16(v, CV_VOL)) / 0xFFF) * ((u32)(U16(v, CV_VOL2) * v[CV_X2A]) / 100) / 0xFFF +
          lfo;
    if (vol >= 0x1000)
        vol = 0xFFF;
    else if (vol < 0)
        vol = 0;
    pan = v[CV_PAN] + (v[CV_PAN2] - 0x40);
    if (pan >= 0x80)
        pan = 0xFE;
    else if (pan < 0)
        pan = 0;
    else
        pan <<= 1;
    l = (vol * func_088A0BE4(pan)) / 0x7FFF;
    r = (vol * (s16)cse_sin(pan)) / 0x7FFF;
    lfo = cse_lfo_value(v + CV_LFO1);
    rate = U32(v, CV_RATE);
    f = powf(2.0f, (f32)(S16(v, CV_BEND) + S16(v, CV_PITCH) + S16(v, CV_PITCH2) + lfo) / 1200.0f);
    f = (f32)rate * f;
    rate = ((u32)f << 12) / 44100;
    sas = (u8 *)U32(v, CV_SAS);
    if (D_09A0D13C == 0) {
        S16(sas, SV_VOL_L) = l;
        S16(sas, SV_VOL_R) = r;
    } else {
        S16(sas, SV_VOL_L) = (l + r) / 2;
        S16(sas, SV_VOL_R) = (l + r) / 2;
    }
    U16(sas, SV_ENV_A) = U16(v, CV_ENV);
    U16(sas, SV_ENV_B) = U16(v, CV_ENV);
    U32(sas, SV_FLAGS) |= 0x400;
    if (rate > 0x3FFF)
        rate = 0x3FFF;
    S16(sas, SV_PITCH) = rate;
    U32(sas, SV_FLAGS) |= 4;
    b = v[CV_FLAGS2] & 0xC0;
    if (b == 0) {
        if ((v[CV_FLAGS] & 0xC0) == 0x80)
            v[CV_X1A] = -(s8)v[CV_X1A];
    } else if (b == 0x80 && (v[CV_FLAGS] & 0xC0) == 0) {
        v[CV_X1A] = -(s8)v[CV_X1A];
    }
    v[CV_FLAGS2] = v[CV_FLAGS];
    S16(sas, SV_VOL_L) = S16(sas, SV_VOL_L) * (s8)v[CV_X1A];
    S16(sas, SV_VOL_R) = S16(sas, SV_VOL_R) * (s8)v[CV_X1A];
    if ((v[CV_FLAGS] & 0x80) != 0)
        S16(sas, SV_VOL_L) = -S16(sas, SV_VOL_L);
    return 0;
}

u32 cse_voice_stop(u8 *v)
{
    sasv_reset((u8 *)U32(v, CV_SAS));
    return 0;
}

/* the ended voices go back to the free list */
u32 cse_voices_release(void)
{
    u8 *v = cse_used_first(), *next;

    while (v != NULL) {
        if (U32(v, CV_STATE) == 7) {
            U32(v, CV_STATE) = 0;
            next = cse_used_next();
            cse_voice_give((u32)v);
        } else {
            next = cse_used_next();
        }
        v = next;
    }
    return 0;
}

/* the conditions of a request (bits of the request flags) */
u8 cse_req_mask(u16 *req)
{
    u16 f = req[0];
    u8 m = (f & 1) != 0;

    if ((f & 2) != 0)
        m |= 2;
    else if ((f & 8) != 0)
        m |= 4;
    else if ((f & 4) != 0)
        m |= 0x80;
    if ((f & 0x10) != 0)
        m |= 8;
    if ((f & 0x20) != 0)
        m |= 0x10;
    if ((f & 0x40) != 0)
        m |= 0x20;
    if ((f & 0x80) != 0)
        m |= 0x40;
    return m;
}

/* the voice conditions c match the request r for the bits of mask */
u32 cse_match(u8 *c, u8 *r, u32 mask)
{
    u32 bit = 1, b;
    u8 x;

    for (;; mask &= ~bit, bit <<= 1) {
        if (mask == 0)
            return 1;
        b = mask & bit;
        if (b == 0x40) {
            if (U32(c, 0) != U32(r, 0x18))
                return 0;
        } else if (b == 0x20) {
            if (c[0xB] != r[7])
                return 0;
        } else if (b == 0x10) {
            if (c[0xA] != r[6])
                return 0;
        } else if (b == 8) {
            if (c[9] != r[5])
                return 0;
        } else if (b == 0x80 || b == 4) {
            if (b == 4 && (c[8] & 0x80) != 0)
                return 0;
            x = c[8];
            if ((x & 0xF) != (r[4] & 0xF))
                return 0;
        } else if (b == 2) {
            if ((c[8] & 0x80) == 0)
                return 0;
        } else if (b != 1 && b != 0) {
            printf(D_089B2374);
            printf(D_089B237C);
            printf(D_089B239C);
        }
    }
}

/* change (0x10000003), release (0x10000001) or stop (0x10000002) the matching voices */
u32 cse_voices_command(u16 *req, u32 mask, s32 cmd)
{
    u8 *v, *r = (u8 *)req;
    u32 ret = 1;
    s32 x;
    s16 p;

    for (v = cse_used_first(); v != NULL; v = cse_used_next()) {
        if (U32(v, CV_STATE) == 0 || cse_match(v + CV_ID, r, mask) == 0)
            continue;
        if (r[3] < v[CV_PRIORITY]) {
            ret = 0;
        } else if (cmd == 0x10000003) {
            if ((req[0] & 0x100) != 0) {
                x = (s16)req[4];
                if (x >= 0 && x < 0x80)
                    U16(v, CV_VOL2) = x < 1 ? 0 : x * 0xFFF / 0x7F;
            }
            if ((req[0] & 0x200) != 0) {
                x = (s16)req[5];
                if (x >= 0 && x < 0x80) {
                    v[CV_PAN2] = x;
                    v[CV_FLAGS] = (v[CV_FLAGS] & 0x3F) | (r[2] & 0xC0);
                }
            }
            if ((req[0] & 0x400) != 0)
                U16(v, CV_PITCH2) = req[6];
            if ((req[0] & 0x800) != 0) {
                if ((s16)req[7] >= 0x2000)
                    req[7] = 0x1FFF;
                else if ((s16)req[7] < -0x2000)
                    req[7] = 0xE000;
                p = req[7];
                if (p < 0)
                    S16(v, CV_BEND) = (s32)(U16(v, CV_BEND_DOWN) * p) / 0x2000;
                else
                    S16(v, CV_BEND) = (s32)(U16(v, CV_BEND_UP) * p) / 0x1FFF;
            }
            if ((req[0] & 0x4000) != 0)
                v[CV_PRIORITY] = r[3];
            if ((req[0] & 0x1000) != 0)
                v[CV_X2A] = r[0x13];
            if ((req[0] & 0x2000) != 0)
                U16(v, CV_ENV) = r[0x11];
        } else if (cmd == 0x10000001) {
            if (U32(v, CV_STATE) == 1) {
                cse_voice_stop(v);
                U32(v, CV_STATE) = 7;
            } else {
                U32(v, CV_STATE) = 3;
            }
        } else if (cmd == 0x10000002) {
            cse_voice_stop(v);
            U32(v, CV_STATE) = 7;
        }
    }
    return ret;
}

/* priority / values of the matching voices */
u32 cse_voices_priority(u8 *req, u32 mask, u16 *upd)
{
    u8 *v, *u = (u8 *)upd;
    u32 s;

    for (v = cse_used_first(); v != NULL; v = cse_used_next()) {
        if (U32(v, CV_STATE) == 0 || cse_match(v + CV_ID, req, mask) == 0)
            continue;
        if ((upd[0] & 0x100) != 0) {
            v[CV_PRIORITY] = (u8)upd[1];
        } else if ((upd[0] & 0x200) != 0) {
            s = v[CV_PRIORITY] + (u8)upd[1];
            v[CV_PRIORITY] = s < 0x80 ? s : 0x7F;
        }
        if ((upd[0] & 0x400) != 0)
            v[CV_ID1] = u[3];
        if ((upd[0] & 0x800) != 0)
            v[CV_ID2] = (u8)upd[2];
    }
    return 0;
}

/* the matching voice to replace: lowest priority, then the oldest */
u8 *cse_voice_to_replace(u16 *req)
{
    u8 mask = cse_req_mask(req);
    u8 *v, *best = NULL;

    for (v = cse_used_first(); v != NULL; v = cse_used_next()) {
        if (U32(v, CV_STATE) != 1 && U32(v, CV_STATE) != 2)
            continue;
        if (cse_match(v + CV_ID, (u8 *)req, mask) == 0)
            continue;
        if (best == NULL)
            best = v;
        else if (v[CV_PRIORITY] < best[CV_PRIORITY])
            best = v;
        else if (v[CV_PRIORITY] == best[CV_PRIORITY] && U32(best, CV_AGE) < U32(v, CV_AGE))
            best = v;
    }
    return best;
}

/* matching voices (playing ones only when mode 1) */
s32 cse_voices_count(u16 *req, s32 mode)
{
    u8 mask = cse_req_mask(req);
    u8 *v;
    s32 n = 0;

    for (v = cse_used_first(); v != NULL; v = cse_used_next()) {
        if (cse_match(v + CV_ID, (u8 *)req, mask) != 1)
            continue;
        if (mode == 0)
            n++;
        else if (mode == 1 && U32(v, CV_STATE) - 1 < 2)
            n++;
    }
    return n;
}

/* ---- channels: {value, base, fade (16.16 at 4), step 8, target 0xC} ---- */

void cse_chans_reset(void)
{
    s32 i;

    cse_chan_reset(D_09A0D240);
    for (i = 0; i < 0x10; i++)
        cse_chan_reset(D_09A0D140 + i * 0x10);
}

void cse_chan_reset(u8 *c)
{
    U16(c, 0) = 0xFFF;
    U16(c, 2) = 0xFFF;
    U16(c, 6) = 0xFFF;
    U16(c, 4) = 0;
    U32(c, 8) = 0;
    U16(c, 0xC) = 0;
}

void cse_chans_update(void)
{
    u8 *c = D_09A0D140;
    s32 i;

    cse_chan_fade_step(D_09A0D240);
    for (i = 0; i < 0x10; i++, c += 0x10) {
        cse_chan_fade_step(c);
        S16(c, 0) = (S16(D_09A0D240, 0) * S16(c, 0)) / 0xFFF;
    }
}

void cse_chan_fade_step(u8 *c)
{
    if (U32(c, 8) != 0) {
        U32(c, 4) += U32(c, 8);
        if ((s32)U32(c, 8) < 0) {
            if (S16(c, 6) <= S16(c, 0xC)) {
                U16(c, 6) = U16(c, 0xC);
                U16(c, 4) = 0;
                U32(c, 8) = 0;
            }
        } else if (S16(c, 0xC) <= S16(c, 6)) {
            U16(c, 6) = U16(c, 0xC);
            U16(c, 4) = 0;
            U32(c, 8) = 0;
        }
    }
    S16(c, 0) = (S16(c, 2) * S16(c, 6)) / 0xFFF;
}

void cse_lfo_clear(u8 *l)
{
    U32(l, 0) = 0;
    U16(l, 4) = 0;
    U16(l, 6) = 0;
}

void cse_lfo_step(u8 *l)
{
    U32(l, 0) += U16(l, 4);
}

s32 cse_lfo_value(u8 *l)
{
    return (s32)(U16(l, 6) * (s16)cse_sin(U32(l, 0) >> 8)) / 0x8000;
}

/* sine of x (0x400 = a turn), from the quarter table */
s32 cse_sin(u32 x)
{
    s32 a = x & 0x3FF;
    s32 q;

    if ((s32)x < 0) {
        if (a == 0)
            goto first;
        a -= 0x400;
    }
    if (a < 0x100) {
first:
        q = a & 0xFF;
        if (a < 0 && q != 0)
            q -= 0x100;
        return D_089B2170[q];
    }
    if (a < 0x200)
        return D_089B2170[0x200 - a];
    if (a < 0x300) {
        q = a & 0xFF;
        if (a < 0 && q != 0)
            q -= 0x100;
        return -D_089B2170[q];
    }
    if (a < 0x400)
        return -D_089B2170[0x400 - a];
    return 0;
}

void cse_set_mono(u8 *cmd)
{
    D_09A0D13C = cmd[8] & 1;
}

static u8 *cmd_chan(u8 *cmd)
{
    return cmd[8] == 0xFF ? D_09A0D240 : D_09A0D140 + cmd[8] * 0x10;
}

void cse_chan_volume(u8 *cmd)
{
    u8 *c = cmd_chan(cmd);

    U16(c, 2) = cmd[9] == 0 ? 0 : cmd[9] * 0xFFF / 0x7F;
}

void cse_chan_fade(u8 *cmd)
{
    u8 *c = cmd_chan(cmd);
    s32 t = cmd[0x10] == 0 ? 0 : cmd[0x10] * 0xFFF / 0x7F;

    if (t < S16(c, 6))
        U32(c, 8) = -(s32)U32(cmd, 0xC);
    else
        U32(c, 8) = U32(cmd, 0xC);
    U16(c, 0xC) = cmd[0x10] == 0 ? 0 : cmd[0x10] * 0xFFF / 0x7F;
}

void cse_chan_stop_fade(u8 *cmd)
{
    u8 *c = cmd_chan(cmd);

    U16(c, 6) = 0xFFF;
    U16(c, 4) = 0;
    U32(c, 8) = 0;
    U16(c, 0xC) = 0;
}
