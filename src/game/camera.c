/* Camera: initial settings, shakes and transitions.
 * The camera structure is not yet rebuilt: fields are indicated
 * with their offset. (Guessed names.) */
#include "libc.h"

#define F32(p, off) (*(f32 *)((u8 *)(p) + (off)))
#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define S16(p, off) (*(s16 *)((u8 *)(p) + (off)))
#define U8(p, off)  (*(u8 *)((u8 *)(p) + (off)))

f32 sinf(f32 x);
extern f32 *D_08A62C40;               /* (pointer to a global scale factor) */
extern const u32 D_08933A70[4];       /* initial camera values */
extern const u32 D_08933A80[4];
extern const s16 D_08933A98[];        /* shake durations per type */
extern const f32 D_08933AA8[];        /* effect radius of the shakes */
extern const f32 D_08933A90;

void func_08816264(void *cam);
void camera_look_at(void *cam, const f32 *eye, const f32 *target, const u32 *up);
void camera_apply_projection(void *cam);
void func_08814984(void *cam);
void func_08812BD0(void *cam);
u32 func_088DCEBC(void *a, u8 b);
int func_08865BAC(void *obj);
extern void *D_09A4F0C4;
extern u8 *D_089CC438;

static inline f32 fsqrt(f32 x)
{
    f32 r;
    __asm__("sqrt.s %0, %1" : "=f"(r) : "f"(x));
    return r;
}

/* distance between two 3D vectors */
static inline f32 dist3(const f32 *a, const f32 *b)
{
    f32 dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return fsqrt(dx * dx + dy * dy + dz * dz);
}

/* default projection parameters (near 30, far 65000, aspect, fov) */
void camera_reset_projection(void *cam)
{
    U32(cam, 0x0) = 0x41f00000;   /* 30.0 */
    U32(cam, 0x4) = 0x477de800;   /* 65000.0 */
    U32(cam, 0xC) = 0x3f5f49f5;
    U32(cam, 0x8) = 0x3fe1e1e2;   /* 480/272 */
    U32(cam, 0xB34) = 0;
    camera_apply_projection(cam);
}

void camera_init(void *cam)
{
    u32 a[4], b[4], zero[4];
    int i;

    for (i = 0; i < 4; i++)
        a[i] = D_08933A70[i];
    for (i = 0; i < 4; i++)
        zero[i] = 0;
    for (i = 0; i < 4; i++)
        b[i] = D_08933A80[i];
    *(u32 *)D_08A62C40 = 0x3f800000; /* 1.0 */
    U32(cam, 0x0) = 0x41f00000;
    U32(cam, 0x4) = 0x477de800;
    U32(cam, 0xC) = 0x3f5f49f5;
    U32(cam, 0x8) = 0x3fe1e1e2;
    func_08816264(cam);
    camera_look_at(cam, (f32 *)a, (f32 *)zero, b);
    U32(cam, 0xDC0) = 0;
    U8(cam, 0xAA9) = 0;
    camera_reset_projection(cam);
    U8(cam, 0x14) = 3;
    U8(cam, 0xA80) = 0;
    U32(cam, 0xA7C) = 0;
}

void camera_reset_state(void *cam)
{
    U32(cam, 0xB10) = 0;
    U8(cam, 0xB3C) = 1;
    camera_reset_projection(cam);
    U32(cam, 0xB10) = 0;
    U32(cam, 0xA84) = func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]);
    U8(cam, 0xB00) = 0;
    U8(cam, 0xAE0) = 0;
    U8(cam, 0xAC0) = 0;
    U8(cam, 0xA81) = 0;
    U8(cam, 0xA8A) = 0;
    U8(cam, 0xA88) = 0;
    U32(cam, 0xA7C) = 0;
    U8(cam, 0xA80) = 0;
    func_08814984(cam);
    func_08812BD0(cam);
}

/* Adds to the camera the vertical offset of a shake: a wave that
 * decays with time and with distance from the source. */
