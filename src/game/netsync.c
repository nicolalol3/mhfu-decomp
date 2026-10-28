/* The data exchange between the players of an ad hoc session (PDP packets).
 * The session (D_09A1D5D0): 8 sockets[4], 0x18 broadcast socket, 0x1C active, 0x20 mode,
 * 0x21 state, 0x22 players, 0x23 own index, 0x24 addresses[4] (0x10 bytes), 0x68 player
 * states[4], 0x6C last full packet time, 0x78 time, 0x8C/0xA4/0xA8 byte counters, 0xAC
 * packets[4], 0xD0 last send, 0xD8 socket status, 0x114 sequence[4], 0x118 stamps[4],
 * 0x120/0x124 errors, 0x12C error value, 0x134 timeout.
 * A packet: {size, flags (kind, 0x10: player states), sender, sequence, u16 stamp, data}
 * and the data are messages {size, kind, ...}. (Guessed names.) */
#include "common.h"

#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define S32(p, off) (*(s32 *)((u8 *)(p) + (off)))
#define S16(p, off) (*(s16 *)((u8 *)(p) + (off)))
#define U16(p, off) (*(u16 *)((u8 *)(p) + (off)))
#define F32(p, off) (*(f32 *)((u8 *)(p) + (off)))

typedef struct {
    s32 len;
    s32 cap;
    u8 *ptr;
} NetBuf;

/* a link to another player: {state, f32 t1, f32 t0, address[0x10], socket} */
typedef struct {
    s32 state;
    f32 t1;
    f32 t0;
    u8 addr[0x10];
    s32 sock;
} NetLink;

extern NetBuf D_09A11ED8;         /* the received packet */
extern NetBuf D_09A11EE4;         /* the messages to send to all */
extern NetLink D_09A11EF0[4];
extern u8 D_09A11F70[0x100];      /* the packet to send */
extern NetBuf D_09A1D590;         /* the received messages */
extern NetBuf D_09A1D578;         /* the received messages of the game */
extern NetBuf D_09A1D56C;         /* the messages to send */
extern NetBuf D_09A1D59C[4];      /* the stream of each player */
extern u8 D_09A1D5D0[];
extern u8 D_09A1D5EC, D_09A1D5F1, D_09A1D5F2, D_09A1D5F3;

#define PKT D_09A11F70
#define PKT_DATA ((u8 *)0x09A11F76)
#define BROADCAST ((u8 *)0x09A0FDC8)

void *memset(void *d, int c, u32 n);
void *memcpy(void *d, const void *s, u32 n);
void *memmove(void *d, const void *s, u32 n);
int memcmp(const void *a, const void *b, u32 n);
s64 sceKernelGetSystemTimeWide(void);
int sceKernelSysClock2USecWide(s64 clock, s32 *sec, s32 *usec);
int rand(void);
void func_088AB028(s32 *id);
s32 func_088A9CDC(void);
s32 func_088A9CFC(void);
void func_088A9DF4(u32 *out);
void func_088A9EBC(void);
void func_088A9EF4(void);
s32 func_088AA370(u8 *p);
s32 func_088AA520(s32 id);
s32 func_088AA67C(u8 *p);
s32 func_088AA83C(s32 id, u8 *addr, u8 *port);
s32 func_088AA9B4(s32 id, u8 *stat);
s32 func_088AAD58(s32 sock, u8 *data, s32 size);
void func_088AAE10(s32 *id);
s32 func_088AAED0(u8 *p);
s32 func_088AAF28(s32 sock, u8 *addr, u8 *port, u8 *buf, s32 size);
s32 func_088AAFA4(s32 sock, u8 *addr, u32 port, u8 *data, s32 size);
s32 ns_parse(u8 *s, u8 *data, s32 len, u8 *out);
u16 func_088AD0D4(u32 t);
u16 stub_088ad0f0(u32 t);
void ns_set_state(u8 *s, u32 i, u32 st);
s32 func_088AAC80(s32 sock, u8 *buf, s32 size);

s32 ns_error_send(u8 *s, s32 code, u32 val);
s32 ns_error_recv(u8 *s, s32 code, u32 val);
u32 ns_store(u8 *s, u8 *data, s32 n, u32 kind);
s32 ns_check_links(u8 *s);
s32 ns_send_queue(u8 *s);

void ns_close_all(void)
{
    if (func_088A9CDC() != 0) {
        func_088A9EF4();
        func_088A9EBC();
    }
}

void ns_close(void)
{
    if (func_088A9CDC() != 0)
        func_088A9EF4();
}

u32 ns_value(void)
{
    u32 v[2];

    if (func_088A9CFC() == 0)
        v[0] = 0;
    else
        func_088A9DF4(v);
    return v[0];
}

/* the links: the players before this one connect to it, the ones after wait for it */
void ns_links_init(u8 *s)
{
    NetLink *l = D_09A11EF0;
    s32 i;

    memset(D_09A11EF0, 0, 0x80);
    for (i = 0; i < s[0x22]; i++, l++) {
        memcpy(l->addr, s + 0x24 + i * 0x10, 0x10);
        if (i == s[0x23]) {
            l->state = 6;
        } else if (i < s[0x23]) {
            l->state = 1;
            l->t1 = F32(s, 0x78);
        } else {
            l->state = 4;
        }
        l->t0 = F32(s, 0x78);
        l->sock = -1;
    }
}

