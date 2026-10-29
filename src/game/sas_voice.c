/* The Capcom Sound Engine (5): the sceSas side. The render thread (mixes 256 samples, an
 * optional limiter, output), the 32 sceSas voices of 0x2C bytes at 0x09A0E3CC
 * {start frame, started, .., id 6, bit 8, flags 0x0C, volumes 0x10/0x12, pitch 0x14, ADSR
 * 0x16/0x18, wet volumes 0x20/0x22, data 0x24, size 0x28}, the reverb, the driver thread,
 * the sound map (SPUMAPDT) and the decode thread of the ATRAC3 players. (Guessed names.) */
#include "common.h"
#include "cse.h"

#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define S16(p, off) (*(s16 *)((u8 *)(p) + (off)))
#define U16(p, off) (*(u16 *)((u8 *)(p) + (off)))
#define F32(p, off) (*(f32 *)((u8 *)(p) + (off)))

extern s32 D_09A0DA74, D_09A0DA78, D_09A0DA7C, D_09A0DA84, D_09A0DA88;
extern u8 D_09A0DB44;             /* bit 0: limiter of the effects, bit 1: of the music */
extern u32 D_089B258C;
extern f32 D_089B24DC[21];        /* the limiter parameters of the effects */
extern f32 D_09A0DAE4[4], D_09A0DB04[4];
extern u32 D_09A0DB24, D_09A0DB28;
extern u8 D_09A0DB80[0x800];      /* the 2 output buffers */
extern s32 D_09A0E380;
extern s32 D_09A0E384[3];         /* driver thread {id, count, function} */
extern s32 D_09A0E390, D_09A0E394;
extern u8 D_09A0E3A0[0x40];
extern u8 D_09A0E3CC[0x20 * 0x2C];
extern s32 D_09A0E3B0[7];         /* reverb {type, delay, feedback, volume l, r, dry, wet} */
extern u8 D_09A0E980[0x40], D_09A0E9C0[0x40], D_09A0EA00[0x40], D_09A0EA40[0x40];
extern u32 D_09A0EA80;
extern s32 D_09A0EA84[0x20];      /* offsets and sizes of the sound map */
extern u8 *D_09A0EB04;
extern s32 D_09A0EB08, D_09A0EB0C;
extern s32 D_09A0B564, D_09A0B568;
extern s32 D_09A0DA70;
extern u32 D_089B24D4;
extern f32 D_089B24D0;
extern const char D_089B2590[], D_089B25A4[], D_089B25AC[], D_089B25B4[];
extern const char D_089B25D0[], D_089B25DC[], D_089B25E4[], D_089B25EC[], D_089B2604[];
extern const char D_089B261C[];

int printf(const char *fmt, ...);
void *memset(void *p, int c, u32 n);
void *memcpy(void *d, const void *s, u32 n);
int strncmp(const char *a, const char *b, u32 n);
int sceKernelCreateThread(const char *name, void *entry, int prio, int stack, u32 attr, void *opt);
int sceKernelStartThread(int thid, u32 arglen, void *argp);
int sceKernelDelayThread(u32 us);
int sceKernelDelayThreadCB(u32 us);
int sceKernelExitThread(int s);
int sceKernelSuspendDispatchThread(void);
int sceKernelResumeDispatchThread(int s);
int sceKernelCpuSuspendIntr(void);
void sceKernelCpuResumeIntr(int s);
f32 cosf(f32 x);
f32 sinf(f32 x);
s32 sas_core(void *buf);
s32 sas_init_default(void);
s32 sas_key_on(s32 v);
s32 sas_key_off(s32 v);
s32 sas_set_volume(s32 v, s32 l, s32 r, s32 el, s32 er);
s32 sas_set_pitch(s32 v, s32 p);
s32 sas_set_voice(s32 v, void *data, s32 size, s32 loop);
s32 sas_set_adsr(s32 v, u32 a, u32 b);
s32 sas_end_flags(void);
s32 sas_rev_von(s32 dry, s32 wet);
s32 sas_rev_evol(s32 l, s32 r);
s32 sas_rev_type(s32 t);
s32 sas_rev_param(s32 d, s32 f);
s32 audio_init(void);
s32 audio_set_len(u32 ch, u32 n);
s32 audio_output(u32 ch, u32 l, u32 r, void *buf);
s32 audio_rest(u32 ch);
void cse_sema_wait(s32 i);
void cse_request(u32 x, s32 kind, u32 y, s32 *cmd, u32 z, u32 *out, u32 a, u32 b, u32 c);
void atrac_sema_wait(s32 *s);
void atrac_sema_signal(s32 *s);
s32 atrac_decode(u8 *p);
extern s32 D_09A0D258;

