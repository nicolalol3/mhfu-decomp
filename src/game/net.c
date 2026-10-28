/* The ad hoc network object (2): the lists of the players of the room (4 entries of 200
 * bytes from 0x57C, count 0xEE3), of the rooms found (100 bytes from 0x12DC, count 0x12D8)
 * and of the game session; our own entry (by the console address); library callbacks.
 * (Guessed names.) */
#include "common.h"

#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define U16(p, off) (*(u16 *)((u8 *)(p) + (off)))

extern u8 *g_sys;
extern u8 *g_game;
extern u8 *g_sound;
extern void *g_menu;
extern void *g_players;
extern u8 *g_loader;
extern u8 D_09A2887C[];           /* address of this console */

void *memcpy(void *d, const void *s, u32 n);
void *memset(void *p, int c, u32 n);
int memcmp(const void *a, const void *b, u32 n);
u32 sound_play(void *snd, int a, int b, int c, int d, int e, int f, int g, int h);
void hud_notice_add(void *menu, u8 *name, int kind);
void func_088B3718(u8 *info);
void func_088B3744(u8 *mac);
void func_088B3A3C(u8 *info);
void func_088B3FFC(u8 *mac, u8 *v);
void func_088B0D2C(u8 *buf);
void func_088B0C8C(u8 *data);

u8 *net_my_mac(u8 *self);
void net_sort(u8 *self, s32 n, u8 *list, s32 size, u8 *key, u8 *tmp);
void net_room_add(u8 *self, u8 *info);
void net_room_remove(u8 *self, u8 *mac);
void net_sort_players(u8 *self);
void net_sort_rooms(u8 *self);

#define NET_ONLINE ((U32(g_sys, 0xF7A57C) & 1) != 0)
#define LOADER_QUIET (g_loader[0x2930C])

/* 0 when the 6 bytes are the same */
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

static u8 *find_player(u8 *self, const u8 *mac)
{
    s32 i;

    for (i = 0; i < (s8)self[0xEE3]; i++)
        if (mac_cmp(mac, self + i * 200 + 0x57C) == 0)
            return self + i * 200 + 0x57C;
    return NULL;
}

/* our entry in the list of the players */
static u8 *my_entry(u8 *self)
{
    return find_player(self, net_my_mac(self));
}

/* ---- callbacks: event {.., .., net, result, error} ---- */

void net_cb_peer(const u8 *ev)
{
    u8 info[100];

    func_088B3718(info);
    net_room_add((u8 *)U32(ev, 8), info);
    if (LOADER_QUIET == 0)
        sound_play(g_sound, 3, 0x1C, 0, 0, 0, 0, 0, 0);
}

void net_cb_peer_lost(const u8 *ev)
{
    u8 *n = (u8 *)U32(ev, 8);
    u8 mac[8];

    func_088B3744(mac);
    net_room_remove(n, mac);
    if (mac_cmp(n + 0x12C8, mac) != 0)
        return;
    U32(n, 0x12C4) |= 0x200;
    if (LOADER_QUIET != 0)
        return;
    sound_play(g_sound, 3, 0x1D, 0, 0, 0, 0, 0, 0);
    if ((U32(g_game, 0x6AF14) & 8) != 0)
        memset(n + 0x12C8, 0, 8);
}

void net_cb_busy(const u8 *ev)
{
    u8 *n = (u8 *)U32(ev, 8);
    s8 s = n[0x12D5];

    if (s == 5 || s == 3 || s == 4)
        U32(n, 0x12C4) |= 0x400;
}

void net_cb_peer_info(const u8 *ev)
{
    u8 info[100];
    u8 *n = (u8 *)U32(ev, 8);

    func_088B3A3C(info);
    net_room_add(n, info);
    U32(n, 0x12C4) |= 0x800;
}

void net_cb_peer_info2(const u8 *ev)
{
    u8 info[100];
    u8 *n = (u8 *)U32(ev, 8);

    func_088B3A3C(info);
    net_room_add(n, info);
    U32(n, 0x12C4) |= 0x1000;
}

