/* The Capcom Sound Engine (CSE) for PSP (1): start, semaphores, the timer of the driver, the
 * queue of the commands (RPC) and the list of the 32 voices. (Guessed names; the texts
 * printed by the engine name some of them.) */
#include "common.h"
#include "cse.h"

#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define U16(p, off) (*(u16 *)((u8 *)(p) + (off)))

extern u32 D_09A0B560;            /* sound data */
extern s32 D_09A0B564, D_09A0B568;
extern s32 D_09A0B56C;            /* frame counter, then 16 words */
extern s32 D_089B1F8C;            /* output mode */
extern const char D_089B1F44[], D_089B1F4C[], D_089B1F54[], D_089B1F88[];
extern const char D_089B1F90[], D_089B1F98[], D_089B1FA0[];
extern const char D_089B20A0[], D_089B20B0[], D_089B20B8[], D_089B20C0[];
extern const char D_089B20DC[], D_089B20E4[], D_089B20EC[];
extern const char D_089B2110[], D_089B2118[], D_089B2120[], D_089B2148[];
extern u32 D_089B1FF0[11][4];     /* semaphores {name, init, max, id} */
extern s32 D_09A0B5B0, D_09A0B5B4, D_09A0B5B8;   /* the timer {id, period, count} */
extern s32 D_09A0B5BC, D_09A0B5C0, D_09A0B5C4, D_09A0B5C8;   /* queue {in, out, n, max} */
extern u8 *D_09A0B5CC;
extern u8 D_09A0B5D0[0x1000];
extern u8 D_09A0C5D8[], D_09A0C5F8[], D_09A0C608[], D_09A0C618[], D_09A0C628[];
extern u8 D_09A0C638[], D_09A0C828[], D_09A0C838[];
extern u8 *D_09A0C5D0, *D_09A0C5D4;

int printf(const char *fmt, ...);
void *memset(void *p, int c, u32 n);
void *memcpy(void *d, const void *s, u32 n);
int sceKernelCreateSema(const char *name, u32 attr, s32 init, s32 max, void *opt);
int sceKernelSignalSema(int id, int n);
int sceKernelWaitSema(int id, int n, u32 *timeout);
int sceKernelCreateVTimer(const char *name, void *opt);
int sceKernelSetVTimerHandlerWide(int uid, u32 unused, u32 lo, u32 hi, void *h, void *common);
int sceKernelStartVTimer(int uid);
int sceKernelSuspendDispatchThread(void);
int sceKernelResumeDispatchThread(int s);
int sceKernelCpuSuspendIntr(void);
void sceKernelCpuResumeIntr(int s);
void sas_driver_start(u32 a);
void sas_driver_set(void *cb);
void sas_render_start(u32 a, u32 b);
void stub_0889ee40(int a);   /* -> se_set_mono */
void func_088A464C(u32 a);
void rpcq_init(void);
void cse_ids_init(void);
void se_rpc_309(u32 a);
void setbl_init(void);
void setbl_nop(void);
void rpcq_flush(void);
s32 se_count(void *p);
void se_request(u32 a, u32 b, u32 *p);
void sasv_init(void);
u32 sasv_get(s32 i);
u32 cse_set_data(u32 a);
void cse_count_status(u32 *out, s32 *in);
u32 cse_status(u32 *out);
void cse_driver(void);

s32 cse_create_semas(void);
s32 cse_create_timer(void);
u32 cse_start_timer(void);
u32 cse_rpc_cmd(u32 a, s32 *cmd, u32 *out);
u32 cse_rpc_stat(u32 a, s32 *cmd, u32 *out);
u32 cse_init(u32 a);
u32 cse_timer_handler(int uid, u32 a, u32 b, u8 *t);
void cse_sema_signal(s32 i);
void cse_cmd(u16 a, u16 b, u32 *p);
u8 *cse_voice(u32 i);

void cse_start(u32 data, u32 a, u32 b, u32 c)
{
    printf(D_089B1F44);
    printf(D_089B1F4C);
    printf(D_089B1F54, D_089B1F88, 1, 8);
    cse_create_semas();
    cse_create_timer();
    sas_driver_start(a);
    sas_driver_set(cse_driver);
    sas_render_start(b, c);
    D_09A0B560 = data;
}

/* the requests to the driver (SIF RPC of the PS2 engine) */
void cse_request(u32 x, s32 kind, u32 y, s32 *cmd, u32 z, u32 *out)
{
    if (kind == 0x65)
        return;
    if (kind == 100)
        cse_rpc_stat(100, cmd, out);
    else if (kind == 1)
        cse_rpc_cmd(1, cmd, out);
    else if (kind == 0x309) {
        *out = cse_set_data(D_09A0B560);
        cse_start_timer();
    }
}

