/* Camera: initial settings, shakes, transitions, matrices and zones.
 * The fields are the named offsets of include/camera.h. (Guessed names.) */
#include "game.h"
#include "camera.h"

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
void camera_load_area_limits(void *cam);
void func_08812BD0(void *cam);
u32 player_get(void *a, u8 b);
int entity_in_area(void *obj);
extern void *g_players;
extern u8 *g_game;

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
    U32(cam, CAM_NEAR) = 0x41f00000;   /* 30.0 */
    U32(cam, CAM_FAR) = 0x477de800;   /* 65000.0 */
    U32(cam, CAM_FOV) = 0x3f5f49f5;
    U32(cam, CAM_ASPECT) = 0x3fe1e1e2;   /* 480/272 */
    U32(cam, CAM_ROLL) = 0;
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
    U32(cam, CAM_NEAR) = 0x41f00000;
    U32(cam, CAM_FAR) = 0x477de800;
    U32(cam, CAM_FOV) = 0x3f5f49f5;
    U32(cam, CAM_ASPECT) = 0x3fe1e1e2;
    func_08816264(cam);
    camera_look_at(cam, (f32 *)a, (f32 *)zero, b);
    U32(cam, CAM_XDC0) = 0;
    U8(cam, CAM_XAA9) = 0;
    camera_reset_projection(cam);
    U8(cam, CAM_X014) = 3;
    U8(cam, CAM_MODE) = 0;
    U32(cam, CAM_MODE_TARGET) = 0;
}

void camera_reset_state(void *cam)
{
    U32(cam, CAM_XB10) = 0;
    U8(cam, CAM_XB3C) = 1;
    camera_reset_projection(cam);
    U32(cam, CAM_XB10) = 0;
    U32(cam, CAM_PLAYER) = player_get(g_players, G_MY_SLOT);
    U8(cam, CAM_SHAKE + 2 * SHAKE_SIZE + SHAKE_ACTIVE) = 0;
    U8(cam, CAM_SHAKE + SHAKE_SIZE + SHAKE_ACTIVE) = 0;
    U8(cam, CAM_SHAKE + SHAKE_ACTIVE) = 0;
    U8(cam, CAM_XA81) = 0;
    U8(cam, CAM_XA8A) = 0;
    U8(cam, CAM_XA88) = 0;
    U32(cam, CAM_MODE_TARGET) = 0;
    U8(cam, CAM_MODE) = 0;
    camera_load_area_limits(cam);
    func_08812BD0(cam);
}

/* Adds to the camera the vertical offset of a shake: a wave that
 * decays with time and with distance from the source. */
void camera_apply_shake(void *cam, u8 *shake)
{
    u32 type;
    f32 amp, wave, falloff;

    if (shake[SHAKE_ACTIVE] == 0)
        return;
    type = shake[SHAKE_TYPE] & 0x7f;
    amp = type < 2 ? 10.0f : 12.0f;
    wave = sinf((((f32)S16(shake, SHAKE_TIMER) / (f32)D_08933A98[type]) * 90.0f) / 360.0f * 6.2831855f);
    if ((shake[SHAKE_TYPE] & 0x80) == 0) {
        f32 d = dist3((f32 *)shake, &F32(cam, CAM_EYE));
        f32 r = D_08933AA8[shake[SHAKE_RANGE]];

        falloff = r <= d ? 0.0f : 1.0f - d / r;
    } else {
        falloff = 1.0f;
    }
    amp = amp * wave * falloff;
    if (S16(shake, SHAKE_TIMER) & 2)
        amp = -amp;
    F32(cam, CAM_EYE + 4) += amp;
    F32(cam, CAM_TARGET + 4) += amp;
}

