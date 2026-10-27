/* Constructors of the global objects of the game: each one stores the object in its global
 * pointer (g_* / D_*). (Guessed names.) */
#include "common.h"

extern void *D_09A44C60, *D_09A4F06C, *g_equip_data, *D_08A62C50, *g_result, *g_quest;
extern void *g_net, *g_net_players, *D_09A1FDDC, *D_08A62C60, *D_089CBBE8, *g_font, *g_vram;
extern void *g_loader, *D_08A62C64, *D_08A8B1E8, *D_08A62C40, *g_sys, *g_heap;
extern const u32 D_089BAC20[], D_089BAAA8[], D_089BABAC[];

void op_delete(void *p);
void heap_init(void *a, void *b, int n);
int sceKernelChangeThreadPriority(int thid, int prio);
void func_0890DE14(void);
void func_0890DE0C(void);
void sys_main_loop(void *self);

/* the main thread of the game */
s32 game_main(void)
{
    sceKernelChangeThreadPriority(0, 0x30);
    func_0890DE14();
    sys_main_loop(g_sys);
    func_0890DE0C();
    return 0;
}

/* the file loader (3 levels of classes) */
u32 *loader_dtor(u32 *self, s32 del)
{
    if (self != NULL) {
        self[0] = (u32)D_089BABAC;
        if (self != NULL) {
            self[0] = (u32)D_089BAAA8;
            if (self != NULL)
                self[0] = (u32)D_089BAC20;
        }
        if (self != NULL)
            g_loader = NULL;
        if ((s16)del > 0)
            op_delete(self);
    }
    return self;
}

void *set_d09a44c60(void *p) { D_09A44C60 = p; return p; }
void *set_d09a4f06c(void *p) { D_09A4F06C = p; return p; }
void *equip_data_ctor(void *p) { g_equip_data = p; return p; }
void *set_d08a62c50(void *p) { D_08A62C50 = p; return p; }
void *result_ctor(void *p) { g_result = p; return p; }
void *quest_obj_ctor(void *p) { g_quest = p; return p; }

u8 *net_ctor(u8 *p)
{
    g_net = p;
    p[0] = 0;
    return p;
}

u8 *net_players_ctor(u8 *p)
{
    g_net_players = p;
    p[4] = 0;
    return p;
}

void *set_d09a1fddc(void *p) { D_09A1FDDC = p; return p; }

u8 *set_d08a62c60(u8 *p)
{
    D_08A62C60 = p;
    p[0x74] = 0;
    return p;
}

void *textconv_ctor(void *p) { D_089CBBE8 = p; return p; }
void *font_ctor(void *p) { g_font = p; return p; }
void *vram_ctor(void *p) { g_vram = p; return p; }

u32 *loader_ctor(u32 *self)
{
    g_loader = self;
    self[0] = (u32)D_089BAC20;
    self[0] = (u32)D_089BAAA8;
    self[0] = (u32)D_089BABAC;
    return self;
}

u32 *loader_base_dtor(u32 *self, s32 del)
{
    if (self != NULL) {
        self[0] = (u32)D_089BAC20;
        if ((s16)del > 0)
            op_delete(self);
    }
    return self;
}

void *pad_ctor(void *p) { D_08A62C64 = p; return p; }

/* the scene manager: 4 slots and its heap */
u32 *scenes_ctor(u32 *self)
{
    D_08A8B1E8 = self;
    self[0] = 0;
    self[1] = 0;
    self[2] = 0;
    self[3] = 0;
    heap_init(self + 0x5004, self + 4, 0x14000);
    return self;
}

void *dl_ctor(void *p) { D_08A62C40 = p; return p; }
void *sys_ctor(void *p) { g_sys = p; return p; }
void *heap_ctor(void *p) { g_heap = p; return p; }

void *heap_dtor(void *self, s32 del)
{
    if (self != NULL) {
        if (self != NULL)
            g_heap = NULL;
        if ((s16)del > 0)
            op_delete(self);
    }
    return self;
}
