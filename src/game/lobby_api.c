/* The lobby (3): the functions used by the game. Each request starts a task (with the
 * callback of the game, a and b) or answers at once with func_088B6A08(code, a, b).
 * (Guessed names.) */
#include "common.h"

#define U16(p, off) (*(u16 *)((u8 *)(p) + (off)))
#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define S32(p, off) (*(s32 *)((u8 *)(p) + (off)))

extern u8 D_09A1FDE0;             /* 0 none, 1 member, 2 host, 3 */
extern u8 D_09A1FE98[];
extern s32 D_09A201E8;
extern u8 D_09A20210[0xA4];       /* the info of this player */
extern s32 D_09A202C8, D_09A202D0;
extern u8 D_09A2851C[];
extern u8 D_09A2887C[0xC4];       /* the entry of this player: id, name, info */
extern u8 D_09A28875;             /* a host answered */
extern u8 D_09A28884[], D_09A2889C[];
extern u8 D_09A28944[12];         /* the group */
extern u8 D_09A28AD0[200];        /* the host */
extern u8 D_09A29D3C[0xAC];
extern u8 D_09A29E58[0x324];
extern u8 D_09A4401C[];           /* the task of the join */
extern u8 D_09A4410C[];           /* the task of the choice */
extern s32 D_09A2886C;            /* the state of the slots */
extern u32 D_09A28870;
extern u8 D_09A28876, D_09A28878;
extern u8 D_09A28B98[];
extern u16 D_09A28BF0;
extern u8 D_09A299A8;
extern u8 D_09A299AC[0x324];
extern const char D_089B2A3C[];   /* "%d" */

#define PLAYERS ((u8 *)&D_09A1FDE0)
#define OWN_MAC ((u8 *)0x09A201F0)
#define GROUPS ((u8 *)0x09A28950)  /* 24 groups of 0x10 bytes: name[8], .., 0xC used */

void *memset(void *d, int c, u32 n);
void *memcpy(void *d, const void *s, u32 n);
int memcmp(const void *a, const void *b, u32 n);
int sprintf(char *d, const char *fmt, ...);
u32 strlen(const char *s);
s32 sceNetAdhocctlGetScanInfo(s32 *size, void *buf);
void scan_frame(void);
void net_handler_add(u32 f, u32 arg);
void net_handler_remove(u32 f);
s32 mac_own(u8 *d);
s32 mac_nonzero(const u8 *m);
void bcast_init(const u8 *mac);
s32 pdp_bcast_open(u8 *a);
void lobby_recv_reset(void);
u8 *lobby_pkt(void);
u32 lobby_root(void);
void lobby_pkt_init(u8 *p, u32 kind);
void lobby_pkt_size(u8 *p);
s32 lobby_pkt_send(u8 *p);
void lobby_set_dest(u32 use, const u8 *mac);
void lobby_put_name(u8 *p, u8 *s);
void lobby_put_info(u8 *p, u8 *s);
u8 *lobby_get_name(u8 *d, u8 *src);
u8 *lobby_get_info(u8 *d, u8 *src);
void lobby_own_id(u8 *out);
s32 lobby_sign_differs(s32 v);
s32 lobby_find(u8 *id);
s32 lobby_add(u8 *e);
void lobby_set_info(u8 *id, u8 *info);
u8 lobby_count(void);
void lobby_request(u8 *out, u8 *to, u8 *buf);
s32 func_088B6F64(u32 kind, void *f, u32 a, u32 b);
void func_088B6A08(u32 code, u32 a, u32 b);
void func_088B6A80(u32 ev);
void func_088B7078(u8 *o, u32 a, u32 b);
void func_088B7A5C(void);
void func_088B158C(u8 *out, u8 *info);

void lobbyapi_bye(u8 *out);
void lobbyapi_announce(u8 *out);
void lobby_put8(u8 *p, u32 v);
void lobby_put16(u8 *p, u32 v);
void lobby_pkt_code(u8 *p, u32 code);
u8 *lobby_get8(u8 *d, u8 *src);
void func_088B7D78(u32 a, u8 *to, u8 *h);
s32 func_088B6768(u8 *id);
void select_frame(void);
s32 lobby_lost(u8 *id);
void func_088B6964(void);
void func_088B6B14(u32 x);
s32 func_088B33BC(u8 *e, u32 slot);
void func_088B3328(u8 *e, u32 slot);
u8 func_088B32B8(void);
void func_088B21D0(u8 *out, u32 slot);
s32 func_088B3424(u8 *e);
void func_088B2AAC(void);
void func_088B7A50(u32 x);
s32 func_088B6BC4(void);
void func_088B1CA8(u8 *out, u8 *to, u32 a, u32 b);
u8 *func_088B6BFC(u8 *h, void *f, u32 x);

