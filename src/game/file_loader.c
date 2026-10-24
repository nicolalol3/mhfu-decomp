/* Loader of the game data: DATA.BIN on the UMD (or patch files on the Memory Stick).
 * The files are read in 128 KB blocks by a thread ("fakeRofsLoader"), decrypted
 * (byte table + XOR with a small pseudo-random generator seeded by the sector),
 * checked with SHA-1 ("sha1Thread") and copied to their destination ("transferThread").
 * (Guessed names.)
 *
 * Fields of the loader (p):
 *   0x0004 + i * 0x20  queue of 128 requests (Request)
 *   0x1004 / 0x1008    index of the request being read / first free request
 *   0x100C             state of the reading thread
 *   0x1010             file descriptor of DATA.BIN
 *   0x2AC0 + id * 4    first sector of each file in DATA.BIN
 *   0x9300             read buffer (0x20000 bytes)
 *   0x2931C            semaphore of the file access, 0x29340 current position in DATA.BIN
 *   0x29348 + id * 4   patch file number (0xFFFF = none) and sector in the patch file
 *   0x2FB4C            SHA-1 of the last block, 0x2FBD4 / 0x2FBD8 generator state */
#include "menu.h"

/* system functions (imported by the EBOOT) */
int sceUmdActivate(int unit, const char *drive);
int sceUmdGetErrorStat(void);
int sceUmdGetDriveStat(void);
int sceKernelDelayThreadCB(u32 delay);
int sceIoOpen(const char *file, int flags, int mode);
int sceIoRead(int fd, void *data, u32 size);
s64 sceIoLseek(int fd, s64 offset, int whence);
int sceIoClose(int fd);
int sceIoGetstat(const char *file, void *stat);
int sceIoDopen(const char *dirname);
int sceIoDclose(int fd);
void sceKernelDcacheInvalidateRange(const void *p, u32 size);
void sceKernelDcacheWritebackRange(const void *p, u32 size);
void sceKernelDcacheWritebackAll(void);
int sceKernelCreateThread(const char *name, void *entry, int prio, int stack, u32 attr, void *opt);
int sceKernelStartThread(int thid, u32 arglen, void *argp);
int sceKernelTerminateThread(int thid);
int sceKernelSleepThread(void);
int sceKernelWakeupThread(int thid);
int sceKernelCreateEventFlag(const char *name, int attr, int bits, void *opt);
int sceKernelSetEventFlag(int evid, u32 bits);
int sceKernelClearEventFlag(int evid, u32 bits);
int sceKernelWaitEventFlag(int evid, u32 bits, u32 wait, u32 *outbits, u32 *timeout);
int sceKernelPollEventFlag(int evid, u32 bits, u32 wait, u32 *outbits);
int sceKernelWaitSema(int semaid, int signal, u32 *timeout);
int sceKernelSignalSema(int semaid, int signal);
int sceKernelSuspendDispatchThread(void);
int sceKernelResumeDispatchThread(int state);
int sceKernelLoadModuleByID(int fid, int flags, void *option);
int sceKernelStartModule(int modid, u32 argsize, void *argp, int *status, void *option);

void *memcpy(void *d, const void *s, size_t n);
int memcmp(const void *a, const void *b, size_t n);

extern const u8 D_0899A018[256];   /* byte substitution table of the decryption */
extern const char D_0899A118[], D_0899A120[], D_0899A130[], D_0899A140[], D_0899A14C[], D_0899A160[];
extern const char D_0899A174[], D_0899A19C[], D_0899A1B8[], D_0899A1DC[];
extern const u8 D_08979804[];      /* SHA-1 of the files (0x14 bytes each) */
extern const u8 D_089CC420[0x14];
extern u8 D_0884E678[];            /* entry of the transfer thread */
extern u8 *D_09A4F0D0;
extern u8 *D_09A1FDDC;

