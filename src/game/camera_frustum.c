/* Camera: the planes of the view frustum and the object visibility test.
 * The fields are the named offsets of include/camera.h. (Guessed names.) */
#include "menu.h"
#include "camera.h"
#include "vfpu.h"

void func_08897DE0(f32 *out, const f32 *dir);
void func_08897D7C(f32 *mat, const f32 *in);

/* sets the wanted eye, target and field of view of the camera */
void camera_set_eye_target(u32 val, void *cam, const u32 *eye, const u32 *target)
{
    int i;

    for (i = 0; i < 4; i++)
        U32(cam, CAM_EYE_WANT + i * 4) = eye[i];
    for (i = 0; i < 4; i++)
        U32(cam, CAM_TARGET_WANT + i * 4) = target[i];
    U32(cam, CAM_FOV_WANT) = val;
}

/* "up" direction (column Y of the matrix built from the from->to direction) */
void camera_up_from_dir(f32 *out, const f32 *from, const f32 *to)
{
    f32 dir[4], tmp[8], mat[16];
    int i;

    dir[0] = to[0] - from[0];
    dir[1] = to[1] - from[1];
    dir[2] = to[2] - from[2];
    dir[3] = 1.0f;
    func_08897DE0(tmp, dir);
    func_08897D7C(mat, tmp);
    for (i = 0; i < 3; i++)
        out[i] = mat[i * 4 + 0] * 0.0f + mat[i * 4 + 1] * 1.0f + mat[i * 4 + 2] * 0.0f;
    out[3] = 1.0f;
}

static f32 clamp1(f32 x)
{
    if (x > 1.0f)
        return 1.0f;
    if (x < -1.0f)
        return -1.0f;
    return x;
}

/* normal (normalized) of the plane through points a, b, c */
void camera_plane_normal(void *cam, f32 *out, const f32 *a, const f32 *b, const f32 *c)
{
    f32 u0 = b[0] - a[0], u1 = b[1] - a[1], u2 = b[2] - a[2];
    f32 v0 = c[0] - a[0], v1 = c[1] - a[1], v2 = c[2] - a[2];
    f32 n0 = u1 * v2 - u2 * v1;
    f32 n1 = u2 * v0 - u0 * v2;
    f32 n2 = u0 * v1 - u1 * v0;
    f32 d = n0 * n0 + n1 * n1 + n2 * n2;
    f32 s = d == 0.0f ? 0.0f : vfpu_rsqf(d);

    out[0] = clamp1(n0 * s);
    out[1] = clamp1(n1 * s);
    out[2] = clamp1(n2 * s);
    out[3] = 0.0f;
}

/* computes the four side planes of the field of view (CAM_PLANES) and the depth limits
 * (CAM_DEPTH_NEAR / FAR) from near, far, aspect and fov (cam[0..3]) */
void camera_build_frustum(f32 *cam)
{
    f32 zero[4] = { 0, 0, 0, 0 };
    f32 b[4], c[4];
    f32 far = cam[1], half, co, si, cot, t;

    b[2] = c[2] = 1.5f * -far;
    half = 0.5f * cam[3];
    co = vfpu_cosf(half);
    si = vfpu_sinf(half);
    cot = co / si;
    b[0] = c[0] = -far * cot;
    t = (far * cot) / cam[2];
    b[1] = t;
    c[1] = -t;
    b[3] = c[3] = 0.0f;
    camera_plane_normal(cam, cam + CAM_PLANES / 4, zero, b, c);

    cot = co / si;
    b[0] = c[0] = cam[1] * cot;
    t = (-cam[1] * cot) / cam[2];
    b[1] = t;
    c[1] = -t;
    camera_plane_normal(cam, cam + (CAM_PLANES + 0x10) / 4, zero, b, c);

    t = cam[1] * cot;
    b[0] = t;
    c[0] = -t;
    b[1] = c[1] = t / cam[2];
    camera_plane_normal(cam, cam + (CAM_PLANES + 0x20) / 4, zero, b, c);

    t = -cam[1] * cot;
    b[0] = t;
    c[0] = -t;
    b[1] = c[1] = t / cam[2];
    camera_plane_normal(cam, cam + (CAM_PLANES + 0x30) / 4, zero, b, c);

    cam[CAM_DEPTH_FAR / 4] = -cam[1];
    cam[CAM_DEPTH_NEAR / 4] = -cam[0];
}

/* depth of a point in camera space (row Z of the view matrix) */
static inline f32 view_z(void *cam, const f32 *p)
{
    return F32(cam, CAM_VIEW_MTX + 0x38) + (F32(cam, CAM_VIEW_MTX + 0x28) * p[2] + (F32(cam, CAM_VIEW_MTX + 0x8) * p[0] + F32(cam, CAM_VIEW_MTX + 0x18) * p[1]));
}

/* is the sphere (p, r) between the near and far planes? */
int camera_sphere_in_depth(f32 r, void *cam, const f32 *p)
{
    f32 d;

    if (U8(cam, CAM_CULLING) == 0)
        return 1;
    d = view_z(cam, p);
    if (d - r > F32(cam, CAM_DEPTH_NEAR))
        return 0;
    return !(d + r < F32(cam, CAM_DEPTH_FAR));
}

/* is the sphere (p, r) (at least partly) inside the field of view? */
int camera_sphere_visible(f32 r, void *cam, const f32 *p)
{
    f32 d, x, y;

    if (U8(cam, CAM_CULLING) == 0)
        return 1;
    d = view_z(cam, p);
    if (d - r > F32(cam, CAM_DEPTH_NEAR))
        return 0;
    if (d + r < F32(cam, CAM_DEPTH_FAR))
        return 0;
    x = F32(cam, CAM_VIEW_MTX + 0x30) + (F32(cam, CAM_VIEW_MTX + 0x20) * p[2] + (F32(cam, CAM_VIEW_MTX) * p[0] + F32(cam, CAM_VIEW_MTX + 0x10) * p[1]));
    if (x * F32(cam, CAM_PLANES) + d * F32(cam, CAM_PLANES + 0x8) > r)
        return 0;
    if (x * F32(cam, CAM_PLANES + 0x10) + d * F32(cam, CAM_PLANES + 0x18) > r)
        return 0;
    y = F32(cam, CAM_VIEW_MTX + 0x34) + (F32(cam, CAM_VIEW_MTX + 0x24) * p[2] + (F32(cam, CAM_VIEW_MTX + 0x4) * p[0] + F32(cam, CAM_VIEW_MTX + 0x14) * p[1]));
    if (y * F32(cam, CAM_PLANES + 0x24) + d * F32(cam, CAM_PLANES + 0x28) > r)
        return 0;
    return !(y * F32(cam, CAM_PLANES + 0x34) + d * F32(cam, CAM_PLANES + 0x38) > r);
}

/* viewport parameters (screen centre 240,-136, far/2 depth, 2048) */
void func_08816264(void *cam)
{
    f32 h = F32(cam, CAM_FAR) * 0.5f;

    U32(cam, CAM_SCREEN_SCALE) = 0x43700000;   /* 240.0 */
    U32(cam, CAM_SCREEN_SCALE + 4) = 0xc3080000;   /* -136.0 */
    F32(cam, CAM_SCREEN_SCALE + 8) = h;
    U32(cam, CAM_SCREEN_CENTER) = 0x45000000;   /* 2048.0 */
    U32(cam, CAM_SCREEN_CENTER + 4) = 0x45000000;
    F32(cam, CAM_SCREEN_CENTER + 8) = h;
}