void camera_apply_shake(void *cam, u8 *shake)
{
    u32 type;
    f32 amp, wave, falloff;

    if (shake[16] == 0)
        return;
    type = shake[17] & 0x7f;
    amp = type < 2 ? 10.0f : 12.0f;
    wave = sinf((((f32)S16(shake, 18) / (f32)D_08933A98[type]) * 90.0f) / 360.0f * 6.2831855f);
    if ((shake[17] & 0x80) == 0) {
        f32 d = dist3((f32 *)shake, &F32(cam, 0xB40));
        f32 r = D_08933AA8[shake[20]];

        falloff = r <= d ? 0.0f : 1.0f - d / r;
    } else {
        falloff = 1.0f;
    }
    amp = amp * wave * falloff;
    if (S16(shake, 18) & 2)
        amp = -amp;
    F32(cam, 0xB44) += amp;
    F32(cam, 0xB54) += amp;
}

/* starts a shake of type "type", from the position of object obj */
static inline void start_shake(void *cam, int base, int type, u8 *obj, u8 flag)
{
    if (func_08865BAC(obj)) {
        U8(cam, base + 0x10) = 1;
        U8(cam, base + 0x11) = type;
        S16(cam, base + 0x12) = D_08933A98[type];
        U32(cam, base + 0x0) = U32(obj, 0x200);
        U32(cam, base + 0x4) = U32(obj, 0x204);
        U32(cam, base + 0x8) = U32(obj, 0x208);
        U32(cam, base + 0xC) = U32(obj, 0x20C);
        U8(cam, base + 0x14) = flag;
    }
}

void camera_start_shake(void *cam, int type, u8 *obj)
{
    start_shake(cam, 0xAB0, type, obj, 0);
}

void camera_start_shake_ex(void *cam, int type, u8 *obj, u32 flag_arg)
{
    u8 flag = flag_arg;
    start_shake(cam, 0xAB0, type, obj, flag);
}

void camera_start_shake2(void *cam, int type, u8 *obj)
{
    start_shake(cam, 0xAD0, type, obj, 0);
}

/* step of a transition between camera points */
void camera_transition_step(void *cam, int finish)
{
    u8 *t = (u8 *)cam + 0x7E0;
    int i;

    U8(cam, 0x810) = 3;
    S16(cam, 0x814) = 0xf;
    if (finish == 0) {
        i = U32(cam, 0x834) - 1;
        if (t[U32(cam, 0x834) + 0x30] == 5 && S16(cam, 0x816) != 0) {
            f32 d1 = dist3(&F32(cam, 0x730), (f32 *)(t + i * 0x10 + 0x70));
            f32 d2 = dist3(&F32(cam, 0x740), (f32 *)(t + i * 0x10 + 0xA0));
            s16 n = (s16)(int)(D_08933A90 * (d1 + d2)) + 1;

            S16(cam, 0x816) = n;
            S16(cam, 0x814) = n;
        }
    } else {
        i = U32(cam, 0x834) - 1;
        U32(cam, 0x834) = i;
        if (i == 0) {
            *t = 0;
            U8(cam, 0x810) = 0;
            S16(cam, 0x816) = 0;
        } else {
            *t = 1;
            U8(cam, 0x810) = 2;
            S16(cam, 0x816) = 0xf;
            S16(cam, 0x814) = 0xf;
        }
    }
}

/* --- special camera modes ---------------------------------------------- */

void camera_set_mode(void *cam, u32 mode_arg, u32 target, u32 arg_arg)
{
    u8 mode = mode_arg;
    u8 arg = arg_arg;
    U8(cam, 0xAAA) = 0;
    U8(cam, 0xA80) = mode;
    U32(cam, 0xA7C) = target;
    U8(cam, 0xA7A) = arg;
}

/* like camera_set_mode, but only if there is no mode already */
void camera_request_mode(void *cam, u32 mode_arg, u32 target)
{
    u8 mode = mode_arg;
    U8(cam, 0xAAA) = 0;
    if (U8(cam, 0xA80) == 0) {
        U8(cam, 0xA80) = mode;
        U32(cam, 0xA7C) = target;
    }
}

int camera_is_cutscene_active(void *cam)
{
    u8 s = U8(cam, 0x9BE);

    return s == 5 || s == 4 || s == 3;
}

void camera_set_mode2(void *cam)
{
    if (U8(cam, 0xA8D) != 2)
        camera_set_mode(cam, 2, 0, 0);
}