/* starts a shake of type "type", from the position of object obj */
static inline void start_shake(void *cam, int base, int type, u8 *obj, u8 flag)
{
    if (entity_in_area(obj)) {
        U8(cam, base + SHAKE_ACTIVE) = 1;
        U8(cam, base + SHAKE_TYPE) = type;
        S16(cam, base + SHAKE_TIMER) = D_08933A98[type];
        U32(cam, base + SHAKE_POS + 0x0) = U32(obj, 0x200);    /* position of the entity */
        U32(cam, base + SHAKE_POS + 0x4) = U32(obj, 0x204);
        U32(cam, base + SHAKE_POS + 0x8) = U32(obj, 0x208);
        U32(cam, base + SHAKE_POS + 0xC) = U32(obj, 0x20C);
        U8(cam, base + SHAKE_RANGE) = flag;
    }
}

void camera_start_shake(void *cam, int type, u8 *obj)
{
    start_shake(cam, CAM_SHAKE, type, obj, 0);
}

void camera_start_shake_ex(void *cam, int type, u8 *obj, u32 flag_arg)
{
    u8 flag = flag_arg;
    start_shake(cam, CAM_SHAKE, type, obj, flag);
}

void camera_start_shake2(void *cam, int type, u8 *obj)
{
    start_shake(cam, CAM_SHAKE + SHAKE_SIZE, type, obj, 0);
}

/* step of a transition between camera points */
void camera_transition_step(void *cam, int finish)
{
    u8 *t = (u8 *)cam + CAM_TRANS;
    int i;

    U8(cam, CAM_TRANS_STEP) = 3;
    S16(cam, CAM_TRANS_TIME) = 0xf;
    if (finish == 0) {
        i = U32(cam, CAM_TRANS_COUNT) - 1;
        if (t[U32(cam, CAM_TRANS_COUNT) + 0x30] == 5 && S16(cam, CAM_TRANS_TIME2) != 0) {
            f32 d1 = dist3(&F32(CAM_VIEW(cam, 4), VIEW_EYE), (f32 *)(t + i * 0x10 + 0x70));
            f32 d2 = dist3(&F32(CAM_VIEW(cam, 4), VIEW_TARGET), (f32 *)(t + i * 0x10 + 0xA0));
            s16 n = (s16)(int)(D_08933A90 * (d1 + d2)) + 1;

            S16(cam, CAM_TRANS_TIME2) = n;
            S16(cam, CAM_TRANS_TIME) = n;
        }
    } else {
        i = U32(cam, CAM_TRANS_COUNT) - 1;
        U32(cam, CAM_TRANS_COUNT) = i;
        if (i == 0) {
            *t = 0;
            U8(cam, CAM_TRANS_STEP) = 0;
            S16(cam, CAM_TRANS_TIME2) = 0;
        } else {
            *t = 1;
            U8(cam, CAM_TRANS_STEP) = 2;
            S16(cam, CAM_TRANS_TIME2) = 0xf;
            S16(cam, CAM_TRANS_TIME) = 0xf;
        }
    }
}

/* --- special camera modes ---------------------------------------------- */

void camera_set_mode(void *cam, u32 mode_arg, u32 target, u32 arg_arg)
{
    u8 mode = mode_arg;
    u8 arg = arg_arg;
    U8(cam, CAM_CULLING) = 0;
    U8(cam, CAM_MODE) = mode;
    U32(cam, CAM_MODE_TARGET) = target;
    U8(cam, CAM_MODE_ARG) = arg;
}

/* like camera_set_mode, but only if there is no mode already */
void camera_request_mode(void *cam, u32 mode_arg, u32 target)
{
    u8 mode = mode_arg;
    U8(cam, CAM_CULLING) = 0;
    if (U8(cam, CAM_MODE) == 0) {
        U8(cam, CAM_MODE) = mode;
        U32(cam, CAM_MODE_TARGET) = target;
    }
}

int camera_is_cutscene_active(void *cam)
{
    u8 s = U8(cam, CAM_CUTSCENE_ID);

    return s == 5 || s == 4 || s == 3;
}

void camera_set_mode2(void *cam)
{
    if (U8(cam, CAM_VIEW_SEL) != 2)
        camera_set_mode(cam, 2, 0, 0);
}

void camera_set_mode1(void *cam)
{
    if (G_MAP != 12)
        camera_set_mode(cam, 1, 0, 0);
}

