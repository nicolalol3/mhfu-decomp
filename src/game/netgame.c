/* The messages of the game between the players (on the session of netsync.c).
 * The object (g_net): 1..5 flags of the frame, 6 mode, 7 running, 8 counter, 0x12..0x19,
 * 0x170 closed, 0x171 players that left, 0x174 open, 0x175 keep the session, 0x176
 * WLAN switch. A message: {size, 0, code, category, data}; category 7: the box and the
 * quest slots (the host answers), 10: the state of the players. (Guessed names.) */
#include "common.h"

#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define U16(p, off) (*(u16 *)((u8 *)(p) + (off)))
#define S16(p, off) (*(s16 *)((u8 *)(p) + (off)))
#define F32(p, off) (*(f32 *)((u8 *)(p) + (off)))

#define OWN (g_game[0x28])
#define HOST (g_game[0x420])

extern u8 *g_sys, *g_game, *g_menu;
extern void *g_players, *g_quest, *g_monsters;
extern u8 *g_net;
extern u8 *D_09A4F06C;
extern const u16 D_089ACF9C;      /* text: the WLAN switch is off */
extern u32 D_09A1DB70, D_09A1DB74;
extern u32 D_09A1DB78[8][2];      /* handlers of the events {function, argument} */

void *memcpy(void *d, const void *s, u32 n);
void ns_open(u8 *p);
void ns_close_all(void);
void ns_close(void);
u32 ns_value(void);
void stub_088a72f0(u8 *msg, u32 x);   /* ns_add_msg */
void stub_088a72f8(void);             /* ns_flush */
s32 stub_088a7300(void);              /* ns_running */
void stub_088a7368(void);             /* ns_leave */
void stub_088a7370(void);             /* ns_shutdown */
u8 *player_get(void *players, u8 slot);
void hunter_add_item(u8 *pl, u32 item, s32 n, u32 x);
void hud_box_take_anim(u8 *m, u32 item, u32 slot);
u8 entity_is(u8 *e, u32 a, u32 b);
void entity_hide(u8 *e);
void menu_msg_push_text(u8 *m, u32 text);
void menu_msg_push(u8 *m, u32 a, u32 b, s32 c);
u32 func_0885151C(u8 *game, u32 id);  /* text_menu */
u8 func_0884FA5C(u8 *game);           /* game_quest_locked */
void quest_event(void *q, u32 who, u8 *pkt);
void quest_slot_set(void *q, s32 i, u32 v);
u8 quest_slot_is_set(void *q, s32 i);
void quest_slot_clear(void *q, s32 i);
u8 quest_slot_get(void *q, s32 i);
void quest_slots_copy(void *q, const u8 *s);
s32 quest_slot_find(void *q, u32 v);
void func_088DB534(void *players, u32 a);
void func_088DBE70(void *players, u32 a);
s32 sceWlanGetSwitchState(void);
void netgame_msg7(u8 *g, u32 from, u8 *msg);
void netgame_msg_kind(u8 *cat, u8 *code, u8 *msg);
void netgame_msg10(u8 *g, u32 from, u8 *msg);
void netgame_stop(u8 *g);
void net_send_quest(u8 *g, u32 kind, u8 *data);

/* in the overlays */
#define ovl_box_taken ((void (*)(u8 *, s32))0x09A6FE20)
#define ovl_box_close ((void (*)(u8 *))0x09A70068)
#define ovl_monsters_msg ((void (*)(void *, u32, u8 *))0x09AB3540)

#define NET_ONLINE() ((U32(g_sys, 0xF7A57C) & 1) != 0)

void netgame_open(u8 *g, u8 *info)
{
    u8 p[0x70];
    s32 i;

    for (i = 0; i < info[1]; i++)
        memcpy(p + 3 + i * 0x10, info + 4 + i * 200, 6);
    p[0] = 1;
    p[1] = info[1];
    p[2] = info[0];
    F32(p, 0x44) = 0.05f;
    U32(p, 0x48) = 1000;
    F32(p, 0x4C) = 20.0f;
    F32(p, 0x50) = 15.0f;
    F32(p, 0x54) = 60.0f;
    U32(p, 0x58) = 0x088AB35C;    /* netgame_on_msg */
    U32(p, 0x5C) = (u32)g;
    U32(p, 0x60) = 0x088AB490;
    U32(p, 0x64) = (u32)g;
    U32(p, 0x68) = 0x088AB4B0;
    U32(p, 0x6C) = (u32)g;
    ns_open(p);
    g[6] = info[0];
    g[7] = 0;
    g[0x170] = 0;
    g[0x172] = 0;
    g[0x171] = 0;
    g[0x173] = 0;
    g[0x174] = 1;
    g[0x175] = 0;
    g[1] = 0;
    g[2] = 0;
    g[3] = 0;
    g[4] = 0;
    g[5] = 0;
}