void camera_set_mode1(void *cam)
{
    if (D_089CC438[0x6AF10] != 12)
        camera_set_mode(cam, 1, 0, 0);
}

/* camera mode according to the current zone */
void camera_mode_for_area(void *cam)
{
    s16 area = *(s16 *)(D_089CC438 + 0x6AF0E);
    u8 mode;

    if (area == 0xb0)
        mode = 5;
    else if (area == 0x19)
        mode = 4;
    else if (area == 0xc)
        mode = 3;
    else
        return;
    camera_request_mode(cam, mode, 0);
}

void camera_clear_cutscene(void *cam)
{
    U8(cam, 0x968) = 0;
    S16(cam, 0x964) = 0;
    U8(cam, 0x9BE) = 0;
    U8(cam, 0x9BF) = 0;
    U32(cam, 0x9C0) = 0;
    U8(cam, 0xA0B) = 0;
    U8(cam, 0xAAA) = 1;
    U8(cam, 0xA80) = 0;
    U8(cam, 0xA0A) = 0;
}

int camera_cutscene_state(void *cam)
{
    if (U8(cam, 0x9BF) != 0)
        return (s8)U8(cam, 0x9BF);
    if (U8(cam, 0xA80) != 0 || U8(cam, 0x9BE) != 0)
        return 2;
    return 0;
}

u8 camera_cutscene_id(void *cam)
{
    return U8(cam, 0x9BE);
}

u8 camera_cutscene_flag(void *cam)
{
    return U8(cam, 0xA0A);
}

int camera_is_special_cutscene(void *cam)
{
    u8 id;

    if (U8(cam, 0x961) == 0)
        return 0;
    id = U8(cam, 0x9BE);
    return id >= 0x50 && id <= 0x69;
}

int camera_cutscene_timer(void *cam)
{
    if (D_089CC438[0x422] == 0)
        return -1;
    return S16(cam, 0x964);
}

int camera_blocks_input(void *cam)
{
    if (D_089CC438[0x422] != 1 && U8(cam, 0xA0A) == 0) {
        switch (U8(cam, 0x9BE)) {
        case 3: case 4: case 5:
        case 10: case 11: case 12: case 13: case 14: case 15: case 16: case 17: case 18:
            return 1;
        default:
            return 0;
        }
    }
    return 1;
}

int camera_is_free(void *cam, u8 *player)
{
    if (U8(cam, 0x961) != 0)
        return 0;
    return player[0x564] != 0 || U8(cam, 0x621) != 0;
}

int camera_get_zoom(void *cam, u8 *out_a, u8 *out_b, f32 *out_f)
{
    *out_a = U8(cam, 0x699);
    *out_b = U8(cam, 0x69A);
    if (U8(cam, 0x69A) != 0) {
        f32 f = F32(cam, 0x618) - F32(cam, 0x680);
        *out_f = f;
        *out_f = f * F32(cam, 0x688);
    }
    return (s8)U8(cam, 0x698);
}

void camera_get_eye(void *cam, f32 *out)
{
    out[0] = F32(cam, 0xB40);
    out[1] = F32(cam, 0xB44);
    out[2] = F32(cam, 0xB48);
    U32(out, 12) = 0;
}

void camera_get_target(void *cam, f32 *out)
{
    out[0] = F32(cam, 0xB50);
    out[1] = F32(cam, 0xB54);
    out[2] = F32(cam, 0xB58);
    U32(out, 12) = 0;
}

/* --- camera matrices -------------------------------------------------------- */
f32 cosf(f32 x);
void mat_look_at(f32 *m, const f32 *eye, const f32 *target, const f32 *up);
void camera_build_frustum(void *cam);
void camera_default_area(void *cam);
int func_0884FA5C(void *a, int b);
extern s32 D_08A62C68, D_08A62C6C;
extern u16 D_08A62C78, D_08A62C7A;

