/* Camera: the special modes (object of the mode at self, camera g_camera) and the rails of
 * the cutscene cameras. The modes with VFPU math are still in assembly. (Guessed names.) */
#include "menu.h"

extern u8 *g_camera;
extern u8 *g_game;
extern const f32 D_09A5F114[3];
extern const f32 D_089AD194[3], D_089AD1A0[3], D_089AD1AC[3], D_089AD1B8[3];
extern const f32 D_089AD1C4[3], D_089AD1D0[3];
extern const s32 D_08938234[];    /* camera events: -1 = no rail */

s32 vec3_nonzero(f32 *v);
void func_08886B68(u8 *self);
void func_08888B84(u8 *self);
s32 camscript_update(u8 *self);
s32 func_0888C9F8(u8 *self);
u8 func_0888A73C(u8 *self, u8 *st);
void func_0888A8C0(u8 *self, u8 *st);
s32 camrail_nearest(u8 *self, u8 *rail, f32 *pos);
s32 camrail_locate(u8 *self, f32 *st, u8 *rail, u32 x);
u8 func_0888D708(u8 *self);

void cammode_none(u8 *self);
void cammode_init_3(u8 *self);
void cammode_init_4(u8 *self);
void cammode_init_5(u8 *self);
void camrail_move(f32 target, u8 *self, f32 *st, u8 *rail, s32 k);
u32 camrail_set(u8 *self, f32 *st, u8 *rail, u32 x);
u32 camrail_follow(u8 *self, f32 *st, u8 *rail, u32 x);
void cammode_lockon(u8 *self, u8 *st);

static void set_up(u8 *self, const f32 *src)
{
    f32 v[3];

    v[0] = src[0];
    v[1] = src[1];
    v[2] = src[2];
    if (vec3_nonzero(v) != 0) {
        F32(self, 0x180) = v[0];
        F32(self, 0x184) = v[1];
        F32(self, 0x188) = v[2];
    }
}

/* starts the camera mode m */
void cammode_start(u8 *self, u8 m)
{
    U32(self, 0x98) = 0;
    F32(self, 0x180) = D_09A5F114[0];
    F32(self, 0x184) = D_09A5F114[1];
    F32(self, 0x188) = D_09A5F114[2];
    self[0x18C] = m;
    switch (m) {
    case 0:
        func_08886B68(self);
        set_up(self, D_089AD194);
        break;
    case 1:
        cammode_none(self);
        set_up(self, D_089AD1A0);
        break;
    case 2:
        func_08888B84(self);
        set_up(self, D_089AD1AC);
        break;
    case 3:
        cammode_init_3(self);
        set_up(self, D_089AD1B8);
        break;
    case 4:
        cammode_init_4(self);
        set_up(self, D_089AD1C4);
        break;
    case 5:
        cammode_init_5(self);
        set_up(self, D_089AD1D0);
        break;
    }
}

void cammode_none(u8 *self)
{
}

void cammode_init_3(u8 *self)
{
    U32(self, 0x88) = 0x3F490FDB;     /* pi/4 */
    U32(self, 0x80) = 0;
    U32(self, 0xFC) = 0x3F490FDB;
    U32(self, 0x100) = 0x3F490FDB;
}

void cammode_init_4(u8 *self)
{
    U32(self, 0x88) = 0x3F75BE0B;
    U32(self, 0x80) = 0;
    U32(self, 0xA4) = 0;
    U32(self, 0xB0) = 0;
    U16(self, 0xE4) = 0xF;
    self[0xE0] = 0;
    U32(self, 0x104) = 0;
}

void cammode_update_4(u8 *self)
{
    cammode_lockon(self, self + 0xA4);
    if (self[0x91] == 0)
        func_0888A8C0(self, self + 0xB0);
}

void cammode_init_5(u8 *self)
{
    U32(self, 0x80) = 0;
    U32(self, 0x88) = 0x3F75BE0B;
    self[0xEE] = 0;
    self[0xEF] = 0;
    U32(self, 0xF0) = 0;
    self[0x13A] = 0;
    self[0x13B] = 0;
    U16(self, 0x94) = 0;
}