u32 game_rand(void *a, int b);
void func_088B8548(void *p);
void func_088B85CC(void *p);
void func_088B85F8(void *p);
s32 func_088B8624(void *p);
s32 func_088B863C(void *p);
s32 func_088B8688(void *p);
void func_08903A94(void *ctx, const void *data, u32 size);
void func_08903C64(void *ctx, void *out);
void func_08903E60(void *ctx);
void func_089040A0(const void *data, u32 size, void *out);

#define REQ(p, i) ((p) + 4 + (i) * 0x20)
/* request: u16 used (0), u16 file id (2), u32 destination (4), u32 size (8),
 * u32 offset in the file (0xC), u32 total size (0x10), u32 *done (0x14),
 * u8 mode (0x18), first block (0x19), last block (0x1A), decrypt (0x1B), hash (0x1C) */
#define VCALL(p, off) (*(s32 (**)(void *, u32))(*(u8 **)(p) + (off)))

void fl_set_seed(u8 *p, u32 seed);
void fl_decrypt(u8 *p, u8 *buf, s32 n, u32 off);
const char *fl_data_path(u8 *p);
void fl_build_patch_table(u8 *p);
void fl_loader_thread(u32 args, void *argp);
void fl_sha1_thread(u32 args, void *argp);
s32 fl_read_block(u8 *p, s32 fd, u8 *e);
void fl_copy_block(u8 *p, u8 *e);
int fl_must_copy(u8 *p, u8 *e);
int fl_in_user_ram(u8 *p, u32 addr, s32 size);
void fl_requeue(u8 *p, u8 *e);
void fl_report_error(u8 *p, s32 err);
void fl_set_error(u8 *p, s32 err);

/* opens DATA.BIN, reads its decrypted file tables and starts the three threads */
void fl_init(u8 *p)
{
    u32 st[0x58 / 4];
    u8 *arg;

    func_088B8548(p);
    sceUmdActivate(1, D_0899A118);
    while (sceUmdGetErrorStat() != 0)
        sceKernelDelayThreadCB(10000);
    while ((sceUmdGetDriveStat() & 0x20) == 0)
        sceKernelDelayThreadCB(10000);
    for (;;) {
        s32 fd = sceIoOpen(fl_data_path(p), 1, 0);

        U32(p, 0x1010) = fd;
        if (fd >= 0)
            break;
        sceKernelDelayThreadCB(10000);
    }
    sceKernelDcacheInvalidateRange(p + 0x2AC0, 0x6840);
    while (sceIoRead(U32(p, 0x1010), p + 0x2AC0, 0x6808) < 0)
        sceKernelDelayThreadCB(10000);
    sceKernelDcacheWritebackRange(p + 0x2AC0, 0x6840);
    fl_set_seed(p, 0);
    fl_decrypt(p, p + 0x2AC0, 0x6808, 0);
    sceKernelDcacheInvalidateRange(p + 0x1140, 0x1980);
    while (sceIoRead(U32(p, 0x1010), p + 0x1140, 0x1968) < 0)
        sceKernelDelayThreadCB(10000);
    sceKernelDcacheWritebackRange(p + 0x1140, 0x1980);
    fl_decrypt(p, p + 0x1140, 0x1968, 0x6808);
    while (sceIoGetstat(fl_data_path(p), st) < 0)
        sceKernelDelayThreadCB(10000);
    U32(p, 0x1018) = st[0x10];
    fl_build_patch_table(p);
    p[0x1014] = 3;
    func_088B85CC(p);
    p[0x111C] = 0;
    U32(p, 0x29344) = sceKernelCreateThread(D_0899A120, fl_loader_thread, 0x30, 0x1000, 0, NULL);
    arg = p;
    sceKernelStartThread(U32(p, 0x29344), 4, &arg);
    U32(p, 0x2FBC0) = sceKernelCreateEventFlag(D_0899A130, 0x200, 0, NULL);
    U32(p, 0x2FBC4) = sceKernelCreateThread(D_0899A140, fl_sha1_thread, 0x31, 0x1000, 0, NULL);
    arg = p;
    sceKernelStartThread(U32(p, 0x2FBC4), 4, &arg);
    *(u32 *)(D_08A631A0 + 0xF7F030) = U32(p, 0x2FBC4);
    D_08A631A0[0xF7F034] = 1;
    U32(p, 0x2FBE8) = sceKernelCreateEventFlag(D_0899A14C, 0x200, 0, NULL);
    U32(p, 0x2FBEC) = sceKernelCreateThread(D_0899A160, D_0884E678, 0x13, 0x1000, 0, NULL);
    arg = p;
    sceKernelStartThread(U32(p, 0x2FBEC), 4, &arg);
    p[0x2FBCC] = 1;
    p[0x2FBDC] = 1;
    U32(p, 0x2FBE0) = 0;
    U32(p, 0x2FBE4) = 0;
}