u8 *sasv_ram(void);
void sasv_apply(u8 *v);
void sasv_apply_all(void);
void sasv_reset(u8 *v);
u32 sound_map_select(u32 i);
s32 sas_driver_thread(u32 args, s32 **argp);

#define VOICE(i) (D_09A0E3CC + (i) * 0x2C)

s32 sas_frame(void)
{
    return D_09A0DA78;
}

s32 sas_ended(void)
{
    return D_09A0DA7C;
}

/* the soft limiter: n stereo pairs of 2 samples; returns the counts of the clipped ones */
static void limit4(s16 *s, f32 g, f32 *f, const f32 *k, f32 lo, f32 hi, f32 up, f32 down,
                   s32 *c0, s32 *c1)
{
    f32 v[CV_AGE];
    s32 i;

    for (i = 0; i < 4; i++)
        f[i] = (f32)s[i] * g + f[i] * k[i];
    for (i = 0; i < 4; i++) {
        v[i] = f[i];
        if (v[i] < lo)
            v[i] = lo;
    }
    for (i = 0; i < 4; i++)
        if (hi <= v[i])
            v[i] = hi;
    if (up < v[CV_STATE] || v[CV_STATE] < down)
        (*c0)++;
    if (up < v[1] || v[1] < down)
        (*c1)++;
    if (up < v[2] || v[2] < down)
        (*c0)++;
    if (up < v[3] || v[3] < down)
        (*c1)++;
    for (i = 0; i < 4; i++)
        s[i] = (s32)(v[i] / g);
}

/* the gains of the 2 sides: down quickly when clipping, up slowly */
static void limit_gain(f32 *k, s32 c, f32 thr, f32 min, f32 max, f32 dn, f32 upv)
{
    if (thr < (f32)c) {
        k[0] -= dn;
        k[2] -= dn;
        if (k[0] < min)
            k[0] = min;
        if (k[2] < min)
            k[2] = min;
    } else {
        k[0] += upv;
        k[2] += upv;
        if (max < k[0])
            k[0] = max;
        if (max < k[2])
            k[2] = max;
    }
}

/* the render thread of sceSas */
s32 sas_render_thread(void)
{
    u32 b = 0;
    s16 *s;
    s32 i, c0, c1;
    f32 *P = D_089B24DC;

    while (D_09A0DA84 == 0) {
        sasv_apply_all();
        if (D_09A0DA88 != 0) {
            sceKernelDelayThread(D_089B258C);
            continue;
        }
        s = (s16 *)(D_09A0DB80 + b * 0x400);
        sas_core(s);
        if ((D_09A0DB44 & 1) != 0) {
            c0 = 0;
            c1 = 0;
            for (i = 0; i < 0x200; i += 4, s += 4)
                limit4(s, P[11], D_09A0DAE4, D_09A0DB04, P[17], P[16], P[19], P[20], &c0, &c1);
            limit_gain(D_09A0DB04, c0, P[18], P[15], P[14], 0.1f, 0.05f);
            limit_gain(D_09A0DB04 + 1, c1, P[18], P[15], P[14], 0.1f, 0.05f);
        }
        D_09A0DA7C = sas_end_flags();
        D_09A0DA78++;
        audio_output(D_09A0DA74, 0x8000, 0x8000, D_09A0DB80 + b * 0x400);
        b = (b + 1) & 1;
    }
    while (audio_rest(0) > 0)
        sceKernelDelayThread(1000);
    sceKernelExitThread(0);
    return 0;
}

/* the limiter parameters of the effects (FLT_MIN: keep the old value) */
void sas_limiter_set(f32 *v)
{
    s32 i;

    for (i = 0; i < 0x15; i++)
        if (v[i] != 1.1754944e-38f)
            D_089B24DC[i] = v[i];
    U32(D_09A0DAE4, 4) = 0;
    U32(D_09A0DAE4, 0) = 0;
    U32(D_09A0DB04, 4) = 0;
    U32(D_09A0DB04, 0) = 0;
    D_09A0DB24 = 0;
    D_09A0DB28 = 0;
}

