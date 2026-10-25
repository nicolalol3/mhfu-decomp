/* 3D model objects (vtables D_089BAB18 / D_089BAB28 / D_089BAB34) and their drawing with
 * GE state commands around. (Guessed names.) */
#include "menu.h"

extern u8 D_089BAB18[], D_089BAB28[], D_089BAB34[];
extern u32 *D_089CC000;           /* current GE command pointer */
void func_088626DC(void *o);
void func_08862D34(u8 *m, u32 a, u32 b);
void func_08862354(void *a, void *b);
void func_08862BB8(u8 *m, u32 a, u32 b);
void func_088629DC(u8 *m, u32 a, u32 r, u32 g, u32 b);
void func_08862800(u8 *m, u32 a, u32 r, u32 g, u32 b);
void model_draw_mesh(u32 *m, int unused, u8 *tex, u32 i_arg);
#define func_0886171C(m, a, b, c) model_draw_mesh((u32 *)(m), a, (u8 *)(b), c)
void *memcpy(void *d, const void *s, size_t n);

#define GE(v) (*D_089CC000++ = (u32)(v))

u32 *model_ctor(u32 *o)
{
    u8 *b = (u8 *)o;

    o[0] = (u32)D_089BAB34;
    o[0] = (u32)D_089BAB18;
    o[0x20] = (u32)D_089BAB28;
    U32(b, 0x190) = 0;
    U32(b, 0x194) = 0;
    U32(b, 0x198) = 0;
    U32(b, 0x19C) = 0;
    b[0x1A8] = 0;
    U16(b, 0x1B0) = 0;
    U16(b, 0x1B8) = 0;
    b[0x1A9] = 0;
    U16(b, 0x1B2) = 0;
    U16(b, 0x1BA) = 0;
    b[0x1AA] = 0;
    U16(b, 0x1B4) = 0;
    U16(b, 0x1BC) = 0;
    b[0x1AB] = 0;
    U16(b, 0x1B6) = 0;
    U16(b, 0x1BE) = 0;
    U16(b, 0x1A0) = 0;
    U16(b, 0x1A2) = 0;
    memset(b + 0x50, 0, 0x20);
    memset(b + 0x80, 0, 0x140);
    memset(b + 0x70, 0, 0x10);
    return o;
}

u32 *model_part_dtor(u32 *o, s32 flag)
{
    if (o != NULL) {
        o[0] = (u32)D_089BAB28;
        if ((s16)flag > 0)
            op_delete(o);
    }
    return o;
}

u32 *model_dtor(u32 *o, s32 flag)
{
    if (o != NULL) {
        o[0] = (u32)D_089BAB18;
        o[0x20] = (u32)D_089BAB28;
        o[0] = (u32)D_089BAB34;
        if ((s16)flag > 0)
            func_088626DC(o);
    }
    return o;
}

/* draws the model with the alpha / blend state, white color */
void model_draw(u8 *m, void *a1, void *a2, u32 a3, u32 t0, u32 t1)
{
    func_08862D34(m, a3 & 0xFFFF, t0 & 0xFF);
    GE(0xDBFF0006);
    func_08862354(a2, m + 0x10);
    func_08862BB8(m, a3 & 0xFFFF, t1);
    func_088629DC(m, a3 & 0xFFFF, 0xFF, 0xFF, 0xFF);
    func_0886171C(m, 0, a1, a3 & 0xFF);
    GE(0xDBFF8007);
}

/* same, with the color rgba */
void model_draw_color(u8 *m, void *a1, void *a2, u32 a3, u32 t0, u32 rgba)
{
    u8 c[4];

    memcpy(c, &rgba, 4);
    func_08862D34(m, a3 & 0xFFFF, t0 & 0xFF);
    GE(0xDBFF0006);
    func_08862354(a2, m + 0x10);
    func_08862800(m, a3 & 0xFFFF, c[0], c[1], c[2]);
    func_088629DC(m, a3 & 0xFFFF, c[0], c[1], c[2]);
    func_08862BB8(m, a3 & 0xFFFF, c[3]);
    func_0886171C(m, 0, a1, a3 & 0xFF);
    GE(0xDBFF8007);
}

