/* The lobby (2): the table of the players and the packets. A player entry (200 bytes,
 * at 0xB8 of each of the 4 slots of D_09A1FDE0, stride 200): {id (MAC) [8], name (0x12
 * bytes + length), info (0xA4 bytes) at 0x20, .., 0xC6 seen, 0xC7 silent}.
 * A packet to build: {u16 data size, u16 write offset, header (0x14 bytes) at 4, data at
 * 0x18}; the header: {4, kind, u16 id (big endian), u16 size, u16 sequence, u16 code,
 * destination MAC, repeat, ..}. (Guessed names.) */
#include "common.h"

#define U16(p, off) (*(u16 *)((u8 *)(p) + (off)))
#define S16(p, off) (*(s16 *)((u8 *)(p) + (off)))
#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))

extern u8 D_09A1FDE0;
extern u8 D_09A202C0[8];          /* the sender of the last packet */
extern s32 D_09A202D0;            /* the socket */
extern u16 D_09A202D4;            /* the sequence */
extern u8 D_09A20608, D_09A20609;
extern u8 D_09A20210[];
extern u32 D_09A28518;            /* the bytes received */
extern const u8 D_089B2A24[6], D_089B2A2C[6], D_089B2A34[6];   /* the broadcast MAC */
extern const u8 D_089B295C[], D_089B2984[], D_089B29AC[], D_089B29D4[], D_089B29FC[];

#define PLAYERS ((u8 *)&D_09A1FDE0)
#define RXBUF ((u8 *)0x09A20618)
#define OWN_MAC ((u8 *)0x09A201F0)
#define DEST ((u8 *)0x09A202B8)

void *memset(void *d, int c, u32 n);
void *memcpy(void *d, const void *s, u32 n);
void *memmove(void *d, const void *s, u32 n);
int memcmp(const void *a, const void *b, u32 n);
s32 sceWlanGetEtherAddr(u8 *mac);
s32 sceNetAdhocctlGetPeerList(s32 *size, void *buf);
s32 bcast_recv(s32 sock, u8 *mac, u16 *port, u8 *buf, s32 size);
s32 bcast_send(s32 sock, u8 *mac, u32 port, u8 *data, s32 size, u32 repeat);
u32 bcast_peers_update(s32 *list, s32 n);
void lobby_own_id(u8 *out);
void func_088B1668(u8 *id, u8 *info);
void func_088B7AF4(u8 *e);
void func_088B7BD8(u8 *id);
void stub_088b1584(u8 *id);

u8 *lobby_pkt(void);
u32 lobby_root(void);
void lobby_pkt_init(u8 *p, u32 kind);
void lobby_pkt_code(u8 *p, u32 code);
void lobby_pkt_size(u8 *p);
s32 lobby_pkt_send(u8 *p);
void lobby_set_dest(u32 use, const u8 *mac);
void lobby_put_name(u8 *p, u8 *s);
void lobby_put_info(u8 *p, u8 *s);
u8 *lobby_get_info(u8 *d, u8 *src);
s32 lobby_find(u8 *id);
s32 lobby_add(u8 *e);
u32 lobby_remove(u8 *id);
void lobby_set_info(u8 *id, u8 *info);
void lobby_touch(u8 *id);
u8 lobby_count(void);
s32 mac_nonzero(const u8 *m);
s32 mac_differs(const u8 *a, const u8 *b);

/* the answer of a request with its code */
void lobby_request6(u8 *out, u8 *to, u32 code)
{
    u8 *p = lobby_pkt();
    u8 h[0x14];

    memset(h, 0, 0x14);
    lobby_set_dest(1, to);
    lobby_pkt_init(p, 6);
    lobby_put_name(p, (u8 *)0x09A201F8);
    lobby_put_info(p, D_09A20210);
    lobby_pkt_code(p, code);
    lobby_pkt_size(p);
    if (lobby_pkt_send(p) == 0)
        memcpy(h, p + 4, 0x14);
    memcpy(out, h, 0x14);
}

/* the sender left */
void lobby_leave(void)
{
    u8 id[8];

    lobby_root();
    lobby_pkt();
    memset(id, 0, 8);
    lobby_own_id(id);
    if (lobby_find(id) == 0) {
        lobby_remove(id);
        if (D_09A1FDE0 != 2)
            stub_088b1584(id);
    }
}