void cse_core_start(void)
{
    printf(D_089B1F90);
    printf(D_089B1F98);
    printf(D_089B1FA0, 2, 0);
    cse_init(0);
    stub_0889ee40(0);
}

void cse_set_output(s32 m)
{
    if (m == 2) {
        D_089B1F8C = 2;
        stub_0889ee40(0);
    } else if (m == 1) {
        D_089B1F8C = 1;
        stub_0889ee40(0);
    } else if (m == 0) {
        D_089B1F8C = 0;
        stub_0889ee40(1);
    }
}

void cse_cmd16(u32 a, u32 b, u32 *p)
{
    cse_cmd(a & 0xFFFF, b & 0xFFFF, p);
}

/* a command for the voices: -1 = not set */
u8 cse_cmd_ok(void *p);

u8 cse_voice_cmd(s32 a, s32 b, s32 c)
{
    u8 p[0x1C];
    s32 i;

    for (i = 0; i < 0x1C; i++)
        p[i] = 0;
    if (a == -1) {
        U16(p, 0) |= 2;
    } else {
        p[4] = a;
        U16(p, 0) |= 4;
    }
    if (b != -1) {
        p[6] = b;
        U16(p, 0) |= 0x20;
    }
    if (c != -1) {
        p[7] = c;
        U16(p, 0) |= 0x40;
    }
    return cse_cmd_ok(p);
}

u32 cse_init(u32 a)
{
    s32 *p = &D_09A0B56C;
    u32 i;

    func_088A464C(a);
    rpcq_init();
    cse_ids_init();
    se_rpc_309(a);
    setbl_init();
    D_09A0B56C = 0;
    for (i = 0; i < 0x10; i++)
        p[i + 1] = -1;
    return 0;
}

u32 cse_frame(void)
{
    setbl_nop();
    rpcq_flush();
    D_09A0B56C++;
    return 0;
}

u8 cse_cmd_ok(void *p)
{
    s32 r = se_count(p);

    if (r < 0)
        return 0;
    return r != 0;
}

/* command with the fields of p (mask p[0]) */
void cse_cmd(u16 a, u16 b, u32 *p)
{
    u32 c[15];
    u32 m, bit, i;

    for (i = 0; i < 15; i++)
        c[i] = 0;
    c[0] = p[0];
    c[12] = 100;
    if (p != NULL) {
        for (m = c[0], bit = 1; m != 0; m &= ~bit, bit <<= 1) {
            switch (m & bit) {
            case 0x2000: c[14] = p[14]; break;
            case 0x1000: c[13] = p[13]; break;
            case 0x800: c[12] = p[12]; break;
            case 0x200: c[10] = p[10]; break;
            case 0x100: c[9] = p[9]; break;
            case 0x80: c[8] = p[8]; break;
            case 0x40: c[7] = p[7]; break;
            case 0x20: c[6] = p[6]; break;
            case 0x10: c[5] = p[5]; break;
            case 8: c[4] = p[4]; break;
            case 4: c[3] = p[3]; break;
            case 2: c[2] = p[2]; break;
            case 1: c[1] = p[1]; break;
            }
        }
    }
    se_request(a, b, c);
}

s32 cse_create_semas(void)
{
    u32 i;

    for (i = 0; i < 0xB; i++)
        if (D_089B1FF0[i][0] != 0)
            D_089B1FF0[i][3] = sceKernelCreateSema((const char *)D_089B1FF0[i][0], 0x100,
                                                  D_089B1FF0[i][1], D_089B1FF0[i][2], NULL);
    return 0;
}

void cse_sema_signal(s32 i)
{
    sceKernelSignalSema(D_089B1FF0[i][3], 1);
}

void cse_sema_wait(s32 i)
{
    sceKernelWaitSema(D_089B1FF0[i][3], 1, NULL);
}

s32 cse_create_timer(void)
{
    u8 opt[4];
    s32 i;

    for (i = 0; i < 4; i++)
        opt[i] = 0;
    D_09A0B5B0 = 0;
    D_09A0B5B4 = 0;
    D_09A0B5B8 = 0;
    D_09A0B5B0 = sceKernelCreateVTimer(D_089B20A0, opt);
    if (D_09A0B5B0 < 0) {
        printf(D_089B20B0);
        printf(D_089B20B8);
        printf(D_089B20C0, D_09A0B5B0);
        return 0x80000018;
    }
    D_09A0B5B4 = 0x411A;
    D_09A0B5B8 = 0;
    sceKernelSetVTimerHandlerWide(D_09A0B5B0, 0, 0, 0, cse_timer_handler, &D_09A0B5B0);
    return D_09A0B5B0;
}

u32 cse_start_timer(void)
{
    sceKernelStartVTimer(D_09A0B5B0);
    return 0;
}

/* every period: wakes the driver threads */
u32 cse_timer_handler(int uid, u32 a, u32 b, u8 *t)
{
    U32(t, 8)++;
    cse_sema_signal(2);
    cse_sema_signal(0);
    return U32(t, 4);
}