/* new value of a room (byte 0x56 of its entry) */
void net_cb_room_value(const u8 *ev)
{
    u8 *n = (u8 *)U32(ev, 8);
    u8 mac[8];
    u8 v;
    s32 i;

    func_088B3FFC(mac, &v);
    for (i = 0; i < (s8)n[0x12D8]; i++)
        if (mac_cmp(n + i * 100 + 0x12DC, mac) == 0)
            n[i * 100 + 0x1332] = v;
}

/* data of a player of the room; notices when it enters / leaves a quest (state 5) */
void net_cb_player_data(const u8 *ev)
{
    u8 *n = (u8 *)U32(ev, 8);
    u8 buf[0xA8];
    u8 *e;
    s32 i;
    u8 old;

    func_088B0D2C(buf);
    for (i = 0; i < (s8)n[0xEE3]; i++) {
        e = n + i * 200 + 0x57C;
        if (mac_cmp(e, buf) != 0)
            continue;
        old = e[0xBF];
        if (old != buf[0xA7]) {
            if (buf[0xA7] == 5)
                hud_notice_add(g_menu, n + (i & 0xFF) * 200 + 0x584, 1);
            if (old == 5)
                hud_notice_add(g_menu, n + (i & 0xFF) * 200 + 0x584, 2);
        }
        memcpy(n + i * 200 + 0x59C, buf + 8, 0xA4);
        return;
    }
}

void net_cb_flag8000(const u8 *ev)
{
    U32((u8 *)U32(ev, 8), 0x12C4) |= 0x8000;
}

void net_cb_error(const u8 *ev)
{
    u8 *n = (u8 *)U32(ev, 8);

    U32(n, 0x1AF8) = U32(ev, 0x10) & 0xFFFF;
    n[0x1AFC] = 1;
}

u8 *net_my_mac(u8 *self)
{
    return D_09A2887C;
}

/* bubble sort of n records of size bytes, the biggest key (6 bytes at key - list) first */
void net_sort(u8 *self, s32 n, u8 *list, s32 size, u8 *key, u8 *tmp)
{
    s32 i, j;
    u8 *a, *b;

    for (i = 0; i < n - 1; i++) {
        a = list;
        b = list;
        for (j = 0; j < n - 1; j++) {
            b += size;
            if (mac_cmp(a + (key - list), b + (key - list)) < 0) {
                memcpy(tmp, a, size);
                memcpy(a, b, size);
                memcpy(b, tmp, size);
            }
            a += size;
        }
    }
}

/* the record with the key first */
void net_move_first(u8 *self, s32 n, u8 *list, s32 size, u8 *key, u8 *tmp, const u8 *k)
{
    u8 *a = list;
    s32 i;

    for (i = 0; i < n; i++, a += size) {
        if (mac_cmp(k, a + (key - list)) == 0) {
            memcpy(tmp, list, size);
            memcpy(list, a, size);
            memcpy(a, tmp, size);
            return;
        }
    }
}

void net_sort_players(u8 *self)
{
    u8 tmp[200];

    net_sort(self, (s8)self[0xEE3], self + 0x57C, 200, self + 0x57C, tmp);
}

void net_sort_rooms(u8 *self)
{
    u8 tmp[100];

    net_sort(self, (s8)self[0x12D8], self + 0x12DC, 100, self + 0x12DC, tmp);
}

/* the list of the room (0x25C): the host first, the others sorted */
void net_sort_room_list(u8 *self)
{
    u8 me[100], tmp[200];

    func_088B3A3C(me);
    net_move_first(self, (s8)self[0xEE1], self + 0x25C, 200, self + 0x25C, tmp, me);
    net_sort(self, (s8)self[0xEE1] - 1, self + 0x324, 200, self + 0x324, tmp);
}

/* the members of the session (0xBC0): the host first, the others sorted; 0xBBC our index */
void net_sort_members(u8 *self)
{
    u8 me[100], tmp[200];
    s32 i;

    func_088B3A3C(me);
    net_move_first(self, self[0xBBD], self + 0xBC0, 200, self + 0xBC0, tmp, me);
    net_sort(self, self[0xBBD] - 1, self + 0xC88, 200, self + 0xC88, tmp);
    for (i = 0; i < self[0xBBD]; i++)
        if (mac_cmp(net_my_mac(self), self + i * 200 + 0xBC0) == 0)
            self[0xBBC] = i;
}

