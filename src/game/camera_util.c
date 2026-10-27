/* Camera (4): helpers of the camera of the player: input, lock-on checks, height over the
 * ground, the roots of a polynomial (rail intersections), copies of the script values.
 * (Guessed names.) */
#include "menu.h"

extern u8 *g_camera;
extern u8 *g_game;
extern u8 *g_monsters;
extern u8 *D_08A62C60;
extern void *D_09D1A060;          /* collision of the stage */
extern const f32 D_089AD1DC[10];  /* start points of the roots (on a circle) */
extern const f32 D_089AD204[];    /* exponents of the root bound */

f32 func_08897D4C(f32 x, f32 y);
f32 func_09C3DA00(void *col, f32 *pos);
s32 func_09C3DE10(void *col, f32 *a, u8 *b, u8 *out);
int menu_busy(u8 *self);
u32 entity_flag(u8 *o, u32 f);
int entity_is(u8 *o, u32 a, u32 b);
u8 func_088DACA4(u8 *e);
u8 func_088DA324(u8 *e, u8 *f);
u8 func_09A6ED48(u8 *e);
s8 func_09A6F6B0(u8 *e);
u8 cam_lockon_active(u8 *self);
u8 cam_can_turn(u8 *self);

#define CAM_PLAYER ((u8 *)U32(g_camera, 0xA84))

/* buttons of the camera for this frame (0x124 held, 0x126 pressed) */
void cam_read_buttons(u8 *self)
{
    u8 *cam = g_camera;

    if (cam[0xA8D] == 0) {
        if (cam[0x961] != 0) {
            U16(self, 0x126) = 0;
            U16(self, 0x124) = 0;
            return;
        }
        if (cam_can_turn(self) == 1) {
            U16(self, 0x124) = U16(cam, 0xA70);
            U16(self, 0x126) = U16(cam, 0xA72);
            return;
        }
        if (D_08A62C60[0x74] == 0) {
            U16(self, 0x124) = U16(cam, 0xA70) & 0x100;
            U16(self, 0x126) = U16(cam, 0xA72) & 0x100;
            return;
        }
    }
    U16(self, 0x126) = 0;
    U16(self, 0x124) = 0;
}

/* extra height of the camera when the player is under something (0xD8, 0 to 300) */
f32 cam_roof_height(u8 *self)
{
    u8 buf[16];
    f32 a, b;

    if (func_09C3DE10(D_09D1A060, (f32 *)(self + 0xA0), (u8 *)U32(CAM_PLAYER, 0x190) + 0x2F40,
                      buf) != 0) {
        a = func_09C3DA00(D_09D1A060, (f32 *)(self + 0xA0));
        b = func_09C3DA00(D_09D1A060, (f32 *)(self + 0xB0));
        if (a - b > 150.0f) {
            F32(self, 0xD8) += 20.0f;
            if (F32(self, 0xD8) > 300.0f)
                F32(self, 0xD8) = 300.0f;
            return F32(self, 0xD8);
        }
    }
    F32(self, 0xD8) -= 20.0f;
    if (F32(self, 0xD8) < 0.0f)
        F32(self, 0xD8) = 0.0f;
    return F32(self, 0xD8);
}

/* the player can turn the camera freely */
u8 cam_can_turn(u8 *self)
{
    u8 *pl = CAM_PLAYER;

    if (((u8 *)g_menu)[0x554] != 0)
        return 0;
    if ((u8)menu_busy(g_menu) == 1)
        return 0;
    if ((U32(pl, 0x614) & 2) != 0)
        return 0;
    if (pl[0x564] != 0)
        return 0;
    if (cam_lockon_active(self) != 0)
        return 0;
    if ((pl[0x1E8] == 1 || pl[0x1E8] == 5) && (U32(pl, 0x410) & 8) != 0 &&
        (U32(pl, 0x614) & 0x8000) != 0)
        return 0;
    return (s8)self[0x131] < 0;
}

u8 cam_lockon_active(u8 *self)
{
    return entity_flag(CAM_PLAYER, 0x80000) != 0;
}

/* the player fell from a ledge (0xD5 state, 0xD6 timer): 1 while the camera follows */
u32 cam_fall_follow(u8 *self)
{
    u8 *pl = CAM_PLAYER;
    u32 k = 0;

    if (pl[0x298] == 2) {
        self[0xD5] = 0;
        return 0;
    }
    if (pl[0x280] == 2) {
        k = 1;
        if (F32(pl, 0x204) < F32(pl, 0x214) - 10.0f) {
            k = 2;
            if (F32(pl, 0x204) > func_09C3DA00(D_09D1A060, (f32 *)(pl + 0x200)) + 300.0f)
                k = 3;
        }
    }
    if (self[0xD5] != 1) {
        if (self[0xD5] != 0)
            return 0;
        if (k < 3) {
            U16(self, 0xD6) = 4;
            return 0;
        }
        if (--S16(self, 0xD6) > 0)
            return 0;
        self[0xD5]++;
    }
    if ((k & 1) != 0) {
        U16(self, 0xD6) = 0xF;
        return 1;
    }
    if (--S16(self, 0xD6) < 1) {
        self[0xD5] = 0;
        return 0;
    }
    return 1;
}