/* wakes the thread ev (transfer or SHA-1) and waits until it is done */
static void fl_run_thread(u8 *p, u32 ev)
{
    sceKernelClearEventFlag(U32(p, ev), ~0x10);
    sceKernelSetEventFlag(U32(p, ev), 1);
    sceKernelWaitEventFlag(U32(p, ev), 0x10, 0, NULL, NULL);
}

/* the request has been completed: next one */
static void fl_request_done(u8 *p)
{
    int st = sceKernelSuspendDispatchThread();

    func_088B85F8(p);
    sceKernelResumeDispatchThread(st);
    sceKernelSignalSema(U32(p, 0x2931C), 1);
}

/* thread "fakeRofsLoader": executes the queued requests one block at a time */
void fl_loader_thread(u32 args, void *argp)
{
    u8 *p = *(u8 **)argp;

    for (;;) {
        u8 *e = REQ(p, U32(p, 0x1004));
        s32 n, size;

        if (U16(e, 0) == 0) {
            sceKernelSleepThread();
            continue;
        }
        switch (U32(p, 0x100C)) {
        case 0:
            sceKernelWaitSema(U32(p, 0x2931C), 1, NULL);
            U32(p, 0x100C) = (S8(D_089CC438, 0x6ADDE) != 0) ? 1 : 7;
            continue;
        case 1: {
            /* patch file on the Memory Stick */
            char path[64];
            u16 id = U16(e, 2);
            u16 num = U16(p, 0x29348 + id * 4);
            s32 fd, tries = 30;
            s32 r;

            if (num == 0xFFFF) {
                U32(p, 0x100C) = 7;
                continue;
            }
            sprintf(path, D_0899A174, num);
            for (;;) {
                fd = sceIoOpen(path, 1, 0);
                if (fd >= 0)
                    break;
                sceKernelDelayThreadCB(100000);
                if (--tries < 0)
                    break;
            }
            if (fd < 0) {
                U32(p, 0x100C) = 7;
                fl_report_error(p, fd);
                continue;
            }
            r = sceIoLseek(fd, (s32)(U32(e, 0xC) + (U16(p, 0x2934A + U16(e, 2) * 4) << 11)), 0);
            if (r < 0) {
                sceIoClose(fd);
                U32(p, 0x100C) = 7;
                fl_report_error(p, r);
                continue;
            }
            n = fl_read_block(p, fd, e);
            if (n == (s32)U32(e, 8)) {
                U32(p, 0x2FBC8) = (u32)e;
                if (fl_in_user_ram(p, U32(e, 4) + U32(e, 0xC), U32(e, 8)) == 0)
                    fl_copy_block(p, e);
                else if (fl_must_copy(p, e))
                    fl_run_thread(p, 0x2FBE8);
                if (fl_must_copy(p, e)) {
                    fl_run_thread(p, 0x2FBC0);
                    if (e[0x1C] != 0 && e[0x1A] != 0
                        && memcmp(p + 0x2FB4C, D_08979804 + U16(e, 2) * 0x14, 0x14) != 0) {
                        sceIoClose(fd);
                        U32(p, 0x100C) = 7;
                        fl_report_error(p, n);
                        fl_requeue(p, e);
                        continue;
                    }
                }
                if (fl_must_copy(p, e)) {
                    fl_request_done(p);
                    sceIoClose(fd);
                    continue;
                }
            }
            U32(p, 0x100C) = 7;
            fl_report_error(p, n);
            sceIoClose(fd);
            continue;
        }
        case 7: {
            /* seek to the file in DATA.BIN */
            u32 pos = U32(p, 0x2AC0 + U16(e, 2) * 4) << 11;

            if (U32(p, 0x29340) != pos) {
                if (sceIoLseek(U32(p, 0x1010), (s32)(U32(e, 0xC) + pos), 0) < 0) {
                    sceKernelDelayThreadCB(10000);
                    continue;
                }
                U32(p, 0x29340) = U32(e, 0xC) + (U32(p, 0x2AC0 + U16(e, 2) * 4) << 11);
            }
            U32(p, 0x100C) = 10;
        }
            /* fall through */
        case 10:
            if ((sceUmdGetDriveStat() & 0x20) == 0) {
                sceKernelDelayThreadCB(10000);
                continue;
            }
            U32(p, 0x100C) = 8;
            /* fall through */
        case 8:
            /* read a block from DATA.BIN */
            n = fl_read_block(p, U32(p, 0x1010), e);
            size = U32(e, 8);
            if (n != size) {
                sceKernelDelayThreadCB(10000);
                continue;
            }
            if (size < 0)
                size = VCALL(p, 0x18)(p, U16(e, 2));
            if (n != size) {
                U32(p, 0x100C) = 7;
                sceKernelDelayThreadCB(10000);
                continue;
            }
            U32(p, 0x2FBC8) = (u32)e;
            U32(p, 0x29340) += size;
            if (fl_in_user_ram(p, U32(e, 4) + U32(e, 0xC), U32(e, 8)) == 0)
                fl_copy_block(p, e);
            else
                fl_run_thread(p, 0x2FBE8);
            if (fl_must_copy(p, e))
                fl_run_thread(p, 0x2FBC0);
            if (fl_must_copy(p, e) == 0 && U32(e, 0x14) != 0)
                *(u32 *)U32(e, 0x14) = 1;
            fl_request_done(p);
            continue;
        }
    }
}

