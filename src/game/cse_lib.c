/* The Capcom Sound Engine (4): the game side of the library. The sound banks (chunks
 * "Head", "Prog", "Smpl", "Vagi"), the queue of the requests to the driver (the SIF RPC of
 * the PS2 version) and the functions that build the commands of the sound effects. */
#include "common.h"

#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define S16(p, off) (*(s16 *)((u8 *)(p) + (off)))
#define U16(p, off) (*(u16 *)((u8 *)(p) + (off)))
#define S8(p, off) (*(s8 *)((u8 *)(p) + (off)))

#define MAGIC(s) ((s)[0] | ((s)[1] << 8) | ((s)[2] << 16) | ((s)[3] << 24))

#define RPCQ ((u8 *)0x09A0EB80)   /* 64 requests of 0x48 bytes: {kind, data[0x40], size} */

extern u32 D_09A0EB10[16];        /* the loaded banks */
extern s32 D_09A0EB68;            /* semaphore of the queue */
extern s32 D_09A0EB6C;            /* requests in the queue */
extern s32 D_09A0EB70, D_09A0EB74; /* read and write index */
extern s32 D_09A0EB50;            /* requests of kind 2 in the queue */
extern u8 D_09A0EB54[0x14];       /* the last status */
extern s32 D_09A0B564, D_09A0B568;
extern u32 D_09A0FD84[16];        /* the tables of the sound effects */
extern void (*D_09A0FD80)(u16 *req, u8 *e);
extern const s8 D_089B265C[], D_089B2664[], D_089B266C[], D_089B2674[];
extern const char D_089B267C[], D_089B2684[], D_089B268C[], D_089B2694[];
extern const char D_089B2634[], D_089B263C[], D_089B2644[];
extern const char D_089B269C[], D_089B26A4[], D_089B26AC[], D_089B26C0[], D_089B26D4[],
    D_089B26E8[];
extern const char D_089B26FC[], D_089B2710[], D_089B2718[], D_089B2720[], D_089B2748[];
extern const char D_089B2788[], D_089B2790[], D_089B2798[], D_089B27D4[], D_089B27E8[];

int printf(const char *fmt, ...);
void *memset(void *d, int c, u32 n);
void *memcpy(void *d, const void *s, u32 n);
int strncmp(const char *a, const char *b, u32 n);
int sceKernelCpuSuspendIntr(void);
void sceKernelCpuResumeIntr(int s);
s32 sceKernelCreateSema(const char *name, u32 attr, s32 init, s32 max, void *opt);
s32 sceKernelWaitSema(s32 id, s32 n, u32 *timeout);
s32 sceKernelPollSema(s32 id, s32 n);
s32 sceKernelSignalSema(s32 id, s32 n);
u32 sound_map_load(u8 *map);
u32 sound_map_select(u32 i);
u32 stub_088a4be8(u32 i);         /* sound_map_offset */
u8 *sas_request(s32 kind, const void *data, u32 size);
s32 cse_new_id(void);

void rpcq_reset(void);
u32 rpcq_send(s32 kind, u8 *data, u32 size);
u32 se_note(u8 *e, u16 *req, u32 note);
u32 se_split_play(u8 *req, u32 note);
u32 se_send_change(u32 *req, s32 cmd);
u32 se_send_play(u32 *params, u32 *req);
u32 se_release(u32 *req);
u32 se_stop(u32 *req);
s32 se_check(u16 *req);

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

/* ---- the banks ---- */

s32 bank_init(u8 *map)
{
    s32 r = sound_map_load(map);
    s32 i;

    if (r < 0)
        return r;
    for (i = 0; i < 16; i++)
        D_09A0EB10[i] = 0;
    return 0;
}

s32 bank_select(u32 i)
{
    s32 r = sound_map_select(i);

    if (r >= 0)
        r = 0;
    return r;
}

u32 bank_set(u32 i, u32 bank)
{
    if (i < 16) {
        D_09A0EB10[i] = bank;
        return 0;
    }
    printf(D_089B2634);
    printf(D_089B263C);
    printf(D_089B2644);
    return 0x80000001;
}

u32 bank_get(u32 i)
{
    if (i < 16)
        return D_09A0EB10[i];
    printf(D_089B2634);
    printf(D_089B263C);
    printf(D_089B2644);
    return 0;
}

