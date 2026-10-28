/* The lobby of the ad hoc play: the scan of the groups, the connection, the list of the
 * peers and the packets of the lobby protocol. The task (D_09A43FA4): 0xC counter, 0x20
 * active, 0x21 state, 0x22 step, 0x26 error code. A packet header (0x14 bytes, big
 * endian): {kind, code, u16 id, u16 value 4, u16 sequence 6, u16 value 8, ..}.
 * (Guessed names.) */
#include "common.h"

#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define S32(p, off) (*(s32 *)((u8 *)(p) + (off)))
#define U16(p, off) (*(u16 *)((u8 *)(p) + (off)))

extern u8 D_09A43FA4[];           /* the task */
extern u8 D_09A1FDE0;             /* 2: this player is the host */
extern s32 D_09A1FDE4;
extern u8 D_09A1FDE8, D_09A1FDE9;
extern s32 D_09A1FE04;            /* the peers */
extern u8 D_09A1FE08[0x90];       /* 18 peers of 8 bytes */
extern s32 D_09A201E8, D_09A201EC;
extern u8 D_09A202B4;
extern u8 D_09A202C0[8];          /* the id of this player */
extern s32 D_09A202C8, D_09A202D0;
extern u16 D_09A202DC, D_09A202DE, D_09A202E0, D_09A202E2, D_09A202E6;
extern s8 D_09A202E4;
extern u8 D_09A202E8;
extern u8 D_09A20602[0x14];       /* the last header */
extern u8 D_09A20608, D_09A20609;
extern u8 D_09A28884[], D_09A28944[], D_09A20210[];
extern u8 D_09A29E58[0x324];
extern const u8 D_089B284C[0x26], D_089B2874[0x26], D_089B289C[0x26];
extern void (*const D_089B28C4[0x26])(u32 kind);

#define TASK D_09A43FA4
#define PLAYERS ((u8 *)&D_09A1FDE0)

void *memset(void *d, int c, u32 n);
void *memcpy(void *d, const void *s, u32 n);
int memcmp(const void *a, const void *b, u32 n);
s32 sceNetAdhocctlScan(void);
s32 netconn_update(void);
u32 netconn_term_step(void);
void netconn_connect(const u8 *group);
void netconn_terminate(void);
u32 pdp_bcast_close(s32 *sock);
s32 bcast_flush(s32 sock);
u8 *func_088AFC34(s32 sock, u32 timeout, u8 *x);
u8 *func_088AFE78(void);
u32 func_088AFE84(void);
void func_088AFE90(u8 *p, u32 kind);
void func_088AFFBC(u8 *p, u32 v);
void func_088AFFD0(u8 *p);
void func_088B00DC(u8 *p, u8 *data, u32 n);
s32 func_088B012C(u8 *p);
void func_088B01BC(u32 a, u8 *b);
void func_088B0284(u8 *p, u8 *s);
void func_088B02C8(u8 *p, u8 *s);
u32 func_088B0324(u8 *d, u32 x);
void func_088B0370(u8 *d, u32 x);
void func_088B03CC(u8 *d, u32 x);
void func_088B040C(u8 *d, u32 n, u32 x);
s32 func_088B04B4(u8 *id);
s32 func_088B0514(u8 *a, u8 *b);
void func_088B0604(u8 *list, s32 *n);
s32 func_088AF7E0(u8 *id);
s32 func_088AF848(u8 *id);
void func_088AF978(u8 *id);
void func_088AFAD0(u8 *id);
u8 func_088AFBC0(void);
void func_088AF5E4(u8 *out, u8 *buf, u32 err);
void func_088B6B14(u32 x);
void func_088B6E8C(void);
s32 func_088B6E14(u32 id, u32 seq);
void func_088B7004(void);
void func_088B7078(u8 *o, u32 a, u32 b);
void func_088B70E8(void);
void func_088B72CC(void);
void func_088B7350(u8 *id);
void func_088B7580(void);
void func_088B7E74(u32 id);
void func_088B12E8(u8 *p);
void func_088B1350(u8 *p);

