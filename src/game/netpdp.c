/* The sockets of the session: PTP/PDP of the ad hoc library (mode 1) or TCP sockets of
 * the infrastructure library (mode 2). An address for open/listen: {address[0x10], u16
 * port of the other one (0x10), u16 own port (0x12)}. (Guessed names.) */
#include "common.h"

#define U16(p, off) (*(u16 *)((u8 *)(p) + (off)))
#define S16(p, off) (*(s16 *)((u8 *)(p) + (off)))
#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))

/* the TCP sockets: {socket, IP address, .., port (network order) at 0x14} */
typedef struct {
    s32 sock;
    u8 ip[4];
    u8 pad[0xC];
    u16 port;
    u16 pad2;
} InetSock;

typedef struct {
    u8 len;
    u8 family;
    u16 port;
    u8 addr[4];
    u8 zero[8];
} SockAddr;

extern s32 D_09A1D728;            /* the mode */
extern InetSock D_09A1D72C[16];
extern u8 D_09A1D8AC[0x240];      /* the status of the sockets */

void *memset(void *d, int c, u32 n);
void *memcpy(void *d, const void *s, u32 n);
int memcmp(const void *a, const void *b, u32 n);
s32 ns_errcode(s32 r);
s32 sceNetGetLocalEtherAddr(u8 *mac);
s32 sceNetAdhocPtpOpen(u8 *srcmac, u32 sport, u8 *dstmac, u32 dport, u32 bufsize,
                       u32 delay, s32 count, s32 flag);
s32 sceNetAdhocPtpConnect(s32 id, u32 timeout, s32 nonblock);
s32 sceNetAdhocPtpListen(u8 *srcmac, u32 sport, u32 bufsize, u32 delay, s32 count,
                         s32 queue, s32 flag);
s32 sceNetAdhocPtpAccept(s32 id, u8 *mac, u16 *port, u32 timeout, s32 nonblock);
s32 sceNetAdhocGetPtpStat(s32 *size, void *buf);
s32 sceNetAdhocPtpRecv(s32 id, void *buf, s32 *len, u32 timeout, s32 nonblock);
s32 sceNetAdhocPtpSend(s32 id, const void *buf, s32 *len, u32 timeout, s32 nonblock);
s32 sceNetAdhocPtpClose(s32 id, s32 flag);
s32 sceNetAdhocPdpCreate(u8 *mac, u32 port, u32 bufsize, s32 flag);
s32 sceNetAdhocPdpRecv(s32 id, u8 *mac, u16 *port, void *buf, s32 *len, u32 timeout,
                       s32 nonblock);
s32 sceNetAdhocPdpSend(s32 id, u8 *mac, u32 port, const void *buf, s32 len, u32 timeout,
                       s32 nonblock);
s32 sceNetAdhocPdpDelete(s32 id, s32 flag);
s32 sceNetInetSocket(s32 domain, s32 type, s32 proto);
s32 sceNetInetSetsockopt(s32 s, s32 level, s32 opt, void *val, u32 len);
s32 sceNetInetClose(s32 s);
s32 sceNetInetCloseWithRST(s32 s);
s32 sceNetInetGetPspError(void);
s32 sceNetInetConnect(s32 s, void *addr, u32 len);
s32 sceNetInetBind(s32 s, void *addr, u32 len);
s32 sceNetInetListen(s32 s, s32 n);
s32 sceNetInetAccept(s32 s, void *addr, u32 *len);
s32 sceNetInetGetpeername(s32 s, void *addr, u32 *len);
s32 sceNetInetGetTcpcbstat(s32 *size, void *buf);
s32 sceNetInetRecv(s32 s, void *buf, u32 len, s32 flags);
s32 sceNetInetSend(s32 s, const void *buf, u32 len, s32 flags);

static u16 swap16(u16 v)
{
    return (u16)(v << 8 | v >> 8);
}

static s32 inet_error(void)
{
    return ns_errcode(sceNetInetGetPspError());
}

/* a TCP socket with SO_REUSEADDR and the non blocking option */
static s32 inet_socket(void)
{
    s32 s = sceNetInetSocket(2, 1, 0);
    u32 on;

    if (s < 0)
        return inet_error();
    on = 1;
    if (sceNetInetSetsockopt(s, 0xFFFF, 1, &on, 4) < 0) {
        sceNetInetClose(s);
        return inet_error();
    }
    on = 1;
    if (sceNetInetSetsockopt(s, 0xFFFF, 0x1009, &on, 4) < 0) {
        sceNetInetClose(s);
        return inet_error();
    }
    return s;
}

