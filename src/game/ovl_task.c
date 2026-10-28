/* A task that loads the overlays 0x46 (slot 1) and 0x4C (slot 2) one after the other.
 * Fields: 0 vtable, 4..0xC position, 0x1C state. (Guessed names.) */
#include "common.h"

#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define F32(p, off) (*(f32 *)((u8 *)(p) + (off)))

extern u8 D_089BAB98[], D_089BABF8[];
extern const f32 D_089B2810[3], D_089B281C[3];
extern u32 *D_09A0FDC4;           /* the task */
extern u8 *g_sys, *g_loader, *g_heap, *g_vram;
extern u8 *D_089CC01C;            /* the fade */

s32 vec3_nonzero(f32 *v);
void stub_nop(void *p);
void vram_free_surface(void *m, u32 id);
void temp_free(void *a, int b);
void fade_start(u8 *f, u32 n, u32 c, s32 out);
void sys_load_overlay(u8 *self, u32 id, s32 slot);
s32 sys_loader_busy(u8 *self);
void sys_overlay_ready(u8 *self, u16 id, s32 slot);
u8 sys_overlay_is(u8 *self, s16 id, s32 slot);

static void set_pos(u32 *o, const f32 *src)
{
    f32 v[3];

    v[0] = src[0];
    v[1] = src[1];
    v[2] = src[2];
    if (vec3_nonzero(v) != 0) {
        F32(o, 4) = v[0];
        F32(o, 8) = v[1];
        F32(o, 0xC) = v[2];
    }
}

u32 *ovltask_ctor(u32 *o)
{
    o[0] = (u32)D_089BAB98;
    set_pos(o, D_089B2810);
    D_09A0FDC4 = o;
    o[0] = (u32)D_089BABF8;
    o[8] = 0;
    o[9] = 0;
    o[7] = 0;
    return o;
}

u32 *ovltask_dtor(u32 *o, s32 del)
{
    if (o != NULL) {
        o[0] = (u32)D_089BABF8;
        vram_free_surface(g_vram, 6);
        temp_free(g_heap, 4);
        temp_free(g_heap, 5);
        D_09A0FDC4 = NULL;
        o[0] = (u32)D_089BAB98;
        if ((s16)del > 0)
            stub_nop(o);
    }
    return o;
}

void ovltask_update(u32 *o)
{
    switch ((s32)o[7]) {
    case 0:
        fade_start(D_089CC01C, 1, 0, 1);
        sys_load_overlay(g_sys, 0x46, 1);
        o[7]++;
        return;
    case 1:
        if (sys_loader_busy(g_sys) != 0)
            return;
        if ((U32(g_sys, 0xF7A57C) & 1) != 0)
            g_loader[0x2FBDC] = 0;
        sys_overlay_ready(g_sys, 0x46, 1);
        o[7]++;
        /* fall through */
    case 2:
        if (sys_overlay_is(g_sys, 0x4C, 2) == 0) {
            sys_load_overlay(g_sys, 0x4C, 2);
            o[7]++;
        } else {
            o[7] = -1;
        }
        return;
    case 3:
        if (sys_loader_busy(g_sys) != 0)
            return;
        sys_overlay_ready(g_sys, 0x4C, 2);
        o[7] = 0;
        break;
    default:
        o[7] = 0;
        break;
    }
    set_pos(o, D_089B281C);
}