s32 lobby_dispatch(u32 id, u32 kind);
void lobby_send_ack(u8 *to, u32 id);
void lobby_peers_check(void);
void lobby_timers(void);
void lobby_ping(u8 *out);
void lobby_own_id(u8 *out);
s32 lobby_not_in_list(u8 *id, u8 *list);
void lobby_request3(u8 *out, u8 *to);
void lobby_request4(u8 *out, u8 *to);

/* ---- the network manager ---- */

void netmgr_terminate(u8 *m)
{
    netconn_terminate();
    U32(m, 8) = 0;
    m[0xC] = 1;
}

void netmgr_update(u8 *m)
{
    if (m[0xC] != 0)
        U32(m, 8) = netconn_update();
}

/* ---- the scan of the groups ---- */

static void task_error(u8 *o, u32 state, u32 code)
{
    o[0x21] = state;
    o[0x22] = 0;
    U16(o, 0x26) = code;
}

void scan_step(u8 *o);
void scan_done(u8 *o);
void scan_fail(u8 *o);

void scan_frame(void)
{
    if (TASK[0x20] == 0)
        return;
    if (TASK[0x21] == 2)
        scan_fail(TASK);
    else if (TASK[0x21] == 1)
        scan_done(TASK);
    else if (TASK[0x21] == 0)
        scan_step(TASK);
}

void scan_start(u8 *o);
void scan_wait(u8 *o);

void scan_step(u8 *o)
{
    if (o[0x22] == 1)
        scan_wait(o);
    else if (o[0x22] == 0)
        scan_start(o);
}

void scan_start(u8 *o)
{
    o[0x22]++;
    D_09A1FDE8 = 0;
    if (sceNetAdhocctlScan() < 0)
        task_error(o, 2, 0x8003);
}

void scan_wait(u8 *o)
{
    if (D_09A1FDE8 != 0) {
        o[0x21] = 1;
        o[0x22] = 0;
    } else if (D_09A1FDE4 != 0) {
        task_error(o, 2, 0x8003);
    }
}

void scan_done(u8 *o)
{
    func_088B7078(o, 0, 0);
}

void scan_fail(u8 *o)
{
    func_088B7078(o, 0xFFFF, U16(o, 0x26));
}

/* ---- the connection ---- */

void conn_start(u8 *o);
void conn_wait(u8 *o);
void conn_done(u8 *o);

void conn_step(u8 *o)
{
    if (o[0x22] == 2)
        conn_done(o);
    else if (o[0x22] == 1)
        conn_wait(o);
    else if (o[0x22] == 0)
        conn_start(o);
}

void conn_start(u8 *o)
{
    o[0x22]++;
    netconn_connect(D_09A28944);
}

void conn_wait(u8 *o)
{
    switch (netconn_update()) {
    case 2:
        o[0x22]++;
        break;
    case 3:
        task_error(o, 3, 0x8003);
        break;
    }
}

void conn_done(u8 *o)
{
    o[0x21] = 1;
    o[0x22] = 0;
    U32(o, 0xC) = 0x5A;
}

/* ---- the first list of the peers ---- */

void open_wait(u8 *o);
void open_next(u8 *o);

void open_step(u8 *o)
{
    if (o[0x22] == 1)
        open_next(o);
    else if (o[0x22] == 0)
        open_wait(o);
}

void open_wait(u8 *o)
{
    if (--S32(o, 0xC) >= 0)
        return;
    o[0x22]++;
    memset(D_09A1FE08, 0, 0x90);
    D_09A1FE04 = 0;
    func_088B0604(D_09A1FE08, &D_09A1FE04);
    if (D_09A1FE04 > 3)
        task_error(o, 3, 0x8052);
}

void open_next(u8 *o)
{
    o[0x21]++;
    o[0x22] = 0;
}

void open_done(u8 *o)
{
    func_088B7078(o, 0, 0);
}

void open_fail(u8 *o)
{
    func_088B7078(o, 0xFFFF, U16(o, 0x26));
}

/* ---- the end ---- */

