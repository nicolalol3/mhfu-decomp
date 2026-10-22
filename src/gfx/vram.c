/* Video memory (VRAM) manager: up to 46 "surfaces" (textures, buffers) allocated in
 * blocks of 128 bytes, plus 32 slots of 1 KB for palettes (CLUT) inside surface 45.
 * (Names and fields are inferred from usage: the game has no names for these functions.) */
#include "common.h"
#include <pspge.h>

#define VRAM_NSURF   46
#define VRAM_NBLOCKS 0x4000 /* blocks of 128 bytes (2 MB) */
#define VRAM_CLUTSURF 0x2D

typedef struct {
    u8 id;
    u8 type;        /* 0 = free; 3 = image; 5 = buffer; bit 1 = with palette */
    u16 nblocks;    /* 128-byte blocks in use */
    u16 width;      /* (for buffers: size in bytes) */
    u16 height;
    u32 addr;       /* indirizzo in VRAM */
    u16 format;     /* pixel format (0-5) */
    u16 _pad0e;
    u32 clut;       /* palette address (formats 4 and 5) */
    u16 user;
    u16 _pad16;
} VramSurface;      /* 0x18 byte */

typedef struct {
    VramSurface surf[VRAM_NSURF];
    u8 _pad[0x450 - VRAM_NSURF * 0x18];
    u32 edram_addr;
    u32 edram_size;
    u32 used[VRAM_NBLOCKS / 32]; /* 0x458: one bit per block */
    u32 clut_used;               /* 0xC58: one bit per palette slot */
} VramManager;

int vram_find_free_surface(VramManager *m);
int vram_find_blocks(VramManager *m, u32 count);
int vram_alloc_clut(VramManager *m);
void vram_free_clut(VramManager *m, u32 addr);
void vram_mark_blocks(VramManager *m, u32 first, int count);
void vram_unmark_blocks(VramManager *m, u32 first, int count);
u32 vram_surface_addr(VramManager *m, u32 id);
int vram_create_surface(VramManager *m, u32 id, u32 w, u32 h, u32 format, u32 user, u32 offset);

void vram_init(VramManager *m)
{
    int i;

    m->edram_addr = (u32)sceGeEdramGetAddr();
    m->edram_size = sceGeEdramGetSize();
    for (i = 0; i < VRAM_NSURF; i++) {
        m->surf[i].id = i;
        m->surf[i].type = 0;
    }
    for (i = 0; i < VRAM_NBLOCKS / 32; i++)
        m->used[i] = 0;
    m->clut_used = 0;
    /* fixed surfaces: two 512x272 screens, the z-buffer, and the palette space */
    vram_create_surface(m, 0, 0x200, 0x110, 1, 0, 0xffffffff);
    vram_create_surface(m, 1, 0x200, 0x110, 1, 0, 0xffffffff);
    vram_create_surface(m, 2, 0x200, 0x110, 0, 0, 0xffffffff);
    vram_create_surface(m, VRAM_CLUTSURF, 0x100, 0x20, 3, 0, 0x1f8000);
}

/* Creates a w x h surface in the given format; id 0xff = choose one; offset -1 = find
 * space. Returns the id or 0xff. */
int vram_create_surface(VramManager *m, u32 id, u32 w, u32 h, u32 format_arg, u32 user_arg, u32 offset)
{
    u16 format = format_arg;
    u16 user = user_arg;
    VramSurface *s;
    u32 nblocks;
    int first;
    u16 sw = w, sh = h;

    if ((id & 0xff) == 0xff) {
        id = vram_find_free_surface(m) & 0xff;
        if (id == 0xff)
            return 0xff;
    }
    s = &m->surf[id & 0xff];
    if (s->type != 0)
        return 0xff;
    s->format = format;
    s->user = user;
    switch (s->format) {
    case 0: case 1: case 2: /* 16 bits per pixel */
        s->width = ((sw + 7) >> 3) << 3;
        s->height = ((sh + 7) >> 3) << 3;
        s->clut = 0;
        nblocks = ((u32)s->width * s->height & 0x7fffffff) >> 6;
        break;
    case 3: /* 32 bits per pixel */
        s->width = ((sw + 3) >> 2) << 2;
        s->height = ((sh + 7) >> 3) << 3;
        s->clut = 0;
        nblocks = ((u32)s->width * s->height & 0x3fffffff) >> 5;
        break;
    case 4: /* 4 bits per pixel, with palette */
        s->width = ((sw + 0x1f) >> 5) << 5;
        s->height = ((sh + 7) >> 3) << 3;
        nblocks = ((int)((u32)s->width * s->height) / 2) / 128;
        if ((s->clut = vram_alloc_clut(m)) == (u32)-1)
            return 0xff;
        break;
    case 5: /* 8 bits per pixel, with palette */
        s->width = ((sw + 0xf) >> 4) << 4;
        s->height = ((sh + 7) >> 3) << 3;
        nblocks = (int)((u32)s->width * s->height) / 128;
        if ((s->clut = vram_alloc_clut(m)) == (u32)-1)
            return 0xff;
        break;
    default:
        return 0xff;
    }
    if (offset == 0xffffffff) {
        first = vram_find_blocks(m, nblocks);
        if (first == -1)
            return 0xff;
        s->addr = m->edram_addr + first * 0x80;
    } else {
        first = offset >> 7;
        s->addr = offset + m->edram_addr;
    }
    vram_mark_blocks(m, first, nblocks);
    s->nblocks = nblocks;
    s->type = 3;
    return id;
}