/* kind of camera for the weapon of the player (0xFF: normal) */
u32 cam_weapon_kind(u8 *self)
{
    u8 *pl = CAM_PLAYER;

    if ((u8)entity_is(pl, 0, 0x65) == 1)
        return 1;
    if ((u8)entity_is(pl, 0, 0x66) == 1)
        return 1;
    if (func_088DACA4(pl) == 1)
        return 2;
    if ((u8)entity_is(pl, 8, 0xF) == 1)
        return 3;
    if (pl[0x1E8] == 1)
        return 0;
    if (pl[0x1E8] == 5)
        return 0;
    return 0xFF;
}

/* the 5 complex roots (re, im) of the polynomial c[0..5] (Durand-Kerner, 26 rounds) */
void poly5_roots(u8 *self, f32 *z, f32 *c)
{
    f32 a[6];
    f32 inv = 1.0f / c[0];
    f32 r = 0.0f, v, zr, zi, pr, pi, ni, dr, di, t, den, k;
    s32 i, j, n;

    a[1] = inv * c[1];
    a[2] = inv * c[2];
    a[3] = inv * c[3];
    a[4] = inv * c[4];
    a[5] = inv * c[5];
    for (i = 2; i < 6; i++) {
        v = func_08897D4C(__builtin_fabsf(a[i]), D_089AD204[i]);
        if (r < v)
            r = v;
    }
    r *= 5.0f;
    for (i = 0; i < 10; i++)
        z[i] = r * D_089AD1DC[i];
    for (n = 0x19; n >= 0; n--) {
        for (i = 0; i < 5; i++) {
            zr = z[i * 2];
            zi = z[i * 2 + 1];
            pr = 1.0f;
            pi = 0.0f;
            dr = 1.0f;
            di = 0.0f;
            for (j = 0; j < 5; j++) {
                ni = pr * zi + pi * zr;
                pr = (pr * zr - pi * zi) + a[j + 1];
                if (j != i) {
                    t = di * (zi - z[j * 2 + 1]);
                    di = dr * (zi - z[j * 2 + 1]) + di * (zr - z[j * 2]);
                    dr = dr * (zr - z[j * 2]) - t;
                }
                pi = ni;
            }
            den = dr * dr + di * di;
            if (den >= 0.001f) {
                k = 1.0f / den;
                t = pr * di;
                pr = k * (pr * dr + pi * di);
                pi = k * (pi * dr - t);
            }
            z[i * 2] = zr - pr;
            z[i * 2 + 1] = zi - pi;
        }
    }
}

f32 vec3_dot(u8 *self, f32 *a, f32 *b)
{
    return a[2] * b[2] + a[0] * b[0] + a[1] * b[1];
}

f32 vec2_dot_xz(u8 *self, f32 *a, f32 *b)
{
    return a[0] * b[0] + a[2] * b[2];
}

/* position of the monster followed by the event camera (0 if gone) */
u8 *cam_event_monster(u8 *self)
{
    u8 *m = (u8 *)U32(self, 0xF0);
    u8 *e;
    s32 i;

    if (m != NULL) {
        for (i = 0; i < 0x14; i++) {
            if (i < 0)
                e = NULL;
            else if (i < 0x14)
                e = (u8 *)U32(g_monsters, i * 4 + 0x1220);
            else
                e = NULL;
            if (e == m)
                return m + 0x10;
        }
    }
    self[0x139] = 1;
    return NULL;
}

/* the two angles of the script, moving from the start values during the loop */
void cam_script_angles(u8 *self, s16 *out)
{
    s16 d;

    if (S16(self, 0x96) < 1) {
        out[0] = S16(self, 0xFC);
        out[1] = S16(self, 0xFE);
        return;
    }
    d = S16(self, 0xFC) - S16(self, 0x100);
    out[0] = d;
    out[0] = S16(self, 0x100) + (s16)(d * S16(self, 0x94) / S16(self, 0x96));
    d = S16(self, 0xFE) - S16(self, 0x102);
    out[1] = d;
    out[1] = S16(self, 0x102) + (s16)(d * S16(self, 0x94) / S16(self, 0x96));
}

void cam_blend_start(u8 *self, s32 n)
{
    self[0x9C] = 1;
    S16(self, 0x94) = n - 1;
    F32(self, 0x190) = 1.0f / (f32)n;
}

/* blend frames for the new area (table of {area, frames} at ent+0x1C) */
void cam_blend_area(u8 *self, u8 *ent)
{
    s8 n = 0xF;
    s8 *p;
    s32 i;

    if (g_camera[0xA8B] == 0)
        return;
    if (ent != NULL && (p = (s8 *)U32(ent, 0x1C)) != NULL) {
        for (i = 0x10; i != 0; i--, p += 2) {
            if (p[0] == (s8)g_camera[0xA94]) {
                n = p[1];
                break;
            }
        }
    }
    if (n == 1 || n == 0)
        self[0x9C] = 0;
    else
        cam_blend_start(self, n);
}