void close_start(u8 *o);
void close_wait(u8 *o);
void close_done(u8 *o);

void close_step(u8 *o)
{
    if (o[0x22] == 2)
        close_done(o);
    else if (o[0x22] == 1)
        close_wait(o);
    else if (o[0x22] == 0)
        close_start(o);
}

void close_start(u8 *o)
{
    o[0x22]++;
    pdp_bcast_close(&D_09A202D0);
    netconn_terminate();
    D_09A202C8 = 0;
}

void close_wait(u8 *o)
{
    u32 r = netconn_term_step();

    if (r != 5 && r == 4)
        o[0x22]++;
}

void close_done(u8 *o)
{
    o[0x21] = 1;
    o[0x22] = 0;
}

void close_ok(u8 *o)
{
    func_088B7078(o, 0, 0);
}

void close_fail(u8 *o)
{
    func_088B7078(o, 0xFFFF, 0x8003);
}

/* ---- the packets ---- */

void lobby_send_entry(u8 *buf)
{
    u8 *p = func_088AFE78();

    func_088B01BC(0, NULL);
    func_088AFE90(p, 10);
    func_088B0284(p, D_09A28884);
    func_088B00DC(p, buf, U16(buf, 0x300));
    func_088AFFD0(p);
    func_088B012C(p);
}

static void entry_init(u8 *e, u32 size, u32 tail, u32 n, u32 kind)
{
    u32 x = func_088AFE84();

    memset(e, 0, size);
    lobby_own_id(e);
    x = func_088B0324(e + 8, x);
    func_088B03CC(e + tail, x);
    func_088B040C(e + 0x20, n, x);
    func_088B6B14(kind);
}

void lobby_entry_init9(void)
{
    entry_init(D_09A29E58, 0x324, 0x320, 0x300, 9);
}

void lobby_entry_init10(void)
{
    entry_init((u8 *)0x09A29DEC, 0x6C, 0x62, 0x42, 10);
}

void lobby_request(u8 *out, u8 *to, u8 *buf)
{
    u8 *p = func_088AFE78();
    u8 h[0x14];

    memset(h, 0, 0x14);
    func_088B01BC(1, to);
    func_088AFE90(p, 0xC);
    func_088B0284(p, D_09A28884);
    func_088B00DC(p, buf, U16(buf, 0x300));
    func_088AFFD0(p);
    if (func_088B012C(p) == 0)
        memcpy(h, p + 4, 0x14);
    memcpy(out, h, 0x14);
}

void lobby_entry_init11(void)
{
    entry_init(D_09A29E58, 0x324, 0x320, 0x300, 0xB);
}

void lobby_on_packet(u8 *p);

/* one frame: the packets received, the peers and the timers */
u32 lobby_frame(void)
{
    u8 x[4];
    u8 *p;

    D_09A202B4 = 0;
    bcast_flush(D_09A202D0);
    if (D_09A202C8 != 0 && D_09A202D0 != 0)
        while ((p = func_088AFC34(D_09A202D0, 10000, x)) != NULL)
            lobby_on_packet(p);
    if (D_09A1FDE4 == 0 && D_09A1FDE9 == 0) {
        lobby_peers_check();
        lobby_timers();
    }
    if (D_09A1FDE0 != 2) {
        if (D_09A1FDE4 != 0)
            func_088B7580();
        func_088B7004();
        func_088B70E8();
        func_088B72CC();
        func_088B6E8C();
    }
    return 0;
}

static u16 be16(const u8 *p)
{
    return (u16)(p[0] << 8 | p[1]);
}

