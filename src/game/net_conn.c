/* The start and the end of the network: the libraries (ad hoc or infrastructure), the
 * network configuration dialog, the events, a memory pool and the broadcast packets with
 * a sequence number (sent again until they are received).
 * The connection (D_09A1DB60): 0 phase (0 start, 1 dialog, 2 end), 1 step of the start
 * and of the dialog, 2 step of the end, 4 mode (1 ad hoc, 2 infrastructure), 5 libraries
 * started, 8 error, 0xC handler, 0x10 flags of the events, 0x14 error of the events.
 * (Guessed names.) */
#include "common.h"

#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define S32(p, off) (*(s32 *)((u8 *)(p) + (off)))
#define U16(p, off) (*(u16 *)((u8 *)(p) + (off)))
#define S16(p, off) (*(s16 *)((u8 *)(p) + (off)))

typedef struct {
    s32 len;
    s32 cap;
    u8 *ptr;
} NetBuf;

/* a peer of the broadcast: {address[6], .., sequence (0x10), state (0x14), 0x16} */
typedef struct {
    u8 mac[6];
    u8 pad[0xA];
    u32 seq;
    u16 state;
    u16 x16;
} BcastPeer;

/* a packet waiting in the queue: {time, address, port, size, times to send} */
typedef struct {
    u64 time;
    u8 mac[0x10];
    u16 port;
    u16 len;
    u16 repeat;
    u16 pad;
} BcastRec;

extern u8 D_09A1DB60[0x18];
extern u8 D_09A1DB0C[0x44];       /* the parameters of the dialog */
extern s32 D_09A1DB50;            /* the language */
extern u32 D_09A1DB54;            /* the IP address */
extern u32 D_09A1DB70, D_09A1DB74;
extern u32 D_09A1DB78[8][2];
extern s32 D_09A1DBB8;
extern u8 D_09A1DBBC, D_09A1DBBD;
extern u32 D_09A1DBC0;
extern NetBuf D_09A1FC44;         /* the packets to send */
extern NetBuf D_09A1FC50;         /* the received packet */
extern BcastPeer D_09A1FC5C[16];  /* 0: this player */
extern const char D_089B2828[], D_089B2830[], D_089B2838[], D_089B2840[];

#define CONN D_09A1DB60
#define NETCONF(off) U32(D_09A1DB0C, off)

void *memset(void *d, int c, u32 n);
void *memcpy(void *d, const void *s, u32 n);
void *memmove(void *d, const void *s, u32 n);
int memcmp(const void *a, const void *b, u32 n);
s32 sceUtilityNetconfGetStatus(void);
s32 sceUtilityNetconfUpdate(void);
s32 sceUtilityNetconfShutdownStart(void);
s32 sceUtilityNetconfInitStart(void *p);
s32 sceNetInit(s32 a, s32 b, s32 c, s32 d, s32 e);
s32 sceNetTerm(void);
s32 sceNetAdhocInit(void);
s32 sceNetAdhocTerm(void);
s32 sceNetInetInit(void);
s32 sceNetInetTerm(void);
s32 sceNetAdhocctlInit(s32 stack, s32 prio, void *product);
s32 sceNetAdhocctlTerm(void);
s32 sceNetAdhocctlAddHandler(void *h, void *arg);
s32 sceNetAdhocctlDelHandler(s32 id);
s32 sceNetAdhocctlDisconnect(void);
s32 sceNetApctlInit(s32 stack, s32 prio);
s32 sceNetApctlTerm(void);
s32 sceNetApctlAddHandler(void *h, void *arg);
s32 sceNetApctlDelHandler(s32 id);
s32 sceNetApctlDisconnect(void);
s32 sceNetApctlGetInfo(s32 code, void *info);
s32 sceNetGetLocalEtherAddr(u8 *mac);
u32 sceNetInetInetAddr(const char *ip);
s32 sceOpenPSIDGetOpenPSID(u8 *id);
void sceKernelExitGame(void);
s32 sceKernelCreateCallback(const char *name, void *f, void *arg);
s32 sceKernelRegisterExitCallback(s32 cb);
s32 scePowerRegisterCallback(s32 slot, s32 cb);
s32 sceKernelCreateSema(const char *name, u32 attr, s32 init, s32 max, void *opt);
s32 sceKernelWaitSema(s32 id, s32 n, u32 *timeout);
s32 sceKernelSignalSema(s32 id, s32 n);
u64 sceKernelGetSystemTimeWide(void);
void netev_adhoc(s32 ev, u32 err);
void netev_apctl(s32 old, s32 now, s32 ev, u32 err);
void netgame_nop1(void);
void netgame_nop2(void);
void netgame_nop3(void);
s32 pdp_init(s32 mode);
u32 pdp_nop(void);
s32 pdp_bcast_recv(s32 sock, u8 *addr, u16 *port, u8 *buf, s32 size);
s32 pdp_bcast_send(s32 sock, u8 *addr, u32 port, u8 *data, s32 size);
u32 stub_088ad12c(u32 v);        /* bswap32 */
u32 bswap32(u32 v);

