/* The memory of the game g_heap: a heap with a table of 512 blocks known by an id
 * {id (-1: free), address, size} at +0x83C030. Plus the start of the network object.
 * (Guessed names.) */
#include "common.h"

#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define ENT(self, i) ((u8 *)(self) + (i) * 0xC + 0x83C030)

extern const char D_089B16C0[];   /* "ULES01213" */
extern const char D_089B16CC[];   /* "MHP2Q0" */
extern const char D_089B16D4[];   /* "MHP2A0" */

void heap_init(void *a, void *b, int n);
void *heap_alloc(void *h, u32 size, u32 align);
void heap_free(void *h, void *p);
void *memset(void *p, int c, u32 n);
char *strcpy(char *d, const char *s);
s32 temp_find_free(u8 *self);
void netreq_clear(u8 *p);
void netmsg_clear(u8 *p);
void lobbyapi_term(u8 *self);
void lobbyapi_init(char *name);
void lobbyapi_scan(void *cb, u8 *self);
u8 *net_my_mac(u8 *self);

void temp_init(u8 *self, u32 size)
{
    s32 i;

    heap_init(self, self + 0x30, size);
    for (i = 0; i < 0x200; i++) {
        U32(ENT(self, i), 0) = -1;
        U32(ENT(self, i), 4) = 0;
    }
}

void *temp_alloc(u8 *self, u32 id, u32 size)
{
    s32 i = temp_find_free(self);
    u8 *e;
    void *p;

    if (i == -1)
        return NULL;
    e = ENT(self, i);
    p = heap_alloc(self, size, 0x10);
    U32(e, 4) = (u32)p;
    if (p == NULL)
        return NULL;
    U32(e, 8) = size;
    U32(e, 0) = id;
    return (void *)U32(e, 4);
}

void temp_free(u8 *self, s32 id)
{
    s32 i;

    for (i = 0; i < 0x200; i++) {
        if ((s32)U32(ENT(self, i), 0) == id) {
            heap_free(self, (void *)U32(ENT(self, i), 4));
            U32(ENT(self, i), 0) = -1;
        }
    }
}

void temp_free_ptr(u8 *self, u32 p)
{
    s32 i;

    for (i = 0; i < 0x200; i++) {
        if (U32(ENT(self, i), 4) == p) {
            heap_free(self, (void *)U32(ENT(self, i), 4));
            U32(ENT(self, i), 0) = -1;
            return;
        }
    }
}

s32 temp_find_free(u8 *self)
{
    s32 i;

    for (i = 0; i < 0x200; i++)
        if ((s32)U32(ENT(self, i), 0) == -1)
            return i;
    return -1;
}

u32 temp_get(u8 *self, s32 id)
{
    s32 i;

    for (i = 0; i < 0x200; i++)
        if ((s32)U32(ENT(self, i), 0) == id)
            return U32(ENT(self, i), 4);
    return 0;
}

/* ---- network object ---- */

void net_init_state(u8 *self)
{
    U32(self, 0x12C0) = 0;
    self[0x12D5] = 0;
    self[0x12D6] = 0;
    self[0x1AFC] = 0;
    U32(self, 0x12C4) = 0;
    self[0x146C] = 0;
    self[0x146D] = 0xFF;
    self[0x146E] = 0;
    netreq_clear(self + 0x1B00);
    netmsg_clear(self + 0x1B28);
    netmsg_clear(self + 0x1B48);
}

void net_reset(u8 *self)
{
    lobbyapi_term(self);
    self[4] = 0;
    self[0x1AFC] = 0;
    self[0x12D5] = 0;
    U32(self, 0x12C4) = 0;
    U32(self, 0x12C0) = 0;
}

/* the 8 bytes of the address of the console (from net_my_mac) */
void net_get_mac(u8 *self, u8 *out)
{
    u8 *m;
    s32 i;

    memset(out, 0, 0x20);
    m = net_my_mac(self);
    for (i = 0; i < 3; i++) {
        out[i * 2] = m[i * 2];
        out[i * 2 + 1] = m[i * 2 + 1];
    }
    out[6] = m[6];
    out[7] = m[7];
}