void sas_limiter_defaults(u32 *p)
{
    s32 i;

    for (i = 0; i < 0x15; i++)
        p[i] = 0x800000;
}

/* the driver thread: calls the function every time the timer wakes it */
s32 sas_driver_start(s32 prio)
{
    s32 *arg = D_09A0E384;
    s32 th;

    D_09A0E384[0] = 0;
    D_09A0E384[1] = 0;
    D_09A0E384[2] = 0;
    th = sceKernelCreateThread(D_089B2590, sas_driver_thread, prio, 0x1000, 0, NULL);
    if (th < 0) {
        printf(D_089B25A4);
        printf(D_089B25AC);
        printf(D_089B25B4, th);
        return -1;
    }
    D_09A0E384[2] = 0;
    D_09A0E384[0] = th;
    sceKernelStartThread(th, 4, &arg);
    return D_09A0E384[0];
}

u32 sas_driver_set(u32 f)
{
    D_09A0E384[2] = f;
    return 0;
}

s32 sas_driver_thread(u32 args, s32 **argp)
{
    s32 *t = *argp;

    while (D_09A0E380 == 0) {
        cse_sema_wait(0);
        if (t[2] != 0)
            ((void (*)(s32))t[2])(t[1]);
        t[1]++;
    }
    sceKernelExitThread(0);
    return 0;
}

u32 sas_audio_open(void)
{
    if (D_09A0E390 == 0 && audio_init() != 0)
        return 0x80000014;
    D_09A0E390++;
    return 0;
}

u32 sas_open(void)
{
    if (D_09A0E394 == 0 && sas_init_default() != 0)
        return 0x80000016;
    D_09A0E394++;
    return 0;
}

void sasv_ram_clear(void)
{
    s32 i;

    for (i = 0; i < 0x10; i++)
        U32(D_09A0E3A0, i * 4) = 0;
    D_09A0E3A0[1] = 5;
}

u8 *sasv_ram(void)
{
    return D_09A0E3A0;
}

void sasv_init(void)
{
    u8 *v;
    s32 i;

    sasv_ram_clear();
    memset(D_09A0E3CC, 0, 0x580);
    for (i = 0, v = D_09A0E3CC; i < 0x20; i++, v += 0x2C) {
        S16(v, SV_NUM) = i;
        U32(v, CV_LENGTH) = 1 << (i & 0x1F);
        sasv_reset(v);
    }
    memset(D_09A0E3B0, 0, 0x1C);
    D_09A0E3B0[0] = -1;
    D_09A0E3B0[1] = 0;
    D_09A0E3B0[2] = 0;
    D_09A0E3B0[3] = 0;
    D_09A0E3B0[4] = 0;
    D_09A0E3B0[5] = 1;
    D_09A0E3B0[6] = 0;
}

u8 *sasv_get(u32 i)
{
    if (i < 0x20)
        return VOICE(i);
    return NULL;
}

void sasv_key_on(u8 *v)
{
    s32 d;

    if (U16(v, SV_NUM) >= 0x20)
        return;
    d = sceKernelSuspendDispatchThread();
    U32(v, CV_KEY) = (U32(v, CV_KEY) & ~0x8000) | 0x4000;
    U32(v, CV_STATE) = sas_frame();
    U16(v, CV_AGE) = 0;
    sceKernelResumeDispatchThread(d);
}

void sasv_key_off(u8 *v)
{
    s32 d;

    if (U16(v, SV_NUM) >= 0x20)
        return;
    d = sceKernelSuspendDispatchThread();
    U32(v, CV_KEY) = (U32(v, CV_KEY) & ~0x4000) | 0x8000;
    sceKernelResumeDispatchThread(d);
}

void sasv_reset(u8 *v)
{
    s32 d = sceKernelSuspendDispatchThread();

    U16(v, CV_SAS) = 0;
    U16(v, SV_VOL_R) = 0;
    U16(v, CV_PAN) = 0;
    U16(v, CV_VOL) = 0;
    U16(v, CV_ID) = 0x2000;
    U16(v, SV_ADSR1) = 0x80FF;
    U16(v, CV_FLAGS) = 0x5FC0;
    U32(v, CV_VOL2) = (u32)sasv_ram();
    U32(v, CV_PITCH2) = 0x10;
    U32(v, CV_KEY) = 0x8C04;
    U32(v, CV_STATE) = 0;
    U16(v, CV_AGE) = 0;
    sceKernelResumeDispatchThread(d);
    sasv_apply(v);
}