/* the sender changed its info */
void lobby_update_info(void)
{
    u8 info[0xA4];
    u8 id[8];
    u32 x = lobby_root();

    memset(info, 0, 0xA4);
    lobby_own_id(id);
    lobby_get_info(info, (u8 *)x);
    if (lobby_find(id) == 0) {
        lobby_set_info(id, info);
        if (D_09A1FDE0 != 2)
            func_088B1668(id, info);
    }
}

/* 0 if the player is in the table */
s32 lobby_find(u8 *id)
{
    u8 *e = PLAYERS + 0xB8;
    s32 i;

    for (i = 0; i < 4; i++, e += 200)
        if (mac_differs(e, id) == 0)
            return 0;
    return 1;
}

/* 2: updated, 1: added, 0: the table is full */
s32 lobby_add(u8 *e)
{
    u8 *pl = PLAYERS;
    s32 i, r;

    for (i = 0; i < 4; i++, pl += 200) {
        if (mac_differs(pl + 0xB8, e) == 0) {
            memcpy(pl + 0xB8, e, 200);
            pl[0x17E] = 1;
            return 2;
        }
    }
    r = 0;
    for (i = 0, pl = PLAYERS; i < 4; i++, pl += 200) {
        if (mac_nonzero(pl + 0xB8) == 0) {
            memcpy(pl + 0xB8, e, 200);
            r = 1;
            pl[0x17E] = 1;
            break;
        }
    }
    if (D_09A1FDE0 != 2)
        func_088B7AF4(e);
    return r;
}

u32 lobby_remove(u8 *id)
{
    u8 *pl = PLAYERS;
    s32 i;

    for (i = 0; i < 4; i++, pl += 200) {
        if (mac_differs(pl + 0xB8, id) == 0) {
            memset(PLAYERS + 0xB8 + i * 200, 0, 200);
            break;
        }
    }
    if (D_09A1FDE0 != 2)
        func_088B7BD8(id);
    return 0;
}

void lobby_set_info(u8 *id, u8 *info)
{
    u8 *pl = PLAYERS;
    s32 i;

    for (i = 0; i < 4; i++, pl += 200) {
        if (mac_differs(pl + 0xB8, id) == 0) {
            memcpy(pl + 0xD8, info, 0xA4);
            return;
        }
    }
}

void lobby_touch(u8 *id)
{
    u8 *pl = PLAYERS;
    s32 i;

    for (i = 0; i < 4; i++, pl += 200) {
        if (mac_differs(pl + 0xB8, id) == 0) {
            pl[0x17E] = 1;
            pl[0x17F] = 0;
            return;
        }
    }
}

/* 0 if the player is in the table and not silent */
s32 lobby_lost(u8 *id)
{
    u8 *pl = PLAYERS;
    s32 i;

    for (i = 0; i < 4; i++, pl += 200)
        if (mac_differs(pl + 0xB8, id) == 0 && pl[0x17F] == 0)
            return 0;
    return 1;
}

u8 lobby_count(void)
{
    u8 *pl = PLAYERS;
    u8 n = 0;
    s32 i;

    for (i = 0; i < 4; i++, pl += 200)
        if (mac_nonzero(pl + 0xB8) != 0)
            n++;
    return n;
}

void lobby_recv_reset(void)
{
    D_09A28518 = 0;
}

static u32 pkt_size(void)
{
    return (u16)((RXBUF[4] << 8 | RXBUF[5]) + 0x14);
}

/* the next complete packet for this player (or for all) on the port; *err = -1 while
 * it is not complete */
