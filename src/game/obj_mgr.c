/* Manager of the loaded objects (models, animations and textures of hunters, monsters, ...).
 * The object is 0x80000 bytes of memory followed by its heap (at +0x80000) and by 48 file
 * slots (8 per group): type, preset size, buffer, size, state, file id.
 * (Guessed names.) */
#include "menu.h"
#include "vfpu.h"

#define MGR(o) ((u32 *)(self + 0x80000))

extern void *D_08A62C5C;           /* the manager */
extern void **D_09A4F060;
extern void *D_089CBFFC;           /* render lists */
extern void **D_089CC434;          /* file loader (C++ object) */
extern void *D_09A0B540;
extern void *D_089CBBE0;           /* VRAM manager */
extern const s32 D_089A6060[];
extern const u16 D_09A67EA0[];
extern const s16 D_089B650C[][6], D_089B6524[][6], D_089B653C[];
extern u8 D_089BAB08[];            /* vtable of the bone objects */

typedef s32 (*LdFunc0)(void *);
typedef s32 (*LdFunc1)(void *, u32);
typedef s32 (*LdFunc5)(void *, u32, void *, s32, s32, s32);
#define LDCALL(type, off) ((type)((void **)*D_089CC434)[(off) / 4])

void heap_init(u32 *h, u32 *base, u32 size);
void *heap_alloc(u32 *h, u32 size, u32 align);
void heap_free(u32 *h, u8 *p);
void *light_obj_ret(void *a, void *b);
int texset_build(u32 *t, u32 cmds, const char *f, u32 slot);
u8 *mdl_data(u8 *mdl);
s32 mdl_data_size(u8 *mdl);
u16 mdl_count_1e(u8 *mdl);
void animator_reset_bones(u8 *a);
void animator_clear(u8 *a);
int vram_create_buffer(void *m, u32 id, int size, u32 offset);
void vram_free_surface(void *m, u32 id);
u32 vram_surface_addr(void *m, u32 id);
void *memcpy(void *d, const void *s, size_t n);
s32 func_08898694(void *a, int b, s32 c);
void func_08898748(void *a, int b);
s32 func_088CC4A8(void *a);
u8 *func_088CC478(void *a);
void func_0884C9B8(void *rl, int layer, void *obj, int b);
s32 func_088D6388(u32 a, u32 b, u32 c, u32 d, u32 e);
u8 *func_088B89B0(u32 file, int i);
s32 func_088623DC(void *a, void *b, void *c, void *d);
void func_088640F8(f32 x, f32 y, f32 z, u8 *a, int b);
void func_0886005C(u8 *bone, u8 *src);
void func_0886053C(u8 *bone, f32 *m);
void sceKernelDcacheWritebackInvalidateAll(void);
int sceDmacMemcpy(void *dst, const void *src, u32 size);

u32 objmgr_alloc1(u8 *self, s32 size);
u32 objmgr_alloc2(u8 *self, s32 size);
s32 objmgr_load(u8 *self, u32 id, u32 slot);
void objmgr_unload(u8 *self, u32 slot);
void objmgr_build_bones(f32 x, f32 y, f32 z, u8 *a, u32 *hdr, u8 *buf);

/* picks the light (palette) of the object from the area data, by distance */
void obj_pick_light(u8 *o)
{
    void **st = (void **)*D_09A4F060;
    s32 base;
    u8 *t, *e;
    u32 b, i1, i2;
    f32 d;

    U32(o, 0x1DC) = 0;
    if (st == NULL)
        return;
    base = func_088CC4A8(st);
    if (base == 0)
        return;
    t = ((u8 *(*)(void *))((void **)*st)[0x28 / 4])(st);
    if (t == NULL)
        return;
    b = *(u8 *)(U32(t, 0xC) + o[0x290] * 4);
    i1 = b & 3;
    i2 = (b >> 2) & 1;
    if (i1 != 0) {
        t = func_088CC478(st);
        if (t != NULL) {
            e = t + (u8)i1 * 0x18;
            goto found;
        }
    }
    if ((u8)i2 == 0)
        return;
    t = ((u8 *(*)(void *))((void **)*st)[0x34 / 4])(st);
    if (t == NULL)
        return;
    e = t + (u8)i2 * 0x18;
found:
    d = vfpu_dist3((f32 *)e, (f32 *)(o + 0x200));
    if (!(d <= F32(e, 0x10)))
        return;
    if (d <= F32(e, 0xC))
        U32(o, 0x1DC) = base + U16(e, 0x14) * 0x20;
    else
        U32(o, 0x1DC) = base + U16(e, 0x16) * 0x20;
}

