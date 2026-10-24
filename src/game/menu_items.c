/* Menu: item pouch (compaction), game mode of the menu, list of the other players.
 * Same class as menu_box.c. (Guessed names.) */
#include "menu.h"

extern u8 *D_09A0B544;            /* player data (overlay) */
extern const u8 D_0899D19A[];
extern const u8 D_089386C0[], D_089386D4[];
extern const char *const D_0893AF70;

u8 func_088D7AD8(void *hunter, u8 a, int b);
void func_088D8000(void *hunter);
void func_088D893C(void *hunter);
s32 quest_goal_kind(void *a, s32 i);
s32 quest_goal_target(void *a, s32 i);
u32 game_map_id(void *game);
s32 quest_state(void *a, int b);
s32 quest_goal_count(void *a, u32 id, u16 i);
s32 quest_points(void *a);
s32 func_0882A8B8(void *self, s32 i);
u8 *hud_other_player(void *self, s32 i);
void func_088912BC(void *font, int x, int y, const char *s);
s16 hunter_deliver(void *hunter);
void quest_send_item(void *a, u32 b);
u8 quest_deliveries_done(void *a);
void func_088966C0(void *self, int v);
s32 menu_slot_first(void *self, u32 v);
s32 menu_pouch_cursor(void *self, u8 *hunter);
s32 menu_other_ready(void *self);

/* removes the holes from the item pouch (or from the box when box != 0) and
 * keeps the cursors of the hunter on the same items */
void menu_pouch_compact(void *self, u32 box_arg)
{
    u8 box = box_arg;
    u8 *hunter = (u8 *)player_get(D_09A4F0C4, D_089CC438[0x28]);
    s16 *list;
    s16 *p;
    u16 sel = 0, a = 0, b = 0;
    s32 i, rem, n;

    if (box == 0)
        list = (s16 *)func_08856884(D_089CC438, 0);
    else
        list = (s16 *)(D_089CC438 + 0x43C8);
    if ((*(u32 *)(D_089CC438 + 0x6AF14) & 1) == 0 && (*(u32 *)(D_089CC438 + 0x6AF14) & 8) == 0) {
        u8 k1, k2;

        if (menu_pouch_cursor(self, hunter) < 0)
            return;
        U16(hunter, 0x686) = func_088D7AD8(hunter, U8(hunter, 0x686), 2);
        k1 = U8(hunter, 0x67C);
        k2 = U8(hunter, 0x5F6);
        func_088D8000(hunter);
        U8(hunter, 0x67C) = k1;
        U8(hunter, 0x5F6) = k2;
        sel = *func_08856884(D_089CC438, U16(hunter, 0x648));
        if (U8(hunter, 0x686) == 0xFF)
            a = 0;
        else
            a = *func_08856884(D_089CC438, U8(hunter, 0x686));
        if (U8(hunter, 0x5F6) != 0xFF)
            b = *func_08856884(D_089CC438, U8(hunter, 0x5F6));
    }
    rem = 24;
    n = 0;
    p = list;
    for (i = 24; i != 0 && rem != 0; i--, p += 2) {
        if (p[0] != 0) {
            n++;
            continue;
        }
        rem = i - 1;
        if (rem != 0) {
            s16 *q = p + 2;

            do {
                if (q[0] != 0) {
                    n++;
                    p[0] = q[0];
                    p[1] = q[1];
                    q[1] = 0;
                    q[0] = 0;
                    break;
                }
                rem--;
                q += 2;
            } while (rem != 0);
        }
    }
    func_0885C5DC(list, n);
    if ((*(u32 *)(D_089CC438 + 0x6AF14) & 1) == 0 && (*(u32 *)(D_089CC438 + 0x6AF14) & 8) == 0) {
        u16 *s;
        u32 j;

        if (box != 0)
            return;
        s = func_08856884(D_089CC438, 0);
        for (j = 0; j < 24; j++, s += 2) {
            if (*s == sel) {
                U16(hunter, 0x648) = j;
                break;
            }
        }
        s = func_08856884(D_089CC438, 0);
        for (j = 0; j < 24; j++, s += 2) {
            if (*s == a) {
                U16(hunter, 0x686) = j & 0xFF;
                break;
            }
        }
        if (U8(hunter, 0x5F6) == 0xFF)
            return;
        s = func_08856884(D_089CC438, 0);
        for (j = 0; j < 24; j++, s += 2) {
            if (*s == b) {
                U8(hunter, 0x5F6) = j;
                return;
            }
        }
    } else if (box != 0 && hunter != NULL) {
        func_088D893C(hunter);
    }
}