/* one step of the links; 1 when they are all done */
s32 ns_links_update(u8 *s)
{
    struct {
        u8 addr[0x10];
        s16 port1;
        s16 port2;
        s32 id;
        u8 res[2];
    } c;
    u8 addr[0x10];
    NetLink *l;
    s32 i, n;

    U16(s, 0xDA) = 1;
    func_088AA9B4(-1, s + 0xD8);
    U16(s, 0xDA) = 0;
    n = 0;
    for (i = 0, l = D_09A11EF0; i < s[0x22]; i++, l++) {
        switch (l->state) {
        case 1:
            if (l->t1 + 1.0f < F32(s, 0x78)) {
                l->state = 2;
                l->t1 = F32(s, 0x78);
            }
            break;
        case 2:
            memcpy(c.addr, l->addr, 0x10);
            c.port1 = s[0x23] + 20000;
            c.port2 = 0;
            c.id = func_088AA370((u8 *)&c);
            if (c.id < 0) {
                l->state = 7;
            } else {
                l->sock = c.id;
                l->state = 3;
            }
            break;
        case 3:
            if (l->t0 + F32(s, 0x134) < F32(s, 0x78)) {
                l->state = 7;
                break;
            }
            c.id = func_088AA520(l->sock);
            if (c.id == 0) {
                l->state = 6;
            } else if (c.id != -1 || !(F32(s, 0x78) < l->t1 + 10.0f)) {
                func_088AAE10(&l->sock);
                l->state = 1;
                l->t1 = F32(s, 0x78);
            }
            break;
        case 4:
            c.port2 = i + 20000;
            c.id = func_088AA67C((u8 *)&c);
            if (c.id < 0) {
                l->state = 7;
            } else {
                l->sock = c.id;
                l->state = 5;
            }
            break;
        case 5:
            if (l->t0 + F32(s, 0x134) < F32(s, 0x78)) {
                l->state = 7;
                break;
            }
            c.id = func_088AA83C(l->sock, addr, c.res);
            if (c.id == -1)
                break;
            if (c.id < -1) {
                l->state = 7;
            } else if (s[0x20] == 2 || memcmp(addr, l->addr, 6) == 0) {
                func_088AAE10(&l->sock);
                l->sock = c.id;
                l->state = 6;
            } else {
                func_088AAE10(&c.id);
            }
            break;
        case 7:
            func_088AAE10(&l->sock);
            l->state = 8;
            break;
        }
        n = s[0x22];
    }
    for (i = 0, l = D_09A11EF0; i < n; i++, l++)
        if (l->state != 0 && l->state != 8 && l->state != 6)
            return 0;
    return 1;
}

/* when the links are done: the sockets of the players and the broadcast socket */
s32 ns_links_finish(u8 *s)
{
    struct {
        u8 data[0x12];
        u16 port;
    } p;
    NetLink *l;
    s32 r;
    u32 i;

    r = ns_links_update(s);
    if (r == 0)
        return r;
    for (i = 0, l = D_09A11EF0; (s32)i < s[0x22]; i++, l++) {
        if (l->state == 8)
            ns_set_state(s, i & 0xFF, 6);
        else if (l->state == 6)
            S32(s, 8 + i * 4) = l->sock;
    }
    p.port = 0x4E21;
    S32(s, 0x18) = func_088AAED0((u8 *)&p);
    return r;
}

/* sends the messages for all to the broadcast address */
s32 ns_send_queue(u8 *s)
{
    NetBuf *q = &D_09A11EE4;
    u8 h[4];
    s32 room = 0xFF - 6, used = 0, others = 0;
    u32 size = 6, i;
    u8 *dst = PKT_DATA;
    u8 st;
    u16 t;

    for (i = 0; (s32)i < s[0x22]; i++) {
        if (i == s[0x23])
            continue;
        st = s[0x68 + i];
        if (st != 6 && st != 5 && st != 0) {
            others = 1;
            break;
        }
    }
    PKT[1] = 3;
    PKT[2] = s[0x23];
    while (room > 0 && used < q->len) {
        memcpy(h, q->ptr + used, 4);
        if (room < h[0])
            break;
        memcpy(dst, q->ptr + used, h[0]);
        room -= h[0];
        size += h[0];
        dst += h[0];
        used += h[0];
    }
    if (size == 6)
        return 0;
    PKT[3] = s[0x114 + s[0x23]];
    t = ++U16(s, 0x118 + s[0x23] * 2);
    U16(PKT, 4) = func_088AD0D4(t);
    PKT[0] = size;
    if (others == 1) {
        func_088AAFA4(S32(s, 0x18), BROADCAST, 0x4E21, PKT, (s16)PKT[0]);
        U32(s, 0xA4) += PKT[0];
    }
    if (used != 0) {
        ns_store(s, q->ptr, used, 3);
        if (q->len - used > 0)
            memmove(q->ptr, q->ptr + used, q->len - used);
        q->len -= used;
    }
    return used;
}

void ns_queue_add(u8 *s, u8 *msg)
{
    NetBuf *q = &D_09A11EE4;

    if (q->cap < q->len + msg[0]) {
        ns_error_send(s, 2, q->len + msg[0]);
    } else {
        memcpy(q->ptr + q->len, msg, msg[0]);
        q->len += msg[0];
    }
}