static u32 pad16(u8 *chunk, u32 size)
{
    while ((size & 0xF) != 0)
        chunk[size++] = 0xFF;
    return size;
}

/* joins n banks in one: the indexes of the samples and of the VAG data of each bank are
 * moved after the ones of the banks before it */
void bank_merge(u32 *out, u8 **banks, u32 n)
{
    u32 i, j, k, max, size, off, cnt, dataoff;
    u8 *chunk, *b, *src, *p, *d;
    u16 base;

    out[0] = MAGIC(D_089B265C);
    out[1] = 0x20;
    out[2] = 2;
    out[3] = 0;
    out[4] = 0;
    out[5] = 0;
    out[6] = 0;
    out[7] = 0;
    for (i = 0; i < n; i++)
        out[4] += U32(banks[i], 0x10);
    out[5] = 0x20;

    /* the programs */
    chunk = (u8 *)(out + 8);
    U32(chunk, 0) = MAGIC(D_089B2664);
    U32(chunk, 4) = 0;
    U32(chunk, 8) = 0;
    U32(chunk, 0xC) = -1;
    max = 0;
    for (i = 0; i < n; i++) {
        b = banks[i];
        if (max < U32(b + U32(b, 0x14), 8))
            max = U32(b + U32(b, 0x14), 8);
    }
    U32(chunk, 8) = max;
    for (i = 0; i < max + 1; i++)
        U32(chunk, 0x10 + i * 4) = -1;
    size = (max + 1) * 4 + 0x10;
    base = 0;
    for (i = 0; i < n; i++) {
        b = banks[i];
        src = b + U32(b, 0x14);
        for (j = 0; j < U32(src, 8) + 1; j++) {
            if (U32(src, 0x10 + j * 4) == (u32)-1)
                continue;
            p = src + U32(src, 0x10 + j * 4);
            if (U32(chunk, 0x10 + j * 4) != (u32)-1)
                return;
            U32(chunk, 0x10 + j * 4) = size;
            d = chunk + size;
            size += p[0] * 0xE + 8;
            S16(d, 0) = S16(p, 0);
            S16(d, 2) = S16(p, 2);
            S16(d, 4) = S16(p, 4);
            S16(d, 6) = S16(p, 6);
            for (k = 0; k < d[0]; k++) {
                u32 h;

                for (h = 0; h < 7; h++)
                    S16(d, 8 + k * 0xE + h * 2) = S16(p, 8 + k * 0xE + h * 2);
                U16(d, 8 + k * 0xE + 0xC) += base;
            }
        }
        base += (u16)(U32(b + U32(b, 0x18), 8) + 1);
    }
    size = pad16(chunk, size);
    U32(chunk, 4) = size;
    off = out[5] + size;
    out[6] = off;

    /* the samples */
    chunk = (u8 *)out + off;
    U32(chunk, 0) = MAGIC(D_089B266C);
    U32(chunk, 4) = 0;
    U32(chunk, 8) = 0;
    U32(chunk, 0xC) = -1;
    size = 0x10;
    base = 0;
    for (i = 0; i < n; i++) {
        b = banks[i];
        src = b + U32(b, 0x18);
        d = chunk + size;
        U32(chunk, 8) += U32(src, 8) + 1;
        cnt = 0;
        for (j = 0; j < U32(src, 8) + 1; j++) {
            for (k = 0; k < 6; k++)
                S16(d, k * 2) = S16(src, 0x10 + j * 0xC + k * 2);
            U16(d, 0xA) += base;
            cnt = U32(src, 8) + 1;
            d += 0xC;
        }
        base += (u16)(U32(b + U32(b, 0x1C), 8) + 1);
        size += cnt * 0xC;
    }
    U32(chunk, 8) -= 1;
    size = pad16(chunk, size);
    U32(chunk, 4) = size;
    off += size;
    out[7] = off;

    /* the VAG data */
    chunk = (u8 *)out + off;
    U32(chunk, 0) = MAGIC(D_089B2674);
    U32(chunk, 4) = 0;
    U32(chunk, 8) = 0;
    U32(chunk, 0xC) = -1;
    size = 0x10;
    dataoff = 0;
    for (i = 0; i < n; i++) {
        b = banks[i];
        src = b + U32(b, 0x1C);
        d = chunk + size;
        U32(chunk, 8) += U32(src, 8) + 1;
        cnt = 0;
        for (j = 0; j < U32(src, 8) + 1; j++) {
            for (k = 0; k < 5; k++)
                U32(d, k * 4) = U32(src, 0x10 + j * 0x14 + k * 4);
            U32(d, 0) += dataoff;
            cnt = U32(src, 8) + 1;
            d += 0x14;
        }
        dataoff += U32(b, 0x10);
        size += cnt * 0x14;
    }
    U32(chunk, 8) -= 1;
    size = pad16(chunk, size);
    U32(chunk, 4) = size;
    out[3] = off + size;
}

