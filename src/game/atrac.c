/* The Capcom Sound Engine (4): the ATRAC3 stream players of the music (4 players of 0x204
 * bytes at 0x09A0D260, each with its decode thread and audio channel) and the start of the
 * sceSas render thread. Player: 0 used, 4 thread, 8 atrac id, 0x0C/0x10/0x14 decode results,
 * 0x24 channel, 0x28 buffer index, 0x2C mode, 0x44 loop, 0x54 state (0 idle, 2 set, 3 playing,
 * 4 ending, 5 released, 6 error), 0x6C/0x70 volume, 0x98 the 2 buffers. (Guessed names.) */
#include "common.h"
#include "cse.h"

#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define F32(p, off) (*(f32 *)((u8 *)(p) + (off)))

extern s32 D_09A0D254;            /* priority of the decode threads */
extern void (*D_09A0D250)(u8 *p);  /* error callback */
extern s32 D_09A0D258, D_09A0D25C;   /* semaphores */
extern u8 D_09A0D260[4 * 0x204];
extern s32 D_09A0DA70;            /* master volume of the music */
extern s32 D_09A0DA74, D_09A0DA78, D_09A0DA7C, D_09A0DA80, D_09A0DA88;
extern u8 D_09A0DB44;
extern u32 D_09A0DA8C;
extern const f32 D_089B23EC[20];  /* the parameters of the limiter */
extern const f32 D_089B24E0[7], D_089B2510;
extern const char D_089B243C[], D_089B2450[], D_089B2458[], D_089B2460[], D_089B2480[];
extern const char D_089B2498[], D_089B24B4[], D_089B2530[], D_089B2544[], D_089B254C[];
extern const char D_089B2554[], D_089B2570[];

int printf(const char *fmt, ...);
void *memset(void *p, int c, u32 n);
int sceKernelCreateSema(const char *name, u32 attr, s32 init, s32 max, void *opt);
int sceKernelSignalSema(int id, int n);
int sceKernelWaitSema(int id, int n, u32 *timeout);
int sceKernelCreateThread(const char *name, void *entry, int prio, int stack, u32 attr, void *opt);
int sceKernelStartThread(int thid, u32 arglen, void *argp);
int sceKernelDelayThread(u32 us);
int sceAtracReleaseAtracID(s32 id);
int sceAtracSetLoopNum(s32 id, s32 n);
int sceAtracSetDataAndGetID(void *buf, u32 size);
int sceAtracGetRemainFrame(s32 id, s32 *n);
int sceAtracGetSoundSample(s32 id, s32 *end, s32 *ls, s32 *le);
int sceAtracGetBitrate(s32 id, s32 *rate);
int sceAtracDecodeData(s32 id, void *out, s32 *n, s32 *end, s32 *remain);
int sceAtracGetNextDecodePosition(s32 id, u32 *pos);
int sceAtracGetStreamDataInfo(s32 id, void *a, void *b, void *c);
int sceAtracGetLoopStatus(s32 id, s32 *a, s32 *b);
s32 audio_set_voice_volume(u32 v, u32 l, u32 r);
s32 audio_set_len(u32 ch, u32 n);
s32 audio_set_volume(u32 ch, u32 l, u32 r);
s32 audio_output(u32 ch, u32 l, u32 r, void *buf);
s32 audio_set_format(u32 ch, s32 f);
s32 audio_rest(u32 ch);
void atrac_thread(void);
void sas_render_thread(void);
void sas_audio_open(void);
void sas_open(void);
void sas_limiter_defaults(u8 *p);
void sas_limiter_set(u8 *p);

void atrac_sema_create(s32 *s);
void atrac_sema_wait(s32 *s);
void atrac_sema_signal(s32 *s);
void atrac_players_clear(void);
s32 atrac_set_volume_all(s32 v);
u8 *atrac_player_create(s32 i, s32 ch, u8 *buf, s32 size);
s32 atrac_set_data(u8 *p, void *buf, u32 size);
s32 atrac_start(u8 *p, void *buf, u32 size);
void atrac_stop(u8 *p);
void atrac_error(u8 *p);
void atrac_release(u8 *p);
void atrac_loop_on(u8 *p);
void atrac_loop_off(u8 *p);
void atrac_limiter_defaults(f32 *p);
void atrac_limiter_set(f32 *p, u8 *pl);