void *memcpy(void *d, const void *s, u32 n);
void netreq_start(u8 *p, u8 *mac, u32 a);
void func_088B3A68(u8 *peer, u8 *data, void *cb, u8 *self);
void func_088B3DE4(u8 *peer, void *cb, u8 *self);
void func_088B3684(u8 *n, u8 *list);
void lobbyapi_players(u8 *n, u8 *list);
u8 *net_room_find(u8 *self);
void net_set_state(u8 *self, s32 a);
void net_send_entry(u8 *self);
void net_sort_players(u8 *self);
void net_sort_rooms(u8 *self);
void net_join(u8 *self, s8 i, const u8 *data);
u8 *net_select(u8 *self, u32 i);
void net_select_mac(u8 *self, const u8 *mac);

/* compares 6 bytes: -1, 0, 1 */
static s32 mac_cmp(const u8 *a, const u8 *b)
{
    s32 i;

    for (i = 0; i < 6; i++) {
        if (a[i] < b[i])
            return -1;
        if (b[i] < a[i])
            return 1;
    }
    return 0;
}

/* joins the room of the console mac among the ones found (100 bytes each from 0x12DC) */
u32 net_join_by_mac(u8 *self, const u8 *mac, const u8 *data)
{
    s32 i;

    for (i = 0; i < (s8)self[0x12D8]; i++) {
        if (mac_cmp(mac, self + i * 100 + 0x12DC) == 0) {
            net_join(self, (s8)i, data);
            return 1;
        }
    }
    return 0;
}

u8 *net_connect_index(u8 *self, u32 i, u32 a)
{
    net_select(self, i);
    netreq_start(self + 0x1B00, self + 0x12C8, a);
    return self + 0x12C8;
}

void net_join(u8 *self, s8 i, const u8 *data)
{
    U32(self, 0x12C0) |= 0x100;
    if (data == NULL)
        memset(self + 0x12A8, 0, 0x12);
    else
        memcpy(self + 0x12A8, data, 0x12);
    *(u16 *)(self + 0x12BA) = 0x12;
    func_088B3A68(self + i * 100 + 0x12DC, self + 0x12A8, (void *)0x0889A700, self);
}

static void copy_mac(u8 *d, const u8 *s)
{
    s32 i;

    for (i = 0; i < 3; i++) {
        d[i * 2] = s[i * 2];
        d[i * 2 + 1] = s[i * 2 + 1];
    }
}

u8 *net_select(u8 *self, u32 i)
{
    u8 *p;

    U32(self, 0x12C0) |= 0x200;
    p = self + (i & 0xFF) * 100 + 0x12DC;
    func_088B3DE4(p, (void *)0x0889A828, self);
    copy_mac(self + 0x12C8, p);
    self[0x12CE] = self[(i & 0xFF) * 100 + 0x12E2];
    self[0x12CF] = self[(i & 0xFF) * 100 + 0x12E3];
    return self + 0x12C8;
}

void net_select_mac(u8 *self, const u8 *mac)
{
    U32(self, 0x12C0) |= 0x200;
    func_088B3DE4(net_room_find(self), (void *)0x0889A828, self);
    copy_mac(self + 0x12C8, mac);
    self[0x12CE] = mac[6];
    self[0x12CF] = mac[7];
}

void net_connect_mac(u8 *self, const u8 *mac, u32 a)
{
    net_select_mac(self, mac);
    netreq_start(self + 0x1B00, (u8 *)mac, a);
}

/* ---- callbacks of the ad hoc library: event {.., net, result, error} ---- */

static u8 *net_event(const u8 *ev, u32 clear)
{
    u8 *n = (u8 *)U32(ev, 8);

    U32(n, 0x12C0) &= ~clear;
    n[0x1AF4] = (s8)ev[0xC] == 0;
    U32(n, 0x1AF8) = *(u16 *)(ev + 0x10);
    return n;
}

static void net_event_error(u8 *n)
{
    if (U32(n, 0x1AF8) == 0x8003)
        n[0x1AFC] = 1;
}

void net_cb_create(const u8 *ev)
{
    u8 *n = net_event(ev, 2);

    if (n[0x1AF4] == 0) {
        net_event_error(n);
        return;
    }
    net_set_state(n, 1);
    net_send_entry(n);
}

void net_cb_connect(const u8 *ev)
{
    u8 *n = net_event(ev, 4);

    if (n[0x1AF4] == 0) {
        net_event_error(n);
        return;
    }
    net_set_state(n, 0);
    net_send_entry(n);
}