/* camera events (cutscene rails): 0x98 state, 0xEE event, 0xEF result */
void cammode_update_5(u8 *self)
{
    u8 *cam = g_camera;
    s32 r = 0;

    self[0x91] = 0;
    if (self[0x98] == 2) {
        r = func_0888C9F8(self);
    } else if (self[0x98] == 1) {
        r = camscript_update(self);
    } else if (self[0x98] == 0) {
        if (cam[0xA80] == 0)
            return;
        self[0xEE] = cam[0xA80];
        U32(self, 0xF0) = U32(cam, 0xA7C);
        self[0x13B] = 0;
        U32(self, 0x9C) = 0;
        if (D_08938234[self[0xEE]] == -1) {
            self[0x98] = 2;
            r = func_0888C9F8(self);
        } else {
            self[0x98] = 1;
            r = camscript_update(self);
        }
        self[0x91] = 1;
        cam[0xA80] = 0;
    }
    if (self[0x98] == 0)
        return;
    if (r > 0) {
        self[0x98] = 0;
        self[0xEE] = 0;
        self[0xEF] = 0;
        U32(self, 0xF0) = 0;
        cam[0xAAA] = 1;
        self[0x13A] = 0;
        return;
    }
    self[0x91] = 1;
    if (r == 0)
        self[0xEF] = 1;
    else
        self[0xEF] = 0xFF;
}

u32 camrail_update(u8 *self, u32 x, u8 follow)
{
    u8 *rail = (u8 *)U32(g_camera, 0xA90);

    if (rail == NULL)
        return 0;
    if (follow == 0)
        return camrail_set(self, (f32 *)(g_camera + 0xA98), rail + 0x20, x);
    return camrail_follow(self, (f32 *)(g_camera + 0xA98), rail + 0x20, x);
}

#define SEG_LEN(rail, i) F32(rail, (i) * 0x10 + 0x10C)
#define ST_SEG(st) (((u8 *)(st))[8])

/* moves the position st (distance on the segment ST_SEG) by 0.5 toward target on segment k */
void camrail_move(f32 target, u8 *self, f32 *st, u8 *rail, s32 k)
{
    f32 step = 0.5f;
    f32 d;

    if (ST_SEG(st) < k) {
        for (;;) {
            d = SEG_LEN(rail, ST_SEG(st)) - st[0];
            if (step < d)
                break;
            step -= d;
            ST_SEG(st)++;
            st[0] = 0.0f;
            if (ST_SEG(st) >= k) {
                if (target - st[0] <= step)
                    st[0] = target;
                else
                    st[0] += step;
                return;
            }
        }
        st[0] += step;
    } else {
        d = st[0];
        while (d <= step) {
            step -= d;
            ST_SEG(st)--;
            st[0] = SEG_LEN(rail, k);
            if (ST_SEG(st) <= k) {
                if (st[0] - target <= step)
                    st[0] = target;
                else
                    st[0] -= step;
                return;
            }
            d = st[0];
        }
        st[0] = d - step;
    }
}

/* jumps to the point of the rail for x */
u32 camrail_set(u8 *self, f32 *st, u8 *rail, u32 x)
{
    ST_SEG(st) = camrail_nearest(self, rail, (f32 *)x);
    if (camrail_locate(self, st, rail, x) == 0) {
        st[1] = 0.0f;
        st[0] = 0.0f;
        return 0;
    }
    st[1] = st[0] / SEG_LEN(rail, ST_SEG(st));
    return 1;
}

/* moves along the rail toward the point for x */
u32 camrail_follow(u8 *self, f32 *st, u8 *rail, u32 x)
{
    f32 t[2];
    u8 k;
    f32 c;

    k = camrail_nearest(self, rail, (f32 *)x);
    if (camrail_locate(self, t, rail, x) == 0)
        return 0;
    if (ST_SEG(st) == k) {
        c = st[0];
        if (c < t[0]) {
            if (t[0] - c <= 0.5f)
                st[0] = t[0];
            else
                st[0] = c + 0.5f;
        } else if (c > t[0]) {
            if (c - t[0] <= 0.5f)
                st[0] = t[0];
            else
                st[0] = c - 0.5f;
        }
    } else {
        camrail_move(t[0], self, st, rail, k);
    }
    st[1] = st[0] / SEG_LEN(rail, ST_SEG(st));
    return 1;
}

/* lock-on camera: st[0] active, st+4 target found, st+8 target */
void cammode_lockon(u8 *self, u8 *st)
{
    u8 *pl = (u8 *)U32(g_camera, 0xA84);
    u32 k;

    self[0x91] = 0;
    k = func_0888D708(self) & 0xFF;
    U32(st, 4) = k;
    if (k == 0) {
        st[0] = 0;
        return;
    }
    if (st[0] == 0) {
        st[0]++;
        U32(st, 8) = U32(pl, 0x554);
        if (U16(g_game, 0x6AF0E) == 0x4B)
            U32(self, 0x88) = 0x3F5F66F3;
        else
            U32(self, 0x88) = 0x3F75BE0B;
        U32(self, 0x80) = 0;
    }
    if (func_0888A73C(self, st) == 1)
        self[0x91] = 1;
}