#define SEMA_ATRAC (&D_09A0D25C)
#define SEMA_PLAYERS (&D_09A0D258)

void atrac_sema_create(s32 *s)
{
    *s = sceKernelCreateSema(D_089B243C, 0x100, 1, 1, NULL);
}

void atrac_sema_wait(s32 *s)
{
    sceKernelWaitSema(*s, 1, NULL);
}

void atrac_sema_signal(s32 *s)
{
    sceKernelSignalSema(*s, 1);
}

void atrac_release(u8 *p)
{
    if ((s32)U32(p, AT_ID) == -1)
        return;
    sceAtracReleaseAtracID(U32(p, AT_ID));
    U32(p, AT_ID) = -1;
}

void atrac_loop_on(u8 *p)
{
    sceAtracSetLoopNum(U32(p, AT_ID), 0x7FFFFFFF);
    U32(p, AT_LOOP_ON) = 1;
    U32(p, AT_LOOP_N) = 0;
}

void atrac_loop_off(u8 *p)
{
    sceAtracSetLoopNum(U32(p, AT_ID), 0);
    U32(p, AT_LOOP_ON) = 0;
    U32(p, AT_LOOP_N) = 0;
}

void atrac_init(s32 prio)
{
    atrac_sema_create(SEMA_ATRAC);
    atrac_sema_create(SEMA_PLAYERS);
    atrac_sema_wait(SEMA_PLAYERS);
    atrac_players_clear();
    atrac_set_volume_all(0x7F);
    sas_audio_open();
    atrac_sema_signal(SEMA_PLAYERS);
    D_09A0D254 = prio;
}

void atrac_players_clear(void)
{
    memset(D_09A0D260, 0, 0x810);
    D_09A0D250 = NULL;
}

s32 atrac_set_volume_all(s32 v)
{
    s32 old = D_09A0DA70;

    if (v < 0)
        v = 0;
    if (v > 0x7F)
        v = 0x7F;
    D_09A0DA70 = v;
    return old;
}

u8 *atrac_open(s32 i, s32 ch, u8 *buf, s32 size)
{
    u8 *p;

    atrac_sema_wait(SEMA_PLAYERS);
    p = atrac_player_create(i, ch, buf, size);
    atrac_sema_signal(SEMA_PLAYERS);
    return p;
}

/* player i on the audio channel ch, with the 2 PCM buffers in buf (size bytes) */
u8 *atrac_player_create(s32 i, s32 ch, u8 *buf, s32 size)
{
    u8 *p = D_09A0D260 + i * 0x204;
    u8 *q;
    f32 lim[21];
    s32 th, r, k;

    if (U32(p, AT_USED) == 1) {
        printf(D_089B2450);
        printf(D_089B2458);
        printf(D_089B2460);
        return p;
    }
    th = sceKernelCreateThread(D_089B2480, atrac_thread, D_09A0D254, 0x8000, 0, NULL);
    if (th < 0) {
        printf(D_089B2450);
        printf(D_089B2458);
        printf(D_089B2498, th);
        return NULL;
    }
    q = p;
    memset(q, 0, 0x204);
    U32(q, 4) = th;
    U32(q, 8) = -1;
    for (k = 3; k <= 7; k++)
        U32(q, k * 4) = 0;
    U32(q, 0x20) = i;
    U32(q, 0x24) = ch;
    for (k = 0xA; k <= 0x1A; k++)
        U32(q, k * 4) = 0;
    U32(q, 0x6C) = 0x7F;
    U32(q, 0x70) = (u32)(U32(q, 0x6C) * 0x8000) / 0x7F;
    U32(q, 0x74) = 0x40;
    U32(q, 0x78) = 0;
    U32(q, 0x90) = 0;
    U32(q, 0x94) = size / 2;
    U32(q, 0x80) = U32(q, 0x94) >> 2;
    U32(q, 0x7C) = U32(q, 0x80) - 0x40;
    U32(q, 0x84) = 0;
    U32(q, 0x88) = 0;
    U32(q, 0x8C) = 0;
    U32(q, 0x98) = (u32)buf;
    U32(q, 0x9C) = (u32)(buf + size / 2);
    U32(q, 0xA0) = 0;
    memset((void *)U32(q, 0x98), 0, U32(q, 0x94));
    memset((void *)U32(q, 0x9C), 0, U32(q, 0x94));
    U32(q, 0xA4) = 0;
    U32(q, 0xA8) = 0;
    U32(q, 0xAC) = 0;
    memset(q + 0xB0, 0, 0x80);
    for (k = 0x4C; k <= 0x53; k++)
        U32(q, k * 4) = 0;
    atrac_limiter_defaults(lim);
    for (k = 0; k < 14; k++)
        lim[k] = D_089B23EC[k];
    lim[14] = D_089B23EC[13];
    for (k = 15; k < 21; k++)
        lim[k] = D_089B23EC[k - 1];
    atrac_limiter_set(lim, q);
    audio_set_volume(U32(q, 0x24), U32(q, 0x70), U32(q, 0x70));
    audio_set_format(U32(q, 0x24), 0);
    audio_set_len(U32(q, 0x24), 0x40);
    r = sceKernelStartThread(U32(q, 4), 4, &q);
    if (r == 0) {
        U32(q, 0) = 1;
        return q;
    }
    printf(D_089B2450);
    printf(D_089B2458);
    printf(D_089B24B4, r);
    return NULL;
}