/* puts back in the queue the whole file of a request that failed the SHA-1 check */
void fl_requeue(u8 *p, u8 *e)
{
    u32 c[8];
    s32 n, i;

    for (i = 0; i < 8; i++)
        c[i] = U32(e, i * 4);
    U16(e, 0) = 0;
    n = (s32)(c[4] + 0x1FFFF) / 0x20000;
    U32(p, 0x1004) = (U32(p, 0x1004) - (n - 1)) & 0x7F;
    U32(p, 0x1008) = U32(p, 0x1004);
    (*(void (**)(void *, u32, u32, u32, u32, u32, u32))(*(u8 **)p + 0x34))(p, c[0] >> 16, c[1], c[4],
                                                                          c[6] & 0xFF, c[5], 0);
    if (U16(REQ(p, U32(p, 0x1008)), 0) != 0) {
        for (i = 0;;) {
            i++;
            U32(p, 0x1008) = (U32(p, 0x1008) + 1) & 0x7F;
            if (i >= 0x7F) {
                U32(p, 0x1008) = (U32(p, 0x1004) - 1) & 0x7F;
                return;
            }
            if (U16(REQ(p, U32(p, 0x1008)), 0) == 0)
                return;
        }
    }
}

void fl_hash_block(u8 *p, u8 *e);
void fl_decrypt_block(u8 *p, u8 *e);

/* thread "sha1Thread": decrypts the block and computes its SHA-1 */
void fl_sha1_thread(u32 args, void *argp)
{
    u8 *p = *(u8 **)argp;

    for (;;) {
        sceKernelWaitEventFlag(U32(p, 0x2FBC0), 1, 0, NULL, NULL);
        if (fl_must_copy(p, (u8 *)U32(p, 0x2FBC8))) {
            if (((u8 *)U32(p, 0x2FBC8))[0x1B] != 0)
                fl_decrypt_block(p, (u8 *)U32(p, 0x2FBC8));
            if (((u8 *)U32(p, 0x2FBC8))[0x1C] != 0)
                fl_hash_block(p, (u8 *)U32(p, 0x2FBC8));
        }
        sceKernelClearEventFlag(U32(p, 0x2FBC0), ~1);
        sceKernelSetEventFlag(U32(p, 0x2FBC0), 0x10);
    }
}

