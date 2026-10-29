/* Camera (2): scripted cameras of the events (script at 0xE0, commands of 4 bytes
 * {op, length / 4, a, b}) and the rails they move on. (Guessed names.) */
#include "menu.h"
#include "vfpu.h"

extern u8 *g_camera;
extern u8 *g_game;
extern const s32 D_08938234[];    /* camera scripts of the events */
extern u8 D_08A62DA0[];           /* 64 bytes per rail segment */

f32 func_08897D4C(f32 x, f32 y);
void func_0888E788(u8 *self, u8 *v);
void func_0888E8E8(u8 *self, u8 *v);
void cam_script_copy(u8 *self, s32 i);
void func_0888EAF8(u8 *self, u8 *cmd);
void spline_build(u8 *self, u8 *pts, u32 n);
s32 func_0888BC40(u8 *self, f32 *out, u8 *seg, u32 x, u32 k);
void func_0888B600(f32 f, u8 *self, u32 a, u8 *seg);
s32 camscript_run(u8 *self);
s32 camrail_clamp(u8 *self, f32 *st, u8 *seg, f32 *d, s32 n);

#define ST_SEG(st) (((u8 *)(st))[8])

/* progress i/n with an ease (1: out, 2: in) */
f32 cam_ease(u8 *self, s32 i, s32 n, u8 mode)
{
    f32 f = (1.0f / (f32)n) * (f32)i;

    if (mode == 2)
        f = func_08897D4C(f, 2.25f);
    else if (mode == 1)
        f = 1.0f - func_08897D4C(1.0f - f, 2.25f);
    return f;
}

static void copy4(u8 *d, const u8 *s)
{
    U32(d, 0) = U32(s, 0);
    U32(d, 4) = U32(s, 4);
    U32(d, 8) = U32(s, 8);
    U32(d, 0xC) = U32(s, 0xC);
}

/* event camera on a script (state 0x9C) */
s32 camscript_update(u8 *self)
{
    u8 *cam = g_camera;
    u16 a;
    s32 r;

    if (self[0x13A] != 0 && (U16(cam, 0xA72) & 1) != 0 && G_ARENA == 0)
        return 1;
    if (self[0x9C] == 2)
        return 1;
    if (self[0x9C] != 1) {
        if (self[0x9C] != 0)
            return 0;   /* v0 is not set by the original here */
        self[0x9C] = 1;
        if (self[0xEE] != 0xFF)
            U32(self, 0xE0) = D_08938234[self[0xEE]];
        copy4(self, cam + 0x20);
        copy4(self + 0xA0, cam + 0x20);
        copy4(self + 0xB0, cam + 0x20);
        copy4(self + 0x10, cam + 0x30);
        copy4(self + 0xC0, cam + 0x30);
        copy4(self + 0xD0, cam + 0x30);
        U32(self, 0x80) = U32(cam, 0xA0);
        U32(self, 0x88) = U32(cam, 0xA8);
        a = (s32)(F32(self, 0x80) * 10430.378f);
        U16(self, 0x106) = a;
        U16(self, 0x104) = a;
        a = (s32)(F32(self, 0x88) * 10430.378f);
        U16(self, 0x10A) = a;
        U16(self, 0x108) = a;
        U16(self, 0x126) = 0;
        U16(self, 0x11E) = 0;
        U16(self, 0x116) = 0;
        U16(self, 0x10E) = 0;
        U32(self, 0x12C) = 0;
        self[0x138] = 0;
        F32(self, 0x134) = 1.0f;
        self[0xE8] = 1;
        self[0xE9] = 5;
        self[0xEA] = 4;
        self[0xED] = 0;
        U16(self, 0xFC) = 0;
        U16(self, 0x100) = 0;
        U16(self, 0xFE) = 0;
        U16(self, 0x102) = 0;
        F32(self, 0xF4) = 512.0f;
        F32(self, 0xF8) = 512.0f;
        U16(self, 0x96) = 0xFFFF;
        U16(self, 0x94) = 0xFFFF;
    }
    r = camscript_run(self);
    if (r <= 0)
        return r;
    self[0x9C]++;
    return 1;
}