void netgame_close(u8 *g)
{
    netgame_stop(g);
    g[0x174] = 0;
    g[0x175] = 0;
}

static void close_step(u8 *g)
{
    if (g[0x175] == 0)
        ns_close_all();
    else
        ns_close();
}

void netgame_update(u8 *g)
{
    if (!NET_ONLINE() || g[0x174] == 0)
        return;
    if (g[7] == 1) {
        if (g[0x170] == 0) {
            stub_088a72f8();
            close_step(g);
            if (ns_value() != 0) {
                g[0x170] = 1;
                stub_088a7370();
            }
        }
    } else if (g[7] == 0) {
        if (g[0x170] == 0) {
            if (stub_088a7300() != 0)
                g[7] = 1;
            close_step(g);
            if (ns_value() != 0) {
                g[0x170] = 1;
                stub_088a7370();
                g[7] = 1;
            }
        }
    }
    g[1] = 0;
    g[2] = 0;
    g[3] = 0;
    g[4] = 0;
    g[5] = 0;
}

void netgame_stop(u8 *g)
{
    stub_088a7368();
    close_step(g);
    stub_088a7370();
}

/* a message of a player */
u32 netgame_on_msg(u32 from, u8 *msg, u8 *g)
{
    u8 code, cat;
    u8 *pl;

    if (player_get(g_players, OWN) == NULL)
        return 0;
    netgame_msg_kind(&cat, &code, msg);
    if (cat == 7) {
        netgame_msg7(g, from, msg);
        return 0;
    }
    if ((from & 0xFF) == OWN)
        return 0;
    switch (cat) {
    case 1:
    case 2:
    case 3:
    case 4:
        pl = player_get(g_players, from & 0xFF);
        if (pl != NULL)
            (*(void (**)(u8 *, u8 *, u32))(U32(pl, 0) + 0x58))(pl, msg, from);
        break;
    case 8:
        ovl_monsters_msg(g_monsters, from, msg);
        break;
    case 10:
        netgame_msg10(g, from, msg);
        break;
    }
    return 0;
}

void netgame_post(u32 cat, u32 code, u32 size, u32 kind, u8 *msg)
{
    msg[0] = (size & 0xFF) + 4;
    msg[2] = code;
    msg[3] = cat;
    msg[1] = kind;
    stub_088a72f0(msg, 0);
}

void netgame_msg_kind(u8 *cat, u8 *code, u8 *msg)
{
    *code = msg[2];
    *cat = msg[3];
}

/* the state of this player to the others (category 10) */
void net_send(u8 *g, u32 kind, u8 *data)
{
    u8 m[0x20];
    u8 *p;
    u32 n = 0;
    s32 i;

    if (!NET_ONLINE())
        return;
    switch (kind & 0xFF) {
    case 1:
        m[4] = 4;
        n = 1;
        break;
    case 2:
        p = g_game + OWN * 0x8C;
        m[4] = p[0x39];
        m[5] = p[0x61];
        U16(m, 8) = U16(p, 0x62);
        m[6] = U16(p, 0x64);
        m[0xC] = p[0x40];
        m[0xD] = p[0x3A];
        for (i = 0; i < 8; i++)
            m[0x16 + i] = g[0x12 + i];
        /* fall through */
    case 3:
        m[4] = g_game[0x378] + 1;
        S16(m, 0xA) = S16(g, 0xA + OWN * 2);
        n = 8;
        U16(g, 8) = 0;
        break;
    case 4:
    case 11:
        m[4] = data[0];
        n = 1;
        break;
    case 5:
        n = 8;
        break;
    case 6:
        m[4] = data[0];
        U16(m, 6) = U16(data, 2);
        S16(m, 8) = S16(data, 4);
        S16(m, 0xA) = S16(data, 6);
        U16(m, 0xC) = U16(data, 8);
        S16(m, 0xE) = S16(data, 0xA);
        n = 0xC;
        break;
    case 8:
    case 9:
    case 12:
    case 13:
        n = 1;
        break;
    }
    netgame_post(10, kind, n, 0, m);
}