/* Inverse of a 4x4 rotation+scale+translation matrix (orthogonal rows). */
static void mat_inverse_ortho(const f32 *m, f32 *out)
{
    f32 len[3], col[3][3], d[3], n[3];
    int i, j;

    for (i = 0; i < 3; i++)
        len[i] = fsqrt(m[i * 4] * m[i * 4] + m[i * 4 + 1] * m[i * 4 + 1] + m[i * 4 + 2] * m[i * 4 + 2]);
    for (j = 0; j < 3; j++)
        for (i = 0; i < 3; i++)
            col[j][i] = m[i * 4 + j] / len[i];
    for (i = 0; i < 3; i++)
        n[i] = -m[12 + i];
    for (i = 0; i < 3; i++)
        d[i] = col[0][i] * n[0] + col[1][i] * n[1] + col[2][i] * n[2];
    for (j = 0; j < 3; j++) {
        for (i = 0; i < 3; i++)
            out[j * 4 + i] = col[j][i] / len[i];
        out[j * 4 + 3] = 0.0f;
    }
    for (i = 0; i < 3; i++)
        out[12 + i] = d[i] / len[i];
    out[15] = 1.0f;
}

/* Places the camera: eye, look-at point and "up"; computes the matrices. */
void camera_look_at(void *cam, const f32 *eye, const f32 *target, const u32 *up)
{
    f32 dx, dy, dz, d2, s;
    f32 se[4], st[4];
    int i;

    for (i = 0; i < 4; i++) {
        U32(cam, 0xB40 + i * 4) = ((const u32 *)eye)[i];
        U32(cam, 0xB50 + i * 4) = ((const u32 *)target)[i];
        U32(cam, 0xB60 + i * 4) = up[i];
    }
    dx = target[0] - eye[0];
    dy = target[1] - eye[1];
    dz = target[2] - eye[2];
    d2 = dx * dx + dy * dy + dz * dz;
    F32(cam, 0xDB8) = d2;
    F32(cam, 0xDB8) = fsqrt(d2); /* eye-to-target distance */
    mat_look_at(&F32(cam, 0xB80), eye, target, (const f32 *)up);
    s = *D_08A62C40;
    for (i = 0; i < 4; i++) {
        se[i] = eye[i] * s;
        st[i] = target[i] * s;
    }
    mat_look_at(&F32(cam, 0xBC0), se, st, (const f32 *)up);
    mat_inverse_ortho(&F32(cam, 0xB80), &F32(cam, 0xC80));
    mat_inverse_ortho(&F32(cam, 0xBC0), &F32(cam, 0xC40));
}

/* Perspective projection matrix (out: 4x4). */
void camera_perspective(f32 fovy, f32 aspect, f32 znear, f32 zfar, void *unused, f32 *out)
{
    f32 c, s, f, n;

    memset(out, 0, 0x40);
    f = zfar * zfar;
    n = znear * *D_08A62C40;
    c = cosf(fovy * 0.5f);
    s = sinf(fovy * 0.5f);
    out[0] = (c / s) / aspect;
    out[5] = c / s;
    out[10] = -(f + n) / (f - n);
    out[11] = -1.0f;
    out[14] = -(f * 2.0f * n) / (f - n);
}

void camera_update_projection(f32 fovy, f32 aspect, f32 znear, f32 zfar, void *cam)
{
    camera_perspective(fovy, aspect, znear, zfar, cam, &F32(cam, 0xC00));
    mat_inverse_ortho(&F32(cam, 0xC00), &F32(cam, 0xD00));
    camera_build_frustum(cam);
}

void camera_apply_projection(void *cam)
{
    camera_update_projection(F32(cam, 0xC), F32(cam, 0x8), F32(cam, 0x0), F32(cam, 0x4), cam);
    U32(cam, 0x10) = U32(cam, 0xC);
}

/* copies the limits of the current game zone */
void camera_load_area_limits(void *cam)
{
    u8 *a;

    if (U32(cam, 0xAAC) == 0)
        camera_default_area(cam);
    a = (u8 *)U32(cam, 0xAAC);
    S16(cam, 0xB1C) = S16(a, 4);
    S16(cam, 0xB1E) = S16(a, 6);
    S16(cam, 0xB20) = S16(a, 8);
    S16(cam, 0xB22) = S16(a, 10);
    U32(cam, 0xB24) = U32(a, 0xC);
    U32(cam, 0xB28) = U32(a, 0x10);
    U32(cam, 0xB2C) = U32(a, 0x14);
    U32(cam, 0xB30) = U32(a, 0x18);
}