void net_cb_leave(const u8 *ev)
{
    u8 *n = net_event(ev, 8);

    if (n[0x1AF4] == 0) {
        n[0x1AFC] = 1;
        return;
    }
    net_set_state(n, 2);
    net_send_entry(n);
}

void net_cb_join(const u8 *ev)
{
    u8 *n = net_event(ev, 0x10);

    if (n[0x1AF4] == 0) {
        net_event_error(n);
        return;
    }
    net_set_state(n, 1);
    net_send_entry(n);
}

/* list of the players (200 bytes each from 0x57C): our entry gets our data */
void net_cb_players(const u8 *ev)
{
    u8 *n = net_event(ev, 0x20);
    u8 *me, *e = NULL;
    s32 i;

    if (n[0x1AF4] == 0) {
        net_event_error(n);
        return;
    }
    memset(n + 0x57C, 0, 800);
    lobbyapi_players(n + 0xEE3, n + 0x57C);
    net_sort_players(n);
    me = net_my_mac(n);
    for (i = 0; i < (s8)n[0xEE3]; i++) {
        if (mac_cmp(me, n + i * 200 + 0x57C) == 0) {
            e = n + i * 200 + 0x57C;
            break;
        }
    }
    if (e != NULL)
        memcpy(e + 0x20, n + 0x38, 0xA4);
    n[0x12D4] = 1;
}

void net_cb_scan(const u8 *ev)
{
    u8 *n = net_event(ev, 0x40);

    n[0x12D8] = 0;
    memset(n + 0x12DC, 0, 400);
    if (n[0x1AF4] == 0) {
        net_event_error(n);
        return;
    }
    func_088B3684(n + 0x12D8, n + 0x12DC);
    net_sort_rooms(n);
}

void net_set_host(u8 *self, s32 a);
void func_088B3A3C(u8 *info);
void net_set_my_group(u8 *self, u8 *info);
void net_room_add(u8 *self, u8 *info);
void net_set_my_b3(u8 *self, u32 a);
void func_088B3F60(u8 *n, u8 *list);
void net_sort_room_list(u8 *self);
void net_room_remove(u8 *self, u8 *mac);

static void net_event_error3(u8 *n)
{
    s32 e = U32(n, 0x1AF8);

    if (e == 0x8003 || e == 0x8007 || e == 0x8001)
        n[0x1AFC] = 1;
}

/* the room was made (host) */
void net_cb_host(const u8 *ev)
{
    u8 a[0x64], b[0x64];
    u8 *n = net_event(ev, 0x80);

    if (n[0x1AF4] == 0) {
        net_event_error(n);
        return;
    }
    net_set_state(n, 3);
    net_set_host(n, 1);
    func_088B3A3C(b);
    net_set_my_group(n, b);
    func_088B3A3C(a);
    net_room_add(n, a);
    net_set_my_b3(n, a[0x5D]);
    net_send_entry(n);
}

/* joined the room */
void net_cb_joined(const u8 *ev)
{
    u8 a[0x64];
    u8 *n = net_event(ev, 0x100);

    if (U32(n, 0x1AF8) == 0x8006 || U32(n, 0x1AF8) == 0x8031)
        U32(n, 0x1AF8) = 0x8014;
    if (U32(n, 0x1AF8) == 0x8007)
        U32(n, 0x1AF8) = 0x8001;
    if (n[0x1AF4] == 0) {
        if (U32(n, 0x1AF8) == 0x8003)
            n[0x1AFC] = 1;
        else if (U32(n, 0x1AF8) == 0x8014)
            U32(n, 0x12C4) &= ~0x200;
        return;
    }
    net_set_state(n, 3);
    net_set_host(n, 0);
    func_088B3A3C(a);
    net_set_my_group(n, a);
    net_set_my_b3(n, a[0x5D]);
    net_send_entry(n);
}

/* list of the rooms */
void net_cb_rooms(const u8 *ev)
{
    u8 *n = net_event(ev, 0x200);

    if (U32(n, 0x1AF8) == 0x8006 || U32(n, 0x1AF8) == 0x8000)
        U32(n, 0x1AF8) = 0x8014;
    if (U32(n, 0x1AF8) == 0x8007)
        U32(n, 0x1AF8) = 0x8001;
    if (n[0x1AF4] == 0) {
        net_event_error(n);
        return;
    }
    memset(n + 0x25C, 0, 800);
    func_088B3F60(n + 0xEE1, n + 0x25C);
    net_sort_room_list(n);
}