u8 *lobby_recv(s32 sock, u32 port, s32 *err)
{
    u8 any[6];
    u8 mac[0x10];
    u16 p;
    u32 need;
    s32 n;

    memcpy(any, D_089B2A24, 6);
    if (D_09A28518 != 0) {
        need = pkt_size();
        if (D_09A28518 < need) {
            *err = -1;
            return NULL;
        }
        D_09A28518 -= need;
        if (D_09A28518 != 0) {
            /* (the offset in the buffer is 4 times the size) */
            memmove(RXBUF, RXBUF + need * 4, D_09A28518);
            if (D_09A28518 < pkt_size()) {
                *err = -1;
                return NULL;
            }
            *err = 0;
            return RXBUF;
        }
    }
    for (;;) {
        n = bcast_recv(sock, mac, &p, RXBUF + D_09A28518 * 4, (s16)(0x7F00 - D_09A28518));
        if (n == 0) {
            *err = 0;
            return NULL;
        }
        if (n < 0) {
            *err = n;
            return NULL;
        }
        if (memcmp(OWN_MAC, mac, 6) == 0)
            continue;
        if (memcmp(RXBUF + 0xA, any, 6) != 0 && memcmp(OWN_MAC, RXBUF + 0xA, 6) != 0)
            continue;
        if (p != port)
            continue;
        D_09A28518 += n;
        if (D_09A28518 < pkt_size()) {
            *err = -1;
            return NULL;
        }
        *err = 0;
        memcpy(D_09A202C0, mac, 6);
        return RXBUF;
    }
}

u8 *lobby_pkt(void)
{
    return (u8 *)0x09A202EA;
}

u32 lobby_root(void)
{
    return 0x09A2062C;
}

/* a new packet of a kind: the id, the flags and the repeat from the tables */
void lobby_pkt_init(u8 *p, u32 kind)
{
    memset(p + 0x18, 0, 0x300);
    memcpy(p + 0xE, DEST, 6);
    kind &= 0xFFFF;
    p[6] = D_089B295C[kind];
    p[7] = D_089B2984[kind];
    p[5] = D_089B29FC[kind];
    U16(p, 0) = 0;
    U16(p, 2) = 0;
    p[4] = D_089B29AC[kind];
    p[0x14] = D_089B29D4[kind];
    if (p[5] == 2) {
        p[0xA] = D_09A20608;
        p[0xB] = D_09A20609;
    } else {
        D_09A202D4++;
        p[0xA] = D_09A202D4 >> 8;
        p[0xB] = D_09A202D4;
    }
    p[0xC] = 0;
    p[0xD] = 0;
    p[0x16] = 0xFF;
    p[0x17] = 0xFF;
}

void lobby_pkt_code(u8 *p, u32 code)
{
    p[0xC] = (code & 0xFFFF) >> 8;
    p[0xD] = code;
}

void lobby_pkt_size(u8 *p)
{
    p[8] = U16(p, 2) >> 8;
    p[9] = U16(p, 2);
}

void lobby_put8(u8 *p, u32 v)
{
    p[U16(p, 2) + 0x18] = v;
    S16(p, 0)++;
    U16(p, 2)++;
}

void lobby_put16(u8 *p, u32 v)
{
    u16 o = U16(p, 2);

    p[o + 0x18] = (v & 0xFFFF) >> 8;
    p[o + 0x19] = v;
    S16(p, 0) += 2;
    U16(p, 2) += 2;
}

void lobby_put32(u8 *p, u32 v)
{
    u16 o = U16(p, 2);

    p[o + 0x18] = v >> 24;
    p[o + 0x19] = v >> 16;
    p[o + 0x1A] = v >> 8;
    p[o + 0x1B] = v;
    S16(p, 0) += 4;
    U16(p, 2) += 4;
}

void lobby_put_bytes(u8 *p, const u8 *s, s32 n)
{
    n = (s16)n;
    memcpy(p + U16(p, 2) + 0x18, s, n);
    S16(p, 0) += n;
    U16(p, 2) += n;
}

void lobby_put_blob(u8 *p, const u8 *s, u32 n)
{
    lobby_put16(p, n);
    lobby_put_bytes(p, s, n);
}

s32 lobby_pkt_send(u8 *p)
{
    u8 any[6];
    u8 mac[0x10];
    s32 r;

    memcpy(any, D_089B2A2C, 6);
    memcpy(mac, any, 6);
    r = bcast_send(D_09A202D0, mac, 10000, p + 4, (s16)(U16(p, 0) + 0x14), p[0x14]);
    if (r >= 0)
        r = 0;
    return r;
}

