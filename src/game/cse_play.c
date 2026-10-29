/* The Capcom Sound Engine (3): the commands of the driver that start, stop and change the
 * sounds, the reset of the engine and the status for the game. A command is 64 bytes:
 * {code, data, request (from 8): flags, .., priority 0x0B, channel 0x0C, ...}. */
#include "common.h"
#include "cse.h"

#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define S16(p, off) (*(s16 *)((u8 *)(p) + (off)))
#define U16(p, off) (*(u16 *)((u8 *)(p) + (off)))

extern u32 D_09A0D138;            /* sound data */
extern u32 D_09A0D13C;
extern s32 D_09A0B564, D_09A0B568;
extern const char D_089B2374[], D_089B237C[], D_089B23CC[];

int printf(const char *fmt, ...);
int sceKernelCpuSuspendIntr(void);
void sceKernelCpuResumeIntr(int s);
u8 *cse_used_first(void);
u8 *cse_used_next(void);
u8 *cse_free_first(void);
u8 *cse_free_next(void);
u32 cse_free_count(void);
u32 cse_used_count(void);
u8 *cse_voice_take(void);
u32 cse_voices_clear(void);
u32 cse_voices_init(void);
void cse_queue_init(void);
void cse_chans_reset(void);
u8 cse_req_mask(u16 *req);
u32 cse_match(u8 *c, u8 *r, u32 mask);
u32 cse_voices_command(u16 *req, u32 mask, s32 cmd);
u32 cse_voices_priority(u8 *req, u32 mask, u16 *upd);
u8 *cse_voice_to_replace(u16 *req);
s32 cse_voices_count(u16 *req, s32 mode);
u32 cse_voices_release(void);
u32 cse_voice_apply(u8 *v);
u32 cse_voice_stop(u8 *v);
void cse_lfo_clear(u8 *l);
void func_088A13A4(u16 *req);
void sasv_key_on(u8 *sas);
void sas_reverb(u32 *p);

u32 cse_make_room(u16 *req);
void cse_set_priority(u16 *req, u16 *upd);
u32 cse_reset_status(u32 *p);

static void lock(void)
{
    if (D_09A0B564++ == 0)
        D_09A0B568 = sceKernelCpuSuspendIntr();
}

static void unlock(void)
{
    if (D_09A0B564 != 0 && --D_09A0B564 == 0)
        sceKernelCpuResumeIntr(D_09A0B568);
}

/* starts a sound (command 0x10000000) */
void cse_cmd_play(u8 *c)
{
    u8 *v, *sas;
    s32 p;

    for (v = cse_used_first(); v != NULL; v = cse_used_next())
        if (D_09A0D138 + U32(c, 0x34) == U32(U32(v, CV_SAS), 0x24) && v[CV_KEY] != 0)
            return;
    if (cse_make_room((u16 *)(c + 8)) == 0)
        return;
    func_088A13A4((u16 *)(c + 8));
    v = cse_voice_take();
    if (v == NULL) {
        printf(D_089B2374);
        printf(D_089B237C);
        printf(D_089B23CC);
        return;
    }
    U32(v, CV_LENGTH) = U32(c, 0x1C);
    U32(v, CV_AGE) = 0;
    v[CV_KEY] = c[0x3C];
    U32(v, CV_ID) = U32(c, 4);
    v[CV_FLAGS] = c[10];
    v[CV_FLAGS2] = c[10];
    v[CV_X1A] = 1;
    v[CV_PRIORITY] = c[0xB];
    v[CV_CHANNEL] = c[0xC] | ((U16(c, 8) & 2) != 0 ? 0x80 : 0);
    v[CV_X1D] = c[0xD];
    v[CV_ID1] = (U16(c, 8) & 0x20) != 0 ? c[0xE] : 0xFF;
    v[CV_ID2] = (U16(c, 8) & 0x40) != 0 ? c[0xF] : 0xFF;
    U32(v, CV_RATE) = U32(c, 0x30);
    v[CV_PAN] = c[0x25];
    v[CV_PAN2] = (u8)U16(c, 0x12);
    U16(v, CV_VOL) = c[0x24] == 0 ? 0 : c[0x24] * 0xFFF / 0x7F;
    U16(v, CV_VOL2) = S16(c, 0x10) < 1 ? 0 : S16(c, 0x10) * 0xFFF / 0x7F;
    U16(v, CV_PITCH) = U16(c, 0x26);
    U16(v, CV_PITCH2) = U16(c, 0x14);
    v[CV_X2A] = c[0x1B];
    U16(v, CV_BEND_DOWN) = U16(c, 0x28);
    U16(v, CV_BEND_UP) = U16(c, 0x2A);
    U16(v, CV_BEND) = 0;
    if (S16(c, 0x16) >= 0x2000)
        U16(c, 0x16) = 0x1FFF;
    else if (S16(c, 0x16) < -0x2000)
        U16(c, 0x16) = 0xE000;
    p = S16(c, 0x16);
    if (p < 0)
        S16(v, CV_BEND) = (s32)(U16(v, CV_BEND_DOWN) * p) / 0x2000;
    else
        S16(v, CV_BEND) = (s32)(U16(v, CV_BEND_UP) * p) / 0x1FFF;
    cse_lfo_clear(v + CV_LFO1);
    cse_lfo_clear(v + CV_LFO2);
    sas = (u8 *)U32(v, CV_SAS);
    U32(sas, SV_SIZE) = U32(c, 0x38);
    U16(sas, SV_ADSR1) = U16(c, 0x2C);
    U16(sas, SV_ADSR2) = U16(c, 0x2E);
    U32(sas, SV_DATA) = D_09A0D138 + U32(c, 0x34);
    U32(sas, SV_FLAGS) |= 0x900;
    if ((v[CV_FLAGS] & 0xC) == 0)
        U16(v, CV_ENV) = 0;
    else
        U16(v, CV_ENV) = c[0x19] << 4;
    cse_voice_apply(v);
    sasv_key_on((u8 *)U32(v, CV_SAS));
    U32(v, CV_STATE) = 1;
}

