/* Camera (3): the splines of the camera rails (cubic Hermite segments with the tangents from
 * a tridiagonal system) and the event cameras stored with the monster models.
 * (Guessed names.) */
#include "menu.h"

extern u8 *g_camera;
extern u8 *g_game;
extern u8 *g_monster_models;
extern f32 D_08A62DA0[];          /* 16 floats per segment: x/y/z coefficients a, b, c, d */
extern const u16 D_0893843C[];    /* {event, data index, flags}, 0xFFFF-terminated */

f32 func_08897D4C(f32 x, f32 y);
u32 data_table_entry(void *data, u32 i);
u8 func_0888CC10(u8 *self);
void spline_solve(u8 *self, f32 *x, f32 *a, f32 *b, f32 *c, f32 *d, s32 n);

/* linear map of t from [p+8, p+0xC] to [p+0x10, p+0x14], clamped */
f32 cam_remap(f32 t, u8 *self, u8 *p)
{
    f32 a = F32(p, 8), b;

    if (t <= a)
        return F32(p, 0x10);
    b = F32(p, 0xC);
    if (b <= t)
        return F32(p, 0x14);
    if (b == a)
        return (F32(p, 0x10) + F32(p, 0x14)) * 0.5f;
    return (F32(p, 0x14) - F32(p, 0x10)) * (t - a) / (b - a) + F32(p, 0x10);
}

f32 cam_lerp_value(f32 t, u8 *self, u8 *p, s32 i)
{
    p += i * 4;
    return F32(p, 0x200) * (1.0f - t) + F32(p, 0x204) * t;
}

/* angle between two 16-bit angles, in radians */
f32 cam_lerp_angle(f32 t, u8 *self, u8 *p, s32 i)
{
    s16 a, d;

    p += i * 2;
    a = S16(p, 0x240);
    d = (s16)((u32)((s32)(t * 65536.0f) * (S16(p, 0x242) - a)) >> 16);
    return (f32)(s16)(d + a) * 9.58738e-05f;
}

/* the segments of the spline through the n points p (x, y, z, length of the segment) */
void spline_build(u8 *self, f32 *p, s32 n)
{
    f32 a[64], b[64], c[64], rx[64], ry[64], rz[64];
    f32 tx[64], ty[64], tz[64];
    f32 *q = p, *s = D_08A62DA0;
    f32 l0, l1, r, k, i1, i2, i3;
    s32 i;

    a[0] = 0.0f;
    c[0] = p[3];
    b[0] = c[0] + c[0];
    rx[0] = (p[4] - p[0]) * 3.0f;
    ry[0] = (p[5] - p[1]) * 3.0f;
    rz[0] = (p[6] - p[2]) * 3.0f;
    for (i = 1; i < n - 1; i++, q += 4) {
        l0 = q[3];
        l1 = q[7];
        r = l1 / l0;
        k = l0 / l1;
        a[i] = l1;
        b[i] = (q[3] + q[7]) * 2.0f;
        c[i] = q[3];
        rx[i] = (r * (q[4] - q[0]) + k * (q[8] - q[4])) * 3.0f;
        ry[i] = (r * (q[5] - q[1]) + k * (q[9] - q[5])) * 3.0f;
        rz[i] = (r * (q[6] - q[2]) + k * (q[10] - q[6])) * 3.0f;
    }
    a[n - 1] = p[n * 4 - 5];
    b[n - 1] = p[n * 4 - 5] + p[n * 4 - 5];
    c[n - 1] = 0.0f;
    rx[n - 1] = (p[n * 4 - 4] - p[n * 4 - 8]) * 3.0f;
    ry[n - 1] = (p[n * 4 - 3] - p[n * 4 - 7]) * 3.0f;
    rz[n - 1] = (p[n * 4 - 2] - p[n * 4 - 6]) * 3.0f;
    spline_solve(self, tx, a, b, c, rx, n);
    spline_solve(self, ty, a, b, c, ry, n);
    spline_solve(self, tz, a, b, c, rz, n);
    for (i = 0; i < n - 1; i++, p += 4, s += 16) {
        i1 = 1.0f / p[3];
        i2 = i1 * i1;
        i3 = 1.0f / func_08897D4C(p[3], 3.0f);
        s[0] = i2 * (tx[i] + tx[i + 1]) + i3 * (p[0] - p[4]) * 2.0f;
        s[4] = i2 * (p[4] - p[0]) * 3.0f - i1 * (tx[i] * 2.0f + tx[i + 1]);
        s[8] = tx[i];
        s[12] = p[0];
        s[1] = i2 * (ty[i] + ty[i + 1]) + i3 * (p[1] - p[5]) * 2.0f;
        s[5] = i2 * (p[5] - p[1]) * 3.0f - i1 * (ty[i] * 2.0f + ty[i + 1]);
        s[9] = ty[i];
        s[13] = p[1];
        s[2] = i2 * (tz[i] + tz[i + 1]) + i3 * (p[2] - p[6]) * 2.0f;
        s[6] = i2 * (p[6] - p[2]) * 3.0f - i1 * (tz[i] * 2.0f + tz[i + 1]);
        s[10] = tz[i];
        s[14] = p[2];
    }
}