/* the box and the quest slots (category 7) */
void net_send_quest(u8 *g, u32 kind, u8 *data)
{
    u8 m[0x10];
    u32 n = 0;
    s32 i;

    if (!NET_ONLINE())
        return;
    switch (kind & 0xFF) {
    case 1:
    case 2:
        m[4] = data[0];
        m[5] = data[1];
        F32(m, 8) = F32(data, 4);
        F32(m, 0xC) = F32(data, 8);
        n = 0xC;
        break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        m[4] = data[0];
        m[5] = data[1];
        for (i = 0; i < 6; i++)
            m[6 + i] = data[2 + i];
        n = 8;
        break;
    }
    netgame_post(7, kind, n, 0, m);
}

void netgame_msg10(u8 *g, u32 from, u8 *msg)
{
    u8 code, cat;
    u8 *p;

    netgame_msg_kind(&cat, &code, msg);
    switch (code) {
    case 2:
        p = g_game + (from & 0xFF) * 0x8C;
        p[0x40] = msg[0xC];
        p[0x3A] = msg[0xD];
        p[0x39] = msg[4];
        p[0x61] = msg[5];
        U16(p, 0x62) = U16(msg, 8);
        U16(p, 0x64) = msg[6];
        break;
    case 4:
    case 11:
        if (code == 11)
            net_send(g, 5, msg + 4);
        break;
    case 6:
        quest_event(g_quest, from, msg + 4);
        break;
    case 9:
        U16(g_menu, 0x56C + (from & 0xFF) * 2) = 0;
        break;
    case 13:
        D_09A4F06C[0x98] |= 1 << (from & 0xFF);
        break;
    }
}

static void slots(u8 *m)
{
    s32 i;

    for (i = 0; i < 6; i++)
        m[2 + i] = quest_slot_get(g_quest, i);
}

void netgame_msg7(u8 *g, u32 from, u8 *msg)
{
    u8 code, cat;
    u8 m[0x10];
    u8 *pl;
    u32 item, bit;
    s32 v;

    netgame_msg_kind(&cat, &code, msg);
    switch (code) {
    case 1:
        /* a player takes an item of the box: the host gives it to one only */
        if (HOST != OWN)
            break;
        m[0] = from;
        item = msg[5];
        bit = 1 << (item & 0x1F);
        if ((U32(g_game, 0x410 + (item >> 5) * 4) & bit) != 0) {
            m[1] = 0xFF;
        } else {
            m[1] = item;
            U32(g_game, 0x410 + (item >> 5) * 4) |= bit;
        }
        U32(m, 4) = U32(g_game, 0x410);
        U32(m, 8) = U32(g_game, 0x414);
        net_send_quest(g, 2, m);
        break;
    case 2:
        item = msg[5];
        if (item != 0xFF && OWN == msg[4]) {
            pl = player_get(g_players, msg[4]);
            U32(pl, 0x614) &= ~0x10000;
            v = U16(g_game, 0x390 + item * 4);
            hunter_add_item(pl, v, S16(g_game, 0x392 + item * 4), 0);
            hud_box_take_anim(g_menu, v, item);
        } else {
            U16(player_get(g_players, msg[4]), 0x5EC) = 1;
        }
        U32(g_game, 0x410) = U32(msg, 8);
        U32(g_game, 0x414) = U32(msg, 0xC);
        break;
    case 3:
        if (HOST != OWN)
            break;
        m[0] = from;
        item = msg[5];
        if (quest_slot_is_set(g_quest, item) == 1) {
            m[1] = 0xFF;
        } else {
            m[1] = item;
            quest_slot_set(g_quest, item, from);
        }
        slots(m);
        net_send_quest(g, 4, m);
        break;
    case 4:
        v = msg[5];
        if (v != 0xFF && OWN == msg[4]) {
            pl = player_get(g_players, msg[4]);
            if (entity_is(pl, 0, 0x7F) == 1) {
                ovl_box_taken(pl, v);
            } else {
                m[1] = v;
                net_send_quest(g, 5, m);
                U16(pl, 0x5EC) = 0;
            }
        } else {
            U16(player_get(g_players, msg[4]), 0x5EC) = 0;
        }
        quest_slots_copy(g_quest, msg + 6);
        break;
    case 5:
        if (HOST != OWN)
            break;
        quest_slot_clear(g_quest, msg[5]);
        slots(m);
        net_send_quest(g, 6, m);
        break;
    case 6:
        quest_slots_copy(g_quest, msg + 6);
        break;
    case 7:
        if (HOST != OWN)
            break;
        m[0] = from;
        if (quest_slot_is_set(g_quest, 0) == 1) {
            m[1] = 0xFF;
        } else if (func_0884FA5C(g_game) == 0) {
            m[1] = 0;
            quest_slot_set(g_quest, 0, 0x7F);
        }
        slots(m);
        net_send_quest(g, 8, m);
        break;
    case 8:
        if (msg[5] != 0xFF && OWN == msg[4])
            ovl_box_close(player_get(g_players, msg[4]));
        U16(player_get(g_players, msg[4]), 0x5EC) = 0;
        quest_slots_copy(g_quest, msg + 6);
        break;
    }
}