/* in the overlays */
#define ovl_apctl_start ((void (*)(void))0x09A69B58)
#define ovl_apctl_end ((void (*)(void))0x09A69C00)

static u32 fail(u8 *c, s32 err)
{
    S32(c, 8) = err;
    c[1] = 0x5A;
    return 0;
}

/* the network configuration dialog: 2 when connected, 3 when it failed */
u32 netconf_step(u8 *c)
{
    s32 st;

    switch (c[1]) {
    case 0x5B:
        c[0] = 2;
        c[1] = 0;
        return 3;
    case 0x5A:
        st = sceUtilityNetconfGetStatus();
        if (st == 2)
            sceUtilityNetconfUpdate();
        else if (st == 3)
            sceUtilityNetconfShutdownStart();
        else if (st == 0)
            c[1]++;
        break;
    case 12:
        return 2;
    case 11:
        st = sceUtilityNetconfGetStatus();
        if (st < 0) {
            fail(c, st);
        } else if (st == 2) {
            st = sceUtilityNetconfUpdate();
            if (st < 0)
                fail(c, st);
        } else if (st == 3) {
            st = sceUtilityNetconfShutdownStart();
            if (st < 0)
                fail(c, st);
        } else if (st == 0) {
            if (NETCONF(0x1C) == 0)
                c[1]++;
            else
                fail(c, NETCONF(0x1C));
        }
        break;
    case 10:
        memset(D_09A1DB0C, 0, 0x44);
        NETCONF(0) = 0x44;
        if (D_09A1DB50 == 5)
            NETCONF(4) = 5;
        else if (D_09A1DB50 == 4)
            NETCONF(4) = 3;
        else if (D_09A1DB50 == 3)
            NETCONF(4) = 2;
        else if (D_09A1DB50 == 2)
            NETCONF(4) = 4;
        else
            NETCONF(4) = 1;
        NETCONF(8) = 1;
        NETCONF(0xC) = 0x2A;
        NETCONF(0x10) = 0x2C;
        NETCONF(0x14) = 0x2B;
        NETCONF(0x18) = 0x29;
        if (c[4] == 1) {
            NETCONF(0x30) = 2;
            NETCONF(0x34) = 0x09A1DB00;
        } else if (c[4] == 2) {
            NETCONF(0x30) = 0;
            NETCONF(0x38) = 1;
            NETCONF(0x40) = 1;
        }
        st = sceUtilityNetconfInitStart(D_09A1DB0C);
        if (st < 0)
            fail(c, st);
        else
            c[1]++;
        break;
    case 0:
        c[1] = 10;
        c[2] = 0;
        c[3] = 0;
        U32(c, 0x14) = 0;
        U32(c, 0x10) = 0;
        break;
    default:
        return 0;
    }
    return 0;
}