void cse_queue_init(void)
{
    memset(D_09A0B5D0, 0, 0x1000);
    D_09A0B5BC = 0;
    D_09A0B5C0 = 0;
    D_09A0B5C4 = 0;
    D_09A0B5C8 = 0x40;
    D_09A0B5CC = D_09A0B5D0;
}

/* a command of 64 bytes in the queue */
s32 cse_queue_put(s32 q, const void *cmd, u32 size)
{
    s32 d = sceKernelSuspendDispatchThread();
    s32 r;

    if (q != 0) {
        r = 0x80000019;
    } else if (D_09A0B5C4 == D_09A0B5C8) {
        printf(D_089B20DC);
        printf(D_089B20E4);
        printf(D_089B20EC);
        r = 0x8000001A;
    } else {
        memcpy(D_09A0B5CC + D_09A0B5BC * 0x40, cmd, size);
        D_09A0B5BC = (D_09A0B5BC + 1) % D_09A0B5C8;
        r = ++D_09A0B5C4;
    }
    sceKernelResumeDispatchThread(d);
    return r;
}

s32 cse_queue_get(s32 q, void *cmd)
{
    s32 d = sceKernelSuspendDispatchThread();
    s32 r;

    if (q != 0) {
        r = 0x80000019;
    } else if (D_09A0B5C4 == 0) {
        r = 0x8000001B;
    } else {
        memcpy(cmd, D_09A0B5CC + D_09A0B5C0 * 0x40, 0x40);
        D_09A0B5C0 = (D_09A0B5C0 + 1) % D_09A0B5C8;
        r = --D_09A0B5C4;
    }
    sceKernelResumeDispatchThread(d);
    return r;
}

/* ---- the 32 voices: 2 lists of nodes {id, voice, prev, next} (free 0x5E8, used 0x5D8) ---- */

#define NODE(i) (D_09A0C638 + (i) * 0x10)

u32 cse_voices_init(void)
{
    s32 i;
    u8 *n;

    for (i = 0; i < 0x20; i++) {
        n = NODE(i);
        U16(n, 0) = i;
        U32(n, 8) = (u32)(i == 0 ? D_09A0C618 : NODE(i - 1));
        U32(n, 0xC) = (u32)(i == 0x1F ? D_09A0C628 : NODE(i + 1));
        U32(n, 4) = (u32)cse_voice(i);
    }
    U16(D_09A0C618, 0) = 0xFFFF;
    U32(D_09A0C618, 8) = 0;
    U32(D_09A0C618, 0xC) = (u32)D_09A0C638;
    U32(D_09A0C618, 4) = 0;
    U16(D_09A0C628, 0) = 0xFFFF;
    U32(D_09A0C628, 8) = (u32)D_09A0C828;
    U32(D_09A0C628, 0xC) = 0;
    U32(D_09A0C628, 4) = 0;
    U16(D_09A0C5F8, 0) = 0xFFFF;
    U32(D_09A0C5F8, 8) = 0;
    U32(D_09A0C5F8, 0xC) = (u32)D_09A0C608;
    U32(D_09A0C5F8, 4) = 0;
    U16(D_09A0C608, 0) = 0xFFFF;
    U32(D_09A0C608, 8) = (u32)D_09A0C5F8;
    U32(D_09A0C608, 0xC) = 0;
    U32(D_09A0C608, 4) = 0;
    U32(D_09A0C5D8, 0x10) = 0;
    U32(D_09A0C5D8, 0x14) = 0x20;
    U32(D_09A0C5D8, 0x18) = (u32)D_09A0C618;
    U32(D_09A0C5D8, 0x1C) = (u32)D_09A0C628;
    U32(D_09A0C5D8, 0) = 1;
    U32(D_09A0C5D8, 4) = 0;
    U32(D_09A0C5D8, 8) = (u32)D_09A0C5F8;
    U32(D_09A0C5D8, 0xC) = (u32)D_09A0C608;
    return 0;
}

#define FREE_N U32(D_09A0C5D8, 0x14)
#define USED_N U32(D_09A0C5D8, 4)
#define FREE_HEAD ((u8 *)U32(D_09A0C5D8, 0x18))
#define FREE_TAIL ((u8 *)U32(D_09A0C5D8, 0x1C))
#define USED_HEAD ((u8 *)U32(D_09A0C5D8, 8))
#define USED_TAIL ((u8 *)U32(D_09A0C5D8, 0xC))

u32 cse_free_count(void)
{
    return FREE_N;
}

u32 cse_used_count(void)
{
    return USED_N;
}

u32 cse_free_first(void)
{
    if (FREE_N == 0)
        return 0;
    D_09A0C5D0 = (u8 *)U32(FREE_HEAD, 0xC);
    return U32(D_09A0C5D0, 4);
}