/* the sequence and the stamp of a packet: -1 if it is old */
s32 ns_check_seq(u8 *s, u8 *hdr)
{
    u32 i = hdr[2];

    if (i >= s[0x22])
        return 0;
    if (s[0x23] == i)
        return 0;
    if (hdr[3] != s[0x114 + i])
        return -1;
    if (U16(s, 0x118 + i * 2) >= stub_088ad0f0(U16(hdr, 4)))
        return -1;
    if (U32(s, 0xBC + i * 4) < 0xFFFEFFFF)
        U32(s, 0xAC + i * 4)++;
    U16(s, 0x118 + hdr[2] * 2) = stub_088ad0f0(U16(hdr, 4));
    return 0;
}

/* receives the packets of the other players and keeps their messages */
s32 ns_receive(u8 *s)
{
    NetBuf *rx = &D_09A11ED8;
    u8 addr[0x10];
    u8 hdr[6];
    u8 port[2];
    s32 r;
    u32 i;

    for (;;) {
        r = func_088AAF28(S32(s, 0x18), addr, port, rx->ptr, (s16)rx->cap);
        if (r < 0)
            return ns_error_recv(s, 3, r);
        if (r == 0)
            return 0;
        rx->len = r;
        U32(s, 0xA8) += r;
        if (s[0x68 + s[0x23]] == 4)
            continue;
        for (i = 0; (s32)i < s[0x22]; i++) {
            if (i == s[0x23])
                continue;
            if (s[0x20] == 2) {
                if (memcmp(s + 0x24 + i * 0x10, addr, 4) == 0)
                    break;
            } else if (s[0x20] == 1) {
                if (memcmp(s + 0x24 + i * 0x10, addr, 6) == 0)
                    break;
            } else {
                break;
            }
        }
        if (s[0x22] == i)
            continue;
        r = ns_parse(s, rx->ptr, rx->len, hdr);
        if (r <= 0 || r != rx->len || ns_check_seq(s, hdr) < 0)
            continue;
        if (D_09A1D590.cap < D_09A1D590.len + rx->len)
            return ns_error_recv(s, 4, D_09A1D590.len + rx->len);
        memcpy(D_09A1D590.ptr + D_09A1D590.len, rx->ptr, rx->len);
        D_09A1D590.len += rx->len;
    }
}

void ns_bufs_init(void)
{
    memset(&D_09A11EE4, 0, 0xC);
    memset(&D_09A11ED8, 0, 0xC);
    D_09A11EE4.cap = 0x2000;
    D_09A11EE4.ptr = (u8 *)0x09A0FED8;
    D_09A11ED8.cap = 0xFF;
    D_09A11ED8.ptr = (u8 *)0x09A0FDD8;
    memset(BROADCAST, 0xFF, 0x10);
}

/* adds the seconds since the last call to *t */
void ns_timer(f32 *t, s64 *last)
{
    s64 now, d;
    s32 sec, usec;

    if (*last == 0) {
        *last = sceKernelGetSystemTimeWide();
        *t = 0.0f;
        return;
    }
    now = sceKernelGetSystemTimeWide();
    d = now - *last;
    if (d != 0) {
        f64 a, b;

        sceKernelSysClock2USecWide(d, &sec, &usec);
        a = (f32)sec;
        b = (f32)usec;
        b = 1e-6 * b;
        *t += (f32)(a + b);
        *last = now;
    }
}

/* the players whose socket is not connected any more are dropped */
s32 ns_check_links(u8 *s)
{
    u32 i;

    U16(s, 0xDA) = 1;
    func_088AA9B4(-1, s + 0xD8);
    U16(s, 0xDA) = 0;
    for (i = 0; (s32)i < s[0x22]; i++) {
        if (S32(s, 8 + i * 4) < 0)
            continue;
        if (func_088AA9B4(S32(s, 8 + i * 4), s + 0xD8) >= 0 && S16(s, 0xD8) != 4)
            ns_set_state(s, i & 0xFF, 6);
    }
    return 1;
}

s32 ns_error_send(u8 *s, s32 code, u32 val)
{
    S32(s, 0x124) = code;
    U32(s, 0x12C) = val;
    return -code;
}

s32 ns_error_recv(u8 *s, s32 code, u32 val)
{
    S32(s, 0x120) = code;
    U32(s, 0x12C) = val;
    D_09A1D590.len = 0;
    D_09A1D578.len = 0;
    return -code;
}

/* keeps the messages sent by this player too, as a packet without sequence */
u32 ns_store(u8 *s, u8 *data, s32 n, u32 kind)
{
    NetBuf *b = &D_09A1D590;
    u8 h[6];

    if (b->cap < b->len + 6 + n)
        return ns_error_send(s, 4, b->len + 6 + n);
    h[0] = n + 6;
    h[1] = kind;
    h[2] = s[0x23];
    h[3] = 0xFF;
    U16(h, 4) = 0xFFFF;
    memcpy(b->ptr + b->len, h, 6);
    memcpy(b->ptr + b->len + 6, data, n);
    b->len += h[0];
    return 0;
}