/* runs the commands of the camera script until a wait; 1 at the end */
s32 camscript_run(u8 *self)
{
    s8 *p = (s8 *)U32(self, 0xE0);
    s8 *next;
    s32 cont = 1, ret = 0;
    u16 v;

    self[0x139] = 0;
    for (;;) {
        next = p + p[1] * 4;
        v = U16(p, 2);
        switch ((u8)p[0]) {
        case 0: self[0xE8] = p[2]; break;
        case 1: self[0xE9] = p[2]; self[0xEB] = p[3]; break;
        case 2: func_0888E788(self, self + 0xA0); break;
        case 3: func_0888E788(self, self + 0xB0); break;
        case 4: self[0xEA] = p[2]; self[0xEC] = p[3]; break;
        case 5: func_0888E8E8(self, self + 0xC0); break;
        case 6: func_0888E8E8(self, self + 0xD0); break;
        case 7: self[0xED] = p[2]; break;
        case 8: U16(self, 0xFC) = v; break;
        case 9: U16(self, 0x100) = v; break;
        case 10: U16(self, 0xFE) = v; break;
        case 11: U16(self, 0x102) = v; break;
        case 12: F32(self, 0xF4) = (f32)(s32)U32(p, 4) * 0.0625f; break;
        case 13: F32(self, 0xF8) = (f32)(s32)U32(p, 4) * 0.0625f; break;
        case 14: U16(self, 0x104) = v; break;
        case 15: U16(self, 0x106) = v; break;
        case 16: U16(self, 0x108) = v; break;
        case 17: U16(self, 0x10A) = v; break;
        case 18: U16(self, 0x96) = v; U16(self, 0x94) = v; break;
        case 19: U16(self, 0x10C) = v; break;
        case 20: U16(self, 0x10E) = v; break;
        case 21: U16(self, 0x110) = v; break;
        case 22: U16(self, 0x112) = v; break;
        case 23: U16(self, 0x114) = v; break;
        case 24: U16(self, 0x116) = v; break;
        case 25: U16(self, 0x118) = v; break;
        case 26: U16(self, 0x11A) = v; break;
        case 27: U16(self, 0x11C) = v; break;
        case 28: U16(self, 0x11E) = v; break;
        case 29: U16(self, 0x120) = v; break;
        case 30: U16(self, 0x122) = v; break;
        case 31: U16(self, 0x124) = v; break;
        case 32: U16(self, 0x126) = v; break;
        case 33: U16(self, 0x128) = v; break;
        case 34: U16(self, 0x12A) = v; break;
        case 35: self[0x12C + p[2]] = p[3]; break;
        case 36: cam_script_copy(self, p[2]); break;
        case 37:
            self[0x138] = (v >> 14) & 3;
            F32(self, 0x134) = (1.0f / 1024.0f) * (f32)(v & 0x3FFF);
            break;
        case 38:
            U16(self, 0x96) = v;
            U16(self, 0x94) = v;
            U32(self, 0xE4) = (u32)next;
            break;
        case 39:
            if (p[2] != 0)
                func_0888EAF8(self, (u8 *)p);
            S16(self, 0x94)--;
            if (S16(self, 0x96) > 0 && S16(self, 0x94) < 0) {
                /* the loop is over: back to the start values */
                copy4(self + 0xA0, self);
                copy4(self + 0xC0, self + 0x10);
                U16(self, 0xFC) = U16(self, 0x100);
                U16(self, 0xFE) = U16(self, 0x102);
                U16(self, 0x104) = U16(self, 0x106);
                U16(self, 0x108) = U16(self, 0x10A);
                U16(self, 0x10E) = 0;
                U16(self, 0x116) = 0;
                U16(self, 0x11E) = 0;
                U16(self, 0x126) = 0;
                U16(self, 0x110) = U16(self, 0x112);
                U16(self, 0x118) = U16(self, 0x11A);
                U16(self, 0x120) = U16(self, 0x122);
                U16(self, 0x128) = U16(self, 0x12A);
                self[0x138] = 0;
                F32(self, 0x134) = 1.0f;
                U16(self, 0x94) = 0xFFFF;
                U16(self, 0x96) = 0xFFFF;
                break;
            }
            next = (s8 *)U32(self, 0xE4);
            cont = 0;
            ret = 0;
            break;
        case 40: func_0888EAF8(self, (u8 *)p); break;
        case 42: cont = 0; ret = -1; break;
        case 43: self[0x13B] = 1; break;
        case 44:
            if (U32(self, 0x130) != 0)
                next = (s8 *)U32(self, 0x130);
            break;
        case 45: U32(self, 0x130) = (u32)next; break;
        default:
            return 1;
        }
        if (self[0x139] != 0)
            return 1;
        U32(self, 0xE0) = (u32)next;
        if (!cont)
            return ret;
        p = next;
    }
}