static u32 init_fail(u8 *c, s32 err)
{
    S32(c, 8) = err;
    c[0] = 2;
    c[1] = 0;
    return 0;
}

/* the start of the libraries: 1 when done */
u32 netinit_step(u8 *c)
{
    s32 r = 0;

    switch (c[1]) {
    case 14:
        c[0] = 1;
        c[1] = 0;
        return 1;
    case 13:
        if (c[4] == 1) {
            r = sceNetAdhocctlAddHandler(netev_adhoc, NULL);
        } else if (c[4] == 2) {
            r = sceNetApctlAddHandler(netev_apctl, NULL);
            ovl_apctl_start();
        }
        if (r < 0)
            return init_fail(c, r);
        S32(c, 0xC) = r;
        c[5] = 4;
        c[1]++;
        break;
    case 12:
        if (c[4] == 1)
            r = sceNetAdhocctlInit(0x2000, 0x30, (void *)0x09A1DAF0);
        else if (c[4] == 2)
            r = sceNetApctlInit(0x2000, 0x30);
        if (r < 0)
            return init_fail(c, r);
        c[5] = 3;
        c[1]++;
        break;
    case 11:
        if (c[4] == 1)
            r = sceNetAdhocInit();
        else if (c[4] == 2)
            r = sceNetInetInit();
        if (r < 0)
            return init_fail(c, r);
        c[5] = 2;
        c[1]++;
        break;
    case 10:
        r = sceNetInit(0x20000, 0x20, 0, 0x20, 0);
        if (r < 0)
            return init_fail(c, r);
        if (c[4] == 1)
            pdp_init(0);
        else if (c[4] == 2)
            pdp_init(1);
        netgame_nop3();
        c[5] = 1;
        c[1]++;
        break;
    case 0:
        c[1] = 10;
        c[5] = 0;
        c[2] = 0;
        c[3] = 0;
        U32(c, 0x14) = 0;
        U32(c, 0x10) = 0;
        U32(c, 8) = 0;
        break;
    default:
        return 0;
    }
    return 0;
}

/* the end: disconnects and stops the libraries started; 4 when disconnected, 5 at the
 * end */
u32 netterm_step(u8 *c)
{
    switch (c[2]) {
    case 24:
        return 5;
    case 23:
        if (c[5] == 0) {
            c[2] = 24;
            return 0;
        }
        netgame_nop3();
        pdp_nop();
        sceNetTerm();
        c[5] = 0;
        c[2]++;
        return 0;
    case 22:
        if (c[5] == 0) {
            c[2] = 23;
            return 0;
        }
        if (c[4] == 1)
            sceNetAdhocTerm();
        else if (c[4] == 2)
            sceNetInetTerm();
        c[5] = 1;
        c[2]++;
        return 0;
    case 21:
        if (c[5] < 2) {
            c[2] = 22;
            return 0;
        }
        if (c[4] == 1)
            sceNetAdhocctlTerm();
        else if (c[4] == 2)
            sceNetApctlTerm();
        c[5] = 2;
        c[2]++;
        return 0;
    case 20:
        if (c[5] < 4) {
            c[2] = 21;
            return 0;
        }
        if (c[4] == 1) {
            sceNetAdhocctlDelHandler(S32(c, 0xC));
        } else if (c[4] == 2) {
            ovl_apctl_end();
            sceNetApctlDelHandler(S32(c, 0xC));
        }
        S32(c, 0xC) = -1;
        c[5] = 3;
        c[2]++;
        return 0;
    case 12:
        c[2] = 20;
        return 4;
    case 11:
        if ((U32(c, 0x10) & 4) != 0)
            c[2] = 12;
        return 0;
    case 10:
        if ((U32(c, 0x10) & 2) == 0 || (U32(c, 0x10) & 4) != 0) {
            c[2] = 12;
            return 0;
        }
        if (c[4] == 1)
            sceNetAdhocctlDisconnect();
        else if (c[4] == 2)
            sceNetApctlDisconnect();
        c[2]++;
        return 0;
    case 0:
        c[2] = 10;
        return 0;
    }
    return 0;
}