/* Creates a buffer of size bytes; like vram_create_surface. */
int vram_create_buffer(VramManager *m, u32 id, int size, u32 offset)
{
    VramSurface *s;
    u32 nblocks;
    int first;

    if ((id & 0xff) == 0xff) {
        id = vram_find_free_surface(m) & 0xff;
        if (id == 0xff)
            return 0xff;
    }
    s = &m->surf[id & 0xff];
    if (s->type != 0)
        return 0xff;
    nblocks = (u32)(size + 0x7f) >> 7;
    if (offset == 0xffffffff) {
        first = vram_find_blocks(m, nblocks);
        if (first == -1)
            return 0xff;
        s->addr = m->edram_addr + first * 0x80;
    } else {
        first = offset >> 7;
        s->addr = offset + m->edram_addr;
    }
    vram_mark_blocks(m, first, nblocks);
    s->nblocks = nblocks;
    s->type = 5;
    *(u32 *)&s->width = nblocks << 7;
    return id;
}

void vram_free_surface(VramManager *m, u32 id)
{
    VramSurface *s = &m->surf[id & 0xff];

    if (s->type == 0)
        return;
    if ((s->type & 2) && s->clut != 0)
        vram_free_clut(m, s->clut);
    vram_unmark_blocks(m, (s->addr - m->edram_addr) >> 7, s->nblocks);
    s->type = 0;
}

int vram_find_free_surface(VramManager *m)
{
    int i;

    for (i = 0; i < VRAM_NSURF; i++) {
        if (m->surf[i].type == 0)
            return i;
    }
    return 0xff;
}

/* first group of count consecutive free blocks, -1 if there is none */
int vram_find_blocks(VramManager *m, u32 count)
{
    u32 run = 0;
    int start = 0, i;

    for (i = 0; i < VRAM_NBLOCKS; i++) {
        if ((m->used[i / 32] & (1 << (i % 32))) == 0) {
            if (run == 0)
                start = i;
            if (++run >= count)
                return start;
        } else {
            run = 0;
        }
    }
    return -1;
}

/* one 1 KB palette slot in surface 45 */
int vram_alloc_clut(VramManager *m)
{
    int base = vram_surface_addr(m, VRAM_CLUTSURF);
    int i;

    if (base == -1)
        return -1;
    for (i = 0; i < 32; i++) {
        u32 *w = &(&m->clut_used)[i / 32];
        u32 bit = 1 << (i % 32);

        if ((*w & bit) == 0) {
            *w |= bit;
            return base + i * 0x400;
        }
    }
    return -1;
}

void vram_free_clut(VramManager *m, u32 addr)
{
    int base = vram_surface_addr(m, VRAM_CLUTSURF);
    u32 off;

    if (base == -1)
        return;
    off = addr - base;
    (&m->clut_used)[off >> 15] &= ~(1 << ((off & 0x7fff) >> 10));
}

void vram_mark_blocks(VramManager *m, u32 first, int count)
{
    u32 end = first + count;
    int i;

    for (; first < end; first++) {
        i = (int)first;
        m->used[i / 32] |= 1 << (i % 32);
    }
}

void vram_unmark_blocks(VramManager *m, u32 first, int count)
{
    u32 end = first + count;
    int i;

    for (; first < end; first++) {
        i = (int)first;
        m->used[i / 32] &= ~(1 << (i % 32));
    }
}

/* copies the surface data into out; 0 if the surface is free */
int vram_get_surface(VramManager *m, u32 id, VramSurface *out)
{
    VramSurface *s = &m->surf[id & 0xff];

    if (s->type != 0)
        *out = *s;
    return s->type != 0;
}

u32 vram_surface_addr(VramManager *m, u32 id)
{
    VramSurface *s = &m->surf[id & 0xff];

    return s->type == 0 ? (u32)-1 : s->addr;
}
