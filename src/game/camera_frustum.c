/* Telecamera: piani del "campo visivo" (frustum) e test di visibilità degli oggetti.
 * Stessa struttura (non ancora ricostruita) di camera.c: campi indicati per offset.
 * (Nomi ipotetici.) */
#include "common.h"
#include "vfpu.h"

#define F32(p, off) (*(f32 *)((u8 *)(p) + (off)))
#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define U8(p, off)  (*(u8 *)((u8 *)(p) + (off)))

void func_08897DE0(f32 *out, const f32 *dir);
void func_08897D7C(f32 *mat, const f32 *in);

/* imposta occhio (0x20) e bersaglio (0x30) della telecamera */
void camera_set_eye_target(u32 val, void *cam, const u32 *eye, const u32 *target)
{
    int i;

    for (i = 0; i < 4; i++)
        U32(cam, 0x20 + i * 4) = eye[i];
    for (i = 0; i < 4; i++)
        U32(cam, 0x30 + i * 4) = target[i];
    U32(cam, 0xA8) = val;
}

/* direzione "in alto" (colonna Y della matrice costruita dalla direzione from->to) */
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

/* normale (normalizzata) del piano che passa per i punti a, b, c */
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

/* calcola i quattro piani laterali del campo visivo (0xD50..0xD8F) e i limiti
 * vicino/lontano (0xD90/0xD94) da vicino (0x0), lontano (0x4), aspetto (0x8), fov (0xC) */
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
    camera_plane_normal(cam, cam + 0xD50 / 4, zero, b, c);

    cot = co / si;
    b[0] = c[0] = cam[1] * cot;
    t = (-cam[1] * cot) / cam[2];
    b[1] = t;
    c[1] = -t;
    camera_plane_normal(cam, cam + 0xD60 / 4, zero, b, c);

    t = cam[1] * cot;
    b[0] = t;
    c[0] = -t;
    b[1] = c[1] = t / cam[2];
    camera_plane_normal(cam, cam + 0xD70 / 4, zero, b, c);

    t = -cam[1] * cot;
    b[0] = t;
    c[0] = -t;
    b[1] = c[1] = t / cam[2];
    camera_plane_normal(cam, cam + 0xD80 / 4, zero, b, c);

    cam[0xD94 / 4] = -cam[1];
    cam[0xD90 / 4] = -cam[0];
}

/* profondità di un punto nello spazio della telecamera (riga Z della matrice a 0xB80) */
static inline f32 view_z(void *cam, const f32 *p)
{
    return F32(cam, 0xBB8) + (F32(cam, 0xBA8) * p[2] + (F32(cam, 0xB88) * p[0] + F32(cam, 0xB98) * p[1]));
}

/* la sfera (p, r) è fra il piano vicino e quello lontano? */
int camera_sphere_in_depth(f32 r, void *cam, const f32 *p)
{
    f32 d;

    if (U8(cam, 0xAAA) == 0)
        return 1;
    d = view_z(cam, p);
    if (d - r > F32(cam, 0xD90))
        return 0;
    return !(d + r < F32(cam, 0xD94));
}

/* la sfera (p, r) è (almeno in parte) dentro il campo visivo? */
int camera_sphere_visible(f32 r, void *cam, const f32 *p)
{
    f32 d, x, y;

    if (U8(cam, 0xAAA) == 0)
        return 1;
    d = view_z(cam, p);
    if (d - r > F32(cam, 0xD90))
        return 0;
    if (d + r < F32(cam, 0xD94))
        return 0;
    x = F32(cam, 0xBB0) + (F32(cam, 0xBA0) * p[2] + (F32(cam, 0xB80) * p[0] + F32(cam, 0xB90) * p[1]));
    if (x * F32(cam, 0xD50) + d * F32(cam, 0xD58) > r)
        return 0;
    if (x * F32(cam, 0xD60) + d * F32(cam, 0xD68) > r)
        return 0;
    y = F32(cam, 0xBB4) + (F32(cam, 0xBA4) * p[2] + (F32(cam, 0xB84) * p[0] + F32(cam, 0xB94) * p[1]));
    if (y * F32(cam, 0xD74) + d * F32(cam, 0xD78) > r)
        return 0;
    return !(y * F32(cam, 0xD84) + d * F32(cam, 0xD88) > r);
}

/* parametri del viewport (centro schermo 240,-136, profondità far/2, 2048) */
void func_08816264(void *cam)
{
    f32 h = F32(cam, 0x4) * 0.5f;

    U32(cam, 0xDA0) = 0x43700000;   /* 240.0 */
    U32(cam, 0xDA4) = 0xc3080000;   /* -136.0 */
    F32(cam, 0xDA8) = h;
    U32(cam, 0xDAC) = 0x45000000;   /* 2048.0 */
    U32(cam, 0xDB0) = 0x45000000;
    F32(cam, 0xDB4) = h;
}