void lobby_on_packet(u8 *p)
{
    u16 id;

    memcpy(D_09A20602, p, 0x14);
    D_09A202E8 = p[0];
    D_09A202DE = p[1];
    id = be16(p + 2);
    D_09A202DC = id;
    D_09A202E6 = be16(p + 8);
    D_09A202E4 = (D_09A202E6 & 0x8000) != 0 ? -1 : 0;
    D_09A202E0 = be16(p + 6);
    D_09A202E2 = be16(p + 4);
    if (D_09A1FDE0 == 2) {
        if (p[0] == 2) {
            if (p[1] == 0x10)
                lobby_dispatch(id, p[1]);
        } else if (p[1] == 1) {
            lobby_send_ack(D_09A202C0, id);
        }
    } else if (p[1] == 0x10 || p[1] == 1) {
        if (lobby_dispatch(id, p[1]) != 0)
            func_088B7E74(id);
    } else if (p[1] == 2) {
        if (func_088B6E14(id, D_09A202E0) == 0 && lobby_dispatch(id, p[1]) != 0)
            func_088B7E74(id);
    }
}

/* the handler of the packet (id, kind) */
s32 lobby_dispatch(u32 id, u32 kind)
{
    s32 i;

    id &= 0xFFFF;
    kind &= 0xFF;
    for (i = 0; i < 0x26; i++) {
        if (id == (u16)(D_089B284C[i] << 8 | D_089B2874[i]) && D_089B289C[i] == kind &&
            D_089B28C4[i] != NULL) {
            D_089B28C4[i](kind);
            return 1;
        }
    }
    return 0;
}

/* the peers that left the list */
void lobby_peers_check(void)
{
    u8 list[0x90];
    s32 n, i;
    u8 *e;

    if (D_09A202C8 == 0)
        return;
    memset(list, 0, 0x90);
    n = 0;
    func_088B0604(list, &n);
    if (memcmp(D_09A1FE08, list, 0x90) == 0)
        return;
    if (D_09A1FE04 != 0) {
        for (i = 0, e = D_09A1FE08; i < 0x12; i++, e += 8) {
            if (func_088B04B4(e) == 0)
                continue;
            if (lobby_not_in_list(e, list) == 0)
                continue;
            if (func_088AF7E0(e) != 0)
                continue;
            func_088AF978(e);
            if (D_09A1FDE0 != 2)
                func_088B7350(e);
        }
    }
    D_09A1FE04 = n;
    if (n != 0)
        memcpy(D_09A1FE08, list, 0x90);
    else
        memset(D_09A1FE08, 0, 0x90);
}

s32 lobby_list_changed(u8 *a, u8 *b)
{
    return memcmp(a, b, 0x90) != 0;
}

s32 lobby_not_in_list(u8 *id, u8 *list)
{
    s32 i;

    for (i = 0; i < 0x12; i++, list += 8)
        if (func_088B0514(id, list) == 0)
            return 0;
    return 1;
}

void lobby_hdr_init(u8 *p, u32 id);

void lobby_send_ack(u8 *to, u32 id)
{
    u8 *p = func_088AFE78();
    u8 h[0x14];

    memset(h, 0, 0x14);
    func_088B01BC(1, to);
    lobby_hdr_init(p, id);
    func_088AFFBC(p, 0x8006);
    func_088AFFD0(p);
    if (func_088B012C(p) == 0)
        memcpy(h, p + 4, 0x14);
}

void lobby_hdr_init(u8 *p, u32 id)
{
    memset(p + 0x18, 0, 0x300);
    memcpy(p + 0xE, (u8 *)0x09A202B8, 6);
    p[6] = id >> 8;
    p[7] = id;
    p[5] = 2;
    U16(p, 0) = 0;
    U16(p, 2) = 0;
    p[4] = 1;
    p[0x14] = 0;
    p[0xA] = D_09A20608;
    p[0xB] = D_09A20609;
    p[0xC] = 0;
    p[0xD] = 0;
    p[0x16] = 0xFF;
    p[0x17] = 0xFF;
}

void lobby_own_id(u8 *out)
{
    memcpy(out, D_09A202C0, 8);
}

void lobby_last_hdr(u8 *out)
{
    memcpy(out, D_09A20602, 0x14);
}

s32 lobby_code_differs(u32 code)
{
    return D_09A202DE != (code & 0xFF);
}

s32 lobby_sign_differs(s32 v)
{
    return D_09A202E4 != (s8)v;
}

u16 lobby_hdr_value(void)
{
    return D_09A202E6;
}