u8 *objmgr_init(u8 *self)
{
    int i;

    D_08A62C5C = self;
    heap_init((u32 *)(self + 0x80000), (u32 *)self, 0x80000);
    for (i = 0; i < 15; i++) {
        U32(self, 0x80028 + i * 4) = 0;
        U32(self, 0x80064 + i * 4) = 0;
    }
    U32(self, 0x800A0) = 0;
    memset(self + 0x80258, 0, 0xC0);
    memset(self + 0x80318, 0, 0xC0);
    memset(self + 0x803D8, 0, 0x30);
    memset(self + 0x80408, 0xFF, 0x60);
    self[0x800A4] = 0;
    memset(self + 0x800A8, 0, 0x180);
    for (i = 0; i < 12; i++)
        U32(self, 0x80228 + i * 4) = 0;
    return self;
}

u8 *objmgr_dtor(u8 *self, s32 flag)
{
    if (self != NULL) {
        D_08A62C5C = NULL;
        if ((s16)flag > 0)
            func_0890BFA4(self);
    }
    return self;
}

/* calls the method 0x14 of all the objects of the list i */
void objmgr_list_call(u8 *self, s32 i)
{
    u32 *p = (u32 *)U32(self, i * 4 + 0x80028);

    while (p != NULL) {
        ((void (*)(void *))((void **)p[0])[0x14 / 4])(p);
        p = (u32 *)p[0x71];
    }
}

/* adds the visible objects of the list i to the render list 5 */
void objmgr_list_draw(u8 *self, s32 i)
{
    u8 *p = (u8 *)U32(self, i * 4 + 0x80028);

    while (p != NULL) {
        if ((U32(p, 4) & 1) != 0 && (U32(p, 4) & 4) == 0)
            func_0884C9B8(D_089CBFFC, 5, p, 0);
        p = (u8 *)U32(p, 0x1C4);
    }
}

/* sets the slots of type 1 / 2 from a table of 48 {type, size} */
void objmgr_setup1(u8 *self, const u8 *t)
{
    int i;

    for (i = 0; i < 0x30; i++, t += 8) {
        if (t[0] != 1)
            continue;
        if (*(s32 *)(t + 4) != -1) {
            U32(self, 0x80258 + i * 4) = objmgr_alloc1(self, *(s32 *)(t + 4));
            U32(self, 0x80318 + i * 4) = *(u32 *)(t + 4);
        }
        self[0x800A8 + i * 8] = t[0];
        U32(self, 0x800AC + i * 8) = *(u32 *)(t + 4);
    }
}

void objmgr_setup2(u8 *self, const u8 *t)
{
    int i;

    for (i = 0; i < 0x30; i++, t += 8) {
        if (t[0] != 2)
            continue;
        if (*(s32 *)(t + 4) != -1) {
            U32(self, 0x80258 + i * 4) = objmgr_alloc2(self, *(s32 *)(t + 4));
            U32(self, 0x80318 + i * 4) = *(u32 *)(t + 4);
        }
        self[0x800A8 + i * 8] = t[0];
        U32(self, 0x800AC + i * 8) = *(u32 *)(t + 4);
    }
}

/* pool 1: memory block 2 and VRAM buffer 7 */
void objmgr_pool1_init(u8 *self, s32 size, u32 vsize)
{
    U32(self, 0x80228) = size;
    U32(self, 0x80230) = func_08898694(D_09A0B540, 2, size);
    U32(self, 0x8022C) = 0;
    U32(self, 0x80234) = vsize;
    vram_create_buffer(D_089CBBE0, 7, vsize, 0xFFFFFFFF);
    U32(self, 0x8023C) = vram_surface_addr(D_089CBBE0, 7);
    U32(self, 0x80238) = 0;
}

