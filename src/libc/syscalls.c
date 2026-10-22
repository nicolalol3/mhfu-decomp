/* Libreria C del PSP: funzioni di base appoggiate al sistema operativo
 * (file, memoria dinamica "sbrk", uscita dal programma, tempo). */
#include "common.h"
#include <pspiofilemgr.h>
#include <pspstdio.h>
#include <pspsysmem.h>
#include <pspthreadman.h>
#include <pspmodulemgr.h>
#include <psputils.h>
#include <pspintrman.h>
#include "sce_extra.h"

#define EBADF 9

extern int sce_newlib_heap_kb_size __attribute__((weak)); /* dimensione heap in KB, scelta dal gioco */
extern u32 g_sbrk_cur;
extern u32 g_sbrk_start;
extern u32 g_sbrk_end;
extern SceUID g_sbrk_block;

extern int func_0880C5A0(void) __attribute__((weak)); /* id thread corrente (pthread) */
extern int func_0880B368(void);                         /* id thread principale */
extern void func_0880E19C(int thid);
extern SceUID func_0880C7E0(int thid);

/* I descrittori 0, 1, 2 della libc corrispondono a stdin/stdout/stderr del sistema. */
static inline int libc_fd_to_sce(int fd)
{
    switch (fd) {
    case 0: return sceKernelStdin();
    case 1: return sceKernelStdout();
    case 2: return sceKernelStderr();
    }
    return fd;
}

int lseek(int fd, int offset, int whence)
{
    fd = libc_fd_to_sce(fd);
    if (fd < 0)
        return -EBADF;
    /* l'originale converte il descrittore una seconda volta */
    return (int)sceIoLseek(libc_fd_to_sce(fd), offset, whence);
}

void _exit(int status)
{
    if (&func_0880C5A0 != NULL) {
        int thid = func_0880C5A0();
        SceUID modid;

        if (thid != func_0880B368()) {
            sceKernelPrintf("libc:%s: should be called from main thread\n", "_exit");
            sceKernelExitThread(status);
        }
        func_0880E19C(thid);
        modid = func_0880C7E0(thid);
        if (sceKernelGetModuleId() != modid) {
            int st[4];
            sceKernelStopModule(modid, 0, NULL, st, NULL);
            sceKernelUnloadModule(modid);
            sceKernelExitThread(status);
        }
    }
    sceKernelStopUnloadSelfModuleWithStatus(1, 0, NULL, NULL, NULL);
}

/* Fa crescere la memoria dinamica della libc; al primo uso alloca il blocco dal sistema. */
void *sbrk(int incr)
{
    u32 next;
    u32 prev;

    if (g_sbrk_start == 0) {
        int size = 64 * 1024;

        if (&sce_newlib_heap_kb_size != NULL) {
            if (sce_newlib_heap_kb_size == 0)
                return (void *)-1;
            size = sce_newlib_heap_kb_size << 10;
        }
        if (size != 0) {
            g_sbrk_block = sceKernelAllocPartitionMemory(2, "UserSbrk", 3 /* basso, allineato */, size, (void *)0x1000);
            if (g_sbrk_block > 0) {
                u32 base = (u32)sceKernelGetBlockHeadAddr(g_sbrk_block);
                g_sbrk_end = base + size;
                g_sbrk_cur = base;
                g_sbrk_start = base;
            }
        }
    }
    if (g_sbrk_start == 0)
        return (void *)-1;
    next = g_sbrk_cur + incr;
    if (next < g_sbrk_start || g_sbrk_end < next)
        return (void *)-1;
    prev = g_sbrk_cur;
    g_sbrk_cur = next;
    return (void *)prev;
}

int read(int fd, void *buf, int n)
{
    fd = libc_fd_to_sce(fd);
    if (fd < 0)
        return -EBADF;
    return sceIoRead(fd, buf, n);
}

int write(int fd, const void *buf, int n)
{
    fd = libc_fd_to_sce(fd);
    if (fd < 0)
        return -EBADF;
    return sceIoWrite(fd, buf, n);
}

int close(int fd)
{
    fd = libc_fd_to_sce(fd);
    if (fd < 0)
        return -EBADF;
    return sceIoClose(fd);
}

/* Tutti i file vengono descritti come dispositivi a caratteri (S_IFCHR). */
int fstat(int fd, u32 *st)
{
    st[1] = 0x2000;
    return 0;
}

int isatty(int fd)
{
    return 1;
}

int rename(const char *from, const char *to)
{
    return sceIoRename(from, to);
}

int getpid(void)
{
    return -1;
}

int kill(int pid, int sig)
{
    return -1;
}

void sbrk_release(void)
{
    if (g_sbrk_block != 0) {
        sceKernelFreePartitionMemory(g_sbrk_block);
        g_sbrk_block = 0;
    }
}

time_t time(time_t *t)
{
    return sceKernelLibcTime(t);
}

int gettimeofday(struct timeval *tv, void *tz)
{
    return sceKernelLibcGettimeofday((SceKernelTimeval *)tv, tz);
}

/* Installa (o rimuove, se handler è NULL) un gestore sull'interrupt di VBlank (30). */
void SetVblankSubIntrHandler(int no, void *handler, void *arg)
{
    if (handler == NULL) {
        sceKernelReleaseSubIntrHandler(PSP_VBLANK_INT, no);
    } else if (sceKernelRegisterSubIntrHandler(PSP_VBLANK_INT, no, handler, arg) >= 0) {
        sceKernelEnableSubIntr(PSP_VBLANK_INT, no);
    }
}