/* one step of the connection; when connected (2), the address of this PSP */
s32 netconn_update(void)
{
    u8 info[0x80];
    u8 mac[8];
    s32 r;

    if (CONN[0] == 2)
        r = netterm_step(CONN);
    else if (CONN[0] == 1)
        r = netconf_step(CONN);
    else if (CONN[0] == 0)
        r = netinit_step(CONN);
    else
        r = 3;
    if (r == 2) {
        if (sceNetGetLocalEtherAddr(mac) < 0)
            memset((u8 *)0x09A1DB58, 0, 6);
        else
            memcpy((u8 *)0x09A1DB58, mac, 6);
        if (CONN[4] == 2) {
            if (sceNetApctlGetInfo(8, info) < 0)
                D_09A1DB54 = 0;
            else
                D_09A1DB54 = sceNetInetInetAddr((char *)info);
            netgame_nop2();
        } else if (CONN[4] == 1) {
            netgame_nop1();
        }
    }
    return r;
}

u32 netconn_term_step(void)
{
    return netterm_step(CONN);
}

void netconn_setup(u32 mode, const char *product)
{
    memset(CONN, 0, 0x18);
    S32(CONN, 0xC) = -1;
    CONN[4] = mode;
    if ((mode & 0xFF) == 1)
        memcpy((u8 *)0x09A1DAF4, product, 9);
    CONN[0] = 0;
}

void netconn_connect(const u8 *group)
{
    CONN[0] = 1;
    CONN[1] = 0;
    if (CONN[4] == 1)
        memcpy((u8 *)0x09A1DB00, group, 8);
}

void netconn_terminate(void)
{
    CONN[0] = 2;
    CONN[1] = 0;
    CONN[2] = 0;
    CONN[3] = 0;
}

void netconn_init(void)
{
    u8 id[0x10];

    D_09A1DBB8 = -1;
    memset(D_09A1DB78, 0, 0x40);
    D_09A1DBBD = 1;
    D_09A1DBBC = 1;
    sceOpenPSIDGetOpenPSID(id);
    memset((u8 *)0x09A1DBC4, 0, 0x80);
    memcpy((u8 *)0x09A1DBC4, id, 0x10);
    D_09A1DBC0 = 0x10;
}

static void call_handlers(u32 ev)
{
    u32 i;

    for (i = 0; i < 8; i++)
        if (D_09A1DB78[i][0] != 0)
            ((void (*)(u32, u32, u32))D_09A1DB78[i][0])(0, ev, D_09A1DB78[i][1]);
}

s32 netcb_exit(void)
{
    call_handlers(6);
    sceKernelExitGame();
    return 0;
}

s32 netcb_power(s32 count, u32 flags)
{
    if ((flags & 0x80000) != 0)
        call_handlers(1);
    if ((flags & 0x10000) != 0)
        call_handlers(2);
    if ((flags & 0x20000) != 0)
        call_handlers(3);
    if ((flags & 0x40000) != 0)
        call_handlers(4);
    if ((flags & 0x100) != 0)
        call_handlers(5);
    return 0;
}

void netcb_register(void)
{
    sceKernelRegisterExitCallback(sceKernelCreateCallback(D_089B2828, netcb_exit, NULL));
    scePowerRegisterCallback(0, sceKernelCreateCallback(D_089B2830, netcb_power, NULL));
}

void net_handler_add(u32 f, u32 arg)
{
    u32 i;

    for (i = 0; i < 8; i++) {
        if (D_09A1DB78[i][0] == 0) {
            D_09A1DB78[i][0] = f;
            D_09A1DB78[i][1] = arg;
            return;
        }
    }
}