/* reads the stick/buttons that move the camera */
void camera_read_input(void *cam)
{
    if (func_0884FA5C(D_089CC438, 1) != 1) {
        if (((u8 *)U32(cam, 0xA84))[0x552] == 0) {
            S16(cam, 0xA70) = (s16)D_08A62C68;
            S16(cam, 0xA72) = (s16)D_08A62C6C;
            S16(cam, 0xA76) = D_08A62C78;
            S16(cam, 0xA78) = D_08A62C7A;
            return;
        }
    }
    S16(cam, 0xA74) = 0;
    S16(cam, 0xA72) = 0;
    S16(cam, 0xA70) = 0;
    S16(cam, 0xA78) = 0;
    S16(cam, 0xA76) = 0;
}

void camera_set_field_142(void *cam, int v)
{
    if (v == 5)
        v = -1;
    if (v < 0)
        v = 0xff;
    U8(cam, 0x142) = v;
}

/* --- "look at" matrix and camera zones ---------------------------------- */
int func_088DA324(u32 player);
void camera_update(void *cam);

/* normalizes v (stays 0 if v is zero) */
static inline void normalize3(f32 *v)
{
    f32 d = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    f32 s = d == 0.0f ? 0.0f : 1.0f / fsqrt(d);

    v[0] *= s;
    v[1] *= s;
    v[2] *= s;
}