/* sends the messages of buf to every player (kind 2: they are kept too) */
s32 ns_send(u8 *s, NetBuf *buf, u32 kind)
{
    u8 h[4];
    s32 room, used, busy = 0;
    u32 size, i;
    u8 *dst;
    u8 st;

    ns_send_queue(s);
    ns_check_links(s);
    U16(s, 0xDA) = 0;
    for (i = 0; (s32)i < s[0x22]; i++) {
        if (S32(s, 8 + i * 4) < 0)
            continue;
        if (func_088AA9B4(S32(s, 8 + i * 4), s + 0xD8) >= 0 && S16(s, 0xD8) == 4 &&
            U16(s, 0xDC) < 0x2000) {
            busy = 1;
            break;
        }
    }
    room = 0xFF - 6;
    PKT[1] = kind & 0xF;
    PKT[2] = s[0x23];
    size = 6;
    dst = PKT_DATA;
    if ((F32(s, 0x6C) + 1.0f < F32(s, 0x78) && buf->len > 0) ||
        F32(s, 0x6C) + 2.0f < F32(s, 0x78)) {
        room -= 4;
        for (i = 0; (s32)i < s[0x22]; i++)
            dst[i] = s[0x68 + i];
        size += 4;
        dst += 4;
        PKT[1] |= 0x10;
    }
    used = 0;
    while (room > 0 && used < buf->len) {
        memcpy(h, buf->ptr + used, 4);
        if (room < h[0])
            break;
        if (busy && h[1] == 1) {
            used += h[0];
            continue;
        }
        memcpy(dst, buf->ptr + used, h[0]);
        room -= h[0];
        size += h[0];
        dst += h[0];
        used += h[0];
    }
    if (size == 6)
        return 0;
    PKT[3] = ++s[0x114 + s[0x23]];
    U16(PKT, 4) = func_088AD0D4(0);
    U16(s, 0x118 + s[0x23] * 2) = 0;
    PKT[0] = size;
    for (i = 0; (s32)i < s[0x22]; i++) {
        if (i == s[0x23])
            continue;
        st = s[0x68 + i];
        if (st != 6 && st != 0 &&
            func_088AAD58(S32(s, 8 + i * 4), PKT, (s16)(u8)size) != (size & 0xFF))
            ns_set_state(s, i & 0xFF, 6);
    }
    U32(s, 0x8C) += size & 0xFF;
    F32(s, 0xD0) = F32(s, 0x78);
    if ((PKT[1] & 0x10) != 0)
        F32(s, 0x6C) = F32(s, 0x78);
    if (used != 0) {
        if ((kind & 0xFF) == 2)
            ns_store(s, buf->ptr, used, kind);
        if (buf->len - used > 0)
            memmove(buf->ptr, buf->ptr + used, buf->len - used);
        buf->len -= used;
    }
    return used;
}

/* the state of a player: the callbacks at 0x148 (joined) and 0x150 (left) */
void ns_set_state(u8 *s, u32 i, u32 st)
{
    u32 n = i & 0xFF;
    u8 old = s[0x68 + n];

    s[0x68 + n] = st;
    if (old != 5 && (st & 0xFF) == 5) {
        if (n != s[0x23])
            (*(void (**)(u32, u32))(s + 0x148))(i, U32(s, 0x14C));
    } else if (old != 6 && (st & 0xFF) == 6 && n != s[0x23]) {
        func_088AAE10((s32 *)(s + 8 + n * 4));
        (*(void (**)(u32, u32))(s + 0x150))(i, U32(s, 0x154));
    }
}

/* the players that the other one sees as gone are dropped */
void ns_apply_states(u8 *s, u8 *states)
{
    u32 i;

    for (i = 0; (s32)i < s[0x22]; i++)
        if (s[0x68 + i] != 6 && states[i] == 6)
            ns_set_state(s, i & 0xFF, 6);
}

/* a message: the ones of kind 1 go to all, the others to the buffer */
s32 ns_put(u8 *s, NetBuf *b, u8 *msg)
{
    if (msg[1] == 1) {
        ns_queue_add(s, msg);
        return 0;
    }
    if (b->cap < b->len + msg[0])
        return ns_error_send(s, 2, b->len + msg[0]);
    memcpy(b->ptr + b->len, msg, msg[0]);
    b->len += msg[0];
    return msg[0];
}

static void ctrl_msg(u8 *s, u32 size, u32 code)
{
    PKT[0] = size;
    PKT[2] = code;
    PKT[1] = 0;
    PKT[3] = 0xFF;
    ns_put(s, &D_09A1D56C, PKT);
}

void ns_msg_ready(u8 *s)
{
    ctrl_msg(s, 4, 1);
}

void ns_msg_wait(u8 *s, u32 secs)
{
    PKT[0] = 5;
    PKT[2] = 2;
    PKT[1] = 0;
    PKT[3] = 0xFF;
    PKT[4] = secs;
    ns_put(s, &D_09A1D56C, PKT);
}

void ns_msg_resume(u8 *s)
{
    ctrl_msg(s, 4, 3);
}

void ns_msg_paused(u8 *s)
{
    ctrl_msg(s, 4, 4);
}