void sasv_apply_all(void)
{
    u32 i;

    for (i = 0; i < 0x20; i++)
        sasv_apply(VOICE(i));
}

/* the changes of the voice (flags 0x0C) to sceSas */
void sasv_apply(u8 *v)
{
    u32 f = U32(v, CV_KEY), bit = 1, b;

    for (;;) {
        if (f == 0)
            return;
        b = f & bit;
        if (b == 0x8000)
            sas_key_off(U16(v, SV_NUM));
        else if (b == 0x4000) {
            sas_set_voice(U16(v, SV_NUM), (void *)U32(v, CV_VOL2), U32(v, CV_PITCH2), 1);
            sas_key_on(U16(v, SV_NUM));
        } else if (b == 0x800)
            sas_set_adsr(U16(v, SV_NUM), U16(v, SV_ADSR1), U16(v, CV_FLAGS));
        else if (b == 4)
            sas_set_pitch(U16(v, SV_NUM), U16(v, CV_ID));
        else if (b == 0x400)
            sas_set_volume(U16(v, SV_NUM), S16(v, CV_SAS), S16(v, SV_VOL_R), U16(v, CV_PAN), U16(v, CV_VOL));
        f = U32(v, CV_KEY) & ~bit;
        bit <<= 1;
        U32(v, CV_KEY) = f;
    }
}

/* the reverb (only the changed values are sent) */
void sas_reverb(s32 *r)
{
    s32 *o = D_09A0E3B0;

    if (o[0] != r[0])
        sas_rev_type(r[0]);
    if (o[1] != r[1] || o[2] != r[2])
        sas_rev_param(r[1], r[2]);
    if (o[3] != r[3] || o[4] != r[4]) {
        if (r[0] == -1)
            sas_rev_evol(0, 0);
        else
            sas_rev_evol(r[3], r[4]);
    }
    if (o[5] != r[5] || o[6] != r[6])
        sas_rev_von(r[5], r[6]);
    o[0] = r[0];
    o[1] = r[1];
    o[2] = r[2];
    o[3] = r[3];
    o[5] = r[5];
    o[4] = r[4];
    o[6] = r[6];
}

/* a request to the driver, in its fixed buffers */
u8 *sas_request(s32 kind, const void *data, u32 size)
{
    u8 *in = NULL, *out = NULL;
    u32 ni = 0, no = 0, n = 0;

    if (kind != 0x65) {
        if (kind == 100) {
            in = D_09A0E9C0;
            out = D_09A0E980;
            ni = 0x40;
            no = 0x20;
            n = size > 0x3F ? 0x40 : size;
        } else if (kind == 2 || kind == 1 || kind == 0x309) {
            ni = 0x40;
            in = D_09A0EA40;
            out = D_09A0EA00;
            no = 0x40;
            n = size > 0x3F ? 0x40 : size;
        }
    }
    memset(in, 0xFF, ni);
    memset(out, 0xFF, no);
    memcpy(in, data, n);
    cse_request(0, kind, 0, (s32 *)in, ni, (u32 *)out, no, 0, 0);
    return out;
}

/* the sound map: offsets and sizes of the 16 banks of the current set */
u32 sound_map_load(u8 *map)
{
    s32 i;

    if (strncmp((const char *)map, D_089B25D0, 8) != 0) {
        printf(D_089B25DC);
        printf(D_089B25E4);
        printf(D_089B25EC);
        return 0x80000002;
    }
    for (i = 0; i < 0x10; i++) {
        D_09A0EA84[i] = 0;
        D_09A0EA84[i + 0x10] = 0;
    }
    D_09A0EA80 = 0;
    D_09A0EB04 = map;
    return sound_map_select(0);
}