s32 lobby_seq_differs(s32 seq)
{
    return (s16)D_09A202E0 != (s16)seq;
}

/* every second the ping, every 5 seconds the players are marked */
void lobby_timers(void)
{
    u8 h[0x14];
    s32 i;
    u8 *pl;

    if (D_09A202C8 == 0)
        return;
    if (--D_09A201E8 < 0) {
        D_09A201E8 = 0x3C;
        lobby_ping(h);
        func_088AFAD0((u8 *)0x09A201F0);
    }
    if (--D_09A201EC < 0) {
        D_09A201EC = 300;
        for (i = 0, pl = PLAYERS; i < 4; i++, pl += 200)
            if (func_088B04B4(pl + 0xB8) != 0 && pl[0x17E] == 0)
                pl[0x17F] = 1;
        PLAYERS[0x17E] = 0;
        PLAYERS[0x17E + 200] = 0;
        PLAYERS[0x17E + 400] = 0;
        PLAYERS[0x17E + 600] = 0;
    }
}

void lobby_ping(u8 *out)
{
    u8 *p = func_088AFE78();
    u8 h[0x14];

    memset(h, 0, 0x14);
    func_088B01BC(0, NULL);
    func_088AFE90(p, 2);
    func_088AFFD0(p);
    if (func_088B012C(p) == 0)
        memcpy(h, p + 4, 0x14);
    memcpy(out, h, 0x14);
}

/* this player is in the list of the peers */
void lobby_check_own(void)
{
    u8 h[0x14];
    u8 id[8];

    memset(id, 0, 8);
    lobby_own_id(id);
    if (D_09A1FE04 == 0 || lobby_not_in_list(id, D_09A1FE08) != 0)
        return;
    if (func_088AF7E0(id) == 0)
        func_088AFAD0(id);
    else
        lobby_request3(h, id);
}

void lobby_request3(u8 *out, u8 *to)
{
    u8 *p = func_088AFE78();
    u8 h[0x14];

    memset(h, 0, 0x14);
    func_088B01BC(1, to);
    func_088AFE90(p, 3);
    func_088AFFD0(p);
    if (func_088B012C(p) == 0)
        memcpy(h, p + 4, 0x14);
    memcpy(out, h, 0x14);
}

void lobby_send_info(void)
{
    u8 h[0x14];
    u8 id[8];

    memset(id, 0, 8);
    lobby_own_id(id);
    lobby_request4(h, id);
}

void lobby_request4(u8 *out, u8 *to)
{
    u8 *p = func_088AFE78();
    u8 h[0x14];

    memset(h, 0, 0x14);
    func_088B01BC(1, to);
    func_088AFE90(p, 4);
    func_088B0284(p, (u8 *)0x09A201F8);
    func_088B02C8(p, D_09A20210);
    func_088AFFD0(p);
    if (func_088B012C(p) == 0)
        memcpy(h, p + 4, 0x14);
    memcpy(out, h, 0x14);
}

static void own_entry(u8 *e)
{
    u32 x = func_088AFE84();

    func_088AFE78();
    memset(e, 0, 200);
    lobby_own_id(e);
    x = func_088B0324(e + 8, x);
    func_088B0370(e + 0x20, x);
}

void lobby_join(void)
{
    u8 e[200];
    s32 a, b;

    own_entry(e);
    a = func_088AF7E0(e);
    b = func_088AF848(e);
    func_088AFAD0(e);
    if (a != 0 && b != 0 && D_09A1FDE0 != 2)
        func_088B1350(e);
}

void lobby_join_reply(void)
{
    u8 h[0x14];
    u8 e[200];
    u32 err = 0;

    own_entry(e);
    if (func_088AFBC0() >= 4)
        err = 0x8072;
    if (err == 0) {
        if (func_088AF7E0(e) == 0) {
            func_088AF848(e);
        } else {
            func_088AF848(e);
            if (D_09A1FDE0 != 2)
                func_088B12E8(e);
        }
    }
    func_088AF5E4(h, e, err);
}