/* camera mode according to the current zone */
void camera_mode_for_area(void *cam)
{
    s16 area = *(s16 *)(g_game + 0x6AF0E);
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
    U8(cam, CAM_X968) = 0;
    S16(cam, CAM_CUTSCENE_TIMER) = 0;
    U8(cam, CAM_CUTSCENE_ID) = 0;
    U8(cam, CAM_CUTSCENE_STATE) = 0;
    U32(cam, CAM_X9C0) = 0;
    U8(cam, CAM_XA0B) = 0;
    U8(cam, CAM_CULLING) = 1;
    U8(cam, CAM_MODE) = 0;
    U8(cam, CAM_CUTSCENE_FLAG) = 0;
}

int camera_cutscene_state(void *cam)
{
    if (U8(cam, CAM_CUTSCENE_STATE) != 0)
        return (s8)U8(cam, CAM_CUTSCENE_STATE);
    if (U8(cam, CAM_MODE) != 0 || U8(cam, CAM_CUTSCENE_ID) != 0)
        return 2;
    return 0;
}

u8 camera_cutscene_id(void *cam)
{
    return U8(cam, CAM_CUTSCENE_ID);
}

u8 camera_cutscene_flag(void *cam)
{
    return U8(cam, CAM_CUTSCENE_FLAG);
}

int camera_is_special_cutscene(void *cam)
{
    u8 id;

    if (U8(cam, CAM_CUTSCENE_ON) == 0)
        return 0;
    id = U8(cam, CAM_CUTSCENE_ID);
    return id >= 0x50 && id <= 0x69;
}

int camera_cutscene_timer(void *cam)
{
    if (G_TIME_STOP == 0)
        return -1;
    return S16(cam, CAM_CUTSCENE_TIMER);
}

int camera_blocks_input(void *cam)
{
    if (G_TIME_STOP != 1 && U8(cam, CAM_CUTSCENE_FLAG) == 0) {
        switch (U8(cam, CAM_CUTSCENE_ID)) {
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
    if (U8(cam, CAM_CUTSCENE_ON) != 0)
        return 0;
    return player[0x564] != 0 || U8(cam, CAM_VIEW3_ACTIVE) != 0;
}

int camera_get_zoom(void *cam, u8 *out_a, u8 *out_b, f32 *out_f)
{
    *out_a = U8(cam, CAM_ZOOM_A);
    *out_b = U8(cam, CAM_ZOOM_ON);
    if (U8(cam, CAM_ZOOM_ON) != 0) {
        f32 f = F32(cam, CAM_VIEW3_FOV) - F32(cam, CAM_ZOOM_BASE);
        *out_f = f;
        *out_f = f * F32(cam, CAM_ZOOM_SCALE);
    }
    return (s8)U8(cam, CAM_ZOOM_STATE);
}

void camera_get_eye(void *cam, f32 *out)
{
    out[0] = F32(cam, CAM_EYE);
    out[1] = F32(cam, CAM_EYE + 4);
    out[2] = F32(cam, CAM_EYE + 8);
    U32(out, 12) = 0;
}

void camera_get_target(void *cam, f32 *out)
{
    out[0] = F32(cam, CAM_TARGET);
    out[1] = F32(cam, CAM_TARGET + 4);
    out[2] = F32(cam, CAM_TARGET + 8);
    U32(out, 12) = 0;
}

/* --- camera matrices -------------------------------------------------------- */
f32 cosf(f32 x);
void mat_look_at(f32 *m, const f32 *eye, const f32 *target, const f32 *up);
void camera_build_frustum(void *cam);
void camera_default_area(void *cam);
int game_quest_locked(void *a, int b);
extern s32 D_08A62C68, g_pad_pressed;
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
        U32(cam, CAM_EYE + i * 4) = ((const u32 *)eye)[i];
        U32(cam, CAM_TARGET + i * 4) = ((const u32 *)target)[i];
        U32(cam, CAM_UP + i * 4) = up[i];
    }
    dx = target[0] - eye[0];
    dy = target[1] - eye[1];
    dz = target[2] - eye[2];
    d2 = dx * dx + dy * dy + dz * dz;
    F32(cam, CAM_DISTANCE) = d2;
    F32(cam, CAM_DISTANCE) = fsqrt(d2); /* eye-to-target distance */
    mat_look_at(&F32(cam, CAM_VIEW_MTX), eye, target, (const f32 *)up);
    s = *D_08A62C40;
    for (i = 0; i < 4; i++) {
        se[i] = eye[i] * s;
        st[i] = target[i] * s;
    }
    mat_look_at(&F32(cam, CAM_VIEW_MTX_S), se, st, (const f32 *)up);
    mat_inverse_ortho(&F32(cam, CAM_VIEW_MTX), &F32(cam, CAM_INV_VIEW));
    mat_inverse_ortho(&F32(cam, CAM_VIEW_MTX_S), &F32(cam, CAM_INV_VIEW_S));
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
    camera_perspective(fovy, aspect, znear, zfar, cam, &F32(cam, CAM_PROJ_MTX));
    mat_inverse_ortho(&F32(cam, CAM_PROJ_MTX), &F32(cam, CAM_INV_PROJ));
    camera_build_frustum(cam);
}