static void start(u32 kind, void *f, u32 a, u32 b)
{
    if (func_088B6F64(kind, f, a, b) != 0)
        func_088B6A08(0x8005, a, b);
}

u32 lobbyapi_scan(u32 a, u32 b)
{
    start(1, scan_frame, a, b);
    return 0;
}

/* the 24 groups of the game and the ones found by the scan */
u32 lobbyapi_groups(u8 *n, u8 *out)
{
    char s[0x20];
    u8 *g, *e;
    s32 size, i;

    size = 0;
    *n = 0x18;
    memset(GROUPS, 0, 0x180);
    for (i = 0, g = GROUPS; i < 0x18; i++, g += 0x10) {
        memcpy(g, (u8 *)0x09A28528, 6);
        sprintf(s, D_089B2A3C, i);
        if (strlen(s) == 1) {
            g[6] = '0';
            g[7] = s[0];
        } else {
            g[6] = s[0];
            g[7] = s[1];
        }
    }
    memcpy(out, GROUPS, 0x180);
    size = 0x32C;
    if (sceNetAdhocctlGetScanInfo(&size, (u8 *)0x09A28534) < 0 || size == 0)
        return 0;
    for (e = (u8 *)0x09A28534; e != NULL; e = (u8 *)U32(e, 0)) {
        for (i = 0, g = GROUPS; i < 0x18; i++, g += 0x10) {
            if (memcmp(g, e + 8, 8) == 0) {
                GROUPS[i * 0x10 + 0xC] = 1;
                break;
            }
        }
    }
    return 0;
}

u32 lobbyapi_connect(const u8 *group, u32 a, u32 b)
{
    if (func_088B6F64(2, (void *)0x088AE0D4, a, b) == 0)
        memcpy(D_09A28944, group, 12);
    else
        func_088B6A08(0x8005, a, b);
    return 0;
}

u32 lobbyapi_open(u32 a, u32 b)
{
    start(3, (void *)0x088AE380, a, b);
    return 0;
}

void lobbyapi_entry(u8 *out)
{
    memcpy(out, D_09A29E58, 0x324);
}

u32 lobbyapi_send_to(u8 *mac, u8 *buf, u32 a, u32 b)
{
    u8 h[0x14];

    if (mac_nonzero(mac) == 0) {
        func_088B6A08(0x8000, a, b);
    } else {
        lobby_request(h, mac, buf);
        func_088B6A08(0, a, b);
    }
    return 0;
}

u32 lobbyapi_init(const u8 *cfg)
{
    u8 mac[0x10];

    memset(D_09A2851C, 0, 0x1C740);
    memset(PLAYERS, 0, 0x873C);
    D_09A1FDE0 = 0;
    memcpy(D_09A2851C, cfg, 0x18);
    net_handler_add(0x088B047C, 0);
    mac_own(OWN_MAC);
    memcpy(mac, OWN_MAC, 6);
    bcast_init(mac);
    return 0;
}

/* the start of the lobby with the name (0x16 bytes) and the info (at 0x18) */
u32 lobbyapi_start(u8 *p)
{
    u8 a[0x14];

    memset(D_09A2851C, 0, 0x1C740);
    mac_own(OWN_MAC);
    memcpy((u8 *)0x09A201F8, p, 0x16);
    memcpy(D_09A20210, p + 0x18, 0xA4);
    if (D_09A1FDE0 == 2) {
        D_09A1FDE0 = 3;
        func_088B7A5C();
    } else if (D_09A1FDE0 != 3 && D_09A1FDE0 != 1 && D_09A1FDE0 == 0) {
        D_09A1FDE0 = 1;
        U16(a, 0x12) = 10000;
        D_09A202D0 = pdp_bcast_open(a);
        lobby_recv_reset();
        D_09A201E8 = 0x3C;
    }
    D_09A202C8 = 1;
    mac_own(D_09A2887C);
    memcpy(D_09A28884, p, 0x16);
    memcpy(D_09A2889C, p + 0x18, 0xA4);
    return 0;
}