/* moves the pouch cursor of the hunter to a non-empty slot (-1 if the pouch is empty) */
s32 menu_pouch_cursor(void *self, u8 *hunter)
{
    u16 i;
    u16 n;

    if (*func_08856884(D_089CC438, U16(hunter, 0x648)) != 0)
        return U16(hunter, 0x648);
    i = U16(hunter, 0x648);
    for (n = 24; n > 0; n--) {
        if (*func_08856884(D_089CC438, i) != 0) {
            U16(hunter, 0x648) = i;
            return i;
        }
        i = i + 1;
        if (i >= 24)
            i = 0;
    }
    return -1;
}

/* kind of the menu from the state of the game (stored in self+0x584) */
u8 menu_get_mode(u8 *self)
{
    u32 f;

    self[0x5E6] = 0;
    f = *(u32 *)((u8 *)D_08A62C7C + 0x28);
    if (f & 0x40000) {
        if (D_089CC438[0x6AF10] == 12)
            self[0x584] = 4;
        else if (D_089CC438[0x6AF10] == 1)
            self[0x584] = 3;
        else
            self[0x584] = 6;
    } else if (f & 4) {
        self[0x584] = 0;
        if (*(u32 *)((u8 *)D_08A62C7C + 0x28) & 0x200000)
            self[0x5E6] |= 1;
        if ((*(u32 *)((u8 *)D_08A62C7C + 0x28) & 0x400000) == 0)
            return self[0x584];
        self[0x5E6] |= 4;
    } else if (f & 1) {
        s32 i;

        self[0x584] = 1;
        for (i = 0; i < 2; i++) {
            if (quest_goal_kind(D_08A62C7C, i) == 3) {
                self[0x5E6] |= 2;
                break;
            }
        }
    } else if (f & 2) {
        self[0x584] = 2;
    } else if (f & 0x10) {
        self[0x584] = 5;
    } else {
        s8 r = menu_other_ready(self);

        if (r < 0) {
            self[0x584] = 6;
        } else {
            self[0x584] = 1;
            if (r == 1)
                self[0x5E6] |= 2;
        }
    }
    return self[0x584];
}

/* state of the other players: -1 none ready, 0 ready, 1 ready in state 3 */
s32 menu_other_ready(void *self)
{
    s32 i, a, b;

    for (i = 0; ; ) {
        a = quest_goal_kind(D_08A62C7C, i);
        if (a == 2 || a == 3) {
            b = quest_goal_target(D_08A62C7C, i);
            if (b == 2)
                break;
            b = quest_goal_target(D_08A62C7C, i);
            if (b == 0x24)
                break;
            b = quest_goal_target(D_08A62C7C, i);
            if (b == 0x47)
                break;
        }
        if (++i > 1)
            return -1;
    }
    return a == 3;
}

/* data of the current quest */
u8 *menu_quest_data(void *self)
{
    if ((*(u32 *)(D_089CC438 + 0x6AF14) & 1) == 0)
        return *(u8 **)((u8 *)D_08A62C7C + 0x68);
    return (u8 *)0x09B03F40;
}