s32 bank_is_head(const char *p)
{
    return strncmp(p, D_089B267C, 4) == 0;
}

s32 bank_is_prog(const char *p)
{
    return strncmp(p, D_089B2684, 4) == 0;
}

s32 bank_is_smpl(const char *p)
{
    return strncmp(p, D_089B268C, 4) == 0;
}

s32 bank_is_vagi(const char *p)
{
    return strncmp(p, D_089B2694, 4) == 0;
}

static void bank_error(const char *msg)
{
    printf(D_089B269C);
    printf(D_089B26A4);
    printf(msg);
}

/* the number of splits (key ranges) of a program */
u32 bank_prog_splits(u8 *bank, u32 prog)
{
    s32 o;

    if (bank_is_head((char *)bank) != 1) {
        bank_error(D_089B26AC);
        return 0x80000006;
    }
    bank += U32(bank, 0x14);
    if (bank_is_prog((char *)bank) != 1) {
        bank_error(D_089B26C0);
        return 0x80000007;
    }
    if (U32(bank, 8) < (prog & 0xFF))
        return 0x8000000E;
    o = U32(bank, 0x10 + (prog & 0xFF) * 4);
    if (o == -1)
        return 0x8000000E;
    return bank[o];
}

/* finds the program, split, sample and VAG data of a note */
u32 bank_find_split(u8 **out, u8 *bank, u32 prog, u32 key, u32 split)
{
    u8 *p, *s, *v, *e;
    u16 vi;

    if (U32(bank, 8) != 2)
        return -1;
    p = bank + U32(bank, 0x14);
    s = bank + U32(bank, 0x18);
    v = bank + U32(bank, 0x1C);
    if (bank_is_head((char *)bank) != 1) {
        bank_error(D_089B26AC);
        return 0x80000006;
    }
    if (bank_is_prog((char *)p) != 1) {
        bank_error(D_089B26C0);
        return 0x80000007;
    }
    if (bank_is_smpl((char *)s) != 1) {
        bank_error(D_089B26D4);
        return 0x80000008;
    }
    if (bank_is_vagi((char *)v) != 1) {
        bank_error(D_089B26E8);
        return 0x80000009;
    }
    key &= 0xFF;
    p += U32(p, 0x10 + (prog & 0xFF) * 4);
    e = p + (split & 0xFF) * 0xE;
    s += U16(e, 0x14) * 0xC;
    vi = U16(s, 0x1A);
    if ((s32)key < e[0xA] || e[0xB] < (s32)key)
        return 0x8000000F;
    out[0] = p;
    out[1] = e + 8;
    out[2] = s + 0x10;
    out[3] = v + vi * 0x14 + 0x10;
    return 0;
}

static s16 clamp_pan(s32 v)
{
    s16 p = v;

    if (p < -0x40)
        return -0x40;
    if (p > 0x3F)
        return 0x3F;
    return p;
}

/* the parameters of a voice from the program, split, sample and VAG entries */
u32 bank_note_params(u8 *o, u8 **e, u32 key, s32 dataoff)
{
    s16 pan, t;

    o[0] = S8(e[0], 2);
    o[0] = (s32)(o[0] * S8(e[1], 8)) / 0x7F;
    o[0] = (s32)(o[0] * S8(e[2], 6)) / 0x7F;
    pan = clamp_pan((s16)(S8(e[0], 3) - 0x40));
    pan = clamp_pan(pan + (s16)(S8(e[1], 9) - 0x40));
    pan = clamp_pan(pan + (s16)(S8(e[2], 7) - 0x40));
    o[1] = pan + 0x40;
    t = S8(e[0], 5) + S8(e[0], 4) * 100;
    S16(o, 2) = t;
    t = t + (s16)(S8(e[1], 0xB) + S8(e[1], 0xA) * 100);
    S16(o, 2) = t;
    t = t + (s16)(S8(e[2], 9) + S8(e[2], 8) * 100);
    S16(o, 2) = t;
    S16(o, 2) = t + (s16)(((key & 0xFF) - e[2][5]) * 100);
    U16(o, 4) = U16(e[1], 4);
    U16(o, 6) = U16(e[1], 6);
    U16(o, 8) = U16(e[2], 0);
    U16(o, 0xA) = U16(e[2], 2);
    U32(o, 0x10) = dataoff + U32(e[3], 0);
    U32(o, 0xC) = U32(e[3], 0xC);
    U32(o, 0x14) = U32(e[3], 4);
    o[0x18] = e[3][0x10];
    return 0;
}