/* a new player (record of 200 bytes) */
void net_player_add(u8 *self, u8 *rec)
{
    s32 n = (s8)self[0xEE3];
    u8 *e;

    if (find_player(self, rec) != NULL || n >= 4)
        return;
    e = self + (s8)self[0xEE3] * 200 + 0x57C;
    memcpy(e, rec, 8);
    memcpy(e + 8, rec + 8, 0x16);
    memcpy(e + 0x20, rec + 0x20, 0xA4);
    e[0xC4] = rec[0xC4];
    e[0xC5] = rec[0xC5];
    e[0xC6] = rec[0xC6];
    e[0xC7] = rec[0xC7];
    self[0xEE3]++;
    net_sort_players(self);
}

/* a room found (record of 100 bytes): updated or added */
void net_room_add(u8 *self, u8 *info)
{
    s32 n = (s8)self[0x12D8], i;
    u8 *e;

    for (i = 0; i < n; i++) {
        if (mac_cmp(info, self + i * 100 + 0x12DC) == 0) {
            memcpy(self + i * 100 + 0x12DC, info, 100);
            return;
        }
    }
    if (n >= 4)
        return;
    e = self + (s8)self[0x12D8] * 100 + 0x12DC;
    memcpy(e, info, 8);
    memcpy(e + 8, info + 8, 0x46);
    U32(e, 0x50) = U32(info, 0x50);
    e[0x54] = info[0x54];
    e[0x55] = info[0x55];
    e[0x56] = info[0x56];
    e[0x57] = info[0x57];
    U16(e, 0x58) = U16(info, 0x58);
    e[0x5A] = info[0x5A];
    e[0x5B] = info[0x5B];
    memcpy(e + 0x5C, info + 0x5C, 8);
    self[0x12D8]++;
    net_sort_rooms(self);
}

void net_player_remove(u8 *self, u8 *mac)
{
    s32 i, found = 0;

    for (i = 0; i < (s8)self[0xEE3]; i++) {
        if (mac_cmp(mac, self + i * 200 + 0x57C) == 0) {
            memset(self + i * 200 + 0x57C, 0, 200);
            found = 1;
        }
    }
    if (!found)
        return;
    net_sort_players(self);
    if ((s8)self[0xEE3] > 1)
        self[0xEE3]--;
}

void net_room_remove(u8 *self, u8 *mac)
{
    s32 i, found = 0;

    for (i = 0; i < (s8)self[0x12D8]; i++) {
        if (mac_cmp(mac, self + i * 100 + 0x12DC) == 0) {
            memset(self + i * 100 + 0x12DC, 0, 100);
            found = 1;
        }
    }
    if (!found)
        return;
    net_sort_rooms(self);
    if ((s8)self[0x12D8] > 0)
        self[0x12D8]--;
}

u8 *net_room_find(u8 *self, const u8 *mac)
{
    s32 i;

    for (i = 0; i < (s8)self[0x12D8]; i++)
        if (mac_cmp(mac, self + i * 100 + 0x12DC) == 0)
            return self + i * 100 + 0x12DC;
    return NULL;
}

/* our state (0x12D5), also in our entry */
void net_set_state(u8 *self, u8 s)
{
    u8 *e;

    self[0x12D5] = s;
    e = my_entry(self);
    if (e != NULL)
        e[0xBF] = self[0x12D5];
}

void net_set_my_b3(u8 *self, u8 v)
{
    u8 *e = my_entry(self);

    if (e != NULL)
        e[0xB3] = v;
}

/* our data for the others (sent every 60 frames) */
void net_send_my_data(u8 *self, const void *data, u32 size)
{
    u8 *e;

    if (!NET_ONLINE)
        return;
    if (self[0x12D5] == 0 || self[0x12D5] == 1 || self[0x12D5] == 5)
        return;
    e = my_entry(self);
    if (e != NULL) {
        memcpy(e + 0x20, data, size);
        func_088B0C8C(e + 0x20);
    }
    U32(self, 0x12D0) = 0x3C;
}

void net_set_host(u8 *self, u8 v)
{
    u8 *e;

    self[0x12D6] = v;
    e = my_entry(self);
    if (e != NULL)
        e[0xBE] = self[0x12D6];
}