/* eased progress of the blend (smooth step from the square) */
f32 cam_blend_step(u8 *self)
{
    f32 t = (f32)S16(self, 0x94) * F32(self, 0x190);

    if (t < 0.5f)
        t = func_08897D4C(2.0f * t, 2.0f) * 0.5f;
    else
        t = 1.0f - 0.5f * func_08897D4C(2.0f * (1.0f - t), 2.0f);
    if (--S16(self, 0x94) == 0)
        self[0x9C] = 0;
    return t;
}

/* the camera may look around (not during some actions) */
u32 cam_can_look(u8 *self)
{
    u8 *pl = CAM_PLAYER;

    if (func_088DA324(pl, pl) == 1 && self[0x12C] != 0)
        return 1;
    if ((u8)game_quest_locked(g_game, 1) == 1)
        return 0;
    if (pl[0x552] != 0)
        return 0;
    if (func_09A6ED48(pl) == 1)
        return 1;
    if (pl[0x1E8] != 10)
        return 0;
    if ((U16(self, 0x126) & 0x200) == 0 || (U32(pl, 0x480) & 0x200) == 0 ||
        (U32(pl, 0x410) & 8) == 0 || pl[0x298] == 2 || pl[0x298] == 6)
        return 0;
    return 1;
}

/* height of the ground under pos; under a roof the ground 150 higher */
f32 cam_ground_height(u8 *self, f32 *pos, u8 *ent)
{
    f32 v[4];
    f32 h = func_09C3DA00(D_09D1A060, pos);

    if (F32(ent, 0x204) <= h)
        return h;
    v[0] = pos[0];
    v[1] = pos[1];
    v[2] = pos[2];
    v[3] = pos[3];
    v[1] += 150.0f;
    return func_09C3DA00(D_09D1A060, v);
}

/* bump of strength k between the angles a and b (16-bit angle x, both directions) */
s32 cam_angle_bump(f32 a, f32 b, f32 k, u8 *self, u32 x)
{
    f32 fx = (f32)x, d0, d1;

    if (fx <= a)
        return 0;
    if (!(fx < b)) {
        if (fx <= 65536.0f - b)
            return 0;
        if (!(fx < 65536.0f - a))
            return 0;
        x = 0x10000 - x;
        k = -k;
    }
    fx = (f32)(s32)x;
    d0 = fx - a;
    d1 = b - fx;
    return (s32)(k * ((1.0f / 65536.0f) * (d0 * d0) * ((1.0f / 65536.0f) * (d1 * d1))));
}

/* copies the start values of the script to the targets (mask) */
void cam_script_copy(u8 *self, u32 m)
{
    if ((m & 1) != 0) {
        U32(self, 0xB0) = U32(self, 0xA0);
        U32(self, 0xB4) = U32(self, 0xA4);
        U32(self, 0xB8) = U32(self, 0xA8);
        U32(self, 0xBC) = U32(self, 0xAC);
    }
    if ((m & 2) != 0) {
        U32(self, 0xD0) = U32(self, 0xC0);
        U32(self, 0xD4) = U32(self, 0xC4);
        U32(self, 0xD8) = U32(self, 0xC8);
        U32(self, 0xDC) = U32(self, 0xCC);
    }
    if ((m & 4) != 0)
        U16(self, 0x106) = U16(self, 0x104);
    if ((m & 8) != 0)
        U16(self, 0x10A) = U16(self, 0x108);
    if ((m & 0x10) != 0)
        U16(self, 0x112) = U16(self, 0x110);
    if ((m & 0x20) != 0)
        U16(self, 0x11A) = U16(self, 0x118);
    if ((m & 0x40) != 0)
        U16(self, 0x122) = U16(self, 0x120);
    if ((m & 0x80) != 0)
        U16(self, 0x12A) = U16(self, 0x128);
}

/* target of the camera chosen by the player (0x131, -1: none; 0x132 frames to keep it) */
void cam_update_target(u8 *self)
{
    u8 *pl = CAM_PLAYER;
    s8 c;
    u8 k;

    self[0x133] = 0;
    if ((GAME_FLAGS & 1) == 0) {
        k = 0;
        if (pl[0x298] == 2) {
            u8 a = pl[0x299];

            if (a == 0xE || a == 7 || a == 5 || a == 2)
                k = 1;
        }
        if (k == 1)
            self[0x133] = 1;
    }
    if (self[0x133] == 0)
        c = func_09A6F6B0(pl);
    else
        c = -1;
    if (c < 0) {
        self[0x132] = 3;
    } else {
        if ((s8)self[0x131] >= 0) {
            self[0x131] = c;
            return;
        }
        if (--((s8 *)self)[0x132] > 0)
            c = -1;
    }
    self[0x131] = c;
}

/* the player looks the same way as the camera (within 90 degrees) */
u8 cam_facing(u8 *self)
{
    u32 d;

    if (self[0x12D] != 0)
        return 1;
    d = (U32(CAM_PLAYER, 0x1F4) - S16(self, 0x120)) & 0xFFFF;
    if (d < 0x4000)
        return 1;
    return d > 0xC000;
}