u32 bank_data_size(u8 *bank)
{
    if (bank_is_head((char *)bank) != 1)
        return 0x80000006;
    return U32(bank, 0x10);
}

/* ---- the queue of the requests ---- */

u32 rpcq_init(void)
{
    D_09A0EB68 = sceKernelCreateSema(D_089B26FC, 0x100, 1, 1, NULL);
    rpcq_reset();
    return 0;
}

void rpcq_reset(void)
{
    sceKernelWaitSema(D_09A0EB68, 1, NULL);
    lock();
    memset(RPCQ, 0, 0x1200);
    D_09A0EB74 = 0;
    D_09A0EB70 = 0;
    D_09A0EB6C = 0;
    D_09A0EB50 = 0;
    unlock();
    sceKernelSignalSema(D_09A0EB68, 1);
}

static void rpcq_full_msg(void)
{
    printf(D_089B2710);
    printf(D_089B2718);
    printf(D_089B2720);
}

u32 rpcq_check(void)
{
    if (D_09A0EB6C == 0x40) {
        rpcq_full_msg();
        return 0x80000010;
    }
    return 0;
}

/* puts a request in the queue; gives it an id if it has none (word 1) and returns it */
u32 rpcq_send(s32 kind, u8 *data, u32 size)
{
    u8 *q = NULL, *e;
    s32 id = 0;

    sceKernelWaitSema(D_09A0EB68, 1, NULL);
    lock();
    if ((s32)rpcq_check() >= 0) {
        q = data;
        if (U32(data, 4) == 0) {
            id = cse_new_id();
            U32(data, 4) = id;
        }
        e = RPCQ + D_09A0EB74 * 0x48;
        U32(e, 0) = kind;
        U32(e, 0x44) = size;
        memset(e + 4, 0, 0x40);
        memcpy(RPCQ + D_09A0EB74 * 0x48 + 4, data, size);
        D_09A0EB74 = (D_09A0EB74 + 1) & 0x3F;
        D_09A0EB6C++;
        if (kind == 2)
            D_09A0EB50++;
    }
    unlock();
    if (id == 0)
        rpcq_full_msg();
    sceKernelSignalSema(D_09A0EB68, 1);
    if (q == NULL)
        return 0;
    return U32(q, 4);
}

/* sends the requests of the queue to the driver; returns the ones left */
s32 rpcq_flush(void)
{
    s32 n, r;
    u8 *e;

    if (sceKernelPollSema(D_09A0EB68, 1) != 0)
        return 0;
    for (n = D_09A0EB6C; n != 0; n--) {
        lock();
        e = RPCQ + D_09A0EB70 * 0x48;
        r = *(s32 *)sas_request(U32(e, 0), e + 4, U32(e, 0x44));
        unlock();
        if (r < 0) {
            printf(D_089B2710);
            printf(D_089B2718);
            printf(D_089B2748);
            break;
        }
        D_09A0EB6C--;
        if (U32(RPCQ + D_09A0EB70 * 0x48, 0) == 2)
            D_09A0EB50--;
        D_09A0EB70 = (D_09A0EB70 + 1) & 0x3F;
    }
    sceKernelSignalSema(D_09A0EB68, 1);
    return D_09A0EB6C;
}

u8 *rpc_call_309(s32 kind, const void *data, u32 size)
{
    if (kind == 0x309)
        return sas_request(kind, data, size);
    return NULL;
}