/* a control message from the player i */
s32 ns_control(u8 *s, u32 i, u8 *msg)
{
    u32 n;

    switch (msg[2]) {
    case 4:
        if (s[0x68 + (i & 0xFF)] == 3)
            ns_set_state(s, i, 5);
        break;
    case 3:
        n = i & 0xFF;
        if (s[0x68 + n] == 4) {
            ns_set_state(s, i, 3);
            U32(s, 0xF0 + n * 4) = 0;
            s[0x138 + n] = 0;
        }
        break;
    case 2:
        n = i & 0xFF;
        if (s[0x68 + n] == 3) {
            ns_set_state(s, i, 4);
            F32(s, 0xF0 + n * 4) = F32(s, 0x78);
            s[0x138 + n] = msg[4];
        }
        break;
    case 1:
        if (s[0x68 + s[0x23]] == 1 && s[0x68 + (i & 0xFF)] == 1) {
            ns_set_state(s, i, 2);
            ns_msg_ready(s);
            U32(s, 0x64) |= 1 << (i & 0xFF);
        }
        break;
    default:
        return 0;
    }
    F32(s, 0xE0 + (i & 0xFF) * 4) = F32(s, 0x78);
    return 0;
}

/* a message of the game: the callback at 0x140 */
s32 ns_game_msg(u8 *s, u32 i, u8 *msg)
{
    return (*(s32 (**)(u32, u8 *, u32))(s + 0x140))(i, msg, U32(s, 0x144));
}

/* a packet was received from the player i */
u32 ns_seen(u8 *s, u32 i, u32 flags, u8 *data)
{
    u32 st;

    i &= 0xFF;
    if (i < s[0x22]) {
        st = s[0x68 + i];
        if (st != 6 && st != 0 && (flags & 0x10) != 0) {
            if (st - 3 < 2)
                ns_apply_states(s, data);
            F32(s, 0xE0 + i * 4) = F32(s, 0x78);
        }
    }
    return 0;
}

/* the messages of a packet of the player i */
s32 ns_dispatch(u8 *s, u32 i, u32 flags, u8 *data, u32 size)
{
    u8 h[4];
    s32 off = 0, r;
    u32 kind;
    u8 st;

    if ((i & 0xFF) >= s[0x22])
        return 0;
    st = s[0x68 + (i & 0xFF)];
    if (st == 6 || st == 5 || st == 0)
        return 0;
    flags &= 0xFF;
    if ((flags & 0xF0) != 0) {
        if ((size & 0xFF) == 4)
            return 0;
        size -= 4;
        data += 4;
    }
    size &= 0xFF;
    if ((s32)size <= 0)
        return 0;
    kind = flags & 0xF;
    while ((s32)size - off >= 4) {
        memcpy(h, data + off, 4);
        if (h[0] < 4)
            return ns_error_recv(s, 5, h[0]);
        memcpy(PKT, data + off, h[0]);
        if (kind == 3 || kind == 2) {
            r = ns_game_msg(s, i, PKT);
            if (r < 0)
                return ns_error_recv(s, 0xC, r);
        } else if (kind == 1) {
            r = ns_control(s, i, PKT);
            if (r < 0)
                return ns_error_recv(s, 0xC, r);
        } else if (kind != 0) {
            return ns_error_recv(s, 5, flags);
        }
        off += h[0];
        if (off >= (s32)size)
            break;
    }
    return 0;
}

/* the sequence of a stream packet */
u32 ns_stream_seq(u8 *s, u8 *hdr)
{
    u32 i = hdr[2];

    if (i < s[0x22] && s[0x23] != i) {
        if (s[0x110 + i] == 0)
            s[0x110 + i] = hdr[3];
        else
            s[0x114 + i]++;
        s[0x114 + hdr[2]] = hdr[3];
        if (U32(s, 0xBC + hdr[2] * 4) < 0xFFFEFFFF)
            U32(s, 0xBC + hdr[2] * 4) += U16(s, 0x118 + hdr[2] * 2);
        U16(s, 0x118 + hdr[2] * 2) = stub_088ad0f0(U16(hdr, 4));
    }
    return 0;
}

/* the size of the packet at data: 0 if it is not complete, -6 if it is empty */
s32 ns_parse(u8 *s, u8 *data, s32 len, u8 *out)
{
    u8 h[6];
    s32 body;

    if (len <= 0 || len < 6)
        return 0;
    memcpy(h, data, 6);
    len -= 6;
    if (h[0] < 6) {
        ns_error_recv(s, 5, h[0]);
        return 0;
    }
    body = h[0] - 6;
    if (body == 0)
        return -6;
    if (len <= 0 || len < body)
        return 0;
    if (out != NULL)
        memcpy(out, h, 6);
    return h[0];
}

/* the messages of the packets in b */
s32 ns_dispatch_all(u8 *s, NetBuf *b)
{
    u8 h[6];
    s32 off = 0, n;

    for (;;) {
        n = ns_parse(s, b->ptr + off, b->len - off, h);
        if (n < 0) {
            off -= n;
            continue;
        }
        if (n == 0)
            break;
        if (ns_dispatch(s, h[2], h[1], b->ptr + off + 6, (n - 6) & 0xFF) < 0)
            return -1;
        off += n;
    }
    b->len -= off;
    if (b->len > 0)
        memmove(b->ptr, b->ptr + off, b->len);
    return 0;
}