u32 lobbyapi_term(void)
{
    net_handler_remove(0x088B047C);
    return 0;
}

u32 lobbyapi_set_info(u8 *info)
{
    u8 h[0x14];

    memcpy(D_09A2889C, info, 0xA4);
    memcpy(D_09A20210, info, 0xA4);
    lobby_set_info(OWN_MAC, info);
    func_088B158C(h, info);
    return 0;
}

u32 lobbyapi_get_29d3c(u8 *out)
{
    memcpy(out, D_09A29D3C, 0xAC);
    return 0;
}

void join_frame(void);

u32 lobbyapi_join(u32 a, u32 b)
{
    start(4, join_frame, a, b);
    return 0;
}

u32 lobbyapi_leave(u32 a, u32 b)
{
    u8 h[0x14];

    lobbyapi_bye(h);
    func_088B6A08(0, a, b);
    return 0;
}

u32 lobbyapi_ok(u32 a, u32 b)
{
    func_088B6A08(0, a, b);
    return 0;
}

u32 lobbyapi_players(u8 *n, u8 *out)
{
    u8 *pl = PLAYERS;
    s32 i;

    *n = lobby_count();
    for (i = 0; i < 4; i++, pl += 200) {
        if (mac_nonzero(pl + 0xB8) != 0) {
            memcpy(out, pl + 0xB8, 200);
            out += 200;
        }
    }
    return 0;
}

u32 lobbyapi_host(u8 *out)
{
    memcpy(out, D_09A28AD0, 200);
    return 0;
}

u32 lobbyapi_host_id(u8 *out)
{
    memcpy(out, D_09A28AD0, 8);
    return 0;
}

/* ---- the task of the join (D_09A4401C) ---- */

void join_step(u8 *o);
void join_done(u8 *o);
void join_fail(u8 *o);

void join_frame(void)
{
    if (D_09A4401C[0x20] == 0)
        return;
    if (D_09A4401C[0x21] == 2)
        join_fail(D_09A4401C);
    else if (D_09A4401C[0x21] == 1)
        join_done(D_09A4401C);
    else if (D_09A4401C[0x21] == 0)
        join_step(D_09A4401C);
}

void join_start(u8 *o);
void join_wait(u8 *o);

void join_step(u8 *o)
{
    if (o[0x22] == 1)
        join_wait(o);
    else if (o[0x22] == 0)
        join_start(o);
}

/* this player becomes the host and says it to all */
void join_start(u8 *o)
{
    u8 e[200];
    u8 h[0x14];

    o[0x22]++;
    D_09A28875 = 0;
    if (D_09A1FDE0 != 1) {
        o[0x21] = 1;
        o[0x22] = 0;
        D_09A1FDE0 = 1;
        return;
    }
    memset(D_09A1FE98, 0, 800);
    memcpy(e, D_09A2887C, 0xC4);
    lobby_add(e);
    S32(o, 0xC) = 0x5A;
    lobbyapi_announce(h);
    if (h[2] == 0) {
        U16(o, 0x26) = 0x8003;
        o[0x21] = 2;
        o[0x22] = 0;
    }
}

void join_wait(u8 *o)
{
    if (D_09A28875 != 0) {
        o[0x21] = 2;
        o[0x22] = 0;
        U16(o, 0x26) = 0x8072;
    } else if (--S32(o, 0xC) < 0) {
        D_09A1FDE0 = 1;
        func_088B7078(o, 0, 0);
    }
}

void join_done(u8 *o)
{
    func_088B7078(o, 0, 0);
}

void join_fail(u8 *o)
{
    func_088B7078(o, 0xFFFF, U16(o, 0x26));
}

static void send_header(u8 *out, u8 *p)
{
    u8 h[0x14];

    memset(h, 0, 0x14);
    if (lobby_pkt_send(p) == 0)
        memcpy(h, p + 4, 0x14);
    memcpy(out, h, 0x14);
}