void camera_apply_projection(void *cam)
{
    camera_update_projection(F32(cam, CAM_FOV), F32(cam, CAM_ASPECT), F32(cam, CAM_NEAR), F32(cam, CAM_FAR), cam);
    U32(cam, CAM_FOV_APPLIED) = U32(cam, CAM_FOV);
}

/* copies the limits of the current game zone */
void camera_load_area_limits(void *cam)
{
    u8 *a;

    if (U32(cam, CAM_AREA) == 0)
        camera_default_area(cam);
    a = (u8 *)U32(cam, CAM_AREA);
    S16(cam, CAM_LIMITS) = S16(a, 4);
    S16(cam, CAM_LIMITS + 2) = S16(a, 6);
    S16(cam, CAM_LIMITS + 4) = S16(a, 8);
    S16(cam, CAM_LIMITS + 6) = S16(a, 10);
    U32(cam, CAM_LIMITS + 0x8) = U32(a, 0xC);
    U32(cam, CAM_LIMITS + 0xC) = U32(a, 0x10);
    U32(cam, CAM_LIMITS + 0x10) = U32(a, 0x14);
    U32(cam, CAM_LIMITS + 0x14) = U32(a, 0x18);
}

/* reads the stick/buttons that move the camera */
void camera_read_input(void *cam)
{
    if (game_quest_locked(g_game, 1) != 1) {
        if (((u8 *)U32(cam, CAM_PLAYER))[0x552] == 0) {
            S16(cam, CAM_STICK_X) = (s16)D_08A62C68;
            S16(cam, CAM_PAD) = (s16)g_pad_pressed;
            S16(cam, CAM_STICK2_X) = D_08A62C78;
            S16(cam, CAM_STICK2_Y) = D_08A62C7A;
            return;
        }
    }
    S16(cam, CAM_XA74) = 0;
    S16(cam, CAM_PAD) = 0;
    S16(cam, CAM_STICK_X) = 0;
    S16(cam, CAM_STICK2_Y) = 0;
    S16(cam, CAM_STICK2_X) = 0;
}

void camera_set_field_142(void *cam, int v)
{
    if (v == 5)
        v = -1;
    if (v < 0)
        v = 0xff;
    U8(cam, CAM_X142) = v;
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
    u8 *a = (u8 *)U32(cam, CAM_AREA);
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
    U32(cam, CAM_AREA) = (u32)a;
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

    if ((zone[6] & 0x80) && !func_088DA324(U32(cam, CAM_PLAYER)))
        return 2;
    part = (u8 *)U32(zone, 0x18);
    for (n = zone[5]; n > 0; n--, part += 0x40) {
        if ((part[0x2C] & mask) == 0) {
            u8 *pl = (u8 *)U32(cam, CAM_PLAYER);
            f32 d[3], h;

            d[0] = F32(pl, E_POS) - F32(part, 0x20);
            d[1] = F32(pl, E_POS + 4) - F32(part, 0x24);
            d[2] = F32(pl, E_POS + 8) - F32(part, 0x28);
            h = F32(part, 0x30) * d[0] + F32(part, 0x34) * d[1] + F32(part, 0x38) * d[2];
            if (0.0f <= h && h <= F32(part, 0x3C) &&
                camera_point_in_quad(cam, (f32 *)part, &F32(pl, E_POS)) == 0)
                return 0;
        }
    }
    return 2;
}