u32 *func_0886272C(void *mdl, u32 i);
void *func_08862E54(u8 *m, u32 i);
void func_08862E64(void *a);
void func_08862748(void *mdl, u32 i);
u8 *func_08862768(void *mdl, u32 i, int a);
u8 *func_088627B4(void *mdl, u32 i, int a);

/* draws the mesh i of the model m: material colors, texture and a CALL for each part */
void model_draw_mesh(u32 *m, int unused, u8 *tex, u32 i_arg)
{
    u8 i = i_arg;
    void *mdl = (void *)m[0];
    u32 *h = func_0886272C(mdl, i);
    u8 *mi, *part;
    u32 cur = 0xFFFFFFFF;
    s32 k;

    func_08862E64(func_08862E54((u8 *)m, i));
    GE((h[0] >> 8) | 0x48000000);
    GE((h[1] >> 8) | 0x49000000);
    func_08862748(mdl, i);
    mi = func_08862768(mdl, i, 0);
    part = func_088627B4(mdl, i, 0);
    for (k = 0; k < ((u16 *)h)[10]; k++, part += 0x10) {
        u8 *mat = (u8 *)m[2] + mi[(s8)part[0]] * 0x10;
        u32 a;

        if (mat[3] == 0)
            continue;
        GE(0x53000003);
        GE(mat[0] | (mat[2] << 16) | 0x56000000 | (mat[1] << 8));
        GE(mat[3] | 0x58000000);
        GE(mat[4] | (mat[6] << 16) | 0x55000000 | (mat[5] << 8));
        if (mat[8] == 0xFF) {
            GE(0x1E000000);
            cur = mat[8];
        } else if (mat[8] != cur) {
            const u32 *t = (const u32 *)(U32(tex, 4) + mat[8] * 0x20);
            int j;

            GE(0x1E000001);
            GE(0xC2000001);
            for (j = 0; j < 8; j++)
                GE(t[j]);
            GE(0xCB000000);
            cur = mat[8];
        }
        a = m[1] + U32(part, 4);
        GE(((a & 0xFF000000) >> 8) | 0x10000000);
        GE(0x13000000);
        GE((a & 0xFFFFFF) | 0x0A000000);
    }
    GE(0x27000000);
}

extern u8 *D_08A62C40;            /* display list manager */
s32 func_088623DC(void *a, void *b, void *c, void *d);
int tmh_get_image(void *unused, const char *f, s32 tex, u32 img, u32 pal, u32 *out);
s32 pow2_log(void *unused, u32 v);

s32 model_set_anim_part(u8 *m, void *b, void *c, void *d)
{
    return func_088623DC(m + 0x50, b, c, d);
}

/* GE commands (0x20 bytes each) that select the textures of a TMH file, at the slot */
int texset_build(u32 *t, u32 cmds, const char *f, u32 slot)
{
    s32 i;

    if (t == NULL)
        return 0;
    t[1] = cmds;
    for (i = 0; i < *(const s32 *)(f + 8); i++) {
        u32 o[6];
        u32 *w;

        if (tmh_get_image(D_08A62C40, f, i, 0, 0, o) == 0)
            return 0;
        w = (u32 *)(t[1] + i * 0x20 + (u8)slot * 0x20);
        w[0] = o[1] | 0xC3000000;
        w[1] = (o[0] & 0xFFFFFF) | 0xA0000000;
        w[2] = ((o[0] & 0xFF000000) >> 8) | 0xA8000000 | ((u16 *)o)[4];
        w[3] = (pow2_log(D_08A62C40, ((u16 *)o)[5]) << 8) | 0xB8000000 | pow2_log(D_08A62C40, ((u16 *)o)[4]);
        w[4] = o[4] | 0xC500FF00;
        w[5] = (o[3] & 0xFFFFFF) | 0xB0000000;
        w[6] = ((o[3] & 0xFF000000) >> 8) | 0xB1000000;
        w[7] = ((o[5] + 7) >> 3) | 0xC4000000;
    }
    t[0] = (u32)f;
    ((u8 *)t)[8] = *(const s32 *)(f + 8);
    return 1;
}

