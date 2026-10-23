/* Display list core: 20 layers chained with BASE/JUMP commands, block copy of commands,
 * TMH texture files, a small block heap. (Guessed names.) */
#include "menu.h"

extern u8 D_08A62C45;             /* set by the GE "finish" callback */
extern u8 D_0899D108[];
int sceGeListEnQueue(const void *list, void *stall, int cbid, void *arg);

#define GE_BASE(a) ((((u32)(a) & 0xFF000000) >> 8) | 0x10000000)
#define GE_JUMP(a) (((u32)(a) & 0xFFFFFF) | 0x08000000)

/* builds the 20 empty layers of the current display list: each one jumps to the next */
void dl_reset_layers(u8 *dl)
{
    u32 *w = (u32 *)(dl + U32(dl, 4) * 0xA0 + 8);
    int i;

    for (i = 0; i < 19; i++) {
        u32 *e = w + i * 2;

        e[0] = GE_BASE(e + 2);
        e[1] = GE_JUMP(e + 2);
        U32(dl, 0x148 + i * 4) = (u32)e;
    }
    w[38] = 0x10080000;
    w[39] = (u32)D_0899D108;
    U32(dl, 0x148 + 19 * 4) = (u32)(w + 38);
}

int dl_link(u8 *dl, u32 *p, s32 n, s32 layer);

/* copies n command words at the end of the display list and links them in the layer */
int dl_add(u8 *dl, const u32 *cmds, s32 n, s32 layer)
{
    u32 cur;
    s32 used, i;

    if (layer < 0 || layer >= 20)
        return 0;
    cur = U32(dl, 0x80198);
    used = (s32)(cur - (u32)(dl + U32(dl, 4) * 0x40000 + 0x198)) / 4;
    if ((u32)(n + used) > 0xFFFF)
        return 0;
    for (i = 0; i < n; i++) {
        **(u32 **)(dl + 0x80198) = cmds[i];
        U32(dl, 0x80198) += 4;
    }
    dl_link(dl, (u32 *)cur, n, layer);
    return 1;
}

/* links the block p (n words, the last 2 reserved for the jump back) at the end of the layer */
int dl_link(u8 *dl, u32 *p, s32 n, s32 layer)
{
    u32 *last = p + (n - 2);
    u32 *tail = *(u32 **)(dl + 0x148 + layer * 4);

    last[0] = tail[0];
    last[1] = tail[1];
    tail[0] = GE_BASE(p);
    tail[1] = GE_JUMP(p);
    *(u32 **)(dl + 0x148 + layer * 4) = last;
    return 1;
}

/* sends the display list of the previous frame to the GE */
void dl_kick(u8 *dl)
{
    D_08A62C45 = 0;
    sceGeListEnQueue(dl + (U32(dl, 4) ^ 1) * 0xA0 + 8, NULL, U32(dl, 0x8019C), NULL);
}

/* waits until the GE has finished the previous display list */
void dl_wait_ge(u8 *dl)
{
    while (*(volatile u8 *)&D_08A62C45 == 0)
        ;
}

u32 *dl_cur(u8 *dl)
{
    return (u32 *)U32(dl, 0x80198);
}

void dl_set_cur(u8 *dl, u32 *p)
{
    U32(dl, 0x80198) = (u32)p;
}

/* log2 of the smallest power of 2 >= v (at most 11) */
s32 pow2_log(void *unused, u32 v)
{
    u32 p = 1;
    s32 k = 0;

    while (p < v) {
        k++;
        if (k >= 11)
            break;
        p <<= 1;
    }
    return k;
}