/* the destination of the next packets (0: all) */
void lobby_set_dest(u32 use, const u8 *mac)
{
    u8 any[6];
    u8 d[8];

    memcpy(any, D_089B2A34, 6);
    if ((use & 0xFF) == 0)
        memcpy(d, any, 6);
    else
        memcpy(d, mac, 6);
    memcpy(DEST, d, 6);
}

void lobby_put_id(u8 *p, const u8 *id)
{
    lobby_put16(p, 8);
    lobby_put_bytes(p, id, 8);
}

void lobby_put_name(u8 *p, u8 *s)
{
    lobby_put16(p, U16(s, 0x12));
    lobby_put_bytes(p, s, U16(s, 0x12));
}

void lobby_put_info(u8 *p, u8 *s)
{
    u16 n = U16(s, 0xA0);

    lobby_put16(p, n);
    lobby_put_bytes(p, s, n);
}

u8 *lobby_get_blob(u8 *d, u32 max, u8 *src);
u8 *lobby_get16(u8 *d, u8 *src);

u8 *lobby_get_mac(u8 *d, u8 *src)
{
    return lobby_get_blob(d, 6, src);
}

u8 *lobby_get_name(u8 *d, u8 *src)
{
    u16 n;

    lobby_get16((u8 *)&n, src);
    U16(d, 0x12) = n;
    return lobby_get_blob(d, 0x12, src);
}

u8 *lobby_get_info(u8 *d, u8 *src)
{
    u16 n;

    lobby_get16((u8 *)&n, src);
    U16(d, 0xA0) = n;
    return lobby_get_blob(d, 0xA0, src);
}

u8 *lobby_get8(u8 *d, u8 *src)
{
    d[0] = src[0];
    return src + 1;
}

u8 *lobby_get16(u8 *d, u8 *src)
{
    d[1] = src[0];
    d[0] = src[1];
    return src + 2;
}

u8 *lobby_get32(u8 *d, u8 *src)
{
    d[3] = src[0];
    d[2] = src[1];
    d[1] = src[2];
    d[0] = src[3];
    return src + 4;
}

/* a blob {u16 size (big endian), data}: copies at most max bytes */
u8 *lobby_get_blob(u8 *d, u32 max, u8 *src)
{
    u32 n = src[0] << 8 | src[1];
    u32 c = n;

    if ((max & 0xFFFF) < n)
        c = max & 0xFFFF;
    if (c != 0)
        memcpy(d, src + 2, c);
    return src + n + 2;
}

s32 mac_nonzero(const u8 *m)
{
    s32 r = 0, i;

    for (i = 0; i < 6; i++)
        if (m[i] != 0)
            r = 1;
    return r;
}

s32 mac_not_own(const u8 *m)
{
    return memcmp(m, OWN_MAC, 6) != 0;
}

s32 mac_differs(const u8 *a, const u8 *b)
{
    return memcmp(a, b, 6) != 0;
}

u32 mac_copy(u8 *d, const u8 *s)
{
    memcpy(d, s, 6);
    return 0;
}

s32 mac_own(u8 *d)
{
    u8 m[8];
    s32 r = sceWlanGetEtherAddr(m);

    if (r >= 0) {
        memcpy(d, m, 6);
        r = 0;
    }
    return r;
}

s32 peers_count(s32 *list)
{
    s32 n = 0;

    for (; list != NULL; list = (s32 *)list[0])
        n++;
    return n;
}

u32 peers_macs(u8 *out, s32 *list)
{
    for (; list != NULL; list = (s32 *)list[0]) {
        mac_copy(out, (u8 *)(list + 0x21));
        out += 8;
    }
    return 0;
}

/* the MACs of the peers of the group (and the peers of the broadcast) */
s32 peers_get(u8 *out, s32 *n)
{
    u8 buf[0xAB0];
    s32 size = 0, r;

    r = sceNetAdhocctlGetPeerList(&size, NULL);
    if (r < 0)
        return 0;
    if (size != 0) {
        if ((u32)size >= 0xAB0)
            size = 0xAB0;
        memset(buf, 0, 0xAB0);
        r = sceNetAdhocctlGetPeerList(&size, buf);
        *n = peers_count((s32 *)buf);
        peers_macs(out, (s32 *)buf);
    }
    bcast_peers_update((s32 *)buf, size);
    return r;
}
