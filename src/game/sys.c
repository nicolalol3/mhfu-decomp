/* The system object g_sys: start of the game, main loop, random generators, threads and
 * timers, system language, system overlays (code loaded in memory). (Guessed names.) */
#include "menu.h"

extern u8 *g_sys;
extern u8 *g_game;
extern void **g_loader;
extern u8 *g_monster_models;
extern u8 *g_sound;
extern u8 *g_vram;
extern u8 *g_camera;
extern void *g_heap;
extern u8 *D_08A62C40;            /* display lists */
extern u8 *D_08A62C64;            /* pad */
extern u8 *D_089CBFFC;            /* render list */
extern u8 **D_08A8B1E8;
extern u8 *D_09A1FDDC;
extern u8 *D_09A44C60;
extern s32 D_08A631A4;
extern u32 D_09A1DB50;
extern u8 D_0899BE20;
extern u8 *const D_089CBA70[];    /* load addresses of the system overlays */
extern const char D_089AD230[];   /* "CheckExitGame" */
extern const char D_089AD240[];
extern const char D_089AD248[];   /* "priorityChanger" */
extern const char D_089AD258[];   /* "ChangeThreadVTimer" */

typedef void (*LdFn0)(void *self);
typedef s32 (*LdFn1)(void *self, u32 a);
typedef void (*LdFn2)(void *self, u32 a, u32 b);
typedef s32 (*LdLoad)(void *self, u32 id, void *dest, s32 size, int a, int b, int c);
#define LOADER(off, T) ((T)((void **)g_loader[0])[(off) / 4])

int sceKernelGetThreadId(void);
int sceKernelChangeCurrentThreadAttr(int a, int b);
int sceKernelCreateThread(const char *name, void *entry, int prio, int stack, u32 attr, void *opt);
int sceKernelStartThread(int thid, u32 arglen, void *argp);
int sceKernelDeleteThread(int thid);
int sceKernelCreateCallback(const char *name, void *f, void *arg);
int sceKernelRegisterExitCallback(int cb);
int sceKernelDelayThreadCB(u32 delay);
int sceKernelDcacheWritebackAll(void);
int sceKernelIcacheInvalidateAll(void);
int sceKernelSuspendDispatchThread(void);
int sceKernelResumeDispatchThread(int s);
int sceKernelCpuSuspendIntr(void);
void sceKernelCpuResumeIntr(int s);
int sceKernelChangeThreadPriority(int thid, int prio);
int sceKernelSleepThread(void);
int sceKernelWakeupThread(int thid);
int sceKernelCreateVTimer(const char *name, void *opt);
int sceKernelSetVTimerHandlerWide(int uid, u32 unused, u32 lo, u32 hi, void *h, void *common);
int sceKernelStartVTimer(int uid);
int sceUtilityLoadModule(int id);
int sceUtilityGetSystemParamInt(int id, s32 *v);

void vram_init(u8 *v);
void camera_init(u8 *c);
void dl_init(u8 *d);
void dl_end_frame(u8 *d);
void dl_set_vblank_handler(u8 *d);
void pad_init(u8 *p);
void pad_read(u8 *p);
void rl_begin_frame(u8 *r);
void rl_set_states(u8 *r);
void rl_draw_all(u8 *r);
void rl_clear(u8 *r);
void sound_update(u8 *s);
void *heap_alloc(void *h, u32 size, u32 align);
u8 *popup_obj_ctor(void *p);
void *func_08897290(u32 size, void *p);
void func_08897BC0(int a);
void func_08898590(void *heap, u32 size);
void func_088DD838(u8 *m);
void func_088DD868(u8 *m);
void func_088DDA44(u8 *m);
s32 func_088DE014(u8 *models, u32 id);
s32 func_088DDD9C(u8 *models, void *dest, s32 idx, s32 size);
void func_0890DD9C(void *p, u32 size);
void func_088907D8(u8 *font);
void func_08890BBC(u8 *font);
void func_08891B14(u8 *font);
void func_08895538(u8 *p);
void func_088ADD3C(u8 *p);
void func_088BA2A0(u8 *p);

void sys_init(u8 *self);
void sys_read_language(u8 *self);
void sys_create_vtimer(u8 *self);
void sys_start_prio_thread(u8 *self);
void sys_set_text_kind(u8 *self);
s32 sys_exit_thread(void);
s32 sys_prio_thread(u32 args, u8 **argp);
u32 sys_vtimer_handler(int uid, u32 a, u32 b, u8 *self);
void sys_prio_restore(u8 *self);
u16 game_rand(u8 *self, s32 i);