static inline void cross3(const f32 *a, const f32 *b, f32 *out)
{
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

/* View matrix: camera x, y, z axes in the columns, then the translation. */
void mat_look_at(f32 *m, const f32 *eye, const f32 *target, const f32 *up)
{
    f32 x[3], y[3], z[3], u[3];
    int i;

    for (i = 0; i < 3; i++) {
        z[i] = eye[i] - target[i];
        u[i] = up[i];
    }
    normalize3(z);
    normalize3(u);
    cross3(u, z, x);
    normalize3(x);
    cross3(z, x, y);
    for (i = 0; i < 3; i++) {
        m[i * 4 + 0] = x[i];
        m[i * 4 + 1] = y[i];
        m[i * 4 + 2] = z[i];
        m[i * 4 + 3] = 0.0f;
    }
    m[12] = -(eye[0] * x[0] + eye[1] * x[1] + eye[2] * x[2]);
    m[13] = -(eye[0] * y[0] + eye[1] * y[1] + eye[2] * y[2]);
    m[14] = -(eye[0] * z[0] + eye[1] * z[1] + eye[2] * z[2]);
    U32(m, 60) = 0x3f800000;
}

/* default zone (when the map has none) */
void camera_default_area(void *cam)
{
    u8 *a = (u8 *)U32(cam, 0xAAC);
    static const u16 off[] = { 0x74, 0x78, 0x80, 0x88, 0x94, 0x98, 0xA0, 0xA8, 0xB4, 0xB8,
                               0xC0, 0xC8, 0xD4, 0xD8, 0xE0, 0xE8, 0xF4, 0xF8, 0x100, 0x108 };
    int i;

    S16(a, 0x00) = 0x102;
    S16(a, 0x02) = 0;
    S16(a, 0x04) = 1;
    S16(a, 0x06) = 1;
    S16(a, 0x08) = 20000;
    S16(a, 0x0A) = 20000;
    U32(a, 0x0C) = 0;
    U32(a, 0x10) = 0;
    U32(a, 0x14) = 20000;
    U32(a, 0x18) = 20000;
    U32(a, 0x1C) = 0;
    U32(a, 0x20) = 0;
    U32(a, 0x24) = 0;
    U32(a, 0x28) = (u32)(a + 0x30);
    U8(a, 0x30) = 0;
    U8(a, 0x31) = 0;
    U8(a, 0x32) = 0;
    U8(a, 0x33) = 2;
    U8(a, 0x34) = 0;
    U8(a, 0x35) = 0;
    U32(a, 0x38) = 0x43c80000; /* 400.0 */
    U32(a, 0x3C) = 0x45160000; /* 2400.0 */
    U32(a, 0x40) = 0x3f800000; /* 1.0 */
    U32(a, 0x44) = 0x3f400000; /* 0.75 */
    U32(a, 0x48) = 0;
    U32(a, 0x4C) = 0;
    /* parameters copied from the table of default values (0x08933A90 + offset) */
    for (i = 0; i < 20; i++)
        U32(a, off[i]) = *(const u32 *)(0x08933A90 + off[i]);
    U32(a, 0x50) = 0x3f5f66f3;
    U32(a, 0x54) = 0;
    U32(a, 0x58) = 0;
    S16(a, 0x5E) = 0;
    S16(a, 0x5C) = 0;
    U32(cam, 0xAAC) = (u32)a;
}

/* is point p inside quadrilateral q (in plan)? 0 = yes, 2 = no */
int camera_point_in_quad(void *cam, const f32 *q, const f32 *p)
{
    f32 x0 = q[0], z0 = q[2];
    f32 dx = p[0] - x0, dz = p[2] - z0;

    if ((z0 - q[6]) * dx - dz * (x0 - q[4]) < 0.0f)
        return 2;
    if ((q[7] - z0) * dx - dz * (q[5] - x0) < 0.0f)
        return 2;
    x0 = q[1];
    z0 = q[3];
    dx = p[0] - x0;
    dz = p[2] - z0;
    if ((q[6] - z0) * dx - dz * (q[4] - x0) < 0.0f)
        return 2;
    if ((z0 - q[7]) * dx - dz * (x0 - q[5]) < 0.0f)
        return 2;
    return 0;
}

/* is the player inside one of the parts of the zone? 0 = yes */
int camera_player_in_zone(void *cam, u8 *zone, u32 mask_arg)
{
    u8 mask = mask_arg;
    u8 *part;
    int n;

    if ((zone[6] & 0x80) && !func_088DA324(U32(cam, 0xA84)))
        return 2;
    part = (u8 *)U32(zone, 0x18);
    for (n = zone[5]; n > 0; n--, part += 0x40) {
        if ((part[0x2C] & mask) == 0) {
            u8 *pl = (u8 *)U32(cam, 0xA84);
            f32 d[3], h;

            d[0] = F32(pl, 0x200) - F32(part, 0x20);
            d[1] = F32(pl, 0x204) - F32(part, 0x24);
            d[2] = F32(pl, 0x208) - F32(part, 0x28);
            h = F32(part, 0x30) * d[0] + F32(part, 0x34) * d[1] + F32(part, 0x38) * d[2];
            if (0.0f <= h && h <= F32(part, 0x3C) &&
                camera_point_in_quad(cam, (f32 *)part, &F32(pl, 0x200)) == 0)
                return 0;
        }
    }
    return 2;
}

/* finds the camera zone where the player is */
int camera_find_zone(void *cam)
{
    u8 *area = (u8 *)U32(cam, 0xAAC);
    u8 *grid;
    int *cell, n;
    u32 *zone;

    if (area == NULL) {
        U32(cam, 0xA90) = 0;
        return -1;
    }
    U32(cam, 0xA90) = U32(area, 0x28);
    if (U8(cam, 0xB14) != 0)
        return 0;
    grid = (u8 *)U32(area, 0x1C);
    if (grid == NULL)
        return 0;
    cell = (int *)(grid + ((u32)*(u16 *)((u8 *)cam + 0xB16) +
                           (u32)*(u16 *)((u8 *)cam + 0xB1C) * *(u16 *)((u8 *)cam + 0xB18)) * 8);
    n = cell[0];
    if (n == 0)
        return 0;
    for (zone = (u32 *)cell[1]; n > 0; n--, zone++) {
        if (camera_player_in_zone(cam, (u8 *)*zone, 1) == 0) {
            U32(cam, 0xA90) = *zone;
            return 1;
        }
    }
    return 0;
}

void camera_set_view(f32 fov, void *cam, const u32 *eye, const u32 *target)
{
    int i;

    U32(cam, 0xB60) = 0;
    U32(cam, 0xB64) = 0x3f800000;
    U32(cam, 0xB68) = 0;
    U32(cam, 0xB6C) = 0;
    for (i = 0; i < 4; i++) {
        U32(cam, 0x20 + i * 4) = eye[i];
        U32(cam, 0x30 + i * 4) = target[i];
    }
    for (i = 0; i < 4; i++) {
        U32(cam, 0xB40 + i * 4) = eye[i];
        U32(cam, 0xB50 + i * 4) = target[i];
    }
    F32(cam, 0xA8) = fov;
    F32(cam, 0xC) = fov;
    camera_update(cam);
}

/* --- angles and per-frame update ------------------------------------------- */
#define PI_F 3.14159265f

/* atan2 (the original uses the VFPU instructions vasin/vrcp; here a polynomial, error ~1e-6) */
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
        r = PI_F / 2 - r;
    if (x < 0)
        r = PI_F - r;
    return y < 0 ? -r : r;
}