/* receives: the broadcast packets and the stream of each player */
s32 ns_update_recv(u8 *s)
{
    NetBuf *in = D_09A1D59C, *b;
    u8 h[6];
    s32 n, off, r;
    u32 i;
    u8 st;

    ns_receive(s);
    ns_check_links(s);
    for (i = 0; (s32)i < s[0x22]; i++, in++) {
        if (i == s[0x23])
            continue;
        st = s[0x68 + i];
        if (st == 6 || st == 0)
            continue;
        n = 0xFF;
        if (in->cap < in->len + 0xFF)
            n = in->cap - in->len;
        if (n == 0)
            continue;
        n = func_088AAC80(S32(s, 8 + i * 4), in->ptr + in->len, (s16)n);
        if (n < 0) {
            ns_set_state(s, i & 0xFF, 6);
            continue;
        }
        if (n == 0)
            continue;
        off = 0;
        in->len += n;
        U32(s, 0x90) += n;
        F32(s, 0xE0 + s[0x23] * 4) = F32(s, 0x78);
        for (;;) {
            n = ns_parse(s, in->ptr + off, in->len - off, h);
            if (n < 0) {
                off -= n;
                continue;
            }
            if (n == 0)
                break;
            if ((s32)ns_stream_seq(s, h) < 0) {
                off += n;
                continue;
            }
            b = (h[1] & 0xF) == 2 ? &D_09A1D590 : &D_09A1D578;
            if (b->cap < b->len + n)
                return ns_error_recv(s, 4, b->len + n);
            memcpy(b->ptr + b->len, in->ptr + off, n);
            ns_seen(s, h[2], h[1], b->ptr + b->len + 6);
            off += n;
            b->len += n;
        }
        r = in->len - off;
        in->len = r;
        if (r > 0)
            memmove(in->ptr, in->ptr + off, r);
    }
    return 0;
}

/* once a second: the players whose stream stays full are dropped; the byte counters */
s32 ns_update_second(u8 *s)
{
    u16 avail, cnt;
    u32 i;

    if (ns_check_links(s) <= 0)
        return 0;
    U16(s, 0xDA) = 0;
    for (i = 0; (s32)i < s[0x22]; i++) {
        if (S32(s, 8 + i * 4) < 0)
            continue;
        if (func_088AA9B4(S32(s, 8 + i * 4), s + 0xD8) < 0 || S16(s, 0xD8) != 4)
            continue;
        avail = U16(s, 0xDC);
        cnt = U16(s, 0x108 + i * 2);
        if (avail >= 0x1FFA)
            cnt = 0;
        else if ((s32)avail <= (s32)(U16(s, 0x100 + i * 2) - U32(s, 0x8C))) {
            if (cnt != 0xFFFF)
                cnt++;
        } else if (cnt != 0) {
            cnt--;
        }
        U16(s, 0x100 + i * 2) = avail;
        U16(s, 0x108 + i * 2) = cnt;
        if (S32(s, 0x13C) <= cnt)
            ns_set_state(s, i & 0xFF, 6);
    }
    U32(s, 0x9C) += U32(s, 0x8C);
    U32(s, 0xA0) += U32(s, 0x90);
    U32(s, 0x94) = U32(s, 0x8C);
    U32(s, 0x98) = U32(s, 0x90);
    U32(s, 0x8C) = 0;
    U32(s, 0x90) = 0;
    return 0;
}

u32 ns_clear_counters(u8 *s)
{
    U32(s, 0x9C) = 0;
    U32(s, 0xA0) = 0;
    U32(s, 0xA4) = 0;
    U32(s, 0xA8) = 0;
    return 0;
}

/* the players silent for too long are dropped */
u32 ns_check_timeouts(u8 *s)
{
    u32 i;
    u8 st;

    for (i = 0; (s32)i < s[0x22]; i++) {
        if (i == s[0x23])
            continue;
        st = s[0x68 + i];
        if (st == 4) {
            if (F32(s, 0xF0 + i * 4) + (f32)s[0x138 + i] <= F32(s, 0x78))
                ns_set_state(s, i & 0xFF, 6);
        } else if (st != 3 && st != 2 && st != 1) {
            continue;
        }
        if (F32(s, 0xE0 + i * 4) + F32(s, 0x130) <= F32(s, 0x78))
            ns_set_state(s, i & 0xFF, 6);
    }
    return 0;
}

void ns_reset_joined(u8 *s)
{
    u32 i;

    U32(s, 0x64) = 0;
    for (i = 0; (s32)i < s[0x22]; i++)
        if (s[0x68 + i] == 3)
            ns_set_state(s, i & 0xFF, 1);
}

/* ---- the session ---- */

#define NS D_09A1D5D0
#define OWN_STATE (NS[0x68 + D_09A1D5F3])