int model_set_textures(u8 *m, u32 cmds, const char *f)
{
    return texset_build((u32 *)(m + 0x70), cmds, f, 0);
}

void model_nop(void)
{
}

/* parts of a model file (offsets in its header) */
u8 *mdl_data(u8 *mdl)
{
    return mdl + U32(mdl, 0x34);
}

s32 mdl_data_size(u8 *mdl)
{
    return U32(mdl, 8) - U32(mdl, 0x34);
}

u32 *mdl_mesh(u8 *mdl, s32 i)
{
    return (u32 *)(mdl + U32(mdl, 0x20) + i * 0x18);
}

u8 mdl_mesh_byte10(u8 *mdl, s32 i)
{
    return ((u8 *)mdl_mesh(mdl, i))[0x10];
}

u8 *mdl_mesh_matidx(u8 *mdl, s32 i, s32 k)
{
    u8 *h = (u8 *)mdl_mesh(mdl, i);

    return mdl + U32(mdl, 0x28) + U16(h, 0x12) + k;
}

u16 mdl_count_1e(u8 *mdl)
{
    return U16(mdl, 0x1E);
}

u8 *mdl_mesh_part(u8 *mdl, s32 i, s32 k)
{
    u8 *h = (u8 *)mdl_mesh(mdl, i);

    return mdl + U32(mdl, 0x24) + U16(h, 0x16) * 0x10 + k * 0x10;
}

u32 *mesh_state(u8 *m, s32 i);

/* blend mode of the mesh i: 1 normal, 2 / 3 additive */
void mesh_set_blend(u8 *m, u32 i, u32 mode)
{
    u32 *s = mesh_state(m, i & 0xFFFF);

    if ((u8)mode == 3)
        s[1] = 0xDF0002A2;
    else if ((u8)mode == 2)
        s[1] = 0xDF0000A2;
    else if ((u8)mode == 1)
        s[1] = 0xDF000032;
}

void mesh_set_flag1(u8 *m, u32 i, u32 on)
{
    u32 *s = mesh_state(m, i & 0xFFFF);

    if ((u8)on == 1)
        s[0] |= 1;
    else
        s[0] &= ~1;
}

void mesh_set_flag2(u8 *m, u32 i, u32 on)
{
    u32 *s = mesh_state(m, i & 0xFFFF);

    if ((u8)on == 1)
        s[0] |= 2;
    else
        s[0] &= ~2;
}

u32 *mesh_state(u8 *m, s32 i)
{
    return (u32 *)(U32(m, 0xC) + i * 8);
}

extern const s32 D_089A5C44[];
f32 func_08862FE0(f32 t, f32 a, f32 b, f32 c, f32 d, f32 e, f32 f, void *m);

/* material colors of the mesh i (diffuse at 0, alpha at 3, ambient at 4) */
void mesh_set_diffuse(u32 *m, u32 i, u32 r, u32 g, u32 b)
{
    u8 *mi = mdl_mesh_matidx((u8 *)m[0], i & 0xFFFF, 0);
    u32 n = mdl_mesh_byte10((u8 *)m[0], i & 0xFFFF);
    u32 k;

    for (k = 0; k < n; k++) {
        u8 *mat = (u8 *)m[2] + mi[k] * 0x10;

        mat[0] = r;
        mat[1] = g;
        mat[2] = b;
    }
}