u32 cse_free_next(void)
{
    if (D_09A0C5D0 == NULL)
        return 0;
    D_09A0C5D0 = (u8 *)U32(D_09A0C5D0, 0xC);
    if (U32(D_09A0C5D0, 0xC) == 0) {
        D_09A0C5D0 = NULL;
        return 0;
    }
    return U32(D_09A0C5D0, 4);
}

u32 cse_used_first(void)
{
    if (USED_N == 0)
        return 0;
    D_09A0C5D4 = (u8 *)U32(USED_HEAD, 0xC);
    return U32(D_09A0C5D4, 4);
}

u32 cse_used_next(void)
{
    if (USED_N == 0)
        return 0;
    if (D_09A0C5D4 == NULL)
        return 0;
    D_09A0C5D4 = (u8 *)U32(D_09A0C5D4, 0xC);
    if (U32(D_09A0C5D4, 0xC) == 0) {
        D_09A0C5D4 = NULL;
        return 0;
    }
    return U32(D_09A0C5D4, 4);
}

/* takes the first free voice and puts it at the end of the used ones */
u32 cse_voice_take(void)
{
    u8 *n;

    if (FREE_N == 0)
        return 0;
    n = (u8 *)U32(FREE_HEAD, 0xC);
    U32(U32(n, 0xC), 8) = U32(n, 8);
    U32(U32(n, 8), 0xC) = U32(n, 0xC);
    FREE_N--;
    U32(n, 8) = U32(USED_TAIL, 8);
    U32(n, 0xC) = (u32)USED_TAIL;
    U32(U32(n, 8), 0xC) = (u32)n;
    U32(U32(n, 0xC), 8) = (u32)n;
    USED_N++;
    return U32(n, 4);
}

/* gives back the used voice v */
u32 cse_voice_give(u32 v)
{
    u8 *n, *next;

    if (USED_N == 0)
        return 0x8000001C;
    n = (u8 *)U32(USED_HEAD, 0xC);
    if (U32(n, 0xC) == 0)
        return 0x8000001C;
    next = (u8 *)U32(n, 0xC);
    while (U32(n, 4) != v) {
        if (U32(next, 0xC) == 0)
            return 0x8000001C;
        n = next;
        next = (u8 *)U32(n, 0xC);
    }
    U32(U32(n, 8), 0xC) = (u32)next;
    U32(U32(n, 0xC), 8) = U32(n, 8);
    USED_N--;
    U32(n, 8) = U32(FREE_TAIL, 8);
    U32(n, 0xC) = (u32)FREE_TAIL;
    U32(U32(n, 8), 0xC) = (u32)n;
    U32(U32(n, 0xC), 8) = (u32)n;
    FREE_N++;
    return 0;
}

/* the 32 voice records (0x48 bytes) */
u32 cse_voices_clear(void)
{
    u32 z[0x12];
    u8 *v = D_09A0C838;
    s32 i, j;

    for (i = 0; i < 0x48; i++)
        ((u8 *)z)[i] = 0;
    sasv_init();
    for (i = 0; i < 0x20; i++, v += 0x48) {
        for (j = 0; j < 0x12; j++)
            U32(v, j * 4) = z[j];
        U32(v, CV_STATE) = 0;
        U32(v, CV_SAS) = sasv_get(i);
    }
    return 0;
}

u8 *cse_voice(u32 i)
{
    return D_09A0C838 + (i & 0x1F) * 0x48;
}

/* commands of the sound (0x10000000-0x1000000D) go to the queue */
u32 cse_rpc_cmd(u32 a, s32 *cmd, u32 *out)
{
    s32 c = *cmd;
    u32 r = 0;

    if (c >= 0x10000000 && c <= 0x1000000D && c != 0x10000005) {
        if (D_09A0B564++ == 0)
            D_09A0B568 = sceKernelCpuSuspendIntr();
        r = cse_queue_put(0, cmd, 0x40);
        if (D_09A0B564 != 0 && --D_09A0B564 == 0)
            sceKernelCpuResumeIntr(D_09A0B568);
    } else if (c == 0x30000000) {
        *out = 0;
        return 0;
    } else if (c != 0) {
        printf(D_089B2110);
        printf(D_089B2118);
        printf(D_089B2120, *cmd);
    }
    *out = r;
    return r;
}

u32 cse_rpc_stat(u32 a, s32 *cmd, u32 *out)
{
    u32 r = 0;

    if (*cmd == 0x20000002) {
        cse_count_status(out, cmd + 2);
        *out = 0;
        return 0;
    }
    if (*cmd == 0x20000001) {
        r = cse_status(out);
    } else {
        printf(D_089B2110);
        printf(D_089B2118);
        printf(D_089B2148, *cmd);
    }
    *out = r;
    return r;
}
