/* Small game objects with a vtable (C++ classes): constructors, destructors, update.
 * (Guessed names.) */
#include "menu.h"

extern u8 D_089BAB98[], D_089BAA6C[];   /* vtables */
extern const f32 D_0893B7A4[3], D_0893B7B0[3];
extern void *D_089CC01C;
extern u8 *D_089CBFFC;
extern u32 *D_089CC000;           /* current GE command pointer */

s32 vec3_nonzero(f32 *v);
void stub_nop(void *p);
s32 func_088901E0(void *a);
void func_088901F8(void *a, int b, int c);
void func_08890144(void *a, int b, int c);
void func_0884D754(void *a, int b, int c, int d);

/* constructor of a camera / position object (vtable D_089BAA6C, base D_089BAB98) */
u32 *pos_obj_ctor(u32 *o)
{
    f32 v[3];

    o[0] = (u32)D_089BAB98;
    v[0] = D_0893B7A4[0];
    v[1] = D_0893B7A4[1];
    v[2] = D_0893B7A4[2];
    if (vec3_nonzero(v) != 0) {
        ((f32 *)o)[1] = v[0];
        ((f32 *)o)[2] = v[1];
        ((f32 *)o)[3] = v[2];
    }
    o[0] = (u32)D_089BAA6C;
    o[7] = 0;
    return o;
}

/* destructor of the base class */
u32 *pos_obj_base_dtor(u32 *o, s32 flag)
{
    if (o != NULL) {
        o[0] = (u32)D_089BAB98;
        if ((s16)flag > 0)
            stub_nop(o);
    }
    return o;
}

u32 *pos_obj_dtor(u32 *o, s32 flag)
{
    if (o != NULL) {
        o[0] = (u32)D_089BAA6C;
        o[0] = (u32)D_089BAB98;
        if ((s16)flag > 0)
            stub_nop(o);
    }
    return o;
}

/* two-step update (state in o[7]) */
void pos_obj_update(u32 *o)
{
    if (o[7] == 1) {
        if (func_088901E0(g_sys) == 0) {
            f32 v[3];

            func_088901F8(g_sys, 0x41, 1);
            o[7] = 0;
            v[0] = D_0893B7B0[0];
            v[1] = D_0893B7B0[1];
            v[2] = D_0893B7B0[2];
            if (vec3_nonzero(v) != 0) {
                ((f32 *)o)[1] = v[0];
                ((f32 *)o)[2] = v[1];
                ((f32 *)o)[3] = v[2];
            }
        }
    } else if (o[7] == 0) {
        func_0884D754(D_089CC01C, 1, 0, 1);
        func_08890144(g_sys, 0x41, 1);
        o[7]++;
    }
}

/* constructor of a list manager (the only instance is D_089CBFFC) */
u8 *list_mgr_ctor(u8 *p)
{
    D_089CBFFC = p;
    U32(p, 0x1BC) = 0;
    U32(p, 0x1C0) = 0;
    U32(p, 0x1C4) = (u32)p;
    U32(p, 0x1C8) = (u32)(p + 4);
    U32(p, 0x1CC) = (u32)(p + 8);
    U32(p, 0x1D0) = (u32)(p + 0xC);
    U32(p, 0x1D4) = (u32)(p + 0x8C);
    U32(p, 0x1D8) = (u32)(p + 0xAC);
    U32(p, 0x1DC) = (u32)(p + 0x1AC);
    U32(p, 0x1E0) = (u32)(p + 0x1B0);
    U32(p, 0x1E4) = 0;
    U32(p, 0x1E8) = 0;
    U32(p, 0x1EC) = 0;
    U32(p, 0x1F0) = 0;
    U32(p, 0x1F4) = 0;
    U32(p, 0x1F8) = 0;
    U32(p, 0x1FC) = (u32)(p + 0x1B4);
    U32(p, 0x200) = 0;
    U32(p, 0x204) = (u32)(p + 0x1B8);
    U32(p, 0x208) = 0;
    p[0x20C] = 0;
    D_089CC000 = NULL;
    U32(p, 0x210) = 0;
    p[0x214] = 0;
    return p;
}

