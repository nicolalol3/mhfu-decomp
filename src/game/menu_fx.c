/* Menu: small animated effects (flashes and sparkles) drawn on top of the menu.
 * 6 effects of 0xC bytes at self+0x112A: s8 state, u8 -, u8 active, u8 kind,
 * s16 x, s16 y, u8 timer, u8 size, u8 alpha, u8 size2. (Guessed names.) */
#include "menu.h"

extern const s16 D_089386F0, D_089386F2, D_089386F4, D_089386F6, D_089386F8;
extern const s16 D_089386FC, D_089386FE, D_08938700, D_08938702, D_08938704, D_08938706;
extern const s16 D_08938708, D_0893870A, D_0893870C, D_0893870E, D_08938710, D_08938712;
extern const s16 D_08938714, D_08938716;

void func_08890E08(void *font, int v);
void func_08832A9C(void *self, int on);
void func_0882C11C(void *self, u16 x, u16 y, s32 w, s32 h, u8 r, u8 g, u8 b, u8 a, int mode);
void func_0882CC9C(void *self, s16 *pts, int r, int g, int b, int a);
void menu_begin_draw(void *self);
void fx_update_flash(void *self, u8 *e);
void fx_update_star(void *self, u8 *e);
void fx_update_ring(void *self, u8 *e);

/* updates and draws the active effects */
void fx_draw_all(u8 *self)
{
    u8 *p = self;
    int i;

    self[0x534] = 12;
    func_08890E08(D_08A631A8, 3);
    menu_begin_draw(self);
    func_08832A9C(self, 1);
    for (i = 0; i < 6; i++, p += 0xC) {
        if (p[0x112C] == 1) {
            if (p[0x112D] == 2)
                fx_update_ring(self, p + 0x112A);
            else if (p[0x112D] == 1)
                fx_update_star(self, p + 0x112A);
            else if (p[0x112D] == 0)
                fx_update_flash(self, p + 0x112A);
        }
    }
    func_08832A9C(self, 0);
}

/* free effect slot (NULL if none) */
u8 *fx_alloc(u8 *self)
{
    int i;

    for (i = 0; i < 6; i++)
        if (self[i * 0xC + 0x112C] == 0)
            return self + i * 0xC + 0x112A;
    return NULL;
}

/* horizontal flash: grows, then fades out */
void fx_update_flash(void *self, u8 *e)
{
    switch ((s8)e[0]) {
    case 4:
        e[2] = 0;
        return;
    case 3:
        if ((s32)e[10] - D_089386F6 <= 0) {
            e[0] = (s8)e[0] + 1;
            return;
        }
        e[10] -= (u8)D_089386F6;
        break;
    case 2:
        e[9] += (u8)D_089386F0;
        e[10] += (u8)D_089386F2;
        e[8] -= 1;
        if (e[8] == 0)
            e[0] = (s8)e[0] + 1;
        break;
    case 1:
        e[8] = D_089386F4;
        e[0] = (s8)e[0] + 1;
        return;
    case 0:
        e[9] = 6;
        e[10] = 64;
        if (e[8] != 0)
            e[8] -= 1;
        else
            e[0] = (s8)e[0] + 1;
        return;
    default:
        return;
    }
    func_0882C11C(self, S16(e, 4) - (e[9] >> 1), S16(e, 6) - (D_089386F8 >> 1), e[9], D_089386F8,
                  128, 32, 204, e[10], 129);
}

/* diamond outline of size e[11] around the effect position */
static void fx_diamond(void *self, u8 *e)
{
    s16 pts[8];
    s16 x = S16(e, 4);
    s16 h = e[11] >> 1;
    s16 y = S16(e, 6);

    pts[0] = x - h;
    pts[1] = y;
    pts[2] = x;
    pts[3] = y - h;
    pts[4] = x;
    pts[5] = y + h;
    pts[6] = x + h;
    pts[7] = y;
    func_0882CC9C(self, pts, 240, 240, 128, 127);
}

/* sparkle: square that fades out with a shrinking diamond */
void fx_update_star(void *self, u8 *e)
{
    switch ((s8)e[0]) {
    case 3:
        e[0] = (s8)e[0] + 1;
        e[2] = 0;
        return;
    case 2:
        if ((s32)e[10] - D_08938700 <= 0) {
            e[0] = (s8)e[0] + 1;
            return;
        }
        e[10] -= (u8)D_08938700;
        func_0882C11C(self, S16(e, 4) - (e[9] >> 1), S16(e, 6) - (e[9] >> 1), (s16)e[9], (s16)e[9],
                      240, 240, 128, e[10], 129);
        fx_diamond(self, e);
        if (D_08938706 < e[11])
            e[11] -= (u8)D_08938706;
        return;
    case 1:
        e[0] = (s8)e[0] + 1;
        e[8] = D_08938704;
        e[11] += (u8)D_08938702;
        fx_diamond(self, e);
        return;
    case 0:
        if (e[8] > 0) {
            e[8] -= 1;
        } else {
            e[0] = (s8)e[0] + 1;
            e[9] = D_089386FC;
            e[10] = D_089386FE;
            e[11] = D_08938702;
        }
        return;
    }
}

/* ring: square with an expanding inner square */
void fx_update_ring(void *self, u8 *e)
{
    switch ((s8)e[0]) {
    case 3:
        e[2] = 0;
        return;
    case 2:
        if ((s32)e[10] - D_0893870E <= 0) {
            e[0] = (s8)e[0] + 1;
            return;
        }
        e[10] -= (u8)D_0893870E;
        func_0882C11C(self, S16(e, 4) - (e[9] >> 1), S16(e, 6) - (e[9] >> 1), (s16)e[9], (s16)e[9],
                      (u8)D_08938716, (u8)D_08938716, (u8)(D_08938716 - 128), e[10], 129);
        e[11] += (u8)D_08938712;
        func_08832A9C(self, 0);
        func_0882C11C(self, S16(e, 4) - (e[11] >> 1), S16(e, 6) - (e[11] >> 1), (s16)e[11], (s16)e[11],
                      (u8)D_08938714, (u8)D_08938714, (u8)D_08938714, e[10], 128);
        func_08832A9C(self, 1);
        return;
    case 1:
        func_0882C11C(self, S16(e, 4) - (e[9] >> 1), S16(e, 6) - (e[9] >> 1), (s16)e[9], (s16)e[9],
                      (u8)D_08938716, (u8)D_08938716, (u8)(D_08938716 - 128), e[10], 129);
        e[0] = (s8)e[0] + 1;
        e[9] += (u8)D_0893870A;
        e[11] = D_08938710;
        return;
    case 0:
        e[0] = (s8)e[0] + 1;
        e[9] = D_08938708;
        e[10] = D_0893870C;
        return;
    }
}