/* 1 if the selected item can be given to another player */
int menu_item_giftable(u8 *self)
{
    if ((*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) == 0)
        return 0;
    if (D_0899D19A[(s16)menu_slot_first(self, self[0x586]) * 0x18] >= 3)
        return 0;
    if ((s8)D_0899D19B[(s16)menu_slot_first(self, self[0x586]) * 0x18] == -1)
        return 0;
    if ((*(u32 *)(D_089CC438 + 0x6AF14) & 1) == 0)
        return U8(*(u8 **)(self + 0x530), 0x1E0) == 0;
    return *(s16 *)(D_089CC438 + 0x6AF0E) == 0x55;
}

/* 1 if item id is needed by the current quest (delivery item) */
int menu_item_for_quest(void *self, u32 id)
{
    u32 i;
    s32 r;

    if ((*(u32 *)(D_089CC438 + 0x6AF14) & 1) != 0)
        return 0;
    {
        u16 q = *(u16 *)(D_089CC438 + 0x6AF0E);

        if (game_map_id(D_089CC438) != q)
            return 0;
    }
    if (quest_state(D_08A62C7C, 1) != 0)
        return 0;
    for (i = 0; (s32)i < 2; i++) {
        r = quest_goal_count(D_08A62C7C, id, i & 0xFFFF);
        if (r >= 0) {
            if ((*(u32 *)((u8 *)D_08A62C7C + 0x28) & 0x10) == 0)
                return 1;
            if ((id & 0xFFFF) != 0x3A0 || quest_points(D_08A62C7C) > 1999)
                return 1;
        }
    }
    return 0;
}

/* quantity box or list of the other players (to give an item) */
void menu_draw_give(u8 *self, int alt)
{
    Layout l;
    s8 col[4];
    const char *name[4];
    u32 me;
    s32 i, k;
    s16 y;

    if (self[0x589] == 0) {
        func_08832E34(self, 0xF0, 0x68, self[0x58A], self[0x58A] + 1 - self[0x58B], 0);
        return;
    }
    if (self[0x589] != 1)
        return;
    if (alt == 0)
        func_0882FA24(&l, D_089386C0, 0, 0, 0);
    else
        func_0882FA24(&l, D_089386D4, 0, 0, 0);
    if ((*(u32 *)(D_089CC438 + 0x6AF14) & 1) == 0) {
        me = D_089CC438[0x28];
        k = 0;
        for (i = 0; i < 4; i++) {
            if (me == (u32)i)
                continue;
            if (D_089CC438[(i & 0xFF) * 0x8C + 0x30] == 0)
                name[k] = NULL;
            else
                name[k] = (const char *)(D_09A0B544 + (i & 0xFF) * 200 + 0xBC8);
            col[k] = 0;
            k++;
        }
        name[k] = NULL;
        col[k] = 0;
    } else {
        l.e = 3;
        l.h = l.lh * 3 + 16;
        for (i = 0, k = 0; i < 3; i++, k++) {
            s8 c;

            col[i] = 0;
            c = func_0882A8B8(self, i);
            if (c == 0) {
                name[i] = (const char *)hud_other_player(self, i) + 8;
            } else if (c == 1) {
                col[i] = 10;
                name[i] = (const char *)hud_other_player(self, i) + 8;
            } else {
                name[i] = NULL;
            }
        }
        name[k] = NULL;
        me = 4;
    }
    k = S8(self, 0x587);
    if ((s32)me < k)
        k--;
    func_0882F580(self, &l, 0, k, 0xFF, 0);
    y = l.ty;
    for (i = 0; i < (u16)l.e; i++) {
        func_08890E38(D_08A631A8, col[i]);
        if (name[i] == NULL) {
            func_08890DE8(D_08A631A8, 14, 14);
            func_08891474(D_08A631A8, l.tx, y, D_0893AF70);
        } else {
            func_08890DE8(D_08A631A8, 16, 14);
            func_088912BC(D_08A631A8, l.tx, y, name[i]);
        }
        y += l.lh;
    }
}