void net_handler_remove(u32 f)
{
    u32 i;

    for (i = 0; i < 8; i++) {
        if (D_09A1DB78[i][0] == f) {
            D_09A1DB78[i][0] = 0;
            D_09A1DB78[i][1] = 0;
            return;
        }
    }
}

u32 bswap16(u32 v)
{
    return (v & 0xFF) << 8 | (v & 0xFFFF) >> 8;
}

u32 bswap32(u32 v)
{
    return v << 24 | (v & 0xFF00) << 8 | v >> 24 | ((v >> 8) & 0xFF00);
}

/* ---- the memory pool: {align, base, size, nodes, node count, u16 locked (0x14), first
 * node, last node}; a node: {size, address, next} ---- */

void pool_lock_init(u8 *p)
{
    if (S16(p, 0x14) != 0 && D_09A1DBB8 < 0)
        D_09A1DBB8 = sceKernelCreateSema(D_089B2838, 0, 1, 1, NULL);
}

void pool_lock(u8 *p)
{
    if (S16(p, 0x14) != 0)
        sceKernelWaitSema(D_09A1DBB8, 1, NULL);
}

void pool_unlock(u8 *p)
{
    if (S16(p, 0x14) != 0)
        sceKernelSignalSema(D_09A1DBB8, 1);
}

void pool_init(s32 *p)
{
    s32 *first, *last;

    if (p[0] <= 0 || p[1] == 0 || p[2] <= 0 || p[3] == 0 || p[4] < 2 ||
        U16(p, 0x14) >= 2)
        return;
    pool_lock_init((u8 *)p);
    memset((void *)p[3], 0, p[4] * 0xC);
    p[6] = p[3];
    p[7] = p[3] + 0xC;
    first = (s32 *)p[6];
    last = (s32 *)p[7];
    first[0] = p[0];
    first[1] = p[1] - p[0];
    first[2] = p[7];
    last = (s32 *)p[7];
    last[0] = p[0];
    last[1] = p[1] + p[2];
    last[2] = 0;
    memset((void *)p[1], 0, p[2]);
    p[8] = 0;
    p[9] = 0;
}

s32 *pool_free_node(u8 *p)
{
    s32 *n = (s32 *)U32(p, 0xC);
    s32 i;

    for (i = 0; i < S32(p, 0x10); i++, n += 3)
        if (n[0] == 0)
            return (s32 *)U32(p, 0xC) + i * 3;
    return NULL;
}

s32 *pool_find(u8 *p, s32 addr)
{
    s32 *n = (s32 *)U32(p, 0x18);

    for (;;) {
        if (n[1] == addr)
            return n;
        n = (s32 *)n[2];
        if (n == (s32 *)U32(p, 0x1C))
            return NULL;
    }
}

/* the first space that is large enough */
s32 pool_alloc(s32 *p, s32 size)
{
    s32 *n, *next, *m;

    if (size == 0)
        return 0;
    if (size % p[0] != 0)
        size += p[0] - size % p[0];
    pool_lock((u8 *)p);
    n = (s32 *)p[6];
    for (;;) {
        next = (s32 *)n[2];
        if (next[1] - (n[1] + n[0]) >= size)
            break;
        if (next == (s32 *)p[7]) {
            pool_unlock((u8 *)p);
            return 0;
        }
        n = next;
    }
    m = pool_free_node((u8 *)p);
    if (m == NULL) {
        pool_unlock((u8 *)p);
        return 0;
    }
    m[0] = size;
    m[1] = n[1] + n[0];
    m[2] = n[2];
    n[2] = (s32)m;
    pool_unlock((u8 *)p);
    return m[1];
}

void pool_free(u8 *p, s32 addr)
{
    s32 *n, *next;

    pool_lock(p);
    n = (s32 *)U32(p, 0x18);
    for (;;) {
        if (n[2] == S32(p, 0x1C)) {
            pool_unlock(p);
            return;
        }
        next = (s32 *)n[2];
        if (next[1] == addr)
            break;
        n = next;
    }
    next[0] = 0;
    next[1] = 0;
    n[2] = next[2];
    next[2] = 0;
    pool_unlock(p);
}