/* angolo 0..0xFFFF (65536 = giro completo) */
static inline u32 angle16(f32 rad)
{
    return (int)((rad * 65536.0f) / 6.2831855f + 0.5f) & 0xffff;
}

/* horizontal direction the camera looks in */
u32 camera_get_yaw(void *cam)
{
    f32 x = F32(cam, 0xB50) - F32(cam, 0xB40);
    f32 y = -(F32(cam, 0xB58) - F32(cam, 0xB48));

    return angle16(atan2_approx(y, x));
}

/* Moves eye, target and field of view towards the wanted values (1/9 per frame), recomputes
 * the matrices and the "billboard" matrix (rotation on the Y axis towards the camera). */
void camera_update(void *cam)
{
    f32 x, y, a, c, s;
    f32 *bb = &F32(cam, 0xCC0);
    int i;

    for (i = 0; i < 3; i++)
        F32(cam, 0xB50 + i * 4) += (F32(cam, 0x30 + i * 4) - F32(cam, 0xB50 + i * 4)) / 9.0f;
    for (i = 0; i < 3; i++)
        F32(cam, 0xB40 + i * 4) += (F32(cam, 0x20 + i * 4) - F32(cam, 0xB40 + i * 4)) / 9.0f;
    F32(cam, 0xC) += (F32(cam, 0xA8) - F32(cam, 0xC)) / 9.0f;
    camera_apply_projection(cam);
    camera_look_at(cam, &F32(cam, 0xB40), &F32(cam, 0xB50), &U32(cam, 0xB60));

    /* camera z axis in the world */
    x = (F32(cam, 0xCA0) + F32(cam, 0xCB0)) - F32(cam, 0xCB0);
    y = -((F32(cam, 0xCA8) + F32(cam, 0xCB8)) - F32(cam, 0xCB8));
    a = atan2_approx(y, x);
    a = ((((f32)((angle16(a) + 0x4000) & 0xffff) * 360.0f) / 65536.0f) / 360.0f) * 6.2831855f;
    c = cosf(a);
    s = sinf(a);
    for (i = 0; i < 16; i++)
        bb[i] = 0.0f;
    bb[0] = c;
    bb[2] = -s;
    bb[5] = 1.0f;
    bb[8] = s;
    bb[10] = c;
    bb[15] = 1.0f;
}

/* Projects the world point p onto the screen (out: x, y, z, w). */
void camera_world_to_screen(void *cam, f32 *out, const f32 *p)
{
    const f32 *v = &F32(cam, 0xBC0), *pr = &F32(cam, 0xC00);
    f32 s = *D_08A62C40, in[4], t[4];
    int r, k;

    for (k = 0; k < 4; k++)
        out[k] = p[k] * s;
    out[3] = 1.0f;
    for (k = 0; k < 4; k++)
        in[k] = out[k];
    for (r = 0; r < 4; r++)
        t[r] = v[r] * in[0] + v[4 + r] * in[1] + v[8 + r] * in[2] + v[12 + r] * in[3];
    for (r = 0; r < 4; r++)
        out[r] = pr[r] * t[0] + pr[4 + r] * t[1] + pr[8 + r] * t[2] + pr[12 + r] * t[3];
    if (-1e-05f < out[3] && out[3] < 1e-05f)
        out[3] = 1.0f;
    out[0] = F32(cam, 0xDAC) + (F32(cam, 0xDA0) * out[0]) / out[3];
    out[1] = F32(cam, 0xDB0) + (F32(cam, 0xDA4) * out[1]) / out[3];
    out[2] = F32(cam, 0xDB4) + (F32(cam, 0xDA8) * out[2]) / out[3];
    out[0] -= F32(cam, 0xB70);
    out[1] -= F32(cam, 0xB74);
}