/* the ATRAC3 data of the stream */
s32 atrac_set_data(u8 *p, void *buf, u32 size)
{
    s32 n, r;

    U32(p, AT_DATA) = (u32)buf;
    U32(p, AT_SIZE) = size;
    atrac_sema_wait(SEMA_ATRAC);
    U32(p, AT_ID) = sceAtracSetDataAndGetID((void *)U32(p, AT_DATA), U32(p, AT_SIZE));
    U32(p, AT_STATE) = 1;
    atrac_sema_signal(SEMA_ATRAC);
    if ((s32)U32(p, AT_ID) < 0)
        return -1;
    sceAtracGetRemainFrame(U32(p, AT_ID), (s32 *)(p + 0x14));
    n = U32(p, AT_LEFT);
    if (n == -1 || n == -2 || n == -3) {
        U32(p, AT_X130) = 0;
    } else {
        U32(p, AT_X130) = n / 2;
        if (n / 2 < 1)
            U32(p, AT_X130) = 1;
    }
    U32(p, AT_X130 + 4) = 0;
    U32(p, AT_X130 + 8) = 0;
    U32(p, AT_X130 + 0xC) = 0;
    sceAtracGetSoundSample(U32(p, AT_ID), (s32 *)(p + 0x30), (s32 *)(p + 0x34), (s32 *)(p + 0x38));
    sceAtracGetBitrate(U32(p, AT_ID), (s32 *)(p + 0x50));
    r = U32(p, AT_X50);
    switch (r) {
    case 0x160: case 0x140: case 0x100: case 0xC0: case 0xA0: case 0x80: case 0x60: case 0x40:
    case 0x30: case 0x20:
        U32(p, AT_DATA_SET) = 1;
        break;
    case 0x84: case 0x69: case 0x42: case 0x34:
        U32(p, AT_DATA_SET) = 0;
        break;
    }
    U32(p, AT_LOOP) = ((s32)U32(p, AT_LOOP_START) == -1 && (s32)U32(p, AT_LOOP_END) == -1) ^ 1;
    U32(p, AT_LOOP_ON) = 0;
    U32(p, AT_X3C) = 0;
    U32(p, AT_X40) = 0;
    U32(p, AT_STATE) = 2;
    return 0;
}

s32 atrac_play(u8 *p, void *buf, u32 size)
{
    s32 r;

    atrac_sema_wait(SEMA_PLAYERS);
    r = atrac_start(p, buf, size);
    U32(p, AT_MODE) = 1;
    U32(p, AT_DECODED) = 0;
    U32(p, AT_END) = 0;
    U32(p, AT_LEFT) = 0;
    atrac_sema_signal(SEMA_PLAYERS);
    return r;
}

