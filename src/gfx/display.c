/* Display list manager (D_08A62C40): two 256 KB display lists used in turn, frame end.
 * (Guessed names.) */
#include "menu.h"

extern void *g_vram;          /* VRAM manager */
extern u8 *D_08A62C60;
extern void *D_09A1FDDC, *g_net, *g_net_players, *D_09A44C60;
extern u8 D_08A62C44, D_08A62C45;
extern const u32 D_0899CD34[2][12];   /* GE lists that start a frame */
extern const u32 D_0899CD94[];
extern u8 D_08859768[], D_08859188[];

int sceDisplaySetMode(int mode, int w, int h);
int sceDisplaySetFrameBuf(void *top, int bufw, int fmt, int sync);
int sceGeEdramSetAddrTranslation(int width);
int sceGeListEnQueue(const void *list, void *stall, int cbid, void *arg);
int sceGeListSync(int qid, int mode);
int sceGeSetCallback(void *cb);
int sceKernelSleepThreadCB(void);
void sceKernelDcacheWritebackAll(void);

void dl_reset_layers(u8 *dl);
void dl_kick(u8 *dl);
void dl_wait_ge(u8 *dl);
void osk_update(void *a);
void func_088ADF24(void *a);
void func_088AB1CC(void *a);
void func_08898980(void *a);
void func_088C0084(void *a);
void SetVblankSubIntrHandler(int a, void *b, int c);
u32 vram_surface_addr(void *m, u32 id);

void dl_swap(u8 *dl);
void dl_show(u8 *dl);
void dl_wait_vblank(u8 *dl);

/* display and GE setup, GE callback */
void dl_init(u8 *dl)
{
    u32 cb[4];

    sceDisplaySetMode(0, 0x1E0, 0x110);
    sceGeEdramSetAddrTranslation(0x400);
    U32(dl, 4) = 0;
    dl_reset_layers(dl);
    sceGeListSync(sceGeListEnQueue(D_0899CD94, NULL, -1, NULL), 0);
    cb[2] = (u32)D_08859768;
    cb[0] = 0;
    cb[1] = 0;
    cb[3] = 0;
    U32(dl, 0x8019C) = sceGeSetCallback(cb);
    dl_swap(dl);
    dl_reset_layers(dl);
    D_08A62C45 = 1;
    dl[0x801A0] = 0;
}

/* end of the frame: networking, sound, wait for the vblank, next display list */
void dl_end_frame(u8 *dl)
{
    dl_wait_ge(dl);
    osk_update(D_08A62C60);
    if ((*(u32 *)(g_sys + 0xF7A57C) & 1) != 0) {
        func_088ADF24(D_09A1FDDC);
        func_088AB1CC(g_net);
        func_08898980(g_net_players);
    }
    func_088C0084(D_09A44C60);
    if (dl[0x801A0] == 0)
        dl_show(dl);
    dl_wait_vblank(dl);
    sceKernelDcacheWritebackAll();
    dl_swap(dl);
    dl_kick(dl);
}

void dl_set_vblank_handler(void)
{
    SetVblankSubIntrHandler(0, D_08859188, 0);
}

void dl_wait_vblank(u8 *dl)
{
    D_08A62C44 = 0;
    sceKernelSleepThreadCB();
}

/* switches to the other display list */
void dl_swap(u8 *dl)
{
    U32(dl, 4) ^= 1;
    sceGeListSync(sceGeListEnQueue(D_0899CD34[U32(dl, 4)], NULL, -1, NULL), 0);
    U32(dl, 0x80198) = (u32)(dl + U32(dl, 4) * 0x40000 + 0x198);
    dl_reset_layers(dl);
}

/* shows the frame buffer drawn in this frame */
void dl_show(u8 *dl)
{
    sceDisplaySetFrameBuf((void *)vram_surface_addr(g_vram, U32(dl, 4) != 0), 0x200, 1, 1);
}
