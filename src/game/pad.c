/* Controller: buttons, analog stick (angle, strength, 16 directions) and the global button
 * masks (held, pressed, released, repeated). (Guessed names.) */
#include "menu.h"

typedef struct Pad {
    u16 buttons;        /* 0x00 */
    s16 x, y;           /* 0x02 stick, -127..127 */
    s16 strength;       /* 0x06 0..127 */
    s16 deg;            /* 0x08 angle of the stick in degrees */
    u16 pad0a;
    f32 angle;          /* 0x0C angle in radians (0..2pi) */
    u32 prev;           /* 0x10 previous held mask */
    u8 repeat;          /* 0x14 repeat counter */
    u8 hold;            /* 0x15 HOLD switch */
    u8 home;            /* 0x16 HOME button */
    u8 disabled;        /* 0x17 */
} Pad;

typedef struct {
    u32 time, buttons;
    u8 lx, ly;
    u8 rsrv[6];
} CtrlData;

extern u32 D_08A62C68;          /* held */
extern u32 D_08A62C6C;          /* pressed */
extern u32 D_08A62C70;          /* released */
extern u32 D_08A62C74;          /* pressed + auto repeat */
extern s16 D_08A62C78;          /* stick angle (0x10000 = full turn) */
extern s16 D_08A62C7A;          /* stick strength */
extern const u16 D_089A607C[];  /* direction bits for each 22.5 degrees */

int sceCtrlSetSamplingCycle(int cycle);
int sceCtrlSetSamplingMode(int mode);
int sceCtrlReadBufferPositive(CtrlData *d, int n);

void pad_init(Pad *p)
{
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(1);
    p->prev = 0;
    p->repeat = 0;
    p->hold = 0;
    p->home = 0;
    p->disabled = 0;
}

/* atan2 (the original uses a VFPU sequence with vasin/vrcp; here a polynomial, error ~1e-6) */
static f32 atan2_approx(f32 y, f32 x)
{
    f32 ax = x < 0 ? -x : x, ay = y < 0 ? -y : y;
    f32 t, t2, r;

    if (ax == 0.0f && ay == 0.0f)
        return 0.0f;
    t = ax > ay ? ay / ax : ax / ay;
    t2 = t * t;
    r = t * (0.99997726f + t2 * (-0.33262347f + t2 * (0.19354346f + t2 * (-0.11643287f +
            t2 * (0.05265332f + t2 * -0.01172120f)))));
    if (ay > ax)
        r = 3.14159265f / 2 - r;
    if (x < 0)
        r = 3.14159265f - r;
    return y < 0 ? -r : r;
}

void pad_read(Pad *p)
{
    CtrlData d;
    int n = sceCtrlReadBufferPositive(&d, 1);
    s32 ax, ay, t, mn;
    s16 s;
    u32 dir, held, old;
    f32 a;

    if (n > 0) {
        p->home = (d.buttons & 0x10000) != 0;
        p->hold = (d.buttons & 0x20000) != 0;
    }
    if (p->disabled == 1 || p->hold == 1 || n <= 0) {
        memset(p, 0, 0x10);
    } else {
        p->buttons = d.buttons;
        p->x = d.lx - 0x80;
        if (p->x < -0x7F)
            p->x = -0x7F;
        p->y = d.ly - 0x80;
        if (p->y < -0x7F)
            p->y = -0x7F;
    }
    if (p->x == 0 && p->y == 0) {
        a = 0.0f;
    } else {
        a = atan2_approx((f32)-p->y, (f32)p->x);
        if (a < 0.0f)
            a += 6.2831855f;
    }
    p->angle = a;
    p->deg = (s32)((p->angle / 6.2831855f) * 360.0f);

    /* strength: approximation of sqrt(x*x + y*y) */
    ax = p->x < 0 ? -p->x : p->x;
    ay = p->y < 0 ? -p->y : p->y;
    if (ax < ay) {
        t = ax;
        ax = ay;
        ay = t;
    }
    mn = ay + (ay >> 1);
    s = (s16)((mn >> 6) + (mn >> 2) + (ax - (ax >> 5) - (ax >> 7)));
    p->strength = s;
    if (s >= 0x80)
        p->strength = 0x7F;

    if (d.lx < 0x40 || d.lx > 0xC0 || d.ly < 0x40 || d.ly > 0xC0) {
        dir = D_089A607C[(u32)((f32)p->deg / 22.5f)];
    } else {
        /* dead zone */
        p->x = 0;
        p->y = 0;
        p->angle = 0.0f;
        p->deg = 0;
        p->strength = 0;
        dir = 0;
    }
    old = D_08A62C68;
    p->prev = old;
    held = p->buttons | (dir & 0xFFFF) << 16;
    D_08A62C68 = held;
    D_08A62C74 = 0;
    D_08A62C70 = ~held & old;
    D_08A62C6C = ~old & held;
    if (old == held) {
        p->repeat++;
        if (p->repeat >= 13) {
            p->repeat = 10;
            D_08A62C74 = D_08A62C68;
        }
    } else {
        D_08A62C74 = D_08A62C6C;
        p->repeat = 0;
    }
    D_08A62C78 = ((s32)p->deg << 16) / 360;
    D_08A62C7A = p->strength;
}