static void set_error(u8 *p, u32 code, u32 why)
{
    U32(p, AT_ERR_CODE) = code;
    U32(p, AT_ERR_WHY) = why;
    U32(p, AT_STATE) = 6;
    atrac_error(p);
}

s32 atrac_start(u8 *p, void *buf, u32 size)
{
    switch (U32(p, AT_STATE)) {
    case 1: case 3: case 4:
        atrac_stop(p);
        /* fallthrough */
    case 0: case 5:
        if (atrac_set_data(p, buf, size) < 0) {
            set_error(p, U32(p, AT_ID), 3);
            return -1;
        }
        /* fallthrough */
    case 2:
        atrac_sema_wait(SEMA_ATRAC);
        if (U32(p, AT_LOOP) == 1)
            atrac_loop_on(p);
        else
            atrac_loop_off(p);
        atrac_sema_signal(SEMA_ATRAC);
        U32(p, AT_X84) = 0;
        U32(p, AT_X8C) = 0;
        U32(p, AT_STATE) = 3;
        return 0;
    default:
        set_error(p, U32(p, AT_STATE), 1);
        return -1;
    }
}

void atrac_close(u8 *p)
{
    atrac_sema_wait(SEMA_PLAYERS);
    atrac_stop(p);
    atrac_sema_signal(SEMA_PLAYERS);
}

void atrac_stop(u8 *p)
{
    void *b;

    switch (U32(p, AT_STATE)) {
    case 3:
        audio_set_voice_volume(U32(p, AT_CHANNEL), 0, 0);
        b = (void *)U32(p, AT_BUF + U32(p, AT_BUF_SEL) * 4);
        memset(b, 0, U32(p, AT_BUF_SIZE));
        audio_set_len(U32(p, AT_CHANNEL), 0x40);
        audio_output(U32(p, AT_CHANNEL), 0, 0, (void *)U32(p, AT_BUF + U32(p, AT_BUF_SEL) * 4));
        audio_output(U32(p, AT_CHANNEL), 0, 0, NULL);
        /* fallthrough */
    case 1: case 2: case 4:
        atrac_release(p);
        U32(p, AT_STATE) = 5;
        /* fallthrough */
    case 0: case 5:
        U32(p, AT_STATE) = 0;
        break;
    default:
        set_error(p, U32(p, AT_STATE), 1);
        break;
    }
}

void atrac_set_pause(u8 *p, s32 on)
{
    U32(p, AT_PAUSED) = on != 0;
}

u32 atrac_state(u8 *p)
{
    u32 s;

    atrac_sema_wait(SEMA_PLAYERS);
    s = U32(p, AT_STATE);
    atrac_sema_signal(SEMA_PLAYERS);
    return s;
}

void atrac_set_volume(u8 *p, s32 v)
{
    s32 x = v;

    if (v < 0)
        x = 0;
    if (v > 0x7F)
        x = 0x7F;
    U32(p, AT_VOLUME) = x;
    U32(p, AT_VOLUME_OUT) = (u32)(x * 0x8000) / 0x7F;
}

void atrac_clear_error(u8 *p)
{
    if (U32(p, AT_STATE) != 6)
        return;
    atrac_release(p);
    U32(p, AT_STATE) = 0;
    U32(p, AT_ERR_WHY) = 0;
    U32(p, AT_ERR_CODE) = 0;
}

void atrac_set_error_cb(void (*cb)(u8 *p))
{
    D_09A0D250 = cb;
}

void atrac_error(u8 *p)
{
    if (D_09A0D250 != NULL)
        D_09A0D250(p);
}

/* the next queued data {buffer, size} (16 entries from 0xB0) */
u32 atrac_queue_pop(u8 *p, u32 *out)
{
    u32 n = U32(p, AT_QUEUE_N) != 0;

    if (n) {
        out[0] = U32(p, AT_QUEUE + U32(p, AT_QUEUE_POS) * 8);
        out[1] = U32(p, 0xB4 + U32(p, AT_QUEUE_POS) * 8);
        U32(p, AT_QUEUE_POS) = (U32(p, AT_QUEUE_POS) + 1) & 0xF;
        U32(p, AT_QUEUE_N)--;
    }
    return n;
}