void mesh_set_ambient(u32 *m, u32 i, u32 r, u32 g, u32 b)
{
    u8 *mi = mdl_mesh_matidx((u8 *)m[0], i & 0xFFFF, 0);
    u32 n = mdl_mesh_byte10((u8 *)m[0], i & 0xFFFF);
    u32 k;

    for (k = 0; k < n; k++) {
        u8 *mat = (u8 *)m[2] + mi[k] * 0x10;

        mat[4] = r;
        mat[5] = g;
        mat[6] = b;
    }
}

void mesh_set_alpha(u32 *m, u32 i, u32 a)
{
    u8 *mi = mdl_mesh_matidx((u8 *)m[0], i & 0xFFFF, 0);
    u32 n = mdl_mesh_byte10((u8 *)m[0], i & 0xFFFF);
    u32 k;

    for (k = 0; k < n; k++)
        ((u8 *)m[2] + mi[k] * 0x10)[3] = a;
}

/* GE states of a mesh (culling, fog, blending) */
void mesh_apply_state(u32 *s)
{
    if ((s[0] & 0x80000000) == 0)
        return;
    GE((s[0] & 1) ? 0x17000001 : 0x17000000);
    GE((s[0] & 2) ? 0x1F000001 : 0x1F000000);
    if (s[0] & 4) {
        GE(0x21000001);
        GE(s[1]);
        if (s[1] == 0xDF0000A2) {
            GE(0x27000001);
            GE(0xD8000003);
            GE(0xD9000000);
        }
    }
}

s32 key_find(f32 t, void *m, u8 *track, u8 **k0, u8 **k1, s32 hint);

/* value of an animation track at the time t (hint: last key used) */
f32 key_value(f32 t, void *m, u8 *track, s16 *hint)
{
    u8 *k0 = NULL, *k1 = NULL;

    *hint = key_find(t, m, track, &k0, &k1, *hint);
    if (k1 == NULL)
        return (f32)S16(k0, 0);
    return func_08862FE0(t, (f32)S16(k0, 0), (f32)S16(k0, 2), (f32)S16(k0, 6), (f32)S16(k1, 0),
                         (f32)S16(k1, 2), (f32)S16(k1, 4), m);
}

/* evaluates all the tracks of the animation of m at the time t */
int anim_eval(u8 *m, f32 *out, u32 t, s16 *hints)
{
    u8 *tr = (u8 *)U32(m, 0x64);
    u8 *k;
    u32 i;

    if (tr == NULL || (U32(tr, 0) & 0x1FF) == 0)
        return 1;
    k = tr + 0xC;
    for (i = 0; i < U32(tr, 4); i++, k += U32(k, 8)) {
        s32 idx = D_089A5C44[U16(k, 0)];
        f32 *o = out + idx;

        *o = key_value((f32)t, m, k, hints + idx);
        if (U16(k, 0) & 0x38)
            *o = *o * 0.0003834952f;
        else
            *o = *o * 0.0625f;
        tr = (u8 *)U32(m, 0x64);
    }
    return 1;
}

/* time of the last key of the track */
f32 track_length(void *m, u8 *k)
{
    return (f32)S16(k + U32(k, 4) * 8, 6);
}

/* keys around the time t: k0 (and k1 if t is between two keys) */
s32 key_find(f32 t, void *m, u8 *track, u8 **k0, u8 **k1, s32 hint)
{
    u8 *keys = track + 0xC;
    u32 n = U32(track, 4);
    s16 i;
    u8 *k;

    *k0 = NULL;
    *k1 = NULL;
    if (t <= (f32)S16(track, 0xE) || n == 1) {
        *k0 = keys;
        *k1 = NULL;
        return 0;
    }
    if (!(t < (f32)S16(keys + (n - 1) * 8, 2))) {
        *k0 = keys + (n - 1) * 8;
        *k1 = NULL;
        return (s16)(U32(track, 4) - 1);
    }
    i = hint;
    if (i < 0)
        i = 0;
    else if (!((u32)i < n))
        i = n - 1;
    k = keys + i * 8;
    for (;;) {
        f32 tk = (f32)S16(k, 2);

        if (tk == t) {
            *k0 = k;
            *k1 = NULL;
            return i;
        }
        if (tk < t && t < (f32)S16(k, 10)) {
            *k0 = k;
            *k1 = k + 8;
            return i;
        }
        if (t < tk) {
            i--;
            k -= 8;
        } else {
            i++;
            k += 8;
        }
    }
}