u8 *list_mgr_dtor(u8 *p, s32 flag)
{
    if (p != NULL) {
        D_089CBFFC = NULL;
        if ((s16)flag > 0)
            op_delete(p);
    }
    return p;
}

extern u8 D_089BAA80[];
extern const f32 D_0893B800[3], D_0893B80C[3], D_089797F8[3];
void func_0884D790(void *a, int b, int c);

/* second object of the same kind (vtable D_089BAA80) */
u32 *pos_obj2_ctor(u32 *o)
{
    f32 v[3];

    o[0] = (u32)D_089BAB98;
    v[0] = D_0893B800[0];
    v[1] = D_0893B800[1];
    v[2] = D_0893B800[2];
    if (vec3_nonzero(v) != 0) {
        ((f32 *)o)[1] = v[0];
        ((f32 *)o)[2] = v[1];
        ((f32 *)o)[3] = v[2];
    }
    o[0] = (u32)D_089BAA80;
    o[7] = 0;
    o[8] = 0;
    return o;
}

u32 *pos_obj2_dtor(u32 *o, s32 flag)
{
    if (o != NULL) {
        o[0] = (u32)D_089BAA80;
        o[0] = (u32)D_089BAB98;
        if ((s16)flag > 0)
            stub_nop(o);
    }
    return o;
}

void pos_obj2_update(u32 *o)
{
    if (o[7] == 1) {
        if (func_088901E0(g_sys) == 0) {
            f32 v[3];

            func_088901F8(g_sys, 0x40, 1);
            func_0884D790(D_089CC01C, 0x10, 0);
            ((u8 *)o)[0x90] = 0;
            o[7] = 0;
            v[0] = D_0893B80C[0];
            v[1] = D_0893B80C[1];
            v[2] = D_0893B80C[2];
            if (vec3_nonzero(v) != 0) {
                ((f32 *)o)[1] = v[0];
                ((f32 *)o)[2] = v[1];
                ((f32 *)o)[3] = v[2];
            }
            o[0xC] = 0;
        }
    } else if (o[7] == 0) {
        func_0884D754(D_089CC01C, 1, 0, 1);
        func_08890144(g_sys, 0x40, 1);
        o[7]++;
    }
}

/* resets an object of the base class */
void pos_obj_reset(u8 *o)
{
    f32 v[3];

    U32(o, 0x1C) = 0;
    U32(o, 0x20) = 0x11;
    U16(o, 0x24) = 0;
    U16(o, 0x26) = 0;
    o[0x28] = 0;
    o[0x29] = 0xFF;
    o[0x2F] = 0;
    o[0x2E] = 0;
    o[0x2D] = 0;
    o[0x2C] = 0;
    v[0] = D_089797F8[0];
    v[1] = D_089797F8[1];
    v[2] = D_089797F8[2];
    if (vec3_nonzero(v) != 0) {
        F32(o, 4) = v[0];
        F32(o, 8) = v[1];
        F32(o, 0xC) = v[2];
    }
}

extern u8 D_089BAA94[];

/* screen fade (the instance is D_089CC01C): starts a fade of n frames, color c */
void fade_start(u8 *f, u32 n, u32 c, s32 out)
{
    U16(f, 0x24) = n;
    U16(f, 0x26) = 0;
    f[0x28] = c;
    if (out == 1) {
        U32(f, 0x1C) = 2;
        f[0x29] = 0xFF;
    } else {
        U32(f, 0x1C) = 1;
        f[0x29] = 0;
    }
    f[0x2A] = f[0x2F];
}

/* fade from / to black */
void fade_start_black(u8 *f, u32 n, s32 out)
{
    fade_start(f, n, 0, out);
    f[0x2C] = 0;
    f[0x2D] = 0;
    f[0x2E] = 0;
}

/* fade from / to white */
void fade_start_white(u8 *f, u32 n, s32 out)
{
    fade_start(f, n, 0, out);
    f[0x2C] = 0xFF;
    f[0x2D] = 0xFF;
    f[0x2E] = 0xFF;
}

u32 *fade_dtor(u32 *o, s32 flag)
{
    if (o != NULL) {
        o[0] = (u32)D_089BAA94;
        D_089CC01C = NULL;
        o[0] = (u32)D_089BAB98;
        if ((s16)flag > 0)
            stub_nop(o);
    }
    return o;
}