/* decodes the next part of the stream in the current buffer; -1 when nothing to play */
s32 atrac_decode(u8 *p)
{
    u32 st[2];
    s32 r = 0, dr, lr, n, i;
    u16 *src, *dst;
    s16 *s;

    switch (U32(p, AT_STATE)) {
    case 3:
        if (U32(p, AT_PAUSED) == 1) {
            U32(p, AT_OUT_LEN) = U32(p, AT_SAMPLES) & ~0x3F;
            U32(p, AT_X8C) = U32(p, AT_SAMPLES) & 0x3F;
            return -1;
        }
        atrac_sema_wait(SEMA_ATRAC);
        dr = sceAtracDecodeData(U32(p, AT_ID), (u8 *)U32(p, AT_BUF + U32(p, AT_BUF_SEL) * 4) +
                                U32(p, AT_X8C) * 4, (s32 *)(p + 0xC), (s32 *)(p + 0x10),
                                (s32 *)(p + 0x14));
        sceAtracGetNextDecodePosition(U32(p, AT_ID), (u32 *)(p + 0x18));
        U32(p, AT_OUT_LEN) = (U32(p, AT_DECODED) + U32(p, AT_X8C)) & ~0x3F;
        U32(p, AT_X8C) = (U32(p, AT_DECODED) + U32(p, AT_X8C)) & 0x3F;
        n = U32(p, AT_LEFT);
        if (n == -1 || n == -2 || (s32)U32(p, AT_X130) < n) {
            U32(p, AT_X130 + 4) = 0;
            U32(p, AT_X130 + 8) = 0;
            U32(p, AT_X130 + 0xC) = 0;
        } else {
            sceAtracGetStreamDataInfo(U32(p, AT_ID), p + 0x134, p + 0x138, p + 0x13C);
        }
        /* the rest after the last 64 samples goes to the start of the other buffer */
        src = (u16 *)(U32(p, AT_BUF + U32(p, AT_BUF_SEL) * 4) + U32(p, AT_OUT_LEN) * 4);
        dst = (u16 *)U32(p, AT_BUF + ((U32(p, AT_BUF_SEL) + 1) & 1) * 4);
        n = U32(p, AT_X8C);
        for (i = 0; i < n; i++, src += 2, dst += 2) {
            dst[0] = src[0];
            dst[1] = src[1];
        }
        if (U32(p, AT_OUT_LEN) == 0) {
            U32(p, AT_OUT_LEN) = 0x40;
            U32(p, AT_X8C) = 0;
        }
        lr = sceAtracGetLoopStatus(U32(p, AT_ID), (s32 *)(p + 0x3C), (s32 *)(p + 0x40));
        if (U32(p, AT_LOOP) == 1) {
            if (U32(p, AT_LOOP_ON) == 1)
                sceAtracSetLoopNum(U32(p, AT_ID), 0x7FFFFFFF);
            else
                sceAtracSetLoopNum(U32(p, AT_ID), 0);
        }
        atrac_sema_signal(SEMA_ATRAC);
        if (dr < 0) {
            set_error(p, U32(p, AT_STATE), 4);
            return -1;
        }
        if (lr < 0) {
            set_error(p, U32(p, AT_STATE), 5);
            return -1;
        }
        U32(p, AT_X84) += U32(p, AT_DECODED);
        if (U32(p, AT_LOOP) == 1 && U32(p, AT_X84) >= U32(p, AT_LOOP_END))
            U32(p, AT_LOOP_N) = (U32(p, AT_X84) - U32(p, AT_LOOP_START)) / (U32(p, AT_LOOP_END) - U32(p, AT_LOOP_START));
        else
            U32(p, AT_LOOP_N) = 0;
        if (U32(p, AT_END) != 0) {
            if (U32(p, AT_MODE) == 2) {
                if (atrac_queue_pop(p, st) == 0) {
                    U32(p, AT_STATE) = 4;
                } else {
                    atrac_release(p);
                    U32(p, AT_STATE) = 5;
                    r = atrac_set_data(p, (void *)st[0], st[1]);
                    if (r < 0) {
                        set_error(p, U32(p, AT_ID), 3);
                        return r;
                    }
                    atrac_loop_off(p);
                    U32(p, AT_STATE) = 3;
                }
            } else if (U32(p, AT_MODE) == 3 || U32(p, AT_MODE) == 1) {
                U32(p, AT_STATE) = 4;
            }
        }
        if ((U32(p, AT_FLAGS) & 0x80) != 0) {
            s = (s16 *)U32(p, AT_BUF + U32(p, AT_BUF_SEL) * 4);
            for (i = 0; (u32)i < U32(p, AT_DECODED); i++, s += 2)
                *s = -*s;
        }
        return r;
    case 4:
        audio_set_voice_volume(U32(p, AT_CHANNEL), 0, 0);
        atrac_release(p);
        U32(p, AT_STATE) = 5;
        /* fallthrough */
    case 0: case 1: case 2: case 5:
        U32(p, AT_OUT_LEN) = U32(p, AT_SAMPLES) & ~0x3F;
        U32(p, AT_X8C) = U32(p, AT_SAMPLES) & 0x3F;
        return -1;
    }
    return 0;
}