u8 *rpc_status(s32 kind, const void *data, u32 size)
{
    u8 *r;

    if (kind != 100)
        return NULL;
    lock();
    r = sas_request(100, data, size);
    memcpy(D_09A0EB54, r, 0x14);
    unlock();
    return D_09A0EB54;
}

/* ---- the tables of the sound effects: entries of 0x20 bytes ---- */

u32 setbl_init(void)
{
    u32 i;

    for (i = 0; i < 16; i++)
        D_09A0FD84[i] = 0;
    D_09A0FD80 = NULL;
    return 0;
}

u32 setbl_nop(void)
{
    return 0;
}

u32 setbl_set_cb(void (*cb)(u16 *req, u8 *e))
{
    D_09A0FD80 = cb;
    return 0;
}

u32 setbl_set(s32 i, u32 tbl)
{
    D_09A0FD84[i] = tbl;
    return 0;
}

u8 *setbl_entry(s32 i, u32 n)
{
    u8 *t = (u8 *)D_09A0FD84[i];

    if (t == NULL)
        return NULL;
    if (n < U32(t, 0xC))
        return t + n * 0x20 + 0x10;
    return NULL;
}

/* the entry plays a note */
u32 se_note(u8 *e, u16 *req, u32 note)
{
    u16 f = U16(e, 4);

    if ((f & 2) == 0)
        req[0] = f | 4;
    else
        req[0] = f & ~4;
    if (D_09A0FD80 != NULL)
        D_09A0FD80(req, e);
    return se_split_play((u8 *)req, note & 0xFFFF);
}

static u8 clamp_u8(s32 v)
{
    if (v < 0)
        return 0;
    if (v < 0x100)
        return v;
    return 0xFF;
}

/* plays the chain of the entries from (tbl, n) with the parameters p (mask p[0], bit 31:
 * the parameters replace the ones of the entries instead of being added) */
u32 se_request(u32 tbl, u32 n, u32 *p)
{
    u8 r[0x1C];
    u32 ret, i, bit, m, b, over;
    u16 cur;
    s32 v;
    u8 *e;

    for (i = 0; i < 0x1C; i++)
        r[i] = 0;
    ret = 0;
    cur = n;
    for (;;) {
        e = setbl_entry(tbl & 0xFFFF, cur);
        if (e == NULL)
            return 0;
        U16(r, 0) = U16(e, 4);
        r[4] = tbl;
        r[5] = e[7] + p[2];
        r[6] = e[0xA] + p[3];
        r[7] = e[0xB] + p[4];
        r[3] = e[9] + p[7];
        S16(r, 8) = e[0xC] + p[8];
        S16(r, 0xA) = e[0xD] + p[9];
        S16(r, 0xC) = S16(e, 0xE) + p[10];
        U32(r, 0x14) = U32(e, 0x10) + p[5];
        r[0x10] = e[0x14] + p[6];
        r[0x13] = r[0x13] + p[0xC];
        r[2] = e[8] | p[0xD];
        v = (u8)U16(e, 0x18) + p[0xE];
        r[0x11] = clamp_u8(v);
        over = (p[0] & 0x80000000) != 0;
        for (m = p[0] & 0x7FFFFFFF, bit = 1; m != 0; m &= ~bit, bit <<= 1) {
            b = m & bit;
            if (b == 0x2000) {
                if (over)
                    v = p[0xE];
                r[0x11] = clamp_u8(v);
            } else if (b == 0x1000) {
                if (over)
                    r[2] = p[0xD];
            } else if (b == 0x800) {
                if (over)
                    r[0x13] = p[0xC];
            } else if (b == 0x20) {
                if (over)
                    r[0x10] = p[6];
            } else if (b == 0x10) {
                if (over)
                    U32(r, 0x14) = p[5];
            } else if (b == 0x200) {
                if (over)
                    S16(r, 0xC) = p[10];
            } else if (b == 0x100) {
                if (over)
                    S16(r, 0xA) = p[9];
            } else if (b == 0x80) {
                if (over)
                    S16(r, 8) = p[8];
            } else if (b == 0x40) {
                if (over)
                    r[3] = p[7];
            } else if (b == 8) {
                if (over)
                    r[7] = p[4];
            } else if (b == 4) {
                if (over)
                    r[6] = p[3];
            } else if (b == 2) {
                if (over)
                    r[5] = p[2];
            }
        }
        switch (e[0]) {
        case 1:
            ret = se_note(e, (u16 *)r, e[6] + p[1]);
            break;
        case 2:
            ret = se_release((u32 *)r);
            break;
        case 3:
            ret = se_stop((u32 *)r);
            break;
        }
        if (U16(e, 0x16) == 0xFFFF || U16(e, 0x16) == cur)
            return ret;
        cur = U16(e, 0x16);
    }
}