/* finds the camera zone where the player is */
int camera_find_zone(void *cam)
{
    u8 *area = (u8 *)U32(cam, CAM_AREA);
    u8 *grid;
    int *cell, n;
    u32 *zone;

    if (area == NULL) {
        U32(cam, CAM_ZONE) = 0;
        return -1;
    }
    U32(cam, CAM_ZONE) = U32(area, 0x28);
    if (U8(cam, CAM_ZONE_LOCK) != 0)
        return 0;
    grid = (u8 *)U32(area, 0x1C);
    if (grid == NULL)
        return 0;
    cell = (int *)(grid + ((u32)*(u16 *)((u8 *)cam + CAM_GRID_X) +
                           (u32)*(u16 *)((u8 *)cam + CAM_LIMITS) * *(u16 *)((u8 *)cam + CAM_GRID_Z)) * 8);
    n = cell[0];
    if (n == 0)
        return 0;
    for (zone = (u32 *)cell[1]; n > 0; n--, zone++) {
        if (camera_player_in_zone(cam, (u8 *)*zone, 1) == 0) {
            U32(cam, CAM_ZONE) = *zone;
            return 1;
        }
    }
    return 0;
}

void camera_set_view(f32 fov, void *cam, const u32 *eye, const u32 *target)
{
    int i;

    U32(cam, CAM_UP) = 0;
    U32(cam, CAM_UP + 4) = 0x3f800000;
    U32(cam, CAM_UP + 8) = 0;
    U32(cam, CAM_UP + 12) = 0;
    for (i = 0; i < 4; i++) {
        U32(cam, CAM_EYE_WANT + i * 4) = eye[i];
        U32(cam, CAM_TARGET_WANT + i * 4) = target[i];
    }
    for (i = 0; i < 4; i++) {
        U32(cam, CAM_EYE + i * 4) = eye[i];
        U32(cam, CAM_TARGET + i * 4) = target[i];
    }
    F32(cam, CAM_FOV_WANT) = fov;
    F32(cam, CAM_FOV) = fov;
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
    f32 x = F32(cam, CAM_TARGET) - F32(cam, CAM_EYE);
    f32 y = -(F32(cam, CAM_TARGET + 8) - F32(cam, CAM_EYE + 8));

    return angle16(atan2_approx(y, x));
}

/* Moves eye, target and field of view towards the wanted values (1/9 per frame), recomputes
 * the matrices and the "billboard" matrix (rotation on the Y axis towards the camera). */
void camera_update(void *cam)
{
    f32 x, y, a, c, s;
    f32 *bb = &F32(cam, CAM_BILLBOARD);
    int i;

    for (i = 0; i < 3; i++)
        F32(cam, CAM_TARGET + i * 4) += (F32(cam, CAM_TARGET_WANT + i * 4) - F32(cam, CAM_TARGET + i * 4)) / 9.0f;
    for (i = 0; i < 3; i++)
        F32(cam, CAM_EYE + i * 4) += (F32(cam, CAM_EYE_WANT + i * 4) - F32(cam, CAM_EYE + i * 4)) / 9.0f;
    F32(cam, CAM_FOV) += (F32(cam, CAM_FOV_WANT) - F32(cam, CAM_FOV)) / 9.0f;
    camera_apply_projection(cam);
    camera_look_at(cam, &F32(cam, CAM_EYE), &F32(cam, CAM_TARGET), &U32(cam, CAM_UP));

    /* camera z axis in the world */
    x = (F32(cam, CAM_INV_VIEW + 0x20) + F32(cam, CAM_INV_VIEW + 0x30)) - F32(cam, CAM_INV_VIEW + 0x30);
    y = -((F32(cam, CAM_INV_VIEW + 0x28) + F32(cam, CAM_INV_VIEW + 0x38)) - F32(cam, CAM_INV_VIEW + 0x38));
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
    const f32 *v = &F32(cam, CAM_VIEW_MTX_S), *pr = &F32(cam, CAM_PROJ_MTX);
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
    out[0] = F32(cam, CAM_SCREEN_CENTER) + (F32(cam, CAM_SCREEN_SCALE) * out[0]) / out[3];
    out[1] = F32(cam, 0xDB0) + (F32(cam, 0xDA4) * out[1]) / out[3];
    out[2] = F32(cam, 0xDB4) + (F32(cam, 0xDA8) * out[2]) / out[3];
    out[0] -= F32(cam, CAM_SCREEN_SHIFT);
    out[1] -= F32(cam, 0xB74);
}