extern u8 D_089BAB44[];
extern const f32 D_089A6048[3], D_089A6054[3];
extern u8 *D_089CBFFC;            /* render lists */
void func_0886423C(u8 *a, u8 *anim, s32 slot);
void bone_set_anim_by_id(u8 *root, u8 *data, u32 id, s32 slot);
void func_08863E70(u8 *a, u8 *anim, s32 t, s32 x, s32 slot);
void func_08860648(void *b);
u32 *model_ctor(u32 *o);
u32 *model_dtor(u32 *o, s32 flag);
void func_0886454C(void *o);
void func_088626E4(void *o);
s32 vec3_nonzero(f32 *v);
void func_0890C370(void *o);
void rl_set_world_matrix(u8 *self, const f32 *m);
void skin_draw_mesh(u32 *m, u8 *anim, u8 *tex, u32 i);
#define func_0886477C(p, a, b, i) skin_draw_mesh((u32 *)(p), (u8 *)(a), (u8 *)(b), i)
void anim_model_draw_meshes(u8 *p, void *a, void *b, const f32 *mtx);

/* sets the animation anim in the slot of all the skeletons of the animator a */
void animator_set(u8 *a, u8 *anim, s32 slot)
{
    s32 i;

    if (anim == NULL)
        return;
    a[0x128 + slot] = 0;
    U16(a, 0x130 + slot * 2) = 0;
    func_0886423C(a, anim, slot);
    for (i = 0; i < U16(a, 0x120); i++)
        bone_set_anim_by_id((u8 *)U32(a, 0x110 + i * 4), anim, slot, 0);
}

void animator_play(f32 t, u8 *a, u8 *anim, s32 x, s32 slot)
{
    if (x != 0 && U32(a + slot * 0x40, 0x38) == 0)
        x = 0;
    if (x == 0) {
        animator_set(a, anim, slot);
    } else if (anim == NULL) {
        a[0x128 + slot] = 0;
        U16(a, 0x130 + slot * 2) = 0;
    } else {
        func_08863E70(a, anim, (s32)t, x, slot);
    }
}

void animator_reset_bones(u8 *a)
{
    s32 i;

    for (i = 0; i < U16(a, 0x120); i++)
        func_08860648((void *)U32(a, 0x110 + i * 4));
}

u8 *animator_bone(u8 *a, s32 i)
{
    return (u8 *)U32(a, 0x110) + i * 0x250;
}

void animator_clear(u8 *a)
{
    U32(a, 0x110) = 0;
    U32(a, 0x114) = 0;
    U32(a, 0x118) = 0;
    U32(a, 0x11C) = 0;
    a[0x128] = 0;
    U16(a, 0x130) = 0;
    U16(a, 0x138) = 0;
    a[0x129] = 0;
    U16(a, 0x132) = 0;
    U16(a, 0x13A) = 0;
    a[0x12A] = 0;
    U16(a, 0x134) = 0;
    U16(a, 0x13C) = 0;
    a[0x12B] = 0;
    U16(a, 0x136) = 0;
    U16(a, 0x13E) = 0;
    U16(a, 0x120) = 0;
    U16(a, 0x122) = 0;
}