u32 se_rpc_309(u32 x)
{
    u32 q[3];

    q[0] = 0;
    q[1] = 0;
    q[2] = x;
    return *(u32 *)rpc_call_309(0x309, q, 0xC);
}

void se_set_mono(u32 on)
{
    u32 q[3];

    q[1] = 0;
    q[2] = 0;
    q[0] = 0x10000009;
    ((u8 *)q)[8] = on & 1;
    rpcq_send(1, (u8 *)q, 0xC);
}

void se_chan_volume(u32 chan, u32 vol, u32 time)
{
    u32 q[5];

    q[1] = 0;
    q[2] = 0;
    q[4] = 0;
    q[0] = 0x1000000B;
    if (chan != 0xFF)
        chan &= 0xF;
    ((u8 *)q)[0x10] = vol & 0x7F;
    ((u8 *)q)[8] = chan;
    q[3] = time;
    rpcq_send(1, (u8 *)q, 0x14);
}

/* plays all the splits of a program that contain the note */
u32 se_split_play(u8 *req, u32 note)
{
    u8 params[0x1C];
    u8 *found[4];
    u32 bank, ret = 0;
    s32 n, i;

    for (i = 0; i < 4; i++)
        found[i] = NULL;
    bank = bank_get(req[4]);
    n = bank_prog_splits((u8 *)bank, note & 0xFF);
    for (i = 0; i < n; i++) {
        if ((s32)bank_find_split(found, (u8 *)bank, note & 0xFF, req[5], i & 0xFF) < 0)
            continue;
        bank_note_params(params, found, req[5], stub_088a4be8(req[4]));
        ret = se_send_play((u32 *)params, (u32 *)req);
    }
    return ret;
}

u32 se_stop(u32 *req)
{
    return se_send_change(req, 0x10000002);
}

u32 se_release(u32 *req)
{
    return se_send_change(req, 0x10000001);
}

static void clear_req(u8 *r)
{
    u32 i;

    for (i = 0; i < 0x1C; i++)
        r[i] = 0;
}

void se_stop_all(void)
{
    u32 r[7];

    clear_req((u8 *)r);
    se_send_change(r, 0x10000002);
}

void se_release_all(void)
{
    u32 r[7];

    clear_req((u8 *)r);
    se_send_change(r, 0x10000001);
}

void se_stop_chan(u32 chan)
{
    u32 r[7];

    clear_req((u8 *)r);
    U16(r, 0) = 4;
    ((u8 *)r)[4] = chan & 0xF;
    se_send_change(r, 0x10000002);
}

void se_release_chan(u32 chan)
{
    u32 r[7];

    clear_req((u8 *)r);
    U16(r, 0) = 4;
    ((u8 *)r)[4] = chan & 0xF;
    se_send_change(r, 0x10000001);
}

void se_stop_id(u32 id)
{
    u32 r[7];

    clear_req((u8 *)r);
    U16(r, 0) = 0x80;
    r[6] = id;
    se_stop(r);
}

void se_release_id(u32 id)
{
    u32 r[7];

    clear_req((u8 *)r);
    U16(r, 0) = 0x80;
    r[6] = id;
    se_release(r);
}

/* a request that matches the voices by channel and ids (-1 = any) */
static void match_req(u8 *r, s32 chan, s32 id1, s32 id2)
{
    clear_req(r);
    if (chan == -1) {
        U16(r, 0) |= 2;
    } else {
        r[4] = chan;
        U16(r, 0) |= 4;
    }
    if (id1 != -1) {
        r[6] = id1;
        U16(r, 0) |= 0x20;
    }
    if (id2 != -1) {
        r[7] = id2;
        U16(r, 0) |= 0x40;
    }
}

void se_stop_match(s32 chan, s32 id1, s32 id2)
{
    u32 r[7];

    match_req((u8 *)r, chan, id1, id2);
    se_stop(r);
}

void se_release_match(s32 chan, s32 id1, s32 id2)
{
    u32 r[7];

    match_req((u8 *)r, chan, id1, id2);
    se_release(r);
}