u32 objmgr_alloc1(u8 *self, s32 size)
{
    u32 used = U32(self, 0x8022C);

    U32(self, 0x8022C) = used + size;
    return U32(self, 0x80230) + used;
}

/* VRAM address of the texture b of the slot group a */
u32 objmgr_vram_addr(u8 *self, s32 a, s32 b)
{
    return U32(self, 0x8023C) + a * 0x1A800 + D_089A6060[b];
}

u32 objmgr_alloc2(u8 *self, s32 size)
{
    u32 used = U32(self, 0x80244);

    U32(self, 0x80244) = used + size;
    return U32(self, 0x80248) + used;
}

/* pool 2: memory block 3 and VRAM buffer 8 */
void objmgr_pool2_init(u8 *self, s32 size, u32 vsize)
{
    if (size > 0) {
        U32(self, 0x80240) = size;
        U32(self, 0x80248) = func_08898694(D_09A0B540, 3, size);
        U32(self, 0x80244) = 0;
    }
    U32(self, 0x8024C) = vsize;
    vram_create_buffer(D_089CBBE0, 8, vsize, 0xFFFFFFFF);
    U32(self, 0x80254) = vram_surface_addr(D_089CBBE0, 8);
    U32(self, 0x80250) = 0;
}

void objmgr_pool1_free(u8 *self)
{
    func_08898748(D_09A0B540, 2);
    U32(self, 0x80230) = 0;
    U32(self, 0x80228) = 0;
    U32(self, 0x8022C) = 0;
    vram_free_surface(D_089CBBE0, 7);
    U32(self, 0x8023C) = 0;
    U32(self, 0x80234) = 0;
    U32(self, 0x80238) = 0;
}

void objmgr_pool2_free(u8 *self)
{
    func_08898748(D_09A0B540, 3);
    U32(self, 0x80248) = 0;
    U32(self, 0x80240) = 0;
    U32(self, 0x80244) = 0;
    vram_free_surface(D_089CBBE0, 8);
    U32(self, 0x80254) = 0;
    U32(self, 0x8024C) = 0;
    U32(self, 0x80250) = 0;
}

/* loads the files of a group (1 when finished) */
int objmgr_load_model(u8 *self, u32 group, u32 kind)
{
    return (u8)objmgr_load(self, D_09A67EA0[(u8)kind], (u8)((u8)group * 8)) == 1;
}

int objmgr_load_part(u8 *self, u32 group, u32 a, u32 b, u32 c, u32 d, u32 e, s32 alt)
{
    s16 id;

    if ((u8)alt == 1 || ((u8)alt == 0 && D_089B6524[(u8)a][(u8)b] == -1)) {
        id = func_088D6388(a, b, c, d, e);
        id = D_089B650C[(u8)a][(u8)b] + id;
    } else {
        id = func_088D6388(a, b, c, d, e);
        id = D_089B6524[(u8)a][(u8)b] + id;
    }
    return (u8)objmgr_load(self, (u16)id, (u8)((u8)b + (u8)group * 8)) == 1;
}

int objmgr_load_anim(u8 *self, u32 group, u32 n)
{
    return (u8)objmgr_load(self, (u16)(n + 0xD50), (u8)((u8)group * 8 + 6)) == 1;
}

int objmgr_load_extra(u8 *self, u32 group, u32 kind, u32 n)
{
    return (u8)objmgr_load(self, (u16)((u8)n + D_089B653C[(u8)kind]), (u8)((u8)group * 8 + 7)) == 1;
}

void objmgr_unload_part(u8 *self, u32 group, u32 i)
{
    objmgr_unload(self, (u8)((u8)i + (u8)group * 8));
}

void objmgr_unload_extra(u8 *self, u32 group)
{
    objmgr_unload(self, (u8)((u8)group * 8 + 7));
}

void objmgr_unload_anim(u8 *self, u32 group)
{
    objmgr_unload(self, (u8)((u8)group * 8 + 6));
}