/* the start: every player says when it is ready; 1 when the start is over */
s32 ns_join_step(u8 *s)
{
    u32 i;
    u8 st;

    if (s[0x22] == 1) {
        ns_set_state(s, s[0x23], 3);
        memset(s, 0, 8);
        return 1;
    }
    if (F32(s, 0x7C) + (f32)S16(s, 0) < F32(s, 0x78)) {
        S16(s, 0)++;
        ns_update_second(s);
        ns_msg_ready(s);
    }
    S16(s, 2) = 0;
    for (i = 0; (s32)i < s[0x22]; i++) {
        if (i == s[0x23])
            continue;
        st = s[0x68 + i];
        if ((st == 2 || st == 1) && (U32(s, 0x64) & (1 << i)) == 0)
            S16(s, 2)++;
    }
    if (S16(s, 2) == 0)
        ns_set_state(s, s[0x23], 2);
    S16(s, 2) = 0;
    S16(s, 4) = 0;
    for (i = 0; (s32)i < s[0x22]; i++) {
        st = s[0x68 + i];
        if (st == 1) {
            S16(s, 2)++;
        } else if (st == 2) {
            S16(s, 4)++;
            S16(s, 2)++;
        }
    }
    if (S16(s, 2) == S16(s, 4)) {
        for (i = 0; (s32)i < s[0x22]; i++)
            if (s[0x68 + i] == 2)
                ns_set_state(s, i & 0xFF, 3);
        memset(s, 0, 8);
        return 1;
    }
    if (!(F32(s, 0x7C) + F32(s, 0x134) < F32(s, 0x78)))
        return 0;
    for (i = 0; (s32)i < s[0x22]; i++) {
        if (i == s[0x23]) {
            ns_set_state(s, i & 0xFF, 3);
            continue;
        }
        st = s[0x68 + i];
        if (st == 2)
            ns_set_state(s, i & 0xFF, 3);
        else if (st == 1)
            ns_set_state(s, i & 0xFF, 6);
    }
    memset(s, 0, 8);
    return 1;
}

s32 ns_error(void);

static void exchange(u8 *s)
{
    ns_send(s, &D_09A1D56C, 1);
    ns_dispatch_all(s, &D_09A1D578);
}

/* one frame of the session */
void ns_update(u8 *s)
{
    ns_timer((f32 *)(s + 0x78), (s64 *)(s + 0x70));
    switch (s[0x21]) {
    case 0:
        s[0x21] = 1;
        break;
    case 1:
        s[0x21] = 2;
        ns_links_init(s);
        break;
    case 2:
        if (ns_links_finish(s) == 0)
            break;
        s[0x21] = 3;
        F32(s, 0x7C) = F32(s, 0x78);
        for (S16(s, 0) = 0; S16(s, 0) < s[0x22]; S16(s, 0)++)
            F32(s, 0xE0 + S16(s, 0) * 4) = F32(s, 0x78);
        memset(s, 0, 8);
        break;
    case 3:
        if (ns_error() != 0) {
            s[0x21] = 7;
            break;
        }
        ns_update_recv(s);
        if (ns_error() != 0) {
            s[0x21] = 7;
            break;
        }
        if (ns_join_step(s) != 0) {
            s[0x21] = 4;
            F32(s, 0x7C) = F32(s, 0x78);
            F32(s, 0x80) = F32(s, 0x78);
        }
        exchange(s);
        break;
    case 4:
        if (ns_error() != 0) {
            s[0x21] = 7;
            break;
        }
        ns_update_recv(s);
        ns_check_timeouts(s);
        if (ns_error() != 0) {
            s[0x21] = 7;
            break;
        }
        if (F32(s, 0x7C) + 1.0f <= F32(s, 0x78)) {
            F32(s, 0x7C) = F32(s, 0x78);
            ns_update_second(s);
        }
        if (F32(s, 0x80) + 60.0f <= F32(s, 0x78)) {
            F32(s, 0x80) = F32(s, 0x78);
            ns_clear_counters(s);
        }
        exchange(s);
        break;
    case 5:
        ns_reset_joined(s);
        s[0x21] = 3;
        break;
    case 6:
        if (ns_error() == 0) {
            ns_update_recv(s);
            exchange(s);
        }
        break;
    }
}

/* opens the session: p = {mode, players, own index, addresses (+3, 0x10 each), 0x44
 * send interval, 0x48 bytes per second, 0x4C silence timeout, 0x50 full stream seconds,
 * 0x54 timeout, 0x58.. the callbacks and their arguments} */
void ns_open(u8 *p)
{
    u32 n = p[1], i;

    if (n > 4)
        n = 4;
    memset(NS, 0, 0x158);
    memset(D_09A1D59C, 0, 0x30);
    memset(&D_09A1D590, 0, 0xC);
    memset((u8 *)0x09A1D584, 0, 0xC);
    memset(&D_09A1D578, 0, 0xC);
    memset(&D_09A1D56C, 0, 0xC);
    D_09A1D5EC = 1;
    for (i = 0; i < n; i++)
        ns_set_state(NS, i & 0xFF, 1);
    for (i = 0; i < n; i++) {
        D_09A1D59C[i].cap = 0xFF;
        D_09A1D59C[i].ptr = (u8 *)0x09A1D170 + i * 0xFF;
    }
    D_09A1D590.cap = 0x8000;
    D_09A1D590.ptr = (u8 *)0x09A15170;
    ((NetBuf *)0x09A1D584)->cap = 0x2000;
    ((NetBuf *)0x09A1D584)->ptr = (u8 *)0x09A13170;
    D_09A1D578.cap = 0x800;
    D_09A1D578.ptr = (u8 *)0x09A12970;
    D_09A1D56C.cap = 0x800;
    D_09A1D56C.ptr = (u8 *)0x09A12170;
    for (i = 0; i < n; i++) {
        S32(NS, 8 + i * 4) = -1;
        NS[0x114 + i] = rand() | 0x80;
        U16(NS, 0x118 + i * 2) = 0;
    }
    NS[0x20] = p[0];
    D_09A1D5F2 = n;
    D_09A1D5F3 = p[2];
    for (i = 0; i < n; i++)
        memcpy(NS + 0x24 + i * 0x10, p + 3 + i * 0x10, 0x10);
    U32(NS, 0xCC) = U32(p, 0x44);
    U32(NS, 0xD4) = U32(p, 0x48);
    U32(NS, 0x130) = U32(p, 0x4C);
    U32(NS, 0x134) = U32(p, 0x54);
    S32(NS, 0x13C) = (s32)F32(p, 0x50);
    U32(NS, 0x140) = U32(p, 0x58);
    U32(NS, 0x144) = U32(p, 0x5C);
    U32(NS, 0x148) = U32(p, 0x60);
    U32(NS, 0x14C) = U32(p, 0x64);
    U32(NS, 0x150) = U32(p, 0x68);
    U32(NS, 0x154) = U32(p, 0x6C);
    ns_bufs_init();
}