/* a socket to the other player (it connects later) */
s32 pdp_open(u8 *a)
{
    u8 mac[8];
    s32 s, i;

    if (D_09A1D728 == 1) {
        s = ns_errcode(sceNetGetLocalEtherAddr(mac));
        if (s >= 0)
            s = ns_errcode(sceNetAdhocPtpOpen(mac, U16(a, 0x12), a, U16(a, 0x10), 0x2000,
                                              250000, 100, 0));
        return s;
    }
    s = inet_socket();
    if (s < 0)
        return s;
    for (i = 0; i < 16; i++) {
        if (D_09A1D72C[i].sock < 0) {
            D_09A1D72C[i].sock = s;
            memcpy(D_09A1D72C[i].ip, a, 4);
            D_09A1D72C[i].port = swap16(U16(a, 0x10));
            return s;
        }
    }
    return s;
}

/* 0: connected, -1: not yet */
s32 pdp_connect(s32 sock)
{
    SockAddr sa;
    s32 r, i;

    if (D_09A1D728 == 1) {
        r = sceNetAdhocPtpConnect(sock, 0, 1);
        if (r == (s32)0x80410709)
            return -1;
        r = ns_errcode(r);
        if (r >= 0)
            r = 0;
        return r;
    }
    for (i = 0; i < 16; i++)
        if (sock == D_09A1D72C[i].sock)
            break;
    if (i == 16)
        return -1;
    memset(&sa, 0, 0x10);
    sa.family = 2;
    memcpy(sa.addr, D_09A1D72C[i].ip, 4);
    sa.port = D_09A1D72C[i].port;
    r = sceNetInetConnect(sock, &sa, 0x10);
    if (r >= 0)
        return 0;
    r = sceNetInetGetPspError();
    if ((u32)(r + 0x7FFEFF89) < 2)
        return -1;
    if (r == (s32)0x8001007F)
        return 0;
    return ns_errcode(r);
}

/* a socket that waits for the other player */
s32 pdp_listen(u8 *a)
{
    SockAddr sa;
    u8 mac[8];
    s32 s;

    if (D_09A1D728 == 1) {
        s = ns_errcode(sceNetGetLocalEtherAddr(mac));
        if (s >= 0)
            s = ns_errcode(sceNetAdhocPtpListen(mac, U16(a, 0x12), 0x2000, 250000, 100, 3,
                                                0));
        return s;
    }
    s = inet_socket();
    if (s < 0)
        return s;
    memset(&sa, 0, 0x10);
    U32(sa.addr, 0) = 0;
    sa.family = 2;
    sa.port = swap16(U16(a, 0x12));
    if (sceNetInetBind(s, &sa, 0x10) < 0) {
        sceNetInetClose(s);
        return inet_error();
    }
    if (sceNetInetListen(s, 3) < 0) {
        sceNetInetClose(s);
        return inet_error();
    }
    return s;
}

/* the socket of a player that connected, -1 if none */
s32 pdp_accept(s32 sock, u8 *addr, u16 *port)
{
    SockAddr sa;
    u32 len;
    s32 s, i;

    if (D_09A1D728 == 1) {
        s = sceNetAdhocPtpAccept(sock, addr, port, 0, 1);
        if (s == (s32)0x80410709)
            return -1;
        return ns_errcode(s);
    }
    len = 0x10;
    s = sceNetInetAccept(sock, &sa, &len);
    if (s < 0) {
        s = sceNetInetGetPspError();
        if (s == (s32)0x8001000B)
            return -1;
        return ns_errcode(s);
    }
    len = 0x10;
    if (sceNetInetGetpeername(s, &sa, &len) < 0)
        return inet_error();
    memcpy(addr, sa.addr, 4);
    *port = swap16(sa.port);
    for (i = 0; i < 16; i++) {
        if (D_09A1D72C[i].sock < 0) {
            D_09A1D72C[i].sock = s;
            memcpy(D_09A1D72C[i].ip, sa.addr, 4);
            D_09A1D72C[i].port = sa.port;
            return s;
        }
    }
    return s;
}

/* the status of a socket: st = {state, refresh (st[1] != 0: reads the list again), free
 * space to send, bytes to receive} */