/* gives the selected item to player p */
void menu_give_item(u8 *self, u32 p)
{
    if (hunter_deliver(*(void **)(self + 0x530)) == 0)
        return;
    quest_send_item(D_08A62C7C, p);
    self[0x1207] = 1;
    if (quest_deliveries_done(D_08A62C7C) == 1)
        func_088966C0(self, 15);
}

extern void *D_08A62C40;          /* display list manager */
extern const s16 D_089391A8[][6];  /* menu sprites: s16 u, v, w, h, u16 texture[2] */
void hud_pause_open(void *self);
void hud_grid_input(void *self, u32 a);
void tex_select(void *self, u16 a, u16 b);
void dl_link(void *dl, u32 *p, int n, u8 layer);

/* 1 if the menu accepts input now (for the player of hunter, when given) */
int menu_input_ready(u8 *self, u8 *hunter)
{
    if (hunter != NULL && U16(hunter, 0x1E6) != D_089CC438[0x28])
        return 0;
    if (self[0x52F] != 0)
        return 0;
    return S16(self, 0x119E) < 1;
}

/* state of player i: 3 absent, 1 in state 5, 2 in state 0, 0 otherwise */
int menu_player_state(void *self, s32 i)
{
    u8 *p = hud_other_player(self, i);

    if (p == NULL)
        return 3;
    if (p[0xBF] == 5)
        return 1;
    if (p[0xBF] == 0)
        return 2;
    return 0;
}

/* opens the "give item" sub-menu (-1 if not possible) */
s32 menu_open_give(u8 *self)
{
    if ((*(u32 *)(D_089CC438 + 0x6AF14) & 1) == 0) {
        if (U8(*(u8 **)(self + 0x530), 0x1E0) != 0)
            return -1;
        self[0x57E] = 0;
    } else {
        if ((*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) == 0)
            return -1;
        self[0x57E] = 0;
    }
    self[0x57F] = 0;
    self[0x580] = 0;
    S16(self, 0x5FC) = 12;
    return 0;
}

/* closes the menu (and the sub-screen of the hunter, if open) */
void menu_close(u8 *self, u32 a)
{
    if ((U8(*(u8 **)(self + 0x530), 0x1E0) != 0) == 1)
        hud_pause_open(self);
    hud_grid_input(self, a);
}

/* draws the menu sprite id at (x, y) */
void menu_draw_sprite(u8 *self, u32 x, u32 y, s32 id)
{
    const s16 *t = D_089391A8[id];
    u8 *dl;
    u32 w[15];
    u32 cur;
    s32 used;
    u8 layer;
    int i;

    tex_select(self, t[4], t[5]);
    dl = D_08A62C40;
    w[6] = 0x1E000001;
    w[8] = 0x50000001;
    w[9] = 0x12800116;
    w[12] = 0x04060002;
    w[13] = 0x10000000;
    w[14] = 0x08000000;
    w[7] = 0xC9000100;
    w[1] = ((x & 0xFFFF) << 16) | 0xFFFF;
    w[0] = ((u32)t[1] << 16) | (u32)(s32)t[0];
    w[2] = y & 0xFFFF;
    w[5] = ((y & 0xFFFF) + t[3]) & 0xFFFF;
    w[3] = ((u32)(t[1] + t[3]) << 16) | (u32)(t[0] + t[2]);
    w[4] = (((x & 0xFFFF) + t[2]) << 16) | 0xFFFF;
    layer = self[0x534];
    if (layer >= 20)
        return;
    cur = U32(dl, 0x80198);
    used = (s32)(cur - (u32)(dl + U32(dl, 4) * 0x40000 + 0x198)) / 4;
    if ((u32)(used + 15) >= 0x10000)
        return;
    w[10] = ((cur & 0xFF000000) >> 8) | 0x10000000;
    w[11] = (U32(dl, 0x80198) & 0xFFFFFF) | 0x01000000;
    for (i = 0; i < 15; i++) {
        **(u32 **)(dl + 0x80198) = w[i];
        U32(dl, 0x80198) += 4;
    }
    dl_link(dl, (u32 *)(cur + 24), 9, layer);
}
