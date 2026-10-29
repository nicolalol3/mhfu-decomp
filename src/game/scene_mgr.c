/* The scene manager (object D_08A8B1E8): 4 slots of scene objects (title, village, quest,
 * ...) made in its own heap at +0x14010; plus the tags of the texts of the font.
 * (Guessed names.) */
#include "libc.h"

#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))

extern u8 *D_089CC01C;            /* the fade object */
extern const u32 D_089BAB98[], D_089BAA94[];
extern const f32 D_089AD40C[3];

void *heap_alloc(void *h, u32 size, u32 align);
void *placement_new(u32 size, void *p);
s32 vec3_nonzero(f32 *v);
u32 scene_slot_used(u8 **self, u32 i);
u8 *popup_obj_ctor(void *p);
u8 *posobj_ctor(void *p);
u8 *pos_obj_ctor(void *p);
u8 *pos_obj2_ctor(void *p);
u8 *func_088B9B0C(void *p);
u8 *func_088B8374(void *p);
u8 *func_088B9DA8(void *p);
u8 *func_088BA0BC(void *p);
u8 *func_089038C4(void *p);
u8 *ovltask_ctor(void *p);
u8 *func_088B97BC(void *p);
u8 *func_088CC568(void *p);
u8 *func_08906744(void *p);

typedef u8 *(*SceneCtor)(void *p);

/* the fade object (scene 12) */
static u8 *fade_obj_ctor(void *mem)
{
    u32 *o = placement_new(0x34, mem);
    f32 v[3];

    if (o == NULL)
        return NULL;
    o[0] = (u32)D_089BAB98;
    v[0] = D_089AD40C[0];
    v[1] = D_089AD40C[1];
    v[2] = D_089AD40C[2];
    if (vec3_nonzero(v) != 0) {
        ((f32 *)o)[1] = v[0];
        ((f32 *)o)[2] = v[1];
        ((f32 *)o)[3] = v[2];
    }
    D_089CC01C = (u8 *)o;
    o[0] = (u32)D_089BAA94;
    return (u8 *)o;
}

/* state of a scene object (0x11: bit 7 paused, 1 started, 2 to show, 3 shown, 4 closing
 * with the counter 0x12): 1 when it is shown */
u32 scene_obj_step(u8 *o)
{
    u8 b = o[0x11];
    u8 k = b & 0x7F;
    s8 c;

    if (k == 0)
        return 0;
    if (k == 2) {
        o[0x11] = b & 0x80;
        o[0x11] |= 3;
        b = o[0x11];
    } else if (k != 3) {
        if (k == 4) {
            if ((b & 0x80) != 0)
                return 0;
            c = o[0x12];
            o[0x12] = c - 1;
            if (c < 1) {
                o[0x12] = 0;
                o[0x11] = 3;
                return 1;
            }
            return 0;
        }
        if (k == 1)
            o[0x11] = 1;
        return 0;
    }
    return (b & 0x80) == 0;
}

/* starts the scene kind in the slot */
void scene_start(u8 **self, u8 slot, u32 kind)
{
    static const u32 size[15] = {
        0, 0x20, 0x30, 0x24, 0xA0, 0x24, 0x3C, 100, 0x68, 0x129B0, 0x78, 0x70, 0x34, 0x218,
        0x20
    };
    static const SceneCtor ctor[15] = {
        NULL, popup_obj_ctor, func_088B9B0C, posobj_ctor, pos_obj2_ctor, pos_obj_ctor,
        func_088B8374, func_088B9DA8, func_088BA0BC, func_089038C4, ovltask_ctor, func_088B97BC,
        NULL, func_088CC568, func_08906744
    };
    void *mem;
    u8 *o;

    if ((u8)scene_slot_used(self, slot) != 0)
        return;
    if (kind == 0 || kind >= 15)
        return;
    if (slot >= 4)
        return;
    mem = heap_alloc((u8 *)self + 0x14010, size[kind], 0x10);
    if (mem == NULL)
        return;
    if (kind == 12) {
        o = fade_obj_ctor(mem);
    } else {
        o = placement_new(size[kind], mem);
        if (o != NULL)
            o = ctor[kind](o);
    }
    self[slot] = o;
    if (o == NULL)
        return;
    o[0x10] = slot;
    U32(self[slot], 0x14) = kind;
    o = self[slot];
    o[0x11] = 2;
    o[0x12] = 0;
}
