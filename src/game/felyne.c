/* Felyne cooks / comrades: creation of a new Felyne, best records, log entries.
 * (Guessed names.) */
#include "menu.h"

extern const u8 D_09CD01E0[];     /* Felyne levels (6 bytes each) */
u32 game_rand(void *a, int b);
u32 func_08857E6C(void *g);
int game_test_flag(u8 *g, s32 i);
void log_push(u8 *g, u8 *e);
void *memcpy(void *d, const void *s, size_t n);
int sceWlanGetEtherAddr(u8 *mac);
int sceRtcGetCurrentClockLocalTime(void *t);

/* level of a Felyne from its experience: 0, 1, 2 */
s32 felyne_level(void *g)
{
    u32 e = func_08857E6C(g);

    if (e < 0x97)
        return 0;
    if (e < 0x12D)
        return 1;
    return 2;
}

/* rank of a value: 0 below 1000, then 2500, 4500, 6000, 7000 */
s32 value_rank(void *g, u32 v_arg)
{
    u16 v = v_arg;

    if (v < 1000)
        return 0;
    if (v < 2500)
        return 1;
    if (v < 4500)
        return 2;
    if (v < 6000)
        return 3;
    if (v < 7000)
        return 4;
    return 5;
}

/* fills the data of a new Felyne e (mode 1: a default one, else: random of level e[3]) */
void felyne_create(u8 *g, u8 *e, s32 mode)
{
    u8 mac[8];
    u16 t[8];
    u16 cnt;

    if (mode == 1) {
        s32 v;

        e[0x16] = 0;
        U16(e, 0x26) = 0;
        if ((s8)e[3] >= 8)
            U16(e, 0x2A) = 0x1194;
        else if ((s8)e[3] >= 4)
            U16(e, 0x2A) = 0xFA0;
        else
            U16(e, 0x2A) = 0xDAC;
        U32(e, 0x18) = 0x41200000;
        U32(e, 0x1C) = 0x41200000;
        e[0x29] = 0xB;
        e[0x28] = 100;
        v = e[1] + (s8)e[2] + (s8)e[6];
        e[0x4B] = v % 16;
        e[0x2C] = ((e[1] + (s8)e[2]) & 1) ? 1 : 0;
    } else {
        const u8 *l;

        e[0x16] = (s8)e[3] - 1;
        if (e[0x16] == 0)
            U16(e, 0x26) = 0;
        else
            U16(e, 0x26) = *(const u16 *)(D_09CD01E0 + (u8)(e[0x16] - 1) * 6);
        U16(e, 0x2A) = 0xDAC;
        l = D_09CD01E0 + e[0x16] * 6;
        F32(e, 0x18) = (f32)(u16)(l[2] + 10);
        l = D_09CD01E0 + e[0x16] * 6;
        F32(e, 0x1C) = (f32)(u16)(l[3] + 10);
        e[0x29] = D_09CD01E0[e[0x16] * 6 + 5] + 0xB;
        e[0x28] = D_09CD01E0[e[0x16] * 6 + 4] + 100;
        e[0x4B] = (u16)game_rand(g_sys, 1) & 0xF;
        e[0x2C] = ((u16)game_rand(g_sys, 1) & 1) ? 0 : 1;
        if ((u8)game_test_flag(g_game, 0x5D) == 1)
            U32(e, 0x30) = 0xF;
    }
    e[0x4C] = 0;
    e[0x4D] = 0;
    e[0x44] = 0xFF;
    e[0x45] = 0xFF;
    e[0x46] = 0xFF;
    memcpy(e + 0x60, g + 0x679A0, 8);
    sceWlanGetEtherAddr(mac);
    sceRtcGetCurrentClockLocalTime(t);
    e[0x68] = mac[3];
    e[0x69] = mac[4];
    e[0x6A] = mac[5];
    e[0x6B] = t[0];
    e[0x6C] = ((t[1] & 0xF) << 4) + ((t[2] & 0x1E) >> 1);
    e[0x6D] = ((t[2] & 1) << 7) + (t[4] & 0x1F);
    cnt = U16(g, 0x6AD76);
    e[0x6E] = (t[5] & 0x3F) + ((cnt & 0x300) >> 2);
    e[0x6F] = U16(g, 0x6AD76);
    U16(g, 0x6AD76) = U16(g, 0x6AD76) + 1;
    memcpy(e + 0x4E, g_game + 0x4A0, 0x12);
}

/* class of a record kind: 0 (< 3), 1 (< 0x11), 2 (< 0x13), 3 */
static u8 rec_class(u32 b)
{
    if ((s32)b < 3)
        return 0;
    if ((s32)b < 0x11)
        return 1;
    if ((s32)b < 0x13)
        return 2;
    return 3;
}

/* keeps the 3 most important entries (kind, a, b) of r, sorted by class */
void record_add_best(void *g, u8 *r, s32 kind, u32 a, u32 b)
{
    u8 cls = rec_class(kind);
    u8 best = 0x1E, idx = 0;
    u8 i, j;

    for (i = 0; i < 3; i++) {
        u8 k = r[8 + i];

        if (k == 0) {
            idx = i;
            best = 0;
            break;
        }
        if (!(best < rec_class(k))) {
            best = rec_class(k);
            idx = i;
        }
    }
    if (cls < best)
        return;
    r[8 + idx] = kind;
    U32(r, 0xC + idx * 4) = a;
    U32(r, 0x18 + idx * 4) = b;
    for (i = 0; i < 2; i++) {
        for (j = i + 1; j < 3; j++) {
            u8 ki = r[8 + i], kj = r[8 + j];

            if (rec_class(ki) < rec_class(kj)) {
                u32 ta = U32(r, 0xC + i * 4), tb = U32(r, 0x18 + i * 4);

                r[8 + i] = kj;
                U32(r, 0xC + i * 4) = U32(r, 0xC + j * 4);
                U32(r, 0x18 + i * 4) = U32(r, 0x18 + j * 4);
                r[8 + j] = ki;
                U32(r, 0xC + j * 4) = ta;
                U32(r, 0x18 + j * 4) = tb;
            }
        }
    }
}

/* adds an entry (a, b) with the current date to the log */
void log_add(u8 *g, u32 a, u32 b)
{
    u8 e[0x88];
    u16 t[8];

    memset(e, 0, 0x88);
    U16(e, 4) = a;
    U16(e, 6) = b;
    sceRtcGetCurrentClockLocalTime(t);
    U16(e, 2) = t[0];
    e[1] = t[1];
    e[0] = t[2];
    log_push(g, e);
}

void log_entry_init(void *g, u8 *e, u32 a, u32 b)
{
    u16 t[8];

    U16(e, 4) = a;
    U16(e, 6) = b;
    sceRtcGetCurrentClockLocalTime(t);
    U16(e, 2) = t[0];
    e[1] = t[1];
    e[0] = t[2];
}