/* the changes of a request from the parameters p (mask p[0]) */
void se_req_changes(u16 *r, u32 *p)
{
    if ((p[0] & 0x80) != 0) {
        r[0] |= 0x100;
        r[4] = p[8];
    }
    if ((p[0] & 0x100) != 0) {
        r[0] |= 0x200;
        r[5] = p[9];
    }
    if ((p[0] & 0x200) != 0) {
        r[0] |= 0x400;
        r[6] = p[10];
    }
    if ((p[0] & 0x1000) != 0) {
        r[0] |= 0x200;
        ((u8 *)r)[2] = p[0xD];
    }
    if ((p[0] & 0x400) != 0) {
        r[0] |= 0x800;
        r[7] = p[0xB];
    }
    if ((p[0] & 0x40) != 0) {
        r[0] |= 0x4000;
        ((u8 *)r)[3] = p[7];
    }
    if ((p[0] & 0x800) != 0) {
        r[0] |= 0x1000;
        ((u8 *)r)[0x13] = p[0xC];
    }
    if ((p[0] & 0x2000) != 0) {
        r[0] |= 0x2000;
        ((u8 *)r)[0x11] = p[0xE];
    }
}

void se_change_match(s32 chan, s32 id1, s32 id2, u32 *p)
{
    u32 r[7];

    match_req((u8 *)r, chan, id1, id2);
    se_req_changes((u16 *)r, p);
    se_send_change(r, 0x10000003);
}

void se_send_7(u32 *p)
{
    u32 q[8];

    q[1] = 0;
    q[0] = 0x10000007;
    q[2] = p[0];
    q[3] = p[1];
    q[4] = p[2];
    q[5] = p[3];
    q[6] = p[4];
    q[7] = p[5];
    rpcq_send(1, (u8 *)q, 0x20);
}

/* the number of voices that match the request */
u32 se_count(u32 *req)
{
    u32 q[9];
    s32 r;

    q[1] = 0;
    q[0] = 0x20000002;
    q[2] = req[0];
    q[3] = req[1];
    q[4] = req[2];
    q[5] = req[3];
    q[6] = req[4];
    q[7] = req[5];
    q[8] = req[6];
    r = se_check((u16 *)(q + 2));
    if (r < 0)
        return r;
    return rpc_status(100, q, 0x24)[0x12];
}

s32 se_check(u16 *req)
{
    u16 f;

    if ((req[0] & 1) == 0) {
        req[0] |= 1;
        ((u8 *)req)[3] = 0x7F;
    } else {
        ((u8 *)req)[3] &= 0x7F;
    }
    f = req[0];
    if ((f & 0x20) != 0) {
        if ((f & 0xE) == 0) {
            printf(D_089B2788);
            printf(D_089B2790);
            printf(D_089B2798);
            return 0x80000012;
        }
    } else if ((f & 0x40) != 0) {
        printf(D_089B2788);
        printf(D_089B2790);
        printf(D_089B27D4);
        return 0x80000013;
    }
    if ((f & 0x10) != 0)
        ((u8 *)req)[5] &= 0x7F;
    return 0;
}

u32 se_send_change(u32 *req, s32 cmd)
{
    u32 q[9];

    q[1] = 0;
    if (cmd != 0x10000001 && cmd != 0x10000002 && cmd != 0x10000003) {
        printf(D_089B2788);
        printf(D_089B2790);
        printf(D_089B27E8);
        return 0;
    }
    q[2] = req[0];
    q[3] = req[1];
    q[4] = req[2];
    q[5] = req[3];
    q[6] = req[4];
    q[7] = req[5];
    q[8] = req[6];
    q[0] = cmd;
    if (se_check((u16 *)(q + 2)) < 0)
        return 0;
    return rpcq_send(1, (u8 *)q, 0x24);
}

u32 se_send_play(u32 *params, u32 *req)
{
    u32 q[16];
    u32 i;

    for (i = 0; i < 16; i++)
        q[i] = 0;
    q[1] = 0;
    q[0] = 0x10000000;
    for (i = 0; i < 7; i++)
        q[9 + i] = params[i];
    for (i = 0; i < 7; i++)
        q[2 + i] = req[i];
    return rpcq_send(1, (u8 *)q, 0x40);
}