/* tridiagonal system: a sub-diagonal, b diagonal, c super-diagonal, d right side */
void spline_solve(u8 *self, f32 *x, f32 *a, f32 *b, f32 *c, f32 *d, s32 n)
{
    f32 t[64];
    f32 g = b[0];
    s32 i;

    if (g != 0.0f)
        g = 1.0f / g;
    x[0] = d[0] * g;
    for (i = 1; i < n; i++) {
        t[i - 1] = g * c[i - 1];
        g = b[i] - a[i] * g * c[i - 1];
        if (g != 0.0f)
            g = 1.0f / g;
        x[i] = g * (d[i] - a[i] * x[i - 1]);
    }
    for (i = n - 2; i >= 0; i--)
        x[i] = x[i] - t[i] * x[i + 1];
}

/* event camera of the monster models (state 0x9C); 1 at the end */
s32 cam_event_update(u8 *self)
{
    u8 *cam = g_camera;
    const u16 *e = NULL;
    const u16 *t;
    void *data;
    s32 i;

    if (self[0x13A] != 0 && (U16(cam, 0xA72) & 1) != 0 && G_ARENA == 0)
        return 1;
    switch (self[0x9C]) {
    case 0:
        U32(self, 0xE0) = 0;
        for (i = 0, t = D_0893843C; t[0] != 0xFFFF; i++, t += 3) {
            if (self[0xEE] != t[0])
                continue;
            data = (U16(g_monster_models, 0) & 2) == 0 ? NULL : (void *)U32(g_monster_models, 4);
            if (data == NULL)
                continue;
            U32(self, 0xE0) = data_table_entry(data, D_0893843C[i * 3 + 1] + 6);
            self[0x13C] = i;
            e = D_0893843C + self[0x13C] * 3;
            break;
        }
        if (U32(self, 0xE0) == 0)
            return 1;
        if ((e[2] & 4) != 0)
            self[0x13A] = 1;
        U16(self, 0x94) = 0;
        if (cam[0xA7A] == 1)
            U16(self, 0x96) = 0;
        else
            U16(self, 0x96) = 2;
        self[0x9C] = 1;
        /* fallthrough */
    case 1:
        func_0888CC10(self);
        if (S16(self, 0x96) != 0) {
            S16(self, 0x96)--;
            return 0;
        }
        self[0x9C] = 2;
        /* fallthrough */
    case 2:
        if (func_0888CC10(self) == 0) {
            S16(self, 0x94)++;
            return 0;
        }
        self[0x9C] = 3;
        /* fallthrough */
    case 3:
        U16(self, 0x94) = 0;
        return 1;
    }
    return 0;
}