void lobbyapi_announce(u8 *out)
{
    u8 *p = lobby_pkt();

    lobby_set_dest(0, NULL);
    lobby_pkt_init(p, 5);
    lobby_put_name(p, D_09A28884);
    lobby_put_info(p, D_09A2889C);
    lobby_pkt_size(p);
    send_header(out, p);
}

static void set_host(u8 *e)
{
    if (mac_nonzero(e) == 0)
        return;
    memcpy(D_09A28AD0, e, 200);
    func_088B6A80(5);
    func_088B6A80(7);
}

void lobbyapi_host_found(u8 *e)
{
    set_host(e);
}

void lobbyapi_host_joined(u8 *e)
{
    set_host(e);
}

/* the announce of another player */
void lobbyapi_on_announce(void)
{
    u8 e[200];
    u8 h1[0x14], h2[0x14];
    u8 *x = (u8 *)lobby_root();

    lobby_pkt();
    if (D_09A4401C[0x20] == 0 && lobby_sign_differs(0) != 0) {
        lobbyapi_bye(h2);
        func_088B6A80(0x14);
        return;
    }
    memset(e, 0, 200);
    if (lobby_sign_differs(0) == 0) {
        if (D_09A28875 != 0)
            return;
        lobby_own_id(e);
        x = lobby_get_name(e + 8, x);
        lobby_get_info(e + 0x20, x);
        if (lobby_find(e) == 0) {
            lobby_add(e);
        } else {
            lobby_add(e);
            func_088B6A80(7);
        }
    } else if (D_09A28875 == 0) {
        D_09A28875 = 1;
        lobbyapi_bye(h1);
    }
}

void lobbyapi_bye(u8 *out)
{
    u8 *p = lobby_pkt();

    lobby_set_dest(0, NULL);
    lobby_pkt_init(p, 7);
    lobby_pkt_size(p);
    send_header(out, p);
}

void lobbyapi_send_info(u8 *out, u8 *info)
{
    u8 *p = lobby_pkt();

    lobby_set_dest(0, NULL);
    lobby_pkt_init(p, 8);
    lobby_put_info(p, info);
    lobby_pkt_size(p);
    send_header(out, p);
}

/* the info of a player changed */
void lobbyapi_on_info(u8 *id, u8 *info)
{
    if (D_09A28876 != 0)
        func_088B6964();
    memcpy(D_09A29D3C, id, 8);
    memcpy(D_09A29D3C + 8, info, 0xA4);
    func_088B6B14(0x12);
}

/* the choice of a slot: the host takes it at once, the others ask the host */
u32 lobbyapi_select(u32 slot, u32 a, u32 b)
{
    u8 h[0x14];

    if (D_09A2886C == 4 || D_09A2886C == 3 || D_09A2886C == 1 || D_09A2886C == 0) {
        func_088B6A08(0x8021, a, b);
    } else if (D_09A2886C == 5) {
        func_088B6A08(0x8031, a, b);
    } else if (D_09A28876 != 0) {
        if ((u16)func_088B33BC(D_09A2887C, slot) == 0)
            func_088B3328(D_09A2887C, slot);
        D_09A299A8 = func_088B32B8();
        func_088B21D0(h, D_09A299A8);
        func_088B6A80(3);
        func_088B6A08(0, a, b);
    } else if (lobby_lost(D_09A28B98) != 0) {
        func_088B6A08(0x8007, a, b);
    } else {
        D_09A28878 = slot;
        start(10, select_frame, a, b);
    }
    return 0;
}

u32 lobbyapi_begin(u32 a, u32 b)
{
    if (D_09A28876 == 0) {
        func_088B6A08(0x8004, a, b);
    } else if (D_09A2886C == 5) {
        func_088B6A08(0x8031, a, b);
    } else if (func_088B3424(D_09A2887C) == 0) {
        func_088B6A08(0x8033, a, b);
    } else if (func_088B6F64(5, func_088B2AAC, a, b) == 0) {
        func_088B7A50(5);
    } else {
        func_088B6A08(0x8005, a, b);
    }
    return 0;
}

u32 lobbyapi_cancel(u32 a, u32 b)
{
    start(6, (void *)0x088B3130, a, b);
    return 0;
}

/* the entry of the host: this player becomes the host */
u32 lobbyapi_host_entry(u8 *out)
{
    memcpy(out, D_09A299AC, 0x324);
    D_09A1FDE0 = 2;
    return 0;
}