/* reads the block of the request in the read buffer */
s32 fl_read_block(u8 *p, s32 fd, u8 *e)
{
    sceKernelDcacheInvalidateRange(p + 0x9300, 0x20000);
    return sceIoRead(fd, p + 0x9300, U32(e, 8));
}

/* copies the read buffer to the destination (or marks the request as done) */
void fl_copy_block(u8 *p, u8 *e)
{
    if (fl_must_copy(p, e) == 0) {
        if (U32(e, 0x14) != 0)
            *(u32 *)U32(e, 0x14) = 1;
    } else {
        memcpy((void *)(U32(e, 4) + U32(e, 0xC)), p + 0x9300, U32(e, 8));
        sceKernelDcacheWritebackAll();
    }
}

/* 0 if the destination of the request is in the user RAM and the loader is stopped */
int fl_must_copy(u8 *p, u8 *e)
{
    if (D_09A4F0D0[0x214] != 0)
        return 1;
    return fl_in_user_ram(p, U32(e, 4) + U32(e, 0xC), U32(e, 8)) == 0;
}

int fl_in_user_ram(u8 *p, u32 addr, s32 size)
{
    return addr >= 0x08400000 && addr + size < 0x08800001;
}

/* SHA-1 of the destination block (whole file over several blocks) */
void fl_hash_block(u8 *p, u8 *e)
{
    u8 *dst = (u8 *)(U32(e, 4) + U32(e, 0xC));

    if (e[0x19] != 0 && e[0x1A] != 0) {
        func_089040A0(dst, U32(e, 8), p + 0x2FB4C);
    } else if (e[0x19] != 0) {
        func_08903E60(p + 0x2FB60);
        func_08903A94(p + 0x2FB60, dst, U32(e, 8));
    } else if (e[0x1A] == 0) {
        func_08903A94(p + 0x2FB60, dst, U32(e, 8));
    } else {
        func_08903A94(p + 0x2FB60, dst, U32(e, 8));
        func_08903C64(p + 0x2FB60, p + 0x2FB4C);
    }
}

/* decrypts the destination block (the generator is seeded at the first block) */
void fl_decrypt_block(u8 *p, u8 *e)
{
    u32 off = U32(e, 0xC) + (U32(p, 0x2AC0 + U16(e, 2) * 4) << 11);
    u8 *dst = (u8 *)(U32(e, 4) + U32(e, 0xC));

    if (e[0x19] != 0)
        fl_set_seed(p, off >> 11);
    fl_decrypt(p, dst, U32(e, 8), off);
}

u32 fl_rand(u8 *p);

/* decryption: byte table, then XOR of each word with the generator */
void fl_decrypt(u8 *p, u8 *buf, s32 n, u32 off)
{
    s32 i;

    for (i = 0; i < n; i += 4, buf += 4) {
        buf[0] = D_0899A018[buf[0]];
        buf[1] = D_0899A018[buf[1]];
        buf[2] = D_0899A018[buf[2]];
        buf[3] = D_0899A018[buf[3]];
        *(u32 *)buf ^= fl_rand(p);
    }
}