void net_cb_left(const u8 *ev)
{
    u8 *n = net_event(ev, 0x400);

    if (n[0x1AF4] == 0) {
        net_event_error3(n);
        return;
    }
    net_set_state(n, 2);
    if (n[0x12D6] != 0)
        net_room_remove(n, net_my_mac(n));
    net_set_host(n, 0);
    net_send_entry(n);
}

void net_cb_closed(const u8 *ev)
{
    u8 *n = net_event(ev, 0x800);

    if (n[0x1AF4] == 0) {
        net_event_error3(n);
        return;
    }
    net_set_state(n, 4);
    net_send_entry(n);
}

void net_cb_info(const u8 *ev)
{
    u8 a[0x64];
    u8 *n = net_event(ev, 0x1000);

    if (n[0x1AF4] == 0) {
        net_event_error3(n);
        return;
    }
    net_set_state(n, 3);
    func_088B3A3C(a);
    net_set_my_group(n, a);
    net_send_entry(n);
}

void net_cb_end(const u8 *ev)
{
    u8 *n = net_event(ev, 0x2000);

    if (U32(n, 0x1AF8) == 0x8003)
        n[0x1AFC] = 1;
}

void lobbyapi_host_entry(u8 *p);
void net_sort_members(u8 *self);
void lobbyapi_slot(u8 *p);
u8 net_all_members_known(u8 *n);

/* the player of the room with the console mac (200 bytes each from 0x57C), or NULL */
static u8 *net_find_player(u8 *n, const u8 *mac)
{
    s32 i;

    for (i = 0; i < (s8)n[0xEE3]; i++)
        if (mac_cmp(mac, n + i * 200 + 0x57C) == 0)
            return n + i * 200 + 0x57C;
    return NULL;
}

/* the members of the game session: they get the data of the players of the room */
void net_cb_members(const u8 *ev)
{
    u8 *n = net_event(ev, 0x4000);
    u8 *e, *p;
    u32 i, cnt;

    if (n[0x1AF4] == 0) {
        n[0x1AFC] = 1;
        return;
    }
    if (n[0x12D5] != 4)
        return;
    memset(n + 0xBBC, 0, 0x324);
    lobbyapi_host_entry(n + 0xBBC);
    net_sort_members(n);
    net_set_state(n, 5);
    net_send_entry(n);
    cnt = n[0xBBD];
    e = n + 0xBC0;
    for (i = 0; i < cnt; i++, e += 200) {
        p = net_find_player(n, e);
        if (p != NULL)
            p += 0x20;
        if (p != NULL)
            memcpy(e + 0x20, p, 0xA4);
    }
    if (net_all_members_known(n) == 0)
        n[0x1AFC] = 1;
}

/* every member of the session is one of the players of the room */
u8 net_all_members_known(u8 *n)
{
    u32 cnt = n[0xBBD], i, found = 0;
    u8 *e = n + 0xBC0;
    s32 j;

    for (i = 0; i < cnt; i++, e += 200) {
        for (j = 0; j < (s8)n[0xEE3]; j++) {
            if (mac_cmp(e, n + (j & 0xFF) * 200 + 0x57C) == 0) {
                found++;
                break;
            }
        }
    }
    return cnt == found;
}

void net_cb_flag1(const u8 *ev)
{
    U32((u8 *)U32(ev, 8), 0x12C4) |= 1;
}

void net_cb_flag2(const u8 *ev)
{
    u8 *n = (u8 *)U32(ev, 8);

    U32(n, 0x12C4) |= 2;
    lobbyapi_slot(n + 0x146C);
}

void net_cb_flag10(const u8 *ev)
{
    U32((u8 *)U32(ev, 8), 0x12C4) |= 0x10;
}

void net_cb_flag40(const u8 *ev)
{
    U32((u8 *)U32(ev, 8), 0x12C4) |= 0x40;
}

/* starts the ad hoc network with the product code and the game name */
void net_start(u8 *self, s32 alt)
{
    strcpy((char *)self + 5, D_089B16C0);
    if (alt == 0)
        strcpy((char *)self + 0x11, D_089B16CC);
    else
        strcpy((char *)self + 0x11, D_089B16D4);
    self[0x19] = 0;
    lobbyapi_init((char *)self + 5);
    U32(self, 0x12C0) |= 1;
    lobbyapi_scan((void *)0x0889A09C, self);
    self[4] = 1;
}