/* the main loop of the game */
void sys_main_loop(u8 *self)
{
    sys_init(self);
    for (;;) {
        dl_end_frame(D_08A62C40);
        pad_read(D_08A62C64);
        func_08890BBC(g_font);
        LOADER(0x14, LdFn0)(g_loader);
        func_088DDA44(g_monster_models);
        rl_set_states(D_089CBFFC);
        func_08895538((u8 *)D_08A8B1E8);
        rl_draw_all(D_089CBFFC);
        rl_clear(D_089CBFFC);
        sound_update(g_sound);
        func_08891B14(g_font);
        U32(self, 0xF7A554)++;
    }
}

void sys_init(u8 *self)
{
    u8 **pp;
    void *p;
    u8 *o;

    U32(self, 0xF7F024) = sceKernelGetThreadId();
    self[0xF7F034] = 0;
    U32(self, 0xF7A554) = 0;
    sys_read_language(self);
    U32(self, 0xF7A568) = 0;
    sceKernelChangeCurrentThreadAttr(0, 0x4000);
    sys_create_vtimer(self);
    func_08897BC0(0);
    sys_start_prio_thread(self);
    U32(self, 0xF7A564) = sceKernelCreateThread(D_089AD230, sys_exit_thread, 0x6F, 0x1000, 0, NULL);
    if (sceKernelStartThread(U32(self, 0xF7A564), 0, NULL) < 0)
        sceKernelDeleteThread(U32(self, 0xF7A564));
    vram_init(g_vram);
    dl_init(D_08A62C40);
    pad_init(D_08A62C64);
    LOADER(0xC, LdFn0)(g_loader);
    func_08898590(g_heap, 0x83C000);
    func_088DD838(g_monster_models);
    dl_set_vblank_handler(D_08A62C40);
    LOADER(0x44, LdFn2)(g_loader, 0x2A, 2);
    sceUtilityLoadModule(0x100);
    sceUtilityLoadModule(0x101);
    rl_begin_frame(D_089CBFFC);
    func_088907D8(g_font);
    camera_init(g_camera);
    func_088ADD3C(D_09A1FDDC);
    pp = D_08A8B1E8;
    p = heap_alloc((u8 *)pp + 0x14010, 0x20, 0x10);
    if (p != NULL) {
        p = func_08897290(0x20, p);
        if (p == NULL) {
            *pp = NULL;
            o = NULL;
        } else {
            o = popup_obj_ctor(p);
            *pp = o;
        }
        if (o != NULL) {
            o[0x10] = 0;
            U32(*pp, 0x14) = 1;
            o = *pp;
            o[0x11] = 2;
            o[0x12] = 0;
        }
    }
    func_088BA2A0(D_09A44C60);
    func_088DD868(g_monster_models);
}

/* thread that waits for the HOME button exit */
s32 sys_exit_thread(void)
{
    D_08A631A4 = sceKernelCreateCallback(D_089AD240, (void *)0x08890060, NULL);
    sceKernelRegisterExitCallback(D_08A631A4);
    for (;;)
        sceKernelDelayThreadCB(10000);
}

void sys_seed(u8 *self, u16 seed)
{
    U16(self, 0xF7A56C) = seed & 0xFF;
    U16(self, 0xF7A56E) = seed & 0xFF;
}

void sys_restore_seed(void)
{
    U16(g_sys, 0xF7A570) = U16(g_game, 0x44BE);
}

/* random generator i (0-2) of the game */
u16 game_rand(u8 *self, s32 i)
{
    u16 *p = &U16(self, 0xF7A56C + i * 2);
    u32 x = *p;

    if (x == 0)
        x = 1;
    *p = (x * 11 * 16) % 0xFF53;
    return *p;
}

/* the generator 2, saved in the game */
u16 game_rand_saved(u8 *self)
{
    u16 r = game_rand(self, 2);

    U16(g_game, 0x44BE) = r;
    return r;
}