/* Links the map's zone data: the offsets in the file become pointers. */
void camera_set_area(void *cam, u8 *area)
{
    u8 *zone;
    int n, count = 0;

    if (area == NULL) {
        U32(cam, 0xAAC) = 0;
        return;
    }
    U32(cam, 0xAAC) = (u32)area;
    zone = area + 0x30;
    U32(area, 0x28) = (u32)zone;
    for (n = *(u16 *)(area + 2); n > 0; n--) {
        if (U32(zone, 0x1C) == 0) {
            U32(zone, 0x18) += (u32)zone;
            zone += 0x300 + zone[5] * 0x40;
        } else {
            U32(zone, 0x18) += (u32)zone;
            U32(zone, 0x1C) += (u32)zone;
            zone += zone[5] * 0x40 + 800;
        }
    }
    if (U32(area, 0x1C) == 0) {
        U32(area, 0x1C) = 0;
    } else {
        int *cell = (int *)(area + U32(area, 0x1C));

        U32(area, 0x1C) = (u32)cell;
        for (n = (int)((u32)*(u16 *)(area + 4) * *(u16 *)(area + 6)); n > 0; n--, cell += 2) {
            if (cell[0] != 0) {
                cell[1] += (int)area;
                count += cell[0];
            }
        }
    }
    if (U32(area, 0x20) == 0) {
        U32(area, 0x20) = 0;
    } else {
        u32 *ptr = (u32 *)(area + U32(area, 0x20));

        U32(area, 0x20) = (u32)ptr;
        for (; count > 0; count--)
            *ptr++ += (u32)area;
    }
}

/* Chooses the active viewpoint (normal or special) and applies the shakes. */
void camera_select_view(void *cam)
{
    u8 *slot = (u8 *)cam;
    u8 *pre;
    s8 sel;
    int i;

    for (i = 0; i < 3; i++, slot += 0x20) {
        if (slot[0xAC0] != 0 && --*(s16 *)(slot + 0xAC2) < 1)
            slot[0xAC0] = 0;
    }
    pre = (u8 *)cam + U8(cam, 0xA8D) * 0x1A0;
    for (i = 0; i < 4; i++) {
        U32(cam, 0x20 + i * 4) = U32(pre, 0xB0 + i * 4);
        U32(cam, 0x30 + i * 4) = U32(pre, 0xC0 + i * 4);
    }
    U32(cam, 0xA0) = U32(pre, 0x130);
    U32(cam, 0xA8) = U32(pre, 0x138);
    U8(cam, 0xDC4) = 0xff;
    if (U8(cam, 0x961) != 0)
        U8(cam, 0xDC4) = 5;
    else if (U8(cam, 0x621) != 0)
        U8(cam, 0xDC4) = 3;
    else if (U8(cam, 0x7C1) != 0)
        U8(cam, 0xDC4) = 4;
    sel = (s8)U8(cam, 0xDC4);
    if (sel < 1) {
        for (i = 0; i < 4; i++) {
            U32(cam, 0xB40 + i * 4) = U32(cam, 0x20 + i * 4);
            U32(cam, 0xB50 + i * 4) = U32(cam, 0x30 + i * 4);
        }
        camera_apply_shake(cam, (u8 *)cam + 0xAB0);
        U32(cam, 0xB34) = U32(cam, 0xA0);
        U32(cam, 0xC) = U32(cam, 0xA8);
    } else {
        pre = (u8 *)cam + sel * 0x1A0;
        for (i = 0; i < 4; i++) {
            U32(cam, 0xB40 + i * 4) = U32(pre, 0xB0 + i * 4);
            U32(cam, 0xB50 + i * 4) = U32(pre, 0xC0 + i * 4);
        }
        sel = (s8)U8(cam, 0xDC4);
        if (sel == 5)
            camera_apply_shake(cam, (u8 *)cam + 0xAF0);
        else if (sel == 3)
            camera_apply_shake(cam, (u8 *)cam + 0xAD0);
        else if (sel == 4)
            camera_apply_shake(cam, (u8 *)cam + 0xAB0);
        U32(cam, 0xB34) = U32(pre, 0x130);
        U32(cam, 0xC) = U32(pre, 0x138);
        U8(cam, 0xA88) = 0;
    }
    camera_set_field_142(cam, (s8)U8(cam, 0xDC4));
}