s32 pdp_status(s32 sock, u16 *st)
{
    s32 size, i, v;
    u8 *e;

    st[0] = 0;
    st[3] = 0;
    st[2] = 0;
    if (D_09A1D728 == 1) {
        if (st[1] != 0) {
            size = 0x240;
            v = ns_errcode(sceNetAdhocGetPtpStat(&size, D_09A1D8AC));
            if (v < 0)
                return v;
            if (size < 1)
                return -1;
        }
        if (sock < 0)
            return -1;
        for (e = D_09A1D8AC; e != NULL; e = (u8 *)U32(e, 0)) {
            if (sock == (s32)U32(e, 4)) {
                v = U32(e, 0x20);
                st[0] = (v >= 1 && v <= 4) ? v : 0;
                st[2] = 0x2000 - (s16)U32(e, 0x18);
                st[3] = U32(e, 0x1C);
                return 0;
            }
        }
        return -1;
    }
    if (st[1] != 0) {
        size = 0x240;
        if (sceNetInetGetTcpcbstat(&size, D_09A1D8AC) < 0)
            return -1;
        if (size < 1)
            return -1;
    }
    if (sock < 0)
        return -1;
    for (i = 0; i < 16; i++)
        if (sock == D_09A1D72C[i].sock)
            break;
    if (i == 16)
        return -1;
    for (e = D_09A1D8AC; e != NULL; e = (u8 *)U32(e, 0)) {
        if (memcmp(e + 0x10, D_09A1D72C[i].ip, 4) == 0 &&
            S16(e, 0x16) == (s16)D_09A1D72C[i].port) {
            v = U16(e, 0x18);
            st[0] = v <= 10 ? v : 0;
            st[2] = 0x2000 - (s16)U32(e, 4);
            st[3] = U32(e, 8);
            return 0;
        }
    }
    return -1;
}

/* the bytes received, 0 if none */
s32 pdp_recv(s32 sock, u8 *buf, s32 size)
{
    s32 len, r;

    if (D_09A1D728 == 1) {
        len = (s16)size;
        r = sceNetAdhocPtpRecv(sock, buf, &len, 0, 1);
        if (r == (s32)0x80410709)
            return 0;
        r = ns_errcode(r);
        if (r >= 0)
            r = len;
        return r;
    }
    r = sceNetInetRecv(sock, buf, (s16)size, 0);
    if (r < 0) {
        r = sceNetInetGetPspError();
        if (r == (s32)0x8001000B)
            return 0;
        return ns_errcode(r);
    }
    if (r == 0)
        return ns_errcode(0x80010068);
    return r;
}

s32 pdp_send(s32 sock, u8 *data, s32 size)
{
    s32 len, r;

    size = (s16)size;
    if (D_09A1D728 == 1) {
        len = size;
        r = ns_errcode(sceNetAdhocPtpSend(sock, data, &len, 0, 0));
        if (r >= 0) {
            r = len;
            if (len != size)
                r = -0xC;
        }
        return r;
    }
    r = sceNetInetSend(sock, data, size, 0);
    if (r == size)
        return size;
    if (r < 0)
        return inet_error();
    return ns_errcode(0x8001000C);
}

u32 pdp_close(s32 *sock)
{
    s32 i;

    if (D_09A1D728 == 1) {
        if (*sock >= 0) {
            sceNetAdhocPtpClose(*sock, 0);
            *sock = -1;
        }
    } else if (*sock >= 0) {
        for (i = 0; i < 16; i++) {
            if (*sock == D_09A1D72C[i].sock) {
                D_09A1D72C[i].sock = -1;
                break;
            }
        }
        sceNetInetCloseWithRST(*sock);
        *sock = -1;
    }
    return 0;
}

/* the broadcast socket (always ad hoc) */
s32 pdp_bcast_open(u8 *a)
{
    u8 mac[8];
    s32 r;

    r = ns_errcode(sceNetGetLocalEtherAddr(mac));
    if (r < 0)
        return r;
    return ns_errcode(sceNetAdhocPdpCreate(mac, U16(a, 0x12), 0x2000, 0));
}

s32 pdp_bcast_recv(s32 sock, u8 *addr, u16 *port, u8 *buf, s32 size)
{
    u8 mac[8];
    s32 len, r;

    len = (s16)size;
    r = sceNetAdhocPdpRecv(sock, mac, port, buf, &len, 0, 1);
    if (r == (s32)0x80410709)
        return 0;
    r = ns_errcode(r);
    if (r < 0)
        return r;
    memcpy(addr, mac, 6);
    return len;
}

s32 pdp_bcast_send(s32 sock, u8 *addr, u32 port, u8 *data, s32 size)
{
    u8 mac[8];
    s32 r;

    memcpy(mac, addr, 6);
    size = (s16)size;
    r = ns_errcode(sceNetAdhocPdpSend(sock, mac, port, data, size, 0, 0));
    if (r >= 0)
        r = size;
    return r;
}

u32 pdp_bcast_close(s32 *sock)
{
    if (*sock >= 0) {
        sceNetAdhocPdpDelete(*sock, 0);
        *sock = -1;
    }
    return 0;
}