u32 sound_map_select(u32 i)
{
    s32 *s, off = 0;
    s32 k;

    if (i >= U32(D_09A0EB04, 8)) {
        printf(D_089B25DC);
        printf(D_089B25E4);
        printf(D_089B2604);
        return 0x80000003;
    }
    s = (s32 *)(D_09A0EB04 + i * 0x40 + 0x10);
    D_09A0EA80 = i;
    for (k = 0; k < 0x10; k++) {
        D_09A0EA84[k] = off;
        D_09A0EA84[k + 0x10] = s[k];
        off += s[k];
    }
    return 0;
}

u32 sound_map_offset(u32 i)
{
    if (i < 0x10)
        return D_09A0EA84[i];
    printf(D_089B25DC);
    printf(D_089B25E4);
    printf(D_089B261C);
    return 0x80000001;
}

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

u32 cse_ids_init(void)
{
    if (D_09A0EB08 == 0) {
        lock();
        D_09A0EB0C = 0;
        unlock();
        D_09A0EB08 = 1;
    }
    return 0;
}

/* a new id (never 0) */
s32 cse_new_id(void)
{
    s32 r;

    lock();
    r = D_09A0EB0C++;
    if (r == 0) {
        D_09A0EB0C = 2;
        r = 1;
    }
    unlock();
    return r;
}

/* the decode thread of an ATRAC3 player */
s32 atrac_thread(u32 args, u8 **argp)
{
    u8 *p = *argp;
    s16 *s;
    s32 r, i, n, c0, c1;
    u32 pan;
    f32 a, c, m, sn;

    while (U32(p, AT_WAKE) == 0) {
        atrac_sema_wait(&D_09A0D258);
        r = atrac_decode(p);
        atrac_sema_signal(&D_09A0D258);
        if (r < 0) {
            sceKernelDelayThread(D_089B24D4 - 2000);
        } else {
            audio_set_len(U32(p, AT_CHANNEL), U32(p, AT_OUT_LEN));
            if ((D_09A0DB44 & 2) != 0) {
                f32 f[4], k[4];

                s = (s16 *)U32(p, AT_BUF + U32(p, AT_BUF_SEL) * 4);
                c0 = 0;
                c1 = 0;
                for (i = 0; i < 4; i++) {
                    f[i] = F32(p, AT_LIM_STATE1 + i * 4);
                    k[i] = F32(p, AT_LIM_STATE2 + i * 4);
                }
                n = U32(p, AT_OUT_LEN) << 1;
                for (i = 0; i < n; i += 4, s += 4) {
                    limit4(s, F32(p, AT_LIM_GAIN), f, k, F32(p, AT_LIM_D), F32(p, AT_LIM_C), F32(p, AT_LIM_F),
                           F32(p, AT_LIM_G), &c0, &c1);
                    if ((i & 0x1FF) == 0) {
                        limit_gain(k, c0, F32(p, AT_LIM_E), F32(p, AT_LIM_B), F32(p, AT_LIM_A), 0.3f,
                                   0.025f);
                        limit_gain(k + 1, c1, F32(p, AT_LIM_E), F32(p, AT_LIM_B), F32(p, AT_LIM_A), 0.3f,
                                   0.025f);
                        c0 = 0;
                        c1 = 0;
                    }
                }
                for (i = 0; i < 4; i++) {
                    F32(p, AT_LIM_STATE1 + i * 4) = f[i];
                    F32(p, AT_LIM_STATE2 + i * 4) = k[i];
                }
            }
            pan = U32(p, AT_PAN);
            a = ((f32)pan / 127.0f) * D_089B24D0;
            c = cosf(a);
            m = (f32)D_09A0DA70;
            sn = sinf(a);
            audio_output(U32(p, AT_CHANNEL), (s32)((m * (f32)U32(p, AT_VOLUME_OUT) * c) / 127.0f),
                         (s32)(((f32)D_09A0DA70 * (f32)U32(p, AT_VOLUME_OUT) * sn) / 127.0f),
                         (void *)U32(p, AT_BUF + U32(p, AT_BUF_SEL) * 4));
            U32(p, AT_BUF_SEL) = (U32(p, AT_BUF_SEL) + 1) & 1;
        }
        sceKernelDelayThread(1000);
        sceKernelDelayThreadCB(1000);
    }
    while (audio_rest(U32(p, AT_CHANNEL)) > 0) {
        sceKernelDelayThread(1000);
        sceKernelDelayThreadCB(1000);
    }
    sceKernelExitThread(0);
    return 0;
}