void camrail_point(u8 *self, u32 a, u8 *rail)
{
    u8 *cam = g_camera;
    u32 k;

    spline_build(self, rail + 0x20, rail[0x280]);
    k = cam[0xAA0];
    func_0888B600(F32(cam, 0xA9C) * F32(rail, k * 0x10 + 0x2C), self, a, D_08A62DA0 + k * 0x40);
}

/* point of the rail nearest to pos (then the segment that contains it) */
s32 camrail_nearest(u8 *self, u8 *rail, f32 *pos)
{
    f32 d[17];
    f32 best = 1e7f;
    s32 n = rail[0x260], i, k = 0;
    f32 *pt = (f32 *)(rail + 0x100);

    for (i = 0; i < n; i++, pt += 3) {
        d[i] = vfpu_dist3(pos, pt);
        if (d[i] < best) {
            best = d[i];
            k = i;
        }
        n = rail[0x260];
    }
    if (k == 0 || k >= n - 1) {
        if (k >= n - 1)
            k = n - 2;
    } else if (d[k - 1] < d[k + 1]) {
        k--;
    }
    return k;
}

/* position on the rail for x (st[0] distance on the segment ST_SEG) */
s32 camrail_locate(u8 *self, f32 *st, u8 *rail, u32 x)
{
    f32 d[5];
    s32 n, r;

    spline_build(self, rail + 0x100, rail[0x260]);
    n = func_0888BC40(self, d, D_08A62DA0 + ST_SEG(st) * 0x40, x, rail[0x261]);
    if (n == 0) {
        st[0] = 0.0f;
        return 0;
    }
    r = camrail_clamp(self, st, rail + ST_SEG(st) * 0x10 + 0x100, d, n);
    if (r > 0) {
        if (ST_SEG(st) >= rail[0x260] - 2)
            return 1;
        n = func_0888BC40(self, d, D_08A62DA0 + (ST_SEG(st) + 1) * 0x40, x, rail[0x261]);
        if (n != 0) {
            ST_SEG(st)++;
            camrail_clamp(self, st, rail + ST_SEG(st) * 0x10 + 0x100, d, n);
        }
    } else if (r < 0 && ST_SEG(st) != 0) {
        n = func_0888BC40(self, d, D_08A62DA0 + (ST_SEG(st) - 1) * 0x40, x, rail[0x261]);
        if (n != 0) {
            ST_SEG(st)--;
            camrail_clamp(self, st, rail + ST_SEG(st) * 0x10 + 0x100, d, n);
        }
    }
    return 1;
}

/* the first of the n solutions d inside the segment (length at seg + 0xC) gives 0;
 * otherwise the nearest end: -1 before the start, 1 after the end */
s32 camrail_clamp(u8 *self, f32 *st, u8 *seg, f32 *d, s32 n)
{
    f32 best = d[0], v, len;
    s32 r;

    if (best < 0.0f) {
        st[0] = 0.0f;
        best = -best;
        r = -1;
    } else {
        len = F32(seg, 0xC);
        if (best <= len) {
            st[0] = best;
            return 0;
        }
        best -= len;
        st[0] = len;
        r = 1;
    }
    if (n != 1) {
        while (--n != 0) {
            v = *++d;
            if (v < 0.0f) {
                if (-v < best) {
                    st[0] = 0.0f;
                    r = -1;
                    best = -v;
                }
            } else {
                len = F32(seg, 0xC);
                if (v <= len) {
                    st[0] = v;
                    return 0;
                }
                if (v - len < best) {
                    st[0] = len;
                    r = 1;
                    best = v - len;
                }
            }
        }
    }
    return r;
}