u32 lobbyapi_slot(u8 *out)
{
    *out = D_09A299A8;
    return 0;
}

/* ---- the task of the choice (D_09A4410C) ---- */

void select_step(u8 *o);
void select_done(u8 *o);
void select_fail(u8 *o);

void select_frame(void)
{
    if (D_09A4410C[0x20] == 0)
        return;
    if (D_09A4410C[0x21] == 2)
        select_fail(D_09A4410C);
    else if (D_09A4410C[0x21] == 1)
        select_done(D_09A4410C);
    else if (D_09A4410C[0x21] == 0)
        select_step(D_09A4410C);
}

void select_start(u8 *o);
void select_wait(u8 *o);

void select_step(u8 *o)
{
    if (o[0x22] == 1)
        select_wait(o);
    else if (o[0x22] == 0)
        select_start(o);
}

/* asks the host and waits for the answer (callback 0x088B1C34) */
void select_start(u8 *o)
{
    u8 *p = lobby_pkt();
    u8 h[0x14];
    u8 *w;

    o[0x22]++;
    if (func_088B6BC4() == 0) {
        U16(o, 0x26) = 0x8002;
        o[0x21] = 2;
        o[0x22] = 0;
        return;
    }
    func_088B1CA8(h, D_09A28B98, D_09A28BF0, D_09A28878);
    if (h[2] == 0) {
        U16(o, 0x26) = 0x8003;
        o[0x21] = 2;
        o[0x22] = 0;
        return;
    }
    w = func_088B6BFC(h, (void *)0x088B1C34, 0);
    w[1] = 1;
    memcpy(w + 0x24, p, 0x318);
}

void select_wait(u8 *o)
{
    if (o[0x23] == 1) {
        o[0x21] = 1;
        o[0x22] = 0;
    } else if (o[0x23] == 2) {
        o[0x21] = 2;
        o[0x22] = 0;
    }
}

void select_done(u8 *o)
{
    D_09A28870 = D_09A28878 != 0;
    func_088B7078(o, 0, 0);
}

void select_fail(u8 *o)
{
    func_088B7078(o, 0xFFFF, U16(o, 0x26));
}

/* ---- the packets of the slots ---- */

/* asks the host for a slot */
void lobbyapi_ask_slot(u8 *out, u8 *to, u32 a, u32 slot)
{
    u8 *p = lobby_pkt();

    lobby_set_dest(1, to);
    lobby_pkt_init(p, 0x1C);
    lobby_put16(p, a);
    lobby_put8(p, slot);
    lobby_pkt_size(p);
    send_header(out, p);
}

void lobbyapi_answer(u8 *out, u8 *to, u32 code)
{
    u8 *p = lobby_pkt();
    u8 h[0x14];

    memset(h, 0, 0x14);
    lobby_set_dest(1, to);
    lobby_pkt_init(p, 0x1D);
    lobby_pkt_code(p, code);
    lobby_pkt_size(p);
    if (lobby_pkt_send(p) == 0) {
        memcpy(h, p + 4, 0x14);
        func_088B7D78(2, to, h);
    }
    memcpy(out, h, 0x14);
}

/* the host gave the slot */
void lobbyapi_on_slot(void)
{
    lobby_get8(&D_09A299A8, (u8 *)lobby_root());
    func_088B6B14(0);
}

void lobbyapi_slot_reply(u8 *out, u8 *to, u32 code);

/* a player asks for its slot */
void lobbyapi_on_ask(void)
{
    u8 h[0x14];
    u8 id[8];
    u32 code;

    lobby_own_id(id);
    if (D_09A28876 == 0)
        code = 0x8000;
    else if (func_088B6768(id) == 0)
        code = 0;
    else
        code = 0x8021;
    lobbyapi_slot_reply(h, id, code);
}

void lobbyapi_slot_reply(u8 *out, u8 *to, u32 code)
{
    u8 *p = lobby_pkt();

    lobby_set_dest(1, to);
    lobby_pkt_init(p, 0x22);
    lobby_pkt_code(p, code);
    if ((code & 0xFFFF) == 0)
        lobby_put8(p, func_088B32B8());
    lobby_pkt_size(p);
    send_header(out, p);
}