u32 ns_active(void)
{
    return D_09A1D5EC;
}

s32 ns_running(void)
{
    return D_09A1D5F1 == 4;
}

/* the error of the session, 7 if this player was dropped */
s32 ns_error(void)
{
    if (S32(NS, 0x120) != 0)
        return S32(NS, 0x120);
    if (S32(NS, 0x124) != 0)
        return S32(NS, 0x124);
    if (S32(NS, 0x128) != 0)
        return S32(NS, 0x128);
    if (OWN_STATE == 6)
        return 7;
    return 0;
}

/* why the game data cannot be sent now (0: it can) */
u32 ns_send_blocked(void)
{
    if (OWN_STATE != 3 && OWN_STATE != 4)
        return 1;
    if (!(S32(NS, 0x8C) < S32(NS, 0xD4)))
        return 2;
    if (F32(NS, 0x78) < F32(NS, 0xD0) + F32(NS, 0xCC))
        return 3;
    return 0;
}

void ns_error_info(u32 *out)
{
    out[0] = ns_error();
    out[1] = U32(NS, 0x12C);
}

void ns_add_msg(u8 *msg)
{
    if (D_09A1D5EC != 0)
        ns_put(NS, (NetBuf *)0x09A1D584, msg);
}

void ns_flush(void)
{
    if (D_09A1D5EC != 0 && ns_error() == 0 && ns_send_blocked() == 0)
        ns_send(NS, (NetBuf *)0x09A1D584, 2);
}

void ns_read(void)
{
    if (D_09A1D5EC != 0)
        ns_dispatch_all(NS, &D_09A1D590);
}

void ns_frame(void)
{
    if (D_09A1D5EC != 0 && (NS[0x20] == 2 || NS[0x20] == 1))
        ns_update(NS);
}

/* this player waits (0: the timeout of the session) */
void ns_wait(u32 secs)
{
    if (OWN_STATE != 3)
        return;
    if ((secs & 0xFF) == 0)
        secs = (u32)F32(NS, 0x134) & 0xFF;
    ns_msg_wait(NS, secs);
    ns_set_state(NS, D_09A1D5F3, 4);
}

void ns_resume(void)
{
    if (OWN_STATE == 4) {
        ns_msg_resume(NS);
        ns_set_state(NS, D_09A1D5F3, 3);
    }
}

void ns_leave(void)
{
    if (OWN_STATE == 3) {
        ns_msg_paused(NS);
        ns_set_state(NS, D_09A1D5F3, 5);
        D_09A1D5F1 = 6;
    }
}

void ns_shutdown(void)
{
    u32 i;

    D_09A1D5F1 = 8;
    D_09A1D5EC = 0;
    for (i = 0; (s32)i < D_09A1D5F2; i++) {
        ns_set_state(NS, i & 0xFF, 0);
        func_088AAE10((s32 *)(NS + 8 + i * 4));
    }
    func_088AB028((s32 *)(NS + 0x18));
}

/* the error codes of the PDP library to the ones of the session */
s32 ns_errcode(s32 r)
{
    if (r >= 0)
        return r;
    switch ((u32)r) {
    case 0x8001006D:
        return -0xD;
    case 0x80010072:
    case 0x80010076:
    case 0x80010075:
        return -7;
    case 0x80410719:
    case 0x80010004:
        return -0xE;
    case 0x80400706:
    case 0x8001000C:
    case 0x80010069:
        return -0xC;
    case 0x80410715:
    case 0x80010074:
        return -0xB;
    case 0x8041070C:
    case 0x8041070B:
    case 0x80410718:
    case 0x80010068:
    case 0x8001006F:
        return -6;
    case 0x80410701:
    case 0x80410707:
    case 0x80010009:
        return -4;
    case 0x8041070F:
    case 0x80410710:
        return -2;
    case 0x80410709:
    case 0x8001000B:
    case 0x80010078:
    case 0x80010077:
        return -1;
    }
    return -0x10;
}

extern s32 D_09A1D728;
extern u8 D_09A1D8AC[0x240], D_09A1D72C[0x180];

s32 pdp_init(s32 mode)
{
    D_09A1D728 = mode == 0 ? 1 : 2;
    memset(D_09A1D8AC, 0xFF, 0x240);
    memset(D_09A1D72C, 0xFF, 0x180);
    return D_09A1D728;
}

u32 pdp_nop(void)
{
    return 0;
}
