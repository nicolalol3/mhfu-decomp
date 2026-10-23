/* Display list manager (D_08A62C40): two 256 KB display lists used in turn, frame end.
 * (Guessed names.) */
#include "menu.h"

extern void *D_089CBBE0;          /* VRAM manager */
extern u8 *D_08A62C60;
extern void *D_09A1FDDC, *D_09A1DAEC, *D_09A0B544, *D_09A44C60;
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

void func_088592E4(u8 *dl);
void func_08859728(u8 *dl);
void func_088597BC(u8 *dl);
void func_08868B6C(void *a);
void func_088ADF24(void *a);
void func_088AB1CC(void *a);
void func_08898980(void *a);
void func_088C0084(void *a);
void func_08804678(int a, void *b, int c);
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
    func_088592E4(dl);
    sceGeListSync(sceGeListEnQueue(D_0899CD94, NULL, -1, NULL), 0);
    cb[2] = (u32)D_08859768;
    cb[0] = 0;
    cb[1] = 0;
    cb[3] = 0;
    U32(dl, 0x8019C) = sceGeSetCallback(cb);
    dl_swap(dl);
    func_088592E4(dl);
    D_08A62C45 = 1;
    dl[0x801A0] = 0;
}

/* end of the frame: networking, sound, wait for the vblank, next display list */
void dl_end_frame(u8 *dl)
{
    func_088597BC(dl);
    func_08868B6C(D_08A62C60);
    if ((*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) != 0) {
        func_088ADF24(D_09A1FDDC);
        func_088AB1CC(D_09A1DAEC);
        func_08898980(D_09A0B544);
    }
    func_088C0084(D_09A44C60);
    if (dl[0x801A0] == 0)
        dl_show(dl);
    dl_wait_vblank(dl);
    sceKernelDcacheWritebackAll();
    dl_swap(dl);
    func_08859728(dl);
}

void dl_set_vblank_handler(void)
{
    func_08804678(0, D_08859188, 0);
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
    func_088592E4(dl);
}

/* shows the frame buffer drawn in this frame */
void dl_show(u8 *dl)
{
    sceDisplaySetFrameBuf((void *)vram_surface_addr(D_089CBBE0, U32(dl, 4) != 0), 0x200, 1, 1);
}