/* moves the 8 slots of the group b to the group a */
void objmgr_move_group(u8 *self, u32 a, u32 b)
{
    u32 d = (u8)(a << 3), s = (u8)(b << 3);
    int i;

    func_08898694(D_09A0B540, 0x19, 0x41000);
    for (i = 0; i < 8; i++) {
        if (U16(self, 0x80408 + s * 2) != 0xFFFF) {
            memcpy((void *)U32(self, 0x80258 + d * 4), (void *)U32(self, 0x80258 + s * 4),
                   U32(self, 0x80318 + d * 4));
            U16(self, 0x80408 + d * 2) = U16(self, 0x80408 + s * 2);
            self[0x803D8 + d] = 3;
            U16(self, 0x80408 + s * 2) = 0xFFFF;
            self[0x803D8 + s] = 0;
        }
        d = (u8)(d + 1);
        s = (u8)(s + 1);
    }
    func_08898748(D_09A0B540, 0x19);
}

/* loading state machine of a slot: 1 when the file is in memory */
s32 objmgr_load(u8 *self, u32 id, u32 slot)
{
    u8 *e, *st;
    s32 size;

    if ((id & 0xFFFF) == 0xFFFF)
        return 1;
    slot &= 0xFFFF;
    e = self + 0x800A8 + slot * 8;
    st = self + 0x803D8 + slot;
    switch (*st) {
    case 3:
        return 1;
    case 0:
        size = LDCALL(LdFunc1, 0x18)(D_089CC434, id);
        if (*(s32 *)(e + 4) == -1) {
            if (e[0] == 2)
                U32(self, 0x80258 + slot * 4) = objmgr_alloc2(self, size);
            else if (e[0] == 1)
                U32(self, 0x80258 + slot * 4) = objmgr_alloc1(self, size);
            U32(self, 0x80318 + slot * 4) = size;
        }
        (*st)++;
        return 0;
    case 1:
        if (LDCALL(LdFunc5, 0x34)(D_089CC434, id, (void *)U32(self, 0x80258 + slot * 4), -1, 0, 0) == 0)
            return 0;
        *st = 2;
        /* fall through */
    case 2:
        if ((u8)LDCALL(LdFunc0, 0x38)(D_089CC434) != 0)
            return 0;
        *st = 3;
        U16(self, 0x80408 + slot * 2) = id;
        return 1;
    }
    return 0;
}

void objmgr_unload(u8 *self, u32 slot)
{
    u8 *st, *e;

    slot &= 0xFFFF;
    st = self + 0x803D8 + slot;
    if (*st == 0)
        return;
    if (*st == 4) {
        U32(self, 0x80318 + slot * 4) = 0;
    } else {
        e = self + 0x800A8 + slot * 8;
        if (*(s32 *)(e + 4) != -1) {
            *st = 0;
            U16(self, 0x80408 + slot * 2) = 0xFFFF;
            return;
        }
        if (e[0] == 2)
            U32(self, 0x80244) -= U32(self, 0x80318 + slot * 4);
        else if (e[0] == 1)
            U32(self, 0x8022C) -= U32(self, 0x80318 + slot * 4);
        U32(self, 0x80318 + slot * 4) = 0;
    }
    *st = 0;
    U16(self, 0x80408 + slot * 2) = 0xFFFF;
}

/* number of bones of a bone file (the last one is not counted when bit 31 is set) */
static u32 bone_count(const u32 *h)
{
    u32 n = h[1];

    if ((h[0] & 0x80000000) != 0)
        n--;
    return n;
}

/* model of a file: display list buffer + mesh at m */
static void build_mesh(u8 *self, u8 *mesh, u8 *m)
{
    u16 n = U16(m, 0x1C);
    void *buf = heap_alloc(MGR(self), mdl_count_1e(m) * 0x10 + n * 8, 0x10);

    func_088623DC(mesh, buf, m, mdl_data(m));
}

/* bones of a file: the scale is in the mesh (+0x10) */
static void build_bones(u8 *self, u8 *mesh, u8 *anim, u32 *b)
{
    u8 *buf = heap_alloc(MGR(self), bone_count(b) * 0x250, 0x10);
    f32 x = F32(mesh, 0x10), y = F32(mesh, 0x14), z = F32(mesh, 0x18);

    if (anim != NULL) {
        animator_clear(anim);
        objmgr_build_bones(x, y, z, anim, b, buf);
        func_088640F8(1.0f, 1.0f, 1.0f, anim, 0);
        animator_reset_bones(anim);
    }
}