s32 pool_realloc(s32 *p, s32 addr, s32 size)
{
    s32 *n;
    s32 old, r;
    u16 lk;

    if (size % p[0] != 0)
        size += p[0] - size % p[0];
    if (addr == 0)
        return pool_alloc(p, size);
    if (size == 0) {
        pool_free((u8 *)p, addr);
        return 0;
    }
    pool_lock((u8 *)p);
    n = pool_find((u8 *)p, addr);
    if (n == NULL) {
        pool_unlock((u8 *)p);
        return 0;
    }
    old = n[0];
    lk = U16(p, 0x14);
    U16(p, 0x14) = 0;
    r = pool_alloc(p, size);
    U16(p, 0x14) = lk;
    if (r == 0) {
        pool_unlock((u8 *)p);
        return 0;
    }
    lk = U16(p, 0x14);
    U16(p, 0x14) = 0;
    pool_free((u8 *)p, addr);
    U16(p, 0x14) = lk;
    memmove((void *)r, (void *)addr, old < size ? old : size);
    pool_unlock((u8 *)p);
    return r;
}

/* ---- the broadcast packets with a sequence number ---- */

void bcast_init(const u8 *mac)
{
    memset(D_09A1FC5C, 0, 0x180);
    memset(&D_09A1FC50, 0, 0xC);
    memset(&D_09A1FC44, 0, 0xC);
    memcpy(D_09A1FC5C[0].mac, mac, 6);
    D_09A1FC5C[0].seq = 1;
    D_09A1FC5C[0].state = 2;
    D_09A1FC5C[0].x16 = 0xAAAA;
    D_09A1FC50.cap = 0x400;
    D_09A1FC50.ptr = (u8 *)0x09A1F844;
    D_09A1FC44.cap = 0x1C00;
    D_09A1FC44.ptr = (u8 *)0x09A1DC44;
}

/* the peers follow the list of the players (address at 0x84 of each one) */
u32 bcast_peers_update(s32 *list, s32 n)
{
    s32 *pl;
    s32 i, j;

    for (pl = n != 0 ? list : NULL; pl != NULL; pl = (s32 *)pl[0]) {
        for (i = 1; i < 16; i++)
            if (D_09A1FC5C[i].state != 0 &&
                memcmp(D_09A1FC5C[i].mac, pl + 0x21, 6) == 0)
                break;
        if (i != 16)
            continue;
        for (j = 1; j < 16; j++) {
            if (D_09A1FC5C[j].state == 0) {
                memcpy(D_09A1FC5C[j].mac, pl + 0x21, 6);
                D_09A1FC5C[j].seq = 1;
                D_09A1FC5C[j].state = 1;
                D_09A1FC5C[j].x16 = 0xAAAA;
                break;
            }
        }
    }
    for (i = 1; i < 16; i++) {
        if (D_09A1FC5C[i].state == 0)
            continue;
        for (pl = n != 0 ? list : NULL; pl != NULL; pl = (s32 *)pl[0])
            if (memcmp(D_09A1FC5C[i].mac, pl + 0x21, 6) == 0)
                break;
        if (pl == NULL)
            D_09A1FC5C[i].state = 0;
    }
    return 0;
}