/* restarts the transfer / SHA-1 threads if they are stuck */
void fl_watchdog(u8 *p)
{
    u8 *arg;

    if (sceKernelPollEventFlag(U32(p, 0x2FBE8), 1, 0, NULL) == 0) {
        sceKernelTerminateThread(U32(p, 0x2FBEC));
        sceKernelClearEventFlag(U32(p, 0x2FBE8), ~1);
        sceKernelSetEventFlag(U32(p, 0x2FBE8), 0x10);
        arg = p;
        sceKernelStartThread(U32(p, 0x2FBEC), 4, &arg);
    }
    if (sceKernelPollEventFlag(U32(p, 0x2FBC0), 1, 0, NULL) == 0) {
        u8 *e = (u8 *)U32(p, 0x2FBC8);

        if (fl_in_user_ram(p, U32(e, 4) + U32(e, 0xC), U32(e, 8)) != 0) {
            sceKernelTerminateThread(U32(p, 0x2FBC4));
            sceKernelClearEventFlag(U32(p, 0x2FBC0), ~1);
            sceKernelSetEventFlag(U32(p, 0x2FBC0), 0x10);
            arg = p;
            sceKernelStartThread(U32(p, 0x2FBC4), 4, &arg);
        }
    }
}

/* seed of the generator (two 16-bit halves, never 0) */
void fl_set_seed(u8 *p, u32 seed)
{
    U32(p, 0x2FBD4) = seed & 0xFFFF;
    U32(p, 0x2FBD8) = seed >> 16;
    if (U32(p, 0x2FBD4) == 0)
        U32(p, 0x2FBD4) = 0x7F8D;
    if (U32(p, 0x2FBD8) == 0)
        U32(p, 0x2FBD8) = 0x2345;
}

/* pseudo-random generator: two multiplicative generators modulo 0xFFF1 and 0xFFD9 */
u32 fl_rand(u8 *p)
{
    U32(p, 0x2FBD4) = (U32(p, 0x2FBD4) * 0x7F8D) % 0xFFF1;
    U32(p, 0x2FBD8) = (U32(p, 0x2FBD8) * 0x2345) % 0xFFD9;
    return (U32(p, 0x2FBD8) << 16) + U32(p, 0x2FBD4);
}

/* reads (synchronously) the whole file id, or size bytes of it, decrypted */
s32 fl_read_file(u8 *p, u32 id, u8 *buf, s32 size)
{
    s32 pos, n;

    sceKernelWaitSema(U32(p, 0x2931C), 1, NULL);
    pos = U32(p, 0x2AC0 + (id & 0xFFFF) * 4) << 11;
    while ((s32)sceIoLseek(U32(p, 0x1010), pos, 0) != pos)
        sceKernelDelayThreadCB(10000);
    n = VCALL(p, 0x18)(p, id);
    if (size != -1)
        n = size;
    while (sceIoRead(U32(p, 0x1010), buf, n) != n)
        sceKernelDelayThreadCB(10000);
    fl_set_seed(p, pos / 0x800);
    fl_decrypt(p, buf, n, 0);
    sceKernelSignalSema(U32(p, 0x2931C), 1);
    return n;
}

/* queues the reading of the file id to dst (size -1 = whole file), in 128 KB blocks */
int fl_queue_file(u8 *p, u32 id, u32 dst, s32 size, u32 mode, u32 done, u32 wake_arg)
{
    u8 wake = wake_arg;
    int st = sceKernelSuspendDispatchThread();
    s32 left, off = 0;
    u8 first = 1, hash = 0;

    if (size == -1)
        size = VCALL(p, 0x18)(p, id);
    left = size;
    if (p[0x2FBCC] != 0 && (u16)game_rand(D_08A631A0, 1) % 100 < 5)
        hash = 1;
    for (;;) {
        u8 *e = REQ(p, U32(p, 0x1008));

        U16(e, 0) = 1;
        U32(e, 0xC) = off;
        U16(e, 2) = id;
        U32(e, 4) = dst;
        e[0x18] = mode;
        U32(e, 0x14) = done;
        e[0x19] = first;
        e[0x1B] = p[0x2FBCC];
        e[0x1C] = hash;
        U32(e, 0x10) = size;
        if (left < 0x20001) {
            U32(e, 8) = left;
            U32(p, 0x1008) = (U32(p, 0x1008) + 1) & 0x7F;
            e[0x1A] = 1;
            break;
        }
        U32(e, 8) = 0x20000;
        off += 0x20000;
        e[0x1A] = 0;
        left -= 0x20000;
        first = 0;
        U32(p, 0x1008) = (U32(p, 0x1008) + 1) & 0x7F;
    }
    sceKernelResumeDispatchThread(st);
    if (wake != 0)
        sceKernelWakeupThread(U32(p, 0x29344));
    return 1;
}