/* 1 if the time t is inside the current range of the animation slot (looping or not) */
int anim_in_range(f32 t, u8 *a, s32 slot)
{
    u8 *s;
    f32 start, end, e;
    int wrap = 0;

    if (S8(a, 0x3E) != 0)
        return 0;
    s = a + slot * 0x40;
    start = F32(s, 0x10);
    end = F32(s, 0x1C);
    e = start + F32(s, 0x14);
    if (e > end) {
        if (U16(s, 0x3C) & 2) {
            wrap = 1;
            e = F32(s, 0x18) + (e - end);
        } else {
            e = end;
        }
    }
    if (!wrap) {
        if (!(t >= start))
            return 0;
        return t < e;
    }
    if (!(t >= start))
        return 0;
    return e < end;
}

int anim_before_start(f32 t, u8 *a, s32 slot)
{
    if (S8(a, 0x3E) != 0)
        return 0;
    return t <= F32(a + slot * 0x40, 0x10);
}

/* constructor of an animated model */
u32 *anim_model_ctor(u32 *o)
{
    u8 *b = (u8 *)o;
    f32 v[3];

    model_ctor(o);
    o[0] = (u32)D_089BAB44;
    U32(b, 0x1C4) = 0;
    U32(b, 0x1C8) = 0;
    b[0x1D7] = 0;
    b[0x1D6] = 0;
    b[0x1D5] = 0;
    b[0x1D4] = 0;
    U32(b, 0x414) = 0;
    b[0x1E9] = 3;
    U32(b, 0x410) = 0;
    v[0] = D_089A6048[0];
    v[1] = D_089A6048[1];
    v[2] = D_089A6048[2];
    if (vec3_nonzero(v) != 0) {
        F32(b, 0x3FC) = v[0];
        F32(b, 0x400) = v[1];
        F32(b, 0x404) = v[2];
    }
    return o;
}

u32 *anim_model_dtor(u32 *o, s32 flag)
{
    if (o != NULL) {
        o[0] = (u32)D_089BAB44;
        model_dtor(o, 0);
        if ((s16)flag > 0)
            func_0886454C(o);
    }
    return o;
}

void *anim_model_ret(void *a, void *b)
{
    return b;
}

void anim_model_nop(void)
{
}

void anim_model_reset(u8 *o)
{
    f32 v[3];

    func_088626E4(o);
    memset(o + 0x1F0, 0, 0x10);
    memset(o + 0x200, 0, 0x10);
    memset(o + 0x210, 0, 0x10);
    U32(o, 0x228) = 0x3F800000;
    U32(o, 0x224) = 0x3F800000;
    U32(o, 0x220) = 0x3F800000;
    U16(o, 0x32A) = 0;
    U16(o, 0x328) = 0;
    U16(o, 0x326) = 0;
    U16(o, 0x324) = 0;
    U32(o, 0x2AC) = 0xFFFFFFFF;
    U32(o, 0x2B0) = 0xFFFFFFFF;
    U32(o, 0x2B4) = 0xFFFFFFFF;
    U16(o, 0x2BA) = 0;
    U16(o, 0x2BC) = 0;
    o[0x230] = 0xFF;
    v[0] = D_089A6054[0];
    v[1] = D_089A6054[1];
    v[2] = D_089A6054[2];
    if (vec3_nonzero(v) != 0) {
        F32(o, 0x3FC) = v[0];
        F32(o, 0x400) = v[1];
        F32(o, 0x404) = v[2];
    }
}

/* update of a model: if it is visible, update then draw (vtable 6 and 7) */
void anim_model_update(u8 *o)
{
    if (vec3_nonzero((f32 *)(o + 0x3FC)) == 0)
        return;
    func_0890C370(o);
    if (U32(o, 4) & 1) {
        (*(void (**)(void *))(U32(o, 0) + 0x18))(o);
        (*(void (**)(void *))(U32(o, 0) + 0x1C))(o);
    }
}

void anim_model_draw_all(u8 *o)
{
    animator_reset_bones(o + 0x80);
    anim_model_draw_meshes(o + 0x50, o + 0x80, o + 0x70, (const f32 *)(o + 0x10));
}

