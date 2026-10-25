/* 3D render lists: 20 layers of objects drawn in order, with the GE state commands
 * written between them. The manager is created by list_mgr_ctor (src/game/task_misc.c).
 * (Guessed names.) */
#include "menu.h"

extern u8 *D_08A62C40;            /* display list manager */
extern u8 *g_camera;            /* camera / viewport */
extern void *g_vram;          /* VRAM manager */
extern u32 *D_089CC000;           /* current GE command pointer */
extern u8 **g_area;
extern const s8 D_0893B7BC[20];   /* number of lists of each layer */
extern const u32 D_0893B7D0[][4];

u32 *dl_cur(void *dl);
void dl_link(void *dl, u32 *p, int n, u8 layer);
void dl_set_cur(void *dl, u32 *end);
void dl_add(void *dl, u32 *cmds, int n, u8 layer);
int vram_get_surface(void *m, u32 id, void *out);
void func_088CACD4(void *a);

#define GE(v) (*D_089CC000++ = (u32)(v))

static u32 fbits(f32 f)
{
    union {
        f32 f;
        u32 u;
    } c;

    c.f = f;
    return c.u;
}

int rl_open(u8 *self, u32 layer);
void rl_close(u8 *self);
void rl_set_mode(u8 *self, u32 mode);
void rl_clear(u8 *self);

/* layer 1: viewport and the initial GE state of the 3D scene */
void rl_begin_frame(u8 *self)
{
    u8 *cam;
    f32 z, d;

    rl_clear(self);
    if (rl_open(self, 1) == 0)
        return;
    cam = g_camera;
    GE((U32(cam, 0xDA0) >> 8) | 0x42000000);
    GE((U32(cam, 0xDA4) >> 8) | 0x43000000);
    GE((U32(cam, 0xDA8) >> 8) | 0x44000000);
    GE((U32(cam, 0xDAC) >> 8) | 0x45000000);
    GE((U32(cam, 0xDB0) >> 8) | 0x46000000);
    GE((U32(cam, 0xDB4) >> 8) | 0x47000000);
    GE(0xD6000000);
    GE(0xD700FFFF);
    GE(0xD4000000);
    GE(0xD50441E0);
    GE(0x4C007100);
    GE(0x4D007780);
    GE(0x22000001);
    GE(0xDBFFC007);
    GE(0x21000001);
    GE(0xDF000032);
    GE(0x27000000);
    GE(0xD8000000);
    GE(0xDAFFFFFF);
    GE(0x24000000);
    GE(0x23000001);
    GE(0xDE000005);
    GE(0x20000000);
    rl_set_mode(self, 0);
    GE(0x25000000);
    GE(0x28000000);
    GE(0x1D000000);
    GE(0x1C000001);
    GE(0x50000001);
    GE(0x5C000000);
    GE(0x5D0000FF);
    GE(0x54000000);
    GE(0x57000000);
    GE(0x5B000000);
    GE(0xC0000000);
    GE(0xC6000101);
    GE(0xC9000100);
    GE(0xC7000000);
    GE(0x4A000000);
    GE(0x4B000000);
    GE(0x483F8000);
    GE(0x493F8000);
    GE(0x5E000000);
    GE(0x17000000);
    GE(0x18000001);
    GE(0x90000000);
    GE(0x8F000000);
    GE(0x91000000);
    GE(0x19000001);
    GE(0x93000000);
    GE(0x92000000);
    GE(0x94000000);
    GE(0x1A000001);
    GE(0x96000000);
    GE(0x95000000);
    GE(0x97000000);
    GE(0x1B000000);
    GE(0x99000000);
    GE(0x98000000);
    GE(0x9A000000);
    GE(0x1F000000);
    /* fog (disabled above): the original reads uninitialized values here */
    z = 65000.0f * *(f32 *)D_08A62C40;
    d = z - z;
    if (d == 0.0f)
        d = 1.0f;
    GE(0xCF000000);
    GE((fbits(z) >> 8) | 0xCD000000);
    GE((fbits(1.0f / d) >> 8) | 0xCE000000);
    rl_close(self);
}