void net_set_my_group(u8 *self, const u8 *mac)
{
    u8 *e = my_entry(self);

    if (e != NULL)
        memcpy(e + 0xB8, mac, 6);
}

void net_set_target(u8 *self, const u8 *mac, u16 a, u16 b)
{
    u8 *e = my_entry(self);

    if (e != NULL) {
        memcpy(e + 0xAD, mac, 6);
        U16(e, 0xB4) = a;
        U16(e, 0xB6) = b;
    }
}

void net_set_target_data(u8 *self, const u8 *mac, const u8 *data)
{
    u8 *e = my_entry(self);

    if (e != NULL) {
        memcpy(e + 0xAD, mac, 6);
        memcpy(e + 0x9C, data, 0x10);
    }
}

void net_clear_target(u8 *self)
{
    u8 *e = my_entry(self);

    if (e != NULL) {
        memset(e + 0xAD, 0, 6);
        U16(e, 0xB4) = 0;
        U16(e, 0xB6) = 0;
    }
}

/* sends our entry again */
void net_send_entry(u8 *self)
{
    s32 i;
    u8 *me;

    if (!NET_ONLINE)
        return;
    for (i = 0; i < (s8)self[0xEE3]; i++) {
        me = net_my_mac(self);
        if (mac_cmp(self + i * 200 + 0x57C, me) == 0) {
            func_088B0C8C(self + i * 200 + 0x59C);
            break;
        }
    }
    U32(self, 0x12D0) = 0x3C;
}

/* the request of the entry e is for us: its values / its data */
u8 *net_request_values(u8 *self, u8 *e)
{
    if (e == NULL)
        return NULL;
    if (memcmp(e + 0xAD, net_my_mac(self), 6) == 0 && U16(e, 0xB4) != 0)
        return e + 0xB4;
    return NULL;
}

u8 *net_request_data(u8 *self, u8 *e)
{
    if (e == NULL)
        return NULL;
    if (memcmp(e + 0xAD, net_my_mac(self), 6) == 0 && U16(e, 0xB4) == 0)
        return e + 0x9C;
    return NULL;
}

extern u8 *g_net_players;         /* the network object */
void func_0889D8A8(u8 *self, const u8 *mac, u32 kind, void *data);

/* the players idx and us are in the same group of a quest */
u32 net_same_group(u8 *self, u32 idx)
{
    u8 *me = my_entry(self);
    u8 *grp = NULL, *e;
    s32 i;
    s8 s;

    if (me != NULL) {
        for (i = 0; i < (s8)self[0xEE3]; i++) {
            if (memcmp(me + 0xB8, self + (i & 0xFF) * 200 + 0x57C, 6) == 0) {
                grp = self + (i & 0xFF) * 200 + 0x57C;
                break;
            }
        }
    }
    s = self[0x12D5];
    if (s != 5 && s != 3 && s != 4)
        return 0;
    if (grp == NULL)
        return 0;
    e = self + (idx & 0xFF) * 200 + 0x57C;
    s = e[0xBF];
    if (s != 5 && s != 3 && s != 4)
        return 0;
    return memcmp(grp, e + 0xB8, 6) == 0;
}

u32 net_same_group_mac(u8 *self, const u8 *mac)
{
    s32 i;

    for (i = 0; i < (s8)self[0xEE3]; i++)
        if (mac_cmp(mac, self + i * 200 + 0x57C) == 0)
            return net_same_group(self, i);
    return 0;
}

u8 net_player_b3(u8 *self, u32 i)
{
    return self[(i & 0xFF) * 200 + 0x62F];
}

/* the smallest address among us and the players not in a quest: the host of the room */
void net_host_mac(u8 *out, u8 *self)
{
    u8 best[8];
    u8 *m = net_my_mac(self), *e;
    s32 i;

    for (i = 0; i < 8; i++)
        best[i] = m[i];
    for (i = 0; i < (s8)self[0xEE3]; i++) {
        e = self + (i & 0xFF) * 200 + 0x57C;
        if (e[0xBF] == 4 || e[0xBF] == 5)
            continue;
        if (mac_cmp(best, e) > 0) {
            memcpy(best, e, 6);
            best[6] = e[6];
            best[7] = e[7];
        }
    }
    for (i = 0; i < 8; i++)
        out[i] = best[i];
}