/* direct reading: seek to the start of the file id */
void fl_open_file(u8 *p, u32 id_arg)
{
    u16 id = id_arg;
    s32 pos;

    p[0x111C] = 1;
    U16(p, 0x111E) = id;
    pos = U32(p, 0x2AC0 + id * 4) << 11;
    while ((s32)sceIoLseek(U32(p, 0x1010), pos, 0) != pos)
        sceKernelDelayThreadCB(10000);
    pos = U32(p, 0x2AC0 + id * 4) << 11;
    U32(p, 0x2FBD0) = pos;
    fl_set_seed(p, pos / 0x800);
}

/* direct reading of n bytes (not decrypted) */
void fl_read(u8 *p, void *buf, s32 n)
{
    while ((sceUmdGetDriveStat() & 0x20) == 0)
        sceKernelDelayThreadCB(10000);
    while (sceIoRead(U32(p, 0x1010), buf, n) != n)
        sceKernelDelayThreadCB(10000);
    U32(p, 0x2FBD0) += n;
}

/* direct reading: seek to the offset off of the open file */
void fl_seek(u8 *p, s32 off)
{
    s32 pos = off + (U32(p, 0x2AC0 + U16(p, 0x111E) * 4) << 11);

    while ((s32)sceIoLseek(U32(p, 0x1010), pos, 0) != pos)
        sceKernelDelayThreadCB(10000);
    U32(p, 0x2FBD0) = pos;
}

/* loads and starts the module (PRX) stored as the file id of DATA.BIN at the address dst */
s32 fl_load_module(u8 *p, u32 id, u32 dst)
{
    u8 opt[0x14];
    s32 fd, mod;
    int status;
    u32 sector;

    memset(opt, 0, 0x14);
    *(u32 *)opt = 0x14;
    opt[0x11] = 1;
    *(u32 *)(opt + 4) = dst;
    *(u32 *)(opt + 8) = dst;
    opt[0x10] = 0;
    sector = U32(p, 0x2AC0 + (id & 0xFFFF) * 4);
    sprintf((char *)(p + 0x101C), D_0899A19C, U32(p, 0x1018) + sector, VCALL(p, 0x18)(p, id));
    while ((fd = sceIoOpen((char *)(p + 0x101C), 1, 0)) < 0)
        sceKernelDelayThreadCB(10000);
    while ((mod = sceKernelLoadModuleByID(fd, 0, opt)) < 0)
        sceKernelDelayThreadCB(10000);
    sceKernelStartModule(mod, 0, NULL, &status, NULL);
    sceIoClose(fd);
    return mod;
}

/* 1 if the file id has been changed by an update (its SHA-1 differs from the original) */
int fl_file_patched(u8 *p, u32 id)
{
    return memcmp(D_08979804 + (id & 0xFFFF) * 0x14, D_089CC420, 0x14) != 0;
}

/* places the patched files in the patch files of the Memory Stick (0x370000 bytes each) */
void fl_build_patch_table(u8 *p)
{
    u16 *t = (u16 *)(p + 0x29348);
    u16 num = 0;
    u32 acc = 0;
    u32 i;

    for (i = 0; i < 0x1A01; i++, t += 2) {
        if (fl_file_patched(p, i & 0xFFFF) == 0) {
            t[0] = 0xFFFF;
            t[1] = 0;
        } else {
            u32 sz = VCALL(p, 0x1C)(p, i & 0xFFFF);

            if (acc + sz < 0x370001) {
                t[0] = num;
                t[1] = acc >> 11;
            } else {
                num++;
                acc = 0;
                t[0] = num;
                t[1] = 0;
            }
            acc += sz;
        }
    }
}