/* empties all the lists of the 20 layers */
void rl_clear(u8 *self)
{
    int i, j;

    for (i = 0; i < 20; i++, self += 4) {
        for (j = 0; j < D_0893B7BC[i]; j++)
            (*(u32 **)(self + 0x1BC))[j] = 0;
    }
}

void rl_draw_copy(u8 *self);

/* draws the 20 layers: each object of a list is drawn with vtable[3] */
void rl_draw_all(u8 *self)
{
    s32 layer;

    for (layer = 0; layer < 20; layer++) {
        int j;

        if (rl_open(self, layer & 0xFF) == 0)
            continue;
        if (layer == 1) {
            u8 *cam;
            int i;

            /* identity world matrix, then the view matrix of the camera */
            GE(0x3C000000);
            GE(0x3D3F8000);
            GE(0x3D000000);
            GE(0x3D000000);
            GE(0x3D000000);
            GE(0x3D3F8000);
            GE(0x3D000000);
            GE(0x3D000000);
            GE(0x3D000000);
            GE(0x3D3F8000);
            GE(0x3D000000);
            GE(0x3D000000);
            GE(0x3D000000);
            cam = g_camera;
            GE(0x3E000000);
            for (i = 0; i < 16; i++)
                GE((U32(cam, 0xC00 + i * 4) >> 8) | 0x3F000000);
        }
        for (j = 0; j < D_0893B7BC[layer]; j++) {
            u32 *o = (*(u32 ***)(self + layer * 4 + 0x1BC))[j];

            for (; o != NULL; o = (u32 *)o[2])
                (*(void (**)(void *))(o[0] + 0xC))(o);
        }
        if (layer == 8 && G_AREA != 0 && *g_area != NULL
            && (*g_area)[0x3D4] == 1)
            func_088CACD4(*g_area);
        rl_close(self);
    }
    if (self[0x214] == 1) {
        rl_draw_copy(self);
        self[0x214] = 0;
    }
}

/* copies the frame buffer to the surface self+0x218 (block transfer) */
void rl_draw_copy(u8 *self)
{
    u32 s[6];
    u32 c[12];

    vram_get_surface(g_vram, (U32(D_08A62C40, 4) ^ 1) & 0xFF, s);
    c[0] = 0xCC000000;
    c[1] = (s[2] & 0xFFFFFF) | 0xB2000000;
    c[2] = ((s[2] & 0xFF000000) >> 8) | 0xB3000100;
    c[3] = (U32(self, 0x218) & 0xFFFFFF) | 0xB4000000;
    c[4] = ((U32(self, 0x218) & 0xFF000000) >> 8) | 0xB5000100;
    c[5] = 0xEB000000;
    c[6] = 0xEC000000;
    c[7] = 0xEE043CFF;
    c[8] = 0xEA000001;
    c[9] = 0xCC000000;
    c[10] = 0x10000000;
    c[11] = 0x08000000;
    dl_add(D_08A62C40, c, 12, self[0x215]);
}