/* the WLAN switch, and the players that left the session */
void netgame_check_left(u8 *g)
{
    u8 m[8];
    u8 *pl;
    u32 i, bit;
    u8 left;
    s32 slot;

    if (g[0x176] != (sceWlanGetSwitchState() == 1) && sceWlanGetSwitchState() != 1)
        menu_msg_push_text(g_menu, func_0885151C(g_game, D_089ACF9C));
    if (g[0x176] != 0)
        g[0x176] = sceWlanGetSwitchState() == 1;
    for (i = 0; i < 4; i++) {
        bit = 1 << (i & 0xFF);
        left = g_net[0x171];
        g_net[0x171] = left & ~bit;
        if ((left & bit) == 0)
            continue;
        g_game[(i & 0xFF) * 0x8C + 0x30] = 0;
        menu_msg_push(g_menu, 3, 3, (s16)i);
        pl = player_get(g_players, i);
        if (pl != NULL) {
            if (U16(pl, 0x29A) == U16(g_game, 0x6AF0E)) {
                if (entity_is(pl, 4, 5) == 0)
                    (*(void (**)(u8 *, u32, u32, u32))(U32(pl, 0) + 0x6C))(pl, 4, 5, 2);
            } else {
                func_088DB534(g_players, pl[0x1E6]);
                func_088DBE70(g_players, U16(pl, 0x1E6));
                entity_hide(pl);
            }
        }
        slot = quest_slot_find(g_quest, i);
        if (slot != -1) {
            m[1] = slot;
            net_send_quest(g_net, 5, m);
        }
    }
}

void netgame_nop1(void)
{
}

void netgame_nop2(void)
{
}

void netgame_nop3(void)
{
}

static void call_handlers(u32 a, u32 b)
{
    u32 i;

    for (i = 0; i < 8; i++)
        if (D_09A1DB78[i][0] != 0)
            ((void (*)(u32, u32, u32))D_09A1DB78[i][0])(a, b, D_09A1DB78[i][1]);
}

/* the events of the ad hoc library */
void netev_adhoc(s32 ev, u32 err)
{
    if (ev == 1) {
        D_09A1DB74 = 0;
        D_09A1DB70 |= 2;
    } else if (ev == 2) {
        D_09A1DB74 = 0;
        D_09A1DB70 |= 4;
    } else if (ev == 0) {
        D_09A1DB74 = err;
        call_handlers(err, 8);
    } else if (ev == 3) {
        call_handlers(0, 7);
    }
}

/* the events of the connection to the access point */
void netev_apctl(s32 old, s32 now, s32 ev, u32 err)
{
    if (old == 3 && now == 4) {
        D_09A1DB74 = 0;
        D_09A1DB70 |= 2;
    }
    if (old == 4 && now == 0) {
        if (ev == 6) {
            D_09A1DB74 = err;
            call_handlers(err, 8);
        } else {
            D_09A1DB74 = 0;
        }
    }
    if (now == 0 && ev == 5)
        D_09A1DB70 |= 4;
}