/* a packet of a known peer that is new (sequence 0: always) */
s32 bcast_recv(s32 sock, u8 *mac, u16 *port, u8 *buf, s32 size)
{
    u8 from[0x10];
    u16 p;
    u32 seq;
    BcastPeer *e;
    s32 n, i;

    size = (s16)size;
    for (;;) {
        n = pdp_bcast_recv(sock, from, &p, D_09A1FC50.ptr, (s16)D_09A1FC50.cap);
        if (n <= 0)
            return n;
        D_09A1FC50.len = n;
        for (i = 1, e = &D_09A1FC5C[1]; i < 16; i++, e++)
            if (e->state != 0 && memcmp(e->mac, from, 6) == 0)
                break;
        if (i == 16)
            continue;
        memcpy(&seq, D_09A1FC50.ptr, 4);
        if (stub_088ad12c(seq) != 0) {
            if (e->state == 1) {
                e->seq = stub_088ad12c(seq);
                e->state = 2;
            } else if (e->state != 2 || !(e->seq < stub_088ad12c(seq))) {
                continue;
            }
        }
        if (stub_088ad12c(seq) != 0)
            e->seq = stub_088ad12c(seq);
        memcpy(mac, e->mac, 6);
        *port = p;
        n = D_09A1FC50.len - 4;
        if (size < n)
            return -0xC;
        memcpy(buf, D_09A1FC50.ptr + 4, n);
        return n;
    }
}

/* puts a packet in the queue: sent repeat times (0: once, without sequence) */
s32 bcast_send(s32 sock, u8 *mac, u32 port, u8 *data, s32 size, u32 repeat)
{
    BcastRec r;
    u32 seq;

    size = (s16)size;
    if ((u32)D_09A1FC44.cap < (u32)(D_09A1FC44.len + size + 0x24))
        return -0xC;
    r.time = 0;
    memcpy(r.mac, mac, 6);
    r.port = port;
    r.len = size + 4;
    repeat &= 0xFFFF;
    r.repeat = repeat == 0 ? 1 : repeat;
    if (repeat == 0) {
        seq = bswap32(0);
    } else {
        seq = bswap32(D_09A1FC5C[0].seq);
        D_09A1FC5C[0].seq += size + 4;
    }
    memcpy(D_09A1FC44.ptr + D_09A1FC44.len, &r, 0x20);
    D_09A1FC44.len += 0x20;
    memcpy(D_09A1FC44.ptr + D_09A1FC44.len, &seq, 4);
    D_09A1FC44.len += 4;
    memcpy(D_09A1FC44.ptr + D_09A1FC44.len, data, size);
    D_09A1FC44.len += size;
    return size;
}

/* sends the packets of the queue (every 0.25 s) */
s32 bcast_flush(s32 sock)
{
    BcastRec r;
    u64 now = sceKernelGetSystemTimeWide();
    s32 off = 0, n, end;

    while (off < D_09A1FC44.len) {
        memcpy(&r, D_09A1FC44.ptr + off, 0x20);
        if (now < r.time + 250000) {
            off += r.len + 0x20;
            continue;
        }
        n = pdp_bcast_send(sock, r.mac, r.port, D_09A1FC44.ptr + off + 0x20, (s16)r.len);
        if (n < 0)
            return n;
        if (n != r.len)
            return -0xC;
        r.time = now;
        r.repeat--;
        if (r.repeat == 0) {
            end = off + r.len + 0x20;
            if (end < D_09A1FC44.len)
                memcpy(D_09A1FC44.ptr + off, D_09A1FC44.ptr + end, D_09A1FC44.len - end);
            D_09A1FC44.len -= r.len + 0x20;
        } else {
            memcpy(D_09A1FC44.ptr + off, &r, 0x20);
            off += r.len + 0x20;
        }
    }
    return 0;
}

/* ---- the network manager ---- */

void netmgr_init(u8 *m)
{
    netconn_init();
    netcb_register();
    net_handler_add(0x088ADEF0, (u32)m);
    m[0xC] = 0;
    m[0xD] = 0;
}

void netmgr_adhoc(u8 *m)
{
    netconn_setup(1, D_089B2840);
    U32(m, 8) = 0;
    m[0xC] = 1;
}

void netmgr_infra(u8 *m)
{
    netconn_setup(2, NULL);
    U32(m, 8) = 0;
    m[0xC] = 1;
}