/* the limiter parameters (FLT_MIN: keep the old value) */
void atrac_limiter_set(f32 *v, u8 *p)
{
    s32 i;

    for (i = 0; i < 0x15; i++)
        if (v[i] != 1.1754944e-38f)
            F32(p, AT_LIMITER + i * 4) = v[i];
    U32(p, AT_LIM_STATE1 + 4) = 0;
    U32(p, AT_LIM_STATE1) = 0;
    U32(p, AT_LIM_STATE2 + 4) = 0;
    U32(p, AT_LIM_STATE2) = 0;
    U32(p, AT_LIM_STATE3) = 0;
    U32(p, AT_LIM_STATE3 + 4) = 0;
}

void atrac_limiter_defaults(f32 *p)
{
    s32 i;

    for (i = 0; i < 0x15; i++)
        ((u32 *)p)[i] = 0x800000;
}

/* starts the sceSas render thread on the audio channel ch (0-2) */
s32 sas_render_start(s32 prio, s32 ch)
{
    u8 b[0xB4];
    s32 r, i;

    D_09A0DB44 = 0;
    D_09A0DA8C = 0;
    sas_limiter_defaults(b);
    for (i = 0; i < 7; i++)
        F32(b, 4 + i * 4) = D_089B24E0[i];
    F32(b, 0x34) = D_089B2510;
    F32(b, 0x38) = D_089B2510;
    sas_limiter_set(b);
    if (ch < 0 || ch >= 3)
        return -1;
    D_09A0DA80 = sceKernelCreateThread(D_089B2530, sas_render_thread, prio, 0x1000, 0, NULL);
    if (D_09A0DA80 < 0) {
        printf(D_089B2544);
        printf(D_089B254C);
        printf(D_089B2554, D_09A0DA80);
        return -1;
    }
    D_09A0DA74 = ch;
    sas_audio_open();
    sas_open();
    audio_set_len(D_09A0DA74, 0x100);
    audio_set_volume(D_09A0DA74, 0x8000, 0x8000);
    audio_set_format(D_09A0DA74, 0);
    r = sceKernelStartThread(D_09A0DA80, 0, NULL);
    if (r != 0) {
        printf(D_089B2544);
        printf(D_089B254C);
        printf(D_089B2570, r);
        return -1;
    }
    D_09A0DA78 = 0;
    D_09A0DA7C = -1;
    return D_09A0DA80;
}

/* pause / resume of the sound output (nested) */
void sas_render_pause(void)
{
    if (++D_09A0DA88 == 1)
        while (audio_rest(0) > 0)
            sceKernelDelayThread(1000);
}

void sas_render_resume(void)
{
    if (D_09A0DA88 == 1) {
        audio_set_len(D_09A0DA74, 0x100);
        audio_set_volume(D_09A0DA74, 0x8000, 0x8000);
        audio_set_format(D_09A0DA74, 0);
    }
    D_09A0DA88--;
}