u8 net_is_host(u8 *self)
{
    u8 h[8];

    if (!NET_ONLINE)
        return 1;
    net_host_mac(h, self);
    return mac_cmp(h, net_my_mac(self)) == 0;
}

/* ---- join request to a room: {active, kind, .., .., tries 0x0C, timer 0x10, data 0x14,
 *      mac 0x18, result 0x20, retries 0x24} ---- */

void netreq_clear(u8 *r)
{
    U32(r, 4) = 0;
    r[0] = 0;
    U32(r, 8) = 0;
    r[0xC] = 0;
    U32(r, 0x10) = 0;
    U32(r, 0x14) = 0;
    r[0x1D] = 0;
    r[0x1C] = 0;
    r[0x1B] = 0;
    r[0x1A] = 0;
    r[0x19] = 0;
    r[0x18] = 0;
    U32(r, 0x20) = 0;
    r[0x24] = 0;
}

void netreq_start(u8 *r, const u8 *mac, u32 kind)
{
    s32 i;

    netreq_clear(r);
    r[0] = 1;
    U32(r, 4) = kind;
    for (i = 0; i < 8; i++)
        r[0x18 + i] = mac[i];
    U32(r, 0x20) = 1;
}

void netreq_update(u8 *r)
{
    if (U32(r, 8) != 1) {
        if (U32(r, 8) != 0)
            return;
        if ((U32(g_net_players, 0x12C0) & 0x200) != 0)
            return;
        if (g_net_players[0x1AF4] == 0) {
            netreq_clear(r);
            U32(r, 0x20) = 3;
        } else {
            U32(r, 0x14) = -1;
            U32(r, 8)++;
        }
    }
    if (net_room_find(g_net_players, r + 0x18) == NULL) {
        netreq_clear(r);
        U32(r, 0x20) = 2;
    }
    if (r[0xC] == 0) {
        if ((s32)++U32(r, 0x14) < 0x25) {
            func_0889D8A8(g_net_players, r + 0x18, 4, r + 0x14);
            U32(r, 0x10) = 0x96;
            r[0xC] = 1;
            r[0x24] = 5;
        } else {
            netreq_clear(r);
            U32(r, 0x20) = 0;
        }
    } else if ((s32)--U32(r, 0x10) < 0) {
        if (--((s8 *)r)[0x24] < 0) {
            netreq_clear(r);
            U32(r, 0x20) = 2;
        } else {
            func_0889D8A8(g_net_players, r + 0x18, 4, r + 0x14);
            U32(r, 0x10) = 0x96;
        }
    }
}

/* ---- message to a player: {.., sent, retries, mac 3.., step 0x0C, timer 0x10,
 *      result 0x14, kind 0x18, data 0x1C} ---- */

void netmsg_clear(u8 *r)
{
    r[0] = 0;
    U32(r, 0xC) = 0;
    r[1] = 0;
    U32(r, 0x10) = 0;
    r[8] = 0;
    r[7] = 0;
    r[6] = 0;
    r[5] = 0;
    r[4] = 0;
    r[3] = 0;
    U32(r, 0x14) = 0;
    r[2] = 0;
}

void netmsg_update(u8 *r)
{
    if (U32(r, 0xC) != 1) {
        if (U32(r, 0xC) != 0)
            return;
        func_0889D8A8(g_net_players, r + 3, U32(r, 0x18), (void *)U32(r, 0x1C));
        U32(r, 0x10) = 0x96;
        r[1] = 1;
        r[2] = 5;
        U32(r, 0xC)++;
    }
    if (find_player(g_net_players, r + 3) == NULL) {
        netmsg_clear(r);
        U32(r, 0x14) = 2;
    }
    if (r[1] == 0) {
        netmsg_clear(r);
        U32(r, 0x14) = 0;
    } else if ((s32)--U32(r, 0x10) < 0) {
        if (--((s8 *)r)[2] < 0) {
            netmsg_clear(r);
            U32(r, 0x14) = 2;
        } else {
            func_0889D8A8(g_net_players, r + 3, U32(r, 0x18), (void *)U32(r, 0x1C));
            U32(r, 0x10) = 0x96;
        }
    }
}