/* loads the system overlay id to the slot */
void sys_load_overlay(u8 *self, u32 id, s32 slot)
{
    u8 *dest = D_089CBA70[slot];
    s32 k = func_088DE014(g_monster_models, id);

    if (k == -1) {
        LOADER(0x34, LdLoad)(g_loader, id, dest, -1, 0, 0, 1);
    } else {
        func_088DDD9C(g_monster_models, dest, k, -1);
        sceKernelDcacheWritebackAll();
    }
}

s32 sys_loader_busy(u8 *self)
{
    return ((s32 (*)(void *))((void **)g_loader[0])[0x38 / 4])(g_loader);
}

/* the overlay id is loaded: flush the caches and remember it */
void sys_overlay_ready(u8 *self, u16 id, s32 slot)
{
    u8 *dest = D_089CBA70[slot];

    func_0890DD9C(dest, LOADER(0x18, LdFn1)(g_loader, id));
    U16(self, 0xF7A580 + slot * 2) = id;
    sceKernelIcacheInvalidateAll();
}

u8 sys_overlay_is(u8 *self, s16 id, s32 slot)
{
    return S16(self, 0xF7A580 + slot * 2) == id;
}

/* kind of text of the language (0: Japanese / English letters, 1: European) */
void sys_set_text_kind(u8 *self)
{
    switch (U32(self, 0xF7A574)) {
    case 1: case 2: case 3: case 4: case 5:
        U32(self, 0xF7A578) = 1;
        break;
    default:
        U32(self, 0xF7A578) = 0;
        break;
    }
}

/* code of the language for the system dialogs */
u32 sys_lang_code(u8 *self)
{
    switch (U32(self, 0xF7A574)) {
    case 0: return 0;
    case 1: return 1;
    case 2: return 4;
    case 3: return 2;
    case 4: return 3;
    case 5: return 5;
    case 6: return 9;
    }
    return 0;
}

void sys_start_prio_thread(u8 *self)
{
    U32(self, 0xF7F020) = sceKernelCreateThread(D_089AD248, sys_prio_thread, 0x28, 0x200, 0, NULL);
    sceKernelStartThread(U32(self, 0xF7F020), 4, &g_sys);
}

/* raises the priority of the game threads when woken (during the loading) */
s32 sys_prio_thread(u32 args, u8 **argp)
{
    u8 *self;
    s32 d, i;

    for (;;) {
        if ((GAME_FLAGS & 0x200) != 0) {
            self = *argp;
            d = sceKernelSuspendDispatchThread();
            i = sceKernelCpuSuspendIntr();
            sceKernelChangeThreadPriority(U32(self, 0xF7F024), 0x26);
            if (self[0xF7F034] != 0)
                sceKernelChangeThreadPriority(U32(self, 0xF7F030), 0x27);
            sceKernelCpuResumeIntr(i);
            sceKernelResumeDispatchThread(d);
        }
        sceKernelSleepThread();
    }
}

void sys_prio_restore(u8 *self)
{
    sceKernelChangeThreadPriority(U32(self, 0xF7F024), 0x30);
    if (self[0xF7F034] != 0)
        sceKernelChangeThreadPriority(U32(self, 0xF7F030), 0x31);
    sceKernelWakeupThread(U32(self, 0xF7F020));
}

void sys_create_vtimer(u8 *self)
{
    U32(self, 0xF7F028) = sceKernelCreateVTimer(D_089AD258, NULL);
}

void sys_start_vtimer(u8 *self, u32 t)
{
    sceKernelSetVTimerHandlerWide(U32(self, 0xF7F028), t, 0, 0, sys_vtimer_handler, self);
    sceKernelStartVTimer(U32(self, 0xF7F028));
    U32(self, 0xF7F02C) = 0;
}

u32 sys_vtimer_handler(int uid, u32 a, u32 b, u8 *self)
{
    sys_prio_restore(g_sys);
    if (++U32(self, 0xF7F02C) < 2)
        return 0x411A;
    return 0;
}

/* language of the system (PSP setting) -> language of the game */
void sys_read_language(u8 *self)
{
    s32 v;
    u32 l;

    if (sceUtilityGetSystemParamInt(8, &v) != 0) {
        l = 1;
    } else if (v == 5) {
        l = 5;
    } else if (v == 3) {
        l = 4;
    } else if (v == 4) {
        l = 2;
    } else if (v == 2) {
        l = 3;
    } else {
        l = 1;
    }
    U32(self, 0xF7A574) = l;
    D_09A1DB50 = l;
    sys_set_text_kind(self);
    D_0899BE20 = l;
}
