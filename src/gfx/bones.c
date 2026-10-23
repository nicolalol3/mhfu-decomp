/* Skeletons of the models: tree of bones (0x114 id, 0x144 parent, 0x148 next sibling,
 * 0x14C first child), with two animation slots per bone (0x150 and 0x1D0).
 * (Guessed names.) */
#include "menu.h"

extern const u16 D_08979758[];    /* bowgun parts: 5 u16 per entry, end 0xFFFF */
void func_0885FFBC(f32 a, f32 b, void *p0, void *p1, void *p2, void *p3);
void func_08863198(void *m, void *src, u32 t, void *out);
void func_088632D8(void *dst, void *a, void *b);

#define PARENT(n) (*(u8 **)((n) + 0x144))
#define NEXT(n) (*(u8 **)((n) + 0x148))
#define CHILD(n) (*(u8 **)((n) + 0x14C))
#define MATCH(n, id) (U16(n, 0x114) == (id) || (id) == 0xFFFFFFFF)

/* index of the bowgun e in the list of the bowgun parts (-1 if none) */
s32 bowgun_part_index(void *p, u8 *e)
{
    const u16 *t = D_08979758;
    s32 i = 0;

    if (e[1] != 5)
        return -1;
    for (; *t != 0xFFFF; t += 5, i++) {
        if (*t == U16(e, 2))
            return i;
    }
    return -1;
}

const u16 *bowgun_part(void *p, s32 i)
{
    return D_08979758 + i * 5;
}

/* gives to the bones id (or all with -1) of the subtree their animation tracks of data */
void bone_set_anim(void *unused, u8 *root, u8 *data, u32 id, s32 slot)
{
    u8 *n = root, *a;
    u8 *t = data + 0x14;

    for (;;) {
        int found = 0;

        if (MATCH(n, id)) {
            u8 *d = slot ? n + 0x1D0 : n + 0x150;

            U32(d, 0x64) = (u32)t;
            t += U32(t, 8);
            if (CHILD(n) != NULL) {
                n = CHILD(n);
                continue;
            }
        }
        a = NEXT(n);
        if (a != NULL && n != root) {
            n = a;
            continue;
        }
        while (n != root) {
            for (a = NEXT(n); a != NULL; a = NEXT(a)) {
                n = a;
                if (MATCH(a, id)) {
                    found = 1;
                    break;
                }
            }
            if (found)
                break;
            n = PARENT(n);
        }
        if (!found)
            return;
    }
}

void bone_anim_time(f32 a, f32 b, void *p0, void *p1, void *p2, void *p3)
{
    if (a < 0.0f)
        func_0885FFBC(0.0f, b - a, p0, p1, p2, p3);
    else
        func_0885FFBC(a, b, p0, p1, p2, p3);
}

/* first bone id of the tree (depth first; -1 = the root) */
u8 *bone_find(void *unused, u8 *root, u32 id)
{
    u8 *n = root;

    for (;;) {
        if (MATCH(n, id))
            return n;
        if (CHILD(n) != NULL) {
            n = CHILD(n);
            continue;
        }
        if (NEXT(n) != NULL) {
            n = NEXT(n);
            continue;
        }
        if (n == root)
            return NULL;
        for (;;) {
            if (NEXT(n) != NULL) {
                n = NEXT(n);
                break;
            }
            n = PARENT(n);
            if (n == root)
                return NULL;
        }
    }
}

void bone_set_anim_by_id(u8 *root, u8 *data, u32 id, s32 slot)
{
    u8 *n = bone_find(root, root, id);

    if (n != NULL)
        bone_set_anim(root, n, data, id, slot);
}

/* computes the matrices of the bones id of the subtree at the time t */
void bone_update(f32 t, u8 *n, u32 id, u32 slot)
{
    u8 *d = ((u8)slot == 0) ? n + 0x150 : n + 0x1D0;

    if (MATCH(n, id)) {
        int i;

        for (i = 0; i < 9; i++)
            U32(d, 0x40 + i * 4) = U32(n, 0x120 + i * 4);
        if (U32(d, 0x64) != 0)
            func_08863198(d, d + 0x40, (u32)t, d + 0x6C);
        func_088632D8(d, d, d + 0x40);
        if (CHILD(n) != NULL)
            bone_update(t, CHILD(n), id, slot);
    }
    if (NEXT(n) != NULL)
        bone_update(t, NEXT(n), id, slot);
}

void bone_update_by_id(f32 t, u8 *root, u32 id, u32 slot)
{
    u8 *n = bone_find(root, root, id);

    if (n != NULL)
        bone_update(t, n, id, slot);
}

void bone_copy_slot(u8 *n, u32 id);

void bone_copy_slot_by_id(u8 *root, u32 id)
{
    u8 *n = bone_find(root, root, id);

    if (n != NULL)
        bone_copy_slot(n, id);
}

/* copies the animation slot 0 to the slot 1 of the bones id of the subtree */
void bone_copy_slot(u8 *n, u32 id)
{
    if (MATCH(n, id)) {
        int i;

        for (i = 0; i < 19; i++)
            U32(n, 0x1D0 + i * 4) = U32(n, 0x150 + i * 4);
        if (CHILD(n) != NULL)
            bone_copy_slot(CHILD(n), id);
    }
    if (NEXT(n) != NULL)
        bone_copy_slot(NEXT(n), id);
}

f32 func_0886364C(void *a, void *b);
void func_0885FC9C(f32 a, f32 b, u8 *n, u32 id);

void bone_blend_by_id(f32 a, f32 b, u8 *root, u32 id)
{
    u8 *n = bone_find(root, root, id);

    if (n != NULL)
        func_0885FC9C(a, b, n, id);
}

/* largest value of func_0886364C over all the keys of the animation anim */
f32 anim_max_value(u8 *m, u8 *anim)
{
    f32 max = 0.0f;
    u8 *t = anim + 0x14;
    u32 i, j;

    for (i = 0; i < U32(anim, 4); i++, t += U32(t, 8)) {
        u8 *k = t + 0xC;

        for (j = 0; j < U32(t, 4); j++, k += U32(k, 8)) {
            f32 v = func_0886364C(m + 0x150, k);

            if (v > max)
                max = v;
        }
    }
    return max;
}