/* data of the model of a slot, copied to VRAM when the slot is of type 1 */
static void *model_data(u8 *self, u8 *m, u32 slot, u32 a, u32 b)
{
    void *d;
    s32 size;

    if (self[0x800A8 + slot * 8] != 1)
        return mdl_data(m);
    d = (void *)objmgr_vram_addr(self, a, b);
    size = mdl_data_size(m);
    sceKernelDcacheWritebackInvalidateAll();
    sceDmacMemcpy(d, mdl_data(m), size);
    return d;
}

/* builds model (file 0), bones (file 1) and textures (file 2) of an object */
int objmgr_build_object(u8 *self, u8 *obj, u32 group)
{
    u32 *file = (u32 *)(self + 0x80258 + (u8)((u8)group * 8) * 4);
    u8 *m = func_088B89B0(*file, 0);
    void *buf;

    build_mesh(self, obj + 0x50, m);
    build_bones(self, obj + 0x50, obj + 0x80, (u32 *)func_088B89B0(*file, 1));
    m = func_088B89B0(*file, 2);
    buf = heap_alloc(MGR(self), U32(m, 8) << 5, 0x10);
    texset_build((u32 *)(obj + 0x70), (u32)buf, (const char *)m, 0);
    return 1;
}

/* creates the bones of an animator from a bone file, scaled by 1/x, 1/y, 1/z */
void objmgr_build_bones(f32 x, f32 y, f32 z, u8 *a, u32 *hdr, u8 *buf)
{
    u32 n = bone_count(hdr);
    s32 i;
    u32 j;
    u8 *src, *b;
    f32 m[16] __attribute__((aligned(16)));

    U32(a, 0x124) = n;
    for (i = 0; i < (s32)n; i++) {
        u32 *p = light_obj_ret((void *)0x250, buf + i * 0x250);

        if (p != NULL) {
            p[0] = (u32)D_089BAB08;
            p[0x6D] = 0;
            p[0x8D] = 0;
            p[0x53] = 0;
            p[0x52] = 0;
            p[0x51] = 0;
        }
    }
    src = (u8 *)hdr + 0xC + hdr[5];
    U16(a, 0x122) = 0;
    b = buf;
    for (i = 0; i < (s32)n; i++, b += 0x250) {
        func_0886005C(b, src);
        if (*(s32 *)(src + 0x10) != -1)
            U32(b, 0x144) = (u32)(buf + *(s32 *)(src + 0x10) * 0x250);
        if (*(s32 *)(src + 0x14) != -1)
            U32(b, 0x14C) = (u32)(buf + *(s32 *)(src + 0x14) * 0x250);
        if (*(s32 *)(src + 0x18) != -1)
            U32(b, 0x148) = (u32)(buf + *(s32 *)(src + 0x18) * 0x250);
        if (U32(src, 0x50) >= U16(a, 0x122))
            U16(a, 0x122) = U32(src, 0x50) + 1;
        src += U32(src, 8);
    }
    for (j = 0; j < hdr[4]; j++)
        U32(a, 0x110 + j * 4) = (u32)(buf + hdr[6 + j] * 0x250);
    U16(a, 0x120) = hdr[4];
    for (i = 0; i < 16; i++)
        m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    m[0] = m[0] / x;
    m[5] = m[5] / y;
    m[10] = m[10] / z;
    for (i = 0; i < U16(a, 0x120); i++)
        func_0886053C((u8 *)U32(a, 0x110 + i * 4), m);
}

/* object with two texture sets: its own one and the shared one of the group (slot +0x80274) */
void objmgr_build_object2(u8 *self, u8 *obj, u32 group)
{
    u32 *file = (u32 *)(self + 0x80258 + (u8)((u8)group + 8) * 4);
    u8 *m = func_088B89B0(*file, 0);
    void *buf;

    build_mesh(self, obj + 0x50, m);
    build_bones(self, obj + 0x50, obj + 0x80, (u32 *)func_088B89B0(*file, 1));
    m = func_088B89B0(*file, 2);
    buf = heap_alloc(MGR(self), 0x40, 0x10);
    texset_build((u32 *)(obj + 0x70), (u32)buf, (const char *)m, 0);
    texset_build((u32 *)(obj + 0x70), (u32)buf, (const char *)U32(self, 0x80274), 1);
}