/* GE states of the layers 1, 2, 5, 6 and 18 */
void rl_set_states(u8 *self)
{
    if (rl_open(self, 1)) {
        GE(0x23000001);
        GE(0xDE000005);
        GE(0x22000001);
        GE(0xC0000000);
        GE(0xC6000107);
        GE(0xC9000100);
        GE(0xC7000000);
        GE(0x4A000000);
        GE(0x4B000000);
        GE(0x1C000001);
        GE(0x1D000000);
        GE(0x1F000001);
        GE(0x50000001);
        GE(0x5C000000);
        GE(0x5D0000FF);
        GE(0xE0FFFFFF);
        GE(0xE1FFFFFF);
        rl_close(self);
    }
    if (rl_open(self, 2)) {
        GE(0xDBFF4007);
        GE(0x21000001);
        GE(0xDF000032);
        GE(0x17000000);
        rl_close(self);
    }
    if (rl_open(self, 5)) {
        GE(0xDBFFC007);
        GE(0x21000000);
        GE(0x17000001);
        rl_close(self);
    }
    if (rl_open(self, 6)) {
        GE(0xDBFF8007);
        GE(0x21000001);
        GE(0xDF000032);
        GE(0x17000000);
        rl_close(self);
    }
    if (rl_open(self, 0x12)) {
        GE(0x23000001);
        GE(0xDE000005);
        GE(0x22000001);
        GE(0xC0000000);
        GE(0xC6000101);
        GE(0xC9000100);
        GE(0xC7000000);
        GE(0x4A000000);
        GE(0x4B000000);
        GE(0x1C000001);
        GE(0x1D000000);
        GE(0x17000000);
        rl_close(self);
    }
}

/* blending mode: 0x20000001 followed by the 4 words of the table */
void rl_set_mode(u8 *self, u32 mode)
{
    const u32 *t = (const u32 *)((const u8 *)D_0893B7D0 + (u8)((mode & 0xFF) << 2) * 4);

    GE(0x20000001);
    GE(t[0]);
    GE(t[1]);
    GE(t[2]);
    GE(t[3]);
}

/* starts writing GE commands for the layer (closes the previous block) */
int rl_open(u8 *self, u32 layer)
{
    if (self[0x20C] != 0)
        rl_close(self);
    D_089CC000 = dl_cur(D_08A62C40);
    if (D_089CC000 != NULL) {
        self[0x20C] = 1;
        U32(self, 0x210) = (u32)D_089CC000;
        self[0x20D] = layer;
    }
    return self[0x20C] != 0;
}

/* links the written commands in their layer */
void rl_close(u8 *self)
{
    s32 n;

    if (self[0x20C] == 0)
        return;
    n = (s32)((u32)D_089CC000 - U32(self, 0x210)) / 4;
    if (n != 0) {
        D_089CC000 += 2;
        dl_link(D_08A62C40, (u32 *)U32(self, 0x210), n + 2, self[0x20D]);
        dl_set_cur(D_08A62C40, D_089CC000);
    }
    D_089CC000 = NULL;
    U32(self, 0x210) = 0;
    self[0x20C] = 0;
}

int camera_sphere_visible(f32 r, void *cam, const f32 *p);
int rl_insert(u8 *self, u32 layer_arg, u8 *o, const f32 *pos, u32 flag_arg);

/* adds a model to its layer if its bounding sphere is visible (flag: always add) */
void rl_add_model(u8 *self, u32 layer, u8 *o, u32 flag)
{
    f32 v[4] __attribute__((aligned(16)));
    f32 r;

    U32(o, 4) &= ~2;
    if ((flag & 0xFF) == 0) {
        r = F32(o, 0x220);
        if (r < F32(o, 0x224))
            r = F32(o, 0x224);
        if (r < F32(o, 0x228))
            r = F32(o, 0x228);
        v[0] = F32(o, 0x200);
        v[1] = F32(o, 0x204) + 0.5f * (r * F32(*(u8 **)(o + 0x50), 0x14));
        v[2] = F32(o, 0x208);
        v[3] = 0.0f;
        if ((u8)camera_sphere_visible(r * F32(*(u8 **)(o + 0x50), 0xC), g_camera, v) == 0)
            return;
    }
    if (o[0x230] != 0xFF && (layer & 0xFF) == 5)
        layer = 7;
    if ((u8)rl_insert(self, layer, o, (f32 *)(o + 0x200), flag) == 1)
        U32(o, 4) |= 2;
}

/* same, with the position at o+0x40 */
void rl_add_object(u8 *self, u32 layer, u8 *o, u32 flag)
{
    U32(o, 4) &= ~2;
    if ((flag & 0xFF) == 0
        && (u8)camera_sphere_visible(F32(*(u8 **)(o + 0x50), 0xC), g_camera, (f32 *)(o + 0x40)) == 0)
        return;
    if ((u8)rl_insert(self, layer, o, (f32 *)(o + 0x40), flag) == 1)
        U32(o, 4) |= 2;
}