/* state of the loader: 1 if an error is pending, else the state of the reader */
s32 fl_busy_a(u8 *p)
{
    if (p[0x2FBDC] != 0 && U32(p, 0x2FBE0) != 0)
        return 1;
    return func_088B8624(p);
}

s32 fl_busy_b(u8 *p)
{
    if (p[0x2FBDC] != 0 && U32(p, 0x2FBE0) != 0)
        return 1;
    return func_088B863C(p);
}

s32 fl_busy_c(u8 *p)
{
    if (p[0x2FBDC] != 0 && U32(p, 0x2FBE0) != 0)
        return 1;
    return func_088B8688(p);
}

/* error while reading: for "file not found" checks that the patch folder exists */
void fl_report_error(u8 *p, s32 err)
{
    if (err == (s32)0x80010002) {
        char path[64];
        s32 d;

        sprintf(path, D_0899A1B8);
        d = sceIoDopen(path);
        if (d >= 0)
            sceIoDclose(d);
        fl_set_error(p, d);
    } else {
        fl_set_error(p, err);
    }
}

/* error message to show (depends on the error and on on-line / off-line) */
void fl_set_error(u8 *p, s32 err)
{
    int online = (*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) != 0;
    u32 code;

    if (err == (s32)0x80020321) {
        if (p[0x2FBDC] != 0)
            code = 1;
        else
            code = online ? 9 : 5;
    } else if (err == (s32)0x80010002) {
        if (p[0x2FBDC] != 0)
            code = 2;
        else
            code = online ? 10 : 6;
    } else if (err < 0 || D_09A1FDDC[0xD] != 0) {
        if (p[0x2FBDC] != 0)
            code = 4;
        else
            code = online ? 12 : 8;
    } else {
        if (p[0x2FBDC] != 0)
            code = 3;
        else
            code = online ? 11 : 7;
    }
    U32(p, 0x2FBE0) = code;
    D_089CC438[0x6ADDE] = 0;
}

const char *fl_data_path(u8 *p)
{
    return D_0899A1DC;
}

extern u8 D_089BAAA8[], D_089BAC20[];

u32 *fl_dtor(u32 *o, s32 flag)
{
    if (o != NULL) {
        o[0] = (u32)D_089BAAA8;
        o[0] = (u32)D_089BAC20;
        if ((s16)flag > 0)
            func_0890BFA4(o);
    }
    return o;
}

/* constructor of the general game state (D_089CC438) */
u8 *game_state_ctor(u8 *g)
{
    int i;
    u8 *q;

    D_089CC438 = g;
    U32(g, 0x6AF14) = 0;
    U16(g, 0x6AF0E) = 0xFF;
    g[0x6AF10] = 0;
    U16(g, 0x6AF12) = 0;
    g[0x2E] = 0;
    g[0x28] = 0;
    g[0x422] = 0;
    U16(g, 0x20) = 0xFFFF;
    U32(g, 0x47C) = 0;
    U32(g, 0x424) = 0;
    memset(g + 0x4A0, 0, 0x6A938);
    for (i = 0, q = g; i < 0x14; i++, q += 0x20) {
        U16(q, 0x47D4) = 0xFFFF;
        U16(q, 0x47D6) = 0xFFFF;
        U16(q, 0x47D8) = 0xFFFF;
        U16(q, 0x47DA) = 0xFFFF;
        U16(q, 0x47DC) = 0xFFFF;
        U16(q, 0x47DE) = 0xFFFF;
    }
    g[0x30] = 0;
    g[0xBC] = 0;
    g[0x148] = 0;
    g[0x1D4] = 0;
    g[0x260] = 0;
    g[0x2EC] = 0;
    g[0x6AF18] = 0xFF;
    memset(g + 0x6ADE8, 0, 0x14);
    g[0x480] = 0;
    g[0x6AF1B] = 0;
    g[0x6AF0A] = 0;
    g[0x6AF0B] = 0;
    g[0x6AF0C] = 0;
    return g;
}