/* finds the image img (and its palette pal) in a TMH file */
int tmh_get_image(void *unused, const char *f, s32 tex, u32 img, u32 pal, u32 *out)
{
    const s32 *p, *q, *r;
    s32 kind, i;
    u16 h;
    s16 w;
    u32 pk = 0, pn = 0;

    if (f[0] != '.' || f[1] != 'T' || f[2] != 'M' || f[3] != 'H')
        return 0;
    if (*(const s32 *)(f + 8) <= tex)
        return 0;
    p = (const s32 *)(f + 0x10);
    for (i = 0; i < tex; i++)
        p = (const s32 *)((const u8 *)p + p[0]);
    if (p[1] != 0)
        return 0;
    if ((u32)p[2] <= img)
        return 0;
    q = p + 4;
    for (i = 0; i < (s32)img; i++)
        q = (const s32 *)((const u8 *)q + q[0]);
    if (q[1] != 1)
        return 0;
    kind = q[2];
    w = q[3];
    h = ((const u16 *)q)[7];
    if (kind == 1 || kind == 3) {
        r = NULL;
    } else {
        u32 k;

        if ((u32)p[3] <= pal)
            return 0;
        r = p + 4;
        for (k = 0; k < (u32)p[2] + pal; k++)
            r = (const s32 *)((const u8 *)r + r[0]);
        if (r[1] != 2)
            return 0;
        pk = r[2];
        pn = (s16)r[3];
        r = r + 4;
    }
    out[0] = (u32)(q + 4);
    out[1] = kind;
    ((s16 *)out)[4] = w;
    ((u16 *)out)[5] = h;
    out[3] = (u32)r;
    out[4] = pk;
    out[5] = pn;
    return 1;
}

/* block heap: nodes of 0x20 bytes before each block, sizes in units of 16 bytes
 * node: 0 next block, 1 previous block, 2/3 free list, 4 used units, 5 free units after,
 * 6 offset of the node from the start of its area */
void heap_init(u32 *h, u32 *base, u32 size)
{
    h[0] = size;
    h[1] = (u32)base;
    base[0] = 0;
    base[1] = 0;
    base[2] = 0;
    base[3] = 0;
    base[6] = 0;
    base[4] = 2;
    base[5] = (h[0] - 0x20) >> 4;
    h[4] = h[1];
}

void *heap_alloc(u32 *h, u32 size, u32 align)
{
    u32 n, a;
    u32 *node, *prev, *nn, *nx;

    if (size == 0)
        return NULL;
    n = (size + align + 0x2F) >> 4;
    prev = h + 2;
    for (node = (u32 *)h[4]; node != NULL; prev = node, node = (u32 *)node[2]) {
        if (node[5] >= n)
            break;
    }
    if (node == NULL)
        return NULL;
    a = -align & ((u32)node + node[4] * 16 + 0x20 - node[6] + (align - 1));
    nn = (u32 *)(a - 0x20);
    nn[0] = node[0];
    if (node[0] != 0)
        ((u32 *)node[0])[1] = (u32)nn;
    nn[1] = (u32)node;
    nn[4] = n;
    nn[6] = node[6] + ((u32)nn - (u32)node) - node[4] * 16;
    nn[5] = node[5] - n;
    node[0] = (u32)nn;
    node[5] = 0;
    node[3] = 0;
    nx = (u32 *)node[2];
    if (nn[5] == 0) {
        nn[2] = 0;
        nn[3] = 0;
        prev[2] = (u32)nx;
        if (nx != NULL)
            nx[3] = (u32)prev;
    } else {
        nn[3] = (u32)prev;
        prev[2] = (u32)nn;
        nn[2] = (u32)nx;
        if (nx != NULL)
            nx[3] = (u32)nn;
    }
    return (void *)a;
}

void heap_free(u32 *h, u8 *p)
{
    u32 *node, *owner, *fn, *fp;

    if (p == NULL)
        return;
    node = (u32 *)(p - 0x20);
    owner = (u32 *)node[1];
    if (owner == NULL)
        return;
    owner[5] += node[4] + node[5];
    node[1] = 0;
    if (node[0] != 0) {
        owner[0] = node[0];
        ((u32 *)node[0])[1] = (u32)owner;
    } else {
        owner[0] = 0;
    }
    fn = (u32 *)node[2];
    fp = (u32 *)node[3];
    if (owner[3] == 0) {
        if (fp != NULL) {
            fp[2] = (u32)owner;
            owner[3] = (u32)fp;
            owner[2] = (u32)fn;
            if (fn != NULL)
                fn[3] = (u32)owner;
        } else {
            u32 *q = (u32 *)owner[1];
            u32 *a = h + 2;
            u32 *nx;

            for (; q != NULL; q = (u32 *)q[1]) {
                if (q[3] != 0) {
                    a = q;
                    break;
                }
            }
            nx = (u32 *)a[2];
            a[2] = (u32)owner;
            owner[3] = (u32)a;
            owner[2] = (u32)nx;
            if (nx != NULL)
                nx[3] = (u32)owner;
        }
    } else if (fp != NULL) {
        owner[2] = (u32)fn;
        if (fn != NULL)
            fn[3] = (u32)owner;
    }
}