/* puts o in the list of the layer chosen by its view depth; layer 7 is kept sorted */
int rl_insert(u8 *self, u32 layer_arg, u8 *o, const f32 *pos, u32 flag_arg)
{
    u8 layer = layer_arg;
    f32 *cam;
    f32 d;
    s32 n, idx;
    u32 **slot;
    u8 *p, *prev;

    if (layer >= 20)
        return 0;
    U32(o, 8) = 0;
    cam = (f32 *)g_camera;
    d = (F32(cam, 0xBB8) + (pos[2] * F32(cam, 0xBA8) + (pos[0] * F32(cam, 0xB88) + pos[1] * F32(cam, 0xB98))))
        * -1.0f;
    if (d < 0.0f) {
        if ((u8)flag_arg != 1 && d < -1600.0f)
            return 0;
        d = 0.0f;
    }
    F32(o, 0xC) = d;
    n = D_0893B7BC[layer];
    idx = (int)((f32)n * ((d - cam[0]) / (cam[1] - cam[0])));
    if (idx >= n)
        return 0;
    slot = (u32 **)(self + 0x1BC + layer * 4);
    p = (u8 *)(*slot)[n - 1 - idx];
    if (layer != 7) {
        U32(o, 8) = (u32)p;
        (*slot)[D_0893B7BC[layer] - 1 - idx] = (u32)o;
        return 1;
    }
    if (p == NULL) {
        (*slot)[n - 1 - idx] = (u32)o;
        return 1;
    }
    prev = NULL;
    for (;;) {
        if (!(d < F32(p, 0xC))) {
            U32(o, 8) = (u32)p;
            if (prev == NULL)
                (*slot)[D_0893B7BC[layer] - 1 - idx] = (u32)o;
            else
                U32(prev, 8) = (u32)o;
            return 1;
        }
        if (U32(p, 8) == 0) {
            U32(p, 8) = (u32)o;
            return 1;
        }
        prev = p;
        p = (u8 *)U32(p, 8);
    }
}

/* world matrix: camera matrix (g_camera+0xBC0) x global scale x m */
void rl_set_world_matrix(u8 *self, const f32 *m)
{
    f32 s[16] __attribute__((aligned(16)));
    f32 r[16] __attribute__((aligned(16)));
    f32 k = *(f32 *)D_08A62C40;
    int i;

    for (i = 0; i < 16; i++)
        s[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    s[0] = k;
    s[5] = k;
    s[10] = k;
    __asm__ volatile(
        "lv.q C100, 0(%1)\n"
        "lv.q C110, 16(%1)\n"
        "lv.q C120, 32(%1)\n"
        "lv.q C130, 48(%1)\n"
        "lv.q C200, 0(%2)\n"
        "lv.q C210, 16(%2)\n"
        "lv.q C220, 32(%2)\n"
        "lv.q C230, 48(%2)\n"
        "vmmul.q E000, E200, E100\n"
        "vmmov.q M200, M000\n"
        "lv.q C100, 0(%3)\n"
        "lv.q C110, 16(%3)\n"
        "lv.q C120, 32(%3)\n"
        "lv.q C130, 48(%3)\n"
        "vmmul.q E000, E200, E100\n"
        "sv.q C000, 0(%0)\n"
        "sv.q C010, 16(%0)\n"
        "sv.q C020, 32(%0)\n"
        "sv.q C030, 48(%0)\n"
        : : "r"(r), "r"(s), "r"(m), "r"(g_camera + 0xBC0) : "memory");
    GE(0x3A000000);
    for (i = 0; i < 16; i++) {
        if ((i & 3) != 3)
            GE((fbits(r[i]) >> 8) | 0x3B000000);
    }
}