/* Links the map's zone data: the offsets in the file become pointers. */
void camera_set_area(void *cam, u8 *area)
{
    u8 *zone;
    int n, count = 0;

    if (area == NULL) {
        U32(cam, CAM_AREA) = 0;
        return;
    }
    U32(cam, CAM_AREA) = (u32)area;
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
        if (slot[CAM_SHAKE + SHAKE_ACTIVE] != 0 && --*(s16 *)(slot + CAM_SHAKE + SHAKE_TIMER) < 1)
            slot[CAM_SHAKE + SHAKE_ACTIVE] = 0;
    }
    pre = CAM_VIEW(cam, U8(cam, CAM_VIEW_SEL));
    for (i = 0; i < 4; i++) {
        U32(cam, CAM_EYE_WANT + i * 4) = U32(pre, VIEW_EYE + i * 4);
        U32(cam, CAM_TARGET_WANT + i * 4) = U32(pre, VIEW_TARGET + i * 4);
    }
    U32(cam, CAM_X0A0) = U32(pre, VIEW_X130);
    U32(cam, CAM_FOV_WANT) = U32(pre, VIEW_FOV);
    U8(cam, CAM_ACTIVE_VIEW) = 0xff;
    if (U8(cam, CAM_CUTSCENE_ON) != 0)
        U8(cam, CAM_ACTIVE_VIEW) = 5;
    else if (U8(cam, CAM_VIEW3_ACTIVE) != 0)
        U8(cam, CAM_ACTIVE_VIEW) = 3;
    else if (U8(cam, CAM_VIEW4_ACTIVE) != 0)
        U8(cam, CAM_ACTIVE_VIEW) = 4;
    sel = (s8)U8(cam, CAM_ACTIVE_VIEW);
    if (sel < 1) {
        for (i = 0; i < 4; i++) {
            U32(cam, CAM_EYE + i * 4) = U32(cam, CAM_EYE_WANT + i * 4);
            U32(cam, CAM_TARGET + i * 4) = U32(cam, CAM_TARGET_WANT + i * 4);
        }
        camera_apply_shake(cam, (u8 *)cam + CAM_SHAKE);
        U32(cam, CAM_ROLL) = U32(cam, CAM_X0A0);
        U32(cam, CAM_FOV) = U32(cam, CAM_FOV_WANT);
    } else {
        pre = CAM_VIEW(cam, sel);
        for (i = 0; i < 4; i++) {
            U32(cam, CAM_EYE + i * 4) = U32(pre, VIEW_EYE + i * 4);
            U32(cam, CAM_TARGET + i * 4) = U32(pre, VIEW_TARGET + i * 4);
        }
        sel = (s8)U8(cam, CAM_ACTIVE_VIEW);
        if (sel == 5)
            camera_apply_shake(cam, (u8 *)cam + CAM_SHAKE + 2 * SHAKE_SIZE);
        else if (sel == 3)
            camera_apply_shake(cam, (u8 *)cam + CAM_SHAKE + SHAKE_SIZE);
        else if (sel == 4)
            camera_apply_shake(cam, (u8 *)cam + CAM_SHAKE);
        U32(cam, CAM_ROLL) = U32(pre, VIEW_X130);
        U32(cam, CAM_FOV) = U32(pre, VIEW_FOV);
        U8(cam, CAM_XA88) = 0;
    }
    camera_set_field_142(cam, (s8)U8(cam, CAM_ACTIVE_VIEW));
}