/* world matrix, then every mesh */
void anim_model_draw_meshes(u8 *p, void *a, void *b, const f32 *mtx)
{
    s32 i;

    rl_set_world_matrix(D_089CBFFC, mtx);
    for (i = 0; i < U16(*(u8 **)p, 0x1C); i++)
        func_0886477C(p, a, b, i);
}

/* draws the skinned mesh i: like model_draw_mesh, plus the bone matrices of each part */
void skin_draw_mesh(u32 *m, u8 *anim, u8 *tex, u32 i)
{
    u8 *bones = (u8 *)U32(anim, 0x110);
    u8 *mdl = (u8 *)m[0];
    u32 *h = mdl_mesh(mdl, i);
    u8 *mi, *part;
    u32 last_mat = 0xFFFFFFFF, cur_tex = 0xFFFFFFFF;
    s32 k;

    mesh_apply_state(mesh_state((u8 *)m, i));
    GE((h[0] >> 8) | 0x48000000);
    GE((h[1] >> 8) | 0x49000000);
    mdl_mesh_byte10(mdl, i);
    mi = mdl_mesh_matidx(mdl, i, 0);
    part = mdl_mesh_part(mdl, i, 0);
    for (k = 0; k < ((u16 *)h)[10]; k++, part += 0x10) {
        u32 idx = mi[(s8)part[0]];
        const u8 *bl;
        s32 j;
        u32 a;

        if (idx == last_mat) {
            GE(0xFF000000);
        } else {
            u8 *mat = (u8 *)m[2] + idx * 0x10;

            if (mat[3] == 0)
                continue;
            GE(0x53000003);
            GE(mat[0] | (mat[2] << 16) | 0x56000000 | (mat[1] << 8));
            GE(mat[3] | 0x58000000);
            GE(mat[4] | (mat[6] << 16) | 0x55000000 | (mat[5] << 8));
            if (mat[8] == 0xFF) {
                GE(0x1E000000);
                cur_tex = mat[8];
            } else if (mat[8] != cur_tex) {
                const u32 *t = (const u32 *)(U32(tex, 4) + mat[8] * 0x20);
                int q;

                GE(0x1E000001);
                GE(0xC2000001);
                for (q = 0; q < 8; q++)
                    GE(t[q]);
                GE(0xCB000000);
                cur_tex = mat[8];
            }
            last_mat = idx;
        }
        bl = mdl + U32(mdl, 0x2C) + U16(part, 2) * 2;
        for (j = 0; j < (s8)part[1]; j++, bl += 2) {
            u8 *b = bones + bl[1] * 0x250;

            GE((bl[0] * 12) | 0x2A000000);
            GE((U32(b, 0x50) >> 8) | 0x2B000000);
            GE((U32(b, 0x54) >> 8) | 0x2B000000);
            GE((U32(b, 0x58) >> 8) | 0x2B000000);
            GE((U32(b, 0x60) >> 8) | 0x2B000000);
            GE((U32(b, 0x64) >> 8) | 0x2B000000);
            GE((U32(b, 0x68) >> 8) | 0x2B000000);
            GE((U32(b, 0x70) >> 8) | 0x2B000000);
            GE((U32(b, 0x74) >> 8) | 0x2B000000);
            GE((U32(b, 0x78) >> 8) | 0x2B000000);
            GE((U32(b, 0x80) >> 8) | 0x2B000000);
            GE((U32(b, 0x84) >> 8) | 0x2B000000);
            GE((U32(b, 0x88) >> 8) | 0x2B000000);
        }
        a = m[1] + U32(part, 4);
        GE(((a & 0xFF000000) >> 8) | 0x10000000);
        GE(0x13000000);
        GE((a & 0xFFFFFF) | 0x0A000000);
    }
    GE(0x27000000);
}