/* weapon model of the hunter (slot 14) */
void objmgr_build_weapon(u8 *self, u8 *obj)
{
    u8 *m = func_088B89B0(U32(self, 0x80290), 0);
    void *buf;

    build_mesh(self, obj + 0xF60, m);
    build_bones(self, obj + 0xF60, obj + 0xF80, (u32 *)func_088B89B0(U32(self, 0x80290), 1));
    m = func_088B89B0(U32(self, 0x80290), 2);
    buf = heap_alloc(MGR(self), U32(m, 8) << 5, 0x10);
    texset_build((u32 *)(obj + 0x10C0), (u32)buf, (const char *)m, 0);
}

int objmgr_build_hunter_part(u8 *self, u8 *obj, u32 group, u32 i);

/* the 6 parts of a hunter (body, head, ...) */
int objmgr_build_hunter(u8 *self, u8 *obj, u32 group)
{
    s32 i;

    for (i = 0; i < 6; i++)
        objmgr_build_hunter_part(self, obj, group, (u8)i);
    return 1;
}

#define PART_MESH(o, i) ((i) == 0 ? (o) + 0x50 : (o) + ((i) - 1) * 0x20 + 0x830)
#define PART_ANIM(o, i) ((i) == 0 ? (o) + 0x80 : (o) + ((i) - 1) * 0x140 + 0x8D0)
#define PART_TEX(o, i)  ((i) == 0 ? (o) + 0x70 : (o) + ((i) - 1) * 0x10 + 0xF10)

int objmgr_build_hunter_part(u8 *self, u8 *obj, u32 group, u32 i)
{
    u32 slot, *file;
    u8 *m;
    void *data, *buf;

    group &= 0xFF;
    i &= 0xFF;
    slot = (u8)(i + group * 8);
    file = (u32 *)(self + 0x80258 + slot * 4);
    m = func_088B89B0(*file, 0);
    {
        u16 n = U16(m, 0x1C);
        buf = heap_alloc(MGR(self), mdl_count_1e(m) * 0x10 + n * 8, 0x10);
    }
    data = model_data(self, m, slot, group, i);
    func_088623DC(PART_MESH(obj, i), buf, m, data);
    build_bones(self, PART_MESH(obj, i), PART_ANIM(obj, i), (u32 *)func_088B89B0(*file, 1));
    m = func_088B89B0(*file, 2);
    buf = heap_alloc(MGR(self), 0x40, 0x10);
    texset_build((u32 *)PART_TEX(obj, i), (u32)buf, (const char *)m, 0);
    texset_build((u32 *)PART_TEX(obj, i), (u32)buf, (const char *)U32(self, 0x80274 + group * 0x20), 1);
    return 1;
}

/* weapon of a hunter group (slot group*8+6); 0 when a file is missing */
int objmgr_build_hunter_weapon(u8 *self, u8 *obj, u32 group)
{
    u32 slot = (u8)((u8)group * 8 + 6);
    u32 *file = (u32 *)(self + 0x80258 + slot * 4);
    u8 *m = func_088B89B0(*file, 0);
    u32 *b;
    void *buf, *data;

    if (m == NULL)
        return 0;
    {
        u16 n = U16(m, 0x1C);
        buf = heap_alloc(MGR(self), mdl_count_1e(m) * 0x10 + n * 8, 0x10);
    }
    data = model_data(self, m, slot, group, 6);
    func_088623DC(obj + 0xF60, buf, m, data);
    b = (u32 *)func_088B89B0(*file, 1);
    if (b == NULL)
        return 0;
    build_bones(self, obj + 0xF60, obj + 0xF80, b);
    m = func_088B89B0(*file, 2);
    if (m == NULL)
        return 0;
    buf = heap_alloc(MGR(self), U32(m, 8) << 5, 0x10);
    texset_build((u32 *)(obj + 0x10C0), (u32)buf, (const char *)m, 0);
    return 1;
}