/* the LFOs of the matching voices (command 0x10000004) */
void cse_cmd_lfo(u8 *c)
{
    u16 *req = (u16 *)(c + 8);
    u8 mask = cse_req_mask(req);
    u8 *v;

    if ((req[0] & 1) == 0) {
        c[0xB] = 0x7F;
        mask |= 1;
    }
    for (v = cse_used_first(); v != NULL; v = cse_used_next()) {
        if (U32(v, CV_STATE) == 0 || U32(v, CV_STATE) == 7)
            continue;
        if (cse_match(v + CV_ID, (u8 *)req, mask) == 0 || v[CV_PRIORITY] > c[0xB])
            continue;
        U32(v, CV_LFO1) = 0;
        U16(v, CV_LFO1_SPEED) = U16(c, 0x24);
        U16(v, CV_LFO1_DEPTH) = U16(c, 0x26);
        U32(v, CV_LFO2) = 0;
        U16(v, CV_LFO2_SPEED) = U16(c, 0x28);
        U16(v, CV_LFO2_DEPTH) = U16(c, 0x2A);
    }
}

void cse_cmd_priority(u8 *c)
{
    cse_set_priority((u16 *)(c + 8), (u16 *)(c + 0x24));
}

/* back to the start state */
u32 cse_reset(void)
{
    u32 p[6];

    cse_voices_clear();
    cse_voices_init();
    cse_queue_init();
    cse_chans_reset();
    D_09A0D13C = 0;
    p[0] = -1;
    p[4] = 1;
    p[1] = 0;
    p[2] = 0;
    p[3] = 0;
    p[5] = 0;
    cse_reset_status(p);
    return 0;
}

/* frees voices for a new sound (at most req[0x10] at the same time); 0 if not allowed */
u32 cse_make_room(u16 *req)
{
    u32 n = cse_voices_count(req, 1);
    u32 max = (u8)req[8];
    u32 ret = 1, i;
    u8 *v;

    if (max == 0) {
        if ((req[0] & 1) != 0)
            ret = cse_voices_command(req, cse_req_mask(req), 0x10000001);
    } else if (max <= n) {
        for (i = 0; i < n + 1 - max; i++) {
            v = cse_voice_to_replace(req);
            if (v != NULL) {
                if ((req[0] & 1) != 0 && ((u8 *)req)[3] < v[CV_PRIORITY]) {
                    ret = 0;
                    break;
                }
                if (U32(v, CV_STATE) == 1) {
                    cse_voice_stop(v);
                    U32(v, CV_STATE) = 7;
                } else {
                    U32(v, CV_STATE) = 4;
                }
            }
            ret = 1;
        }
    }
    cse_voices_release();
    return ret;
}

static u32 req_all(u16 *req, s32 cmd)
{
    u8 mask = cse_req_mask(req);
    u32 r;

    if ((req[0] & 1) == 0) {
        ((u8 *)req)[3] = 0x7F;
        mask |= 1;
    }
    r = cse_voices_command(req, mask, cmd);
    return r;
}

u32 cse_release(u16 *req)
{
    u32 r = req_all(req, 0x10000001);

    cse_voices_release();
    return r;
}

u32 cse_stop(u16 *req)
{
    u32 r = req_all(req, 0x10000002);

    cse_voices_release();
    return r;
}

void cse_change(u16 *req)
{
    req_all(req, 0x10000003);
}

void cse_set_priority(u16 *req, u16 *upd)
{
    u8 mask = cse_req_mask(req);

    if ((req[0] & 1) == 0) {
        ((u8 *)req)[3] = 0x7F;
        mask |= 1;
    }
    cse_voices_priority((u8 *)req, mask, upd);
}

u32 cse_reset_status(u32 *p)
{
    u32 q[7];

    q[0] = p[0];
    q[1] = p[1];
    q[2] = p[2];
    q[3] = p[3];
    q[4] = p[3];
    q[5] = p[4];
    q[6] = p[5];
    sas_reverb(q);
    return 0;
}

/* status: masks of the voices playing and free, counts */
u32 cse_status(u32 *out)
{
    u32 used = 0, freem = 0;
    u8 *v;

    lock();
    for (v = cse_used_first(); v != NULL; v = cse_used_next())
        used |= U32(U32(v, CV_SAS), 8);
    unlock();
    lock();
    for (v = cse_free_first(); v != NULL; v = cse_free_next())
        freem |= U32(U32(v, CV_SAS), 8);
    unlock();
    out[2] = freem;
    out[3] = used;
    ((u8 *)out)[0x10] = cse_free_count();
    ((u8 *)out)[0x11] = cse_used_count();
    out[0] = 0;
    return 0;
}

u32 cse_count_status(u32 *out, u16 *req)
{
    lock();
    ((u8 *)out)[0x12] = cse_voices_count(req, 0);
    out[0] = 0;
    unlock();
    return out[0];
}
