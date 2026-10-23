/* Common code of the game entities (animated models with flags at 0x288 / 0x28C).
 * (Guessed names.) */
#include "menu.h"

extern void *D_08A62D8C;          /* sound */
extern const f32 D_09A5F114[3];
void animator_set(u8 *a, u8 *anim, s32 slot);
void animator_play(f32 t, u8 *a, u8 *anim, s32 x, s32 slot);
u8 *animator_bone(u8 *a, s32 i);
s32 func_08883B54(void *snd, int a, int b, int c, int d, int e, int f, int g, int h, int i, int j);

/* data i (3 decimal digits: table, index) of the entity file at o+0x1AC */
u8 *entity_data(u8 *o, u32 i)
{
    u8 *f = (u8 *)U32(o, 0x1AC);
    u32 *t;
    u32 k;
    s32 off;

    if (f == NULL)
        return NULL;
    k = (i % 1000) % 100;
    t = (u32 *)(f + ((i % 1000) / 100) * 8);
    if (k >= t[0])
        return NULL;
    off = *(s32 *)(f + t[1] + k * 4);
    if (off == -1)
        return NULL;
    return f + off;
}

/* angles (0x10000 = full turn) to radians */
void angles_to_rad(f32 *out, const u16 *a)
{
    out[0] = 9.58738e-05f * (f32)a[0];
    out[1] = 9.58738e-05f * (f32)a[2];
    out[2] = 9.58738e-05f * (f32)a[4];
    out[3] = 0.0f;
}

#define VGET(o, n) (*(u8 *(**)(void *, s32))(U32(o, 0) + 0x20))(o, n)

/* sets the animation id (slot -1: all the slots, with the animations id, id+200, ...) */
void entity_set_anim(u8 *o, s32 id, u8 *anim, s32 slot)
{
    u32 i;

    if (slot != -1) {
        animator_set(o + 0x80, anim, slot);
        return;
    }
    for (i = 0; i < U16(o, 0x1A2); i++) {
        animator_set(o + 0x80, VGET(o, id + i * 200), i);
        U16(o, 0x324 + i * 2) = id;
    }
}

void entity_play_anim(f32 t, u8 *o, s32 id, u8 *anim, s32 slot)
{
    u32 i;

    if (slot != -1) {
        animator_play(0.0f, o + 0x80, anim, (u32)t, slot);
        return;
    }
    for (i = 0; i < U16(o, 0x1A2); i++)
        animator_play(0.0f, o + 0x80, VGET(o, id + i * 200), (u32)t, i);
}

u32 entity_flag(u8 *o, u32 f);
void entity_set_flag(u8 *o, u32 f);
void entity_clear_flag(u8 *o, u32 f);

/* per-frame update of the common counters of an entity */
void entity_update_common(u8 *o)
{
    if (o[0x33C] == 0) {
        if (entity_flag(o, 0x400001) == 0)
            U16(o, 0x45C) = 0;
        else
            U16(o, 0x45C) += 2;
        U32(o, 0x3F4) += 2;
        if ((s32)U32(o, 0x3F4) >= 100000)
            U32(o, 0x3F4) = 10000;
        if (o[0x276] != 0)
            o[0x276]--;
        (*(void (**)(void *))(U32(o, 0) + 0x3C))(o);
    }
    if ((U32(D_089CC438, 0x6AF14) & 1) == 0) {
        entity_clear_flag(o, 0x800);
        if (entity_flag(o, 1) == 0)
            entity_clear_flag(o, 0x2000);
        else
            entity_set_flag(o, 0x2000);
    }
}

/* flags: bit 31 selects the second word */
void entity_set_flag(u8 *o, u32 f)
{
    if ((f & 0x80000000) == 0)
        U32(o, 0x288) |= f;
    else
        U32(o, 0x28C) |= f & 0x7FFFFFFF;
}

void entity_clear_flag(u8 *o, u32 f)
{
    if ((f & 0x80000000) == 0)
        U32(o, 0x288) &= ~f;
    else
        U32(o, 0x28C) &= ~(f & 0x7FFFFFFF);
}

void entity_set_mode(u8 *o, u32 mode, s32 f1, s32 f2)
{
    if ((u8)mode == 2)
        o[0x280] = 2;
    else if ((u8)mode == 1)
        o[0x280] = 1;
    else
        o[0x280] = 0;
    if (((u16)mode & 0x8000) == 0)
        entity_set_flag(o, 8);
    else
        entity_clear_flag(o, 8);
    if (f1 == 0)
        entity_clear_flag(o, 1);
    else
        entity_set_flag(o, 1);
    if (f2 == 0)
        entity_clear_flag(o, 2);
    else
        entity_set_flag(o, 2);
}

/* 1 if the entity is in the current area */
int entity_in_area(u8 *o)
{
    return S16(o, 0x29A) == S16(D_089CC438, 0x6AF0E);
}

u32 entity_flag(u8 *o, u32 f)
{
    if ((f & 0x80000000) == 0)
        return U32(o, 0x288) & f;
    return U32(o, 0x28C) & f & 0x7FFFFFFF;
}

int entity_is(u8 *o, u32 a, u32 b)
{
    if (o[0x298] != (u8)a)
        return 0;
    return o[0x299] == (u8)b;
}

/* hides the entity (out of the screen) */
void entity_hide(u8 *o)
{
    U32(o, 4) &= ~1;
    F32(o, 0x3FC) = D_09A5F114[0];
    F32(o, 0x400) = D_09A5F114[1];
    F32(o, 0x404) = D_09A5F114[2];
}

/* position of the bone i */
void entity_bone_pos(u8 *o, f32 *out, s32 i)
{
    u8 *b = animator_bone(o + 0x80, (s16)i);

    out[0] = F32(b, 0x100);
    out[1] = F32(b, 0x104);
    out[2] = F32(b, 0x108);
}

u8 *entity_bone_matrix(u8 *o, s32 i)
{
    return animator_bone(o + 0x80, (s16)i) + 0xD0;
}

/* plays a sound of the entity (only if it is in the current area) */
s32 entity_sound(u8 *o, int a, int b, int c, int d, int e, int f, int g, int h)
{
    if ((u8)entity_in_area(o) != 1)
        return 0;
    return func_08883B54(D_08A62D8C, 1, a, b, c, d, e, f, g, h, 0);
}