/* frees the 6 parts of a hunter */
void objmgr_free_hunter(u8 *self, u8 *obj)
{
    s32 i;
    u8 *p;

    for (i = 0; i < 6; i++) {
        p = PART_MESH(obj, i);
        if (U32(p, 8) != 0)
            heap_free(MGR(self), (u8 *)U32(p, 8));
        memset(PART_MESH(obj, i), 0, 0x20);
        p = PART_ANIM(obj, i);
        if (U32(p, 0x110) != 0)
            heap_free(MGR(self), (u8 *)U32(p, 0x110));
        memset(PART_ANIM(obj, i), 0, 0x140);
        p = PART_TEX(obj, i);
        if (U32(p, 4) != 0)
            heap_free(MGR(self), (u8 *)U32(p, 4));
        memset(PART_TEX(obj, i), 0, 0x10);
    }
}

void objmgr_clear_1ac(u8 *self, u8 *obj)
{
    U32(obj, 0x1AC) = 0;
}

void objmgr_free_hunter_weapon(u8 *self, u8 *obj)
{
    if (U32(obj, 0xF68) != 0) {
        if (self[0x800A8 + (U16(obj, 0x1E6) * 8 + 6) * 8] == 1)
            U32(self, 0x80238) -= 0x3800;
        heap_free(MGR(self), (u8 *)U32(obj, 0xF68));
    }
    memset(obj + 0xF60, 0, 0x20);
    if (U32(obj, 0x1090) != 0)
        heap_free(MGR(self), (u8 *)U32(obj, 0x1090));
    memset(obj + 0xF80, 0, 0x140);
    if (U32(obj, 0x10C4) != 0)
        heap_free(MGR(self), (u8 *)U32(obj, 0x10C4));
    memset(obj + 0x10C0, 0, 0x10);
    objmgr_clear_1ac(self, obj);
}

/* frees the model of an object (part < 5: body, else: weapon) */
void objmgr_free_object(u8 *self, u8 *obj)
{
    u8 *p;

    p = obj[0x10F1] < 5 ? obj + 0x50 : obj + 0xF60;
    if (U32(p, 8) != 0)
        heap_free(MGR(self), (u8 *)U32(obj[0x10F1] < 5 ? obj + 0x50 : obj + 0xF60, 8));
    memset(obj[0x10F1] < 5 ? obj + 0x50 : obj + 0xF60, 0, 0x20);
    p = obj[0x10F1] < 5 ? obj + 0x80 : obj + 0xF80;
    if (U32(p, 0x110) != 0)
        heap_free(MGR(self), (u8 *)U32(obj[0x10F1] < 5 ? obj + 0x80 : obj + 0xF80, 0x110));
    memset(obj[0x10F1] < 5 ? obj + 0x80 : obj + 0xF80, 0, 0x140);
    p = obj[0x10F1] < 5 ? obj + 0x70 : obj + 0x10C0;
    if (U32(p, 4) != 0)
        heap_free(MGR(self), (u8 *)U32(obj[0x10F1] < 5 ? obj + 0x70 : obj + 0x10C0, 4));
    memset(obj[0x10F1] < 5 ? obj + 0x70 : obj + 0x10C0, 0, 0x10);
}

/* frees model, bones and textures of a simple object */
void objmgr_free_simple(u8 *self, u8 *obj)
{
    if (U32(obj, 0x58) != 0)
        heap_free(MGR(self), (u8 *)U32(obj, 0x58));
    memset(obj + 0x50, 0, 0x20);
    if (U32(obj, 0x190) != 0)
        heap_free(MGR(self), (u8 *)U32(obj, 0x190));
    memset(obj + 0x80, 0, 0x140);
    if (U32(obj, 0x74) != 0)
        heap_free(MGR(self), (u8 *)U32(obj, 0x74));
    memset(obj + 0x70, 0, 0x10);
}

/* same, also frees the VRAM buffer 0x29 when the object has no own texture (+0x1E8 == 0) */
void objmgr_free_with_vram(u8 *self, u8 *obj)
{
    if (U32(obj, 0x58) != 0) {
        heap_free(MGR(self), (u8 *)U32(obj, 0x58));
        if (obj[0x1E8] == 0)
            vram_free_surface(D_089CBBE0, 0x29);
    }
    if (U32(obj, 0x190) != 0)
        heap_free(MGR(self), (u8 *)U32(obj, 0x190));
    if (U32(obj, 0x74) != 0)
        heap_free(MGR(self), (u8 *)U32(obj, 0x74));
}
