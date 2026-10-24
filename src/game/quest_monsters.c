/* Quest (D_08A62C7C): items given by the quest, monsters of the quest and gathering points
 * of the area (20 points of 0x20 bytes after the area object D_09A4F060).
 * (Guessed names.) */
#include "menu.h"

typedef struct QEvent {
    u8 type, b1;
    u16 a, b, c, d;
} QEvent;

extern u8 *D_09A4F060;             /* area: +4 = gathering points */
extern s8 D_08A62C9C[20];          /* active gathering points, in order of creation */
extern u8 D_08A62C80[];            /* min / max monster size for each rank */
extern const s8 D_089A77BC[8];
extern const u8 *D_089A8148[];
extern const u8 D_089A8164[32];
extern const u8 D_09BB89B0[][0x1D0];   /* monster data */
extern const u32 D_09CD7EB0[];
extern u8 *D_09C121B0;
extern void *D_09A1DAEC;
extern u8 *D_08A62CB4;

u8 menu_msg_push(u8 *self, u32 id, u32 x, u32 y);
void func_088AB514(void *a, int b, void *c);
s32 quest_state(u8 *self, s32 mode);
s32 quest_item_add(u8 *self, u32 id, s32 n);
u8 *quest_find_entry(u8 *self, u32 id, s32 which, s32 idx);
void quest_wave_add(u8 *self, s32 i, u16 *m);
u32 quest_rand(u8 *self);
u32 func_088900C4(void *a, int b);
u8 func_088D6A54(u32 h, int a);
s32 func_088D6A1C(u32 h, int a);
u32 func_088D72A4(u8 *h, s32 id, s32 n, s32 c);
s16 func_0886E578(void *q, u32 id, u32 i);
s16 func_0886E6F4(void *q, u32 id);
s32 func_0886F78C(u8 *self);
u32 func_088D7904(u8 *h, u32 id, s32 n);
void func_09C6F4B8(f32 scale, f32 *pos, int a);
void func_09AD7C88(u8 *a, s32 b);
u32 func_0887265C(u32 a, int b);
s32 func_08851144(u8 *g);
s32 func_08874124(u8 *a, u32 b);

void spots_remove(u8 *self, s32 idx);
void spots_make_room(u8 *self);
void spots_list_add(u8 *self, u32 idx);
void spots_list_remove(u8 *self, s32 idx);
u32 spots_item(u8 *self, u32 id);
void quest_send_item(u8 *self, u32 id);
s32 quest_hunter_item_get(u8 *self, u32 id, s32 who);
void quest_monster_init(u8 *self, u8 *m, s32 *counter);
void quest_init_main_monsters(u8 *self, u8 *l, s32 *counter);
void quest_init_sub_monsters(u8 *self, u32 *t, s32 *counter);
u32 quest_random_size(u8 *self);
u32 monster_is_large(u8 *self, s32 id);
void quest_set_entry_offset(u8 *self, u8 *tbl, u32 id, u8 *p, s32 which);

#define SPOT(i) (D_09A4F060 + 4 + (i) * 0x20)
#define IS_PLAYER(h) (U16(h, 0x1E6) == D_089CC438[0x28])

void quest_item_remove(u8 *self, u32 id, s32 n)
{
    quest_item_add(self, id, (s16)-(s16)n);
    self[0x81] = 1;
}

/* gives to the hunter h the items of the quest (with the messages for the player) */
void quest_give_items(u8 *self, u8 *h)
{
    s32 given = 0, i;
    s8 k;
    u32 r;
    u8 *e;

    if (*(s8 *)(self + 0x81) == 0) {
        if (IS_PLAYER(h))
            menu_msg_push(D_089CBBEC, 0, 0, 0);
        return;
    }
    k = D_089A77BC[func_088900C4(D_08A631A0, 0) & 0xFFFF & 7];
    if (func_088D6A54((u32)h, 3) == 1)
        k = 0;
    if (k == 2) {
        if (IS_PLAYER(h))
            menu_msg_push(D_089CBBEC, 0, 0, 0);
    } else if (k == 1) {
        r = func_088D72A4(h, 0xF6, 1, 0) & 0xFFFF;
        if (r == 3) {
            if (IS_PLAYER(h))
                menu_msg_push(D_089CBBEC, 1, 3, 0xF6);
            if (IS_PLAYER(h))
                menu_msg_push(D_089CBBEC, 0, 0, 0);
        } else if (r == 2 || r == 1 || r == 0) {
            if (IS_PLAYER(h))
                menu_msg_push(D_089CBBEC, 1, 0, 0xF6);
        } else {
            if (IS_PLAYER(h))
                menu_msg_push(D_089CBBEC, 0, 0, 0);
        }
    } else {
        for (i = 0, e = self; i < 5; i++, e += 4) {
            if (U16(e, 0x6C) == 0)
                continue;
            r = func_088D72A4(h, U16(e, 0x6C), *(s8 *)(e + 0x6E), 0) & 0xFFFF;
            if (r == 3) {
                if (IS_PLAYER(h))
                    menu_msg_push(D_089CBBEC, 1, 3, S16(e, 0x6C));
                given = 1;
            } else if (r == 2 || r == 1 || r == 0) {
                if (IS_PLAYER(h))
                    menu_msg_push(D_089CBBEC, 1, 0, S16(e, 0x6C));
                given = 1;
            }
        }
        if (!given && IS_PLAYER(h))
            menu_msg_push(D_089CBBEC, 0, 0, 0);
    }
    self[0x81] = 0;
    for (i = 0; i < 5; i++) {
        U16(self, 0x6C + i * 4) = 0;
        self[0x6E + i * 4] = 0;
    }
    self[0x80] = 0;
    self[0x81] = 0;
}

/* sets the count of a delivery item of the hunter h (0: no goal, 1: set, 2: goal reached) */
s32 quest_hunter_item_set(u8 *self, u8 *h, u32 id, s32 n)
{
    s16 max = 0, i, v, cur;

    for (i = 0; i < 2; i++) {
        v = func_0886E578(D_08A62C7C, id, i & 0xFFFF);
        if (v >= 0) {
            v = v - func_0886E6F4(D_08A62C7C, id);
            if (max < v)
                max = v;
        }
    }
    if (max <= 0)
        return 0;
    cur = 0;
    for (i = 0; i < 4; i++)
        if (U16(h, 0x542 + i * 4) == (id & 0xFFFF))
            cur = S16(h, 0x544 + i * 4);
    if (cur != 0) {
        for (i = 0; i < 4; i++) {
            if (U16(h, 0x542 + i * 4) == (id & 0xFFFF)) {
                S16(h, 0x544 + i * 4) = (s8)n;
                return (s16)n < max ? 1 : 2;
            }
        }
        return 1;
    }
    for (i = 0; i < 4; i++) {
        if (U16(h, 0x542 + i * 4) == 0) {
            U16(h, 0x542 + i * 4) = id;
            S16(h, 0x544 + i * 4) = (s8)n;
            return (s16)n < max ? 0 : 2;
        }
    }
    return 0;
}

/* count of a delivery item: who 1 = player, 0 = all the hunters, 2.. = hunter who-2 */
s32 quest_hunter_item_get(u8 *self, u32 id, s32 who)
{
    s16 sum = 0;
    s32 p, i;
    u8 *h;

    if (who == 0) {
        for (p = 0; p < 4; p++) {
            h = (u8 *)func_088DCEBC(D_09A4F0C4, p);
            if (h == NULL)
                continue;
            for (i = 0; i < 4; i++)
                if (U16(h, 0x542 + i * 4) == (id & 0xFFFF))
                    sum += S16(h, 0x544 + i * 4);
        }
        return sum;
    }
    if (who == 1)
        h = (u8 *)func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]);
    else
        h = (u8 *)((u32 (*)(void *, s32))func_088DCEBC)(D_09A4F0C4, who - 2);
    for (i = 0; i < 4; i++)
        if (U16(h, 0x542 + i * 4) == (id & 0xFFFF))
            return S16(h, 0x544 + i * 4);
    return 0;
}

void quest_send_item(u8 *self, u32 id)
{
    QEvent ev;

    ev.b = id;
    ev.type = 9;
    ev.a = 0;
    ev.c = quest_hunter_item_get(self, id, 1);
    func_088AB514(D_09A1DAEC, 6, &ev);
    menu_msg_push(D_089CBBEC, 1, 8, (s16)id);
}

/* delivery goals: the hunter h puts the items of the pouch in the delivery box */
u32 quest_check_deliver(u8 *self, u8 *h)
{
    u32 r = 0;
    u32 *g;
    s32 i, s;
    u16 *p, *q;

    if (quest_state(self, 1) != 0)
        return 0;
    for (i = 0, g = (u32 *)(self + 0x728); i < 2; i++, g += 2) {
        if (((g[0] & 0x20) | 0x12) != 0x12)
            continue;
        if (U16(g, 4) == 0)
            continue;
        if (U16(g, 4) == 0x3A0 && (U32(self, 0x28) & 0x10) != 0 && func_0886F78C(self) < 2000)
            continue;
        for (s = 0; s < 0x18; s++) {
            u16 id = U16(g, 4);

            if (id != *func_08856884(D_089CC438, s))
                continue;
            p = func_08856884(D_089CC438, s);
            q = func_08856884(D_089CC438, s);
            if ((func_088D7904(h, p[0], (s16)q[1]) & 0xFFFF) != 0) {
                r = 1;
                quest_send_item(self, U16(g, 4));
            }
        }
    }
    return r;
}

/* --- gathering points ------------------------------------------------------------- */
void spots_init(u8 *self)
{
    s32 i;

    self[0x21] = 0;
    for (i = 0; i < 20; i++) {
        U16(SPOT(i), 0x10) = 0xFFFF;
        D_08A62C9C[i] = -1;
    }
}

/* timers of the points (with the sparkle effect every 64 frames) */
void spots_update(u8 *self)
{
    s32 i;
    s16 t;
    u8 *e;
    f32 pos[3], k;

    for (i = 0; i < *(s8 *)(self + 0x21); i++) {
        e = SPOT(D_08A62C9C[i]);
        if (U16(e, 0x10) == 0xFFFF)
            continue;
        if (S16(e, 0x1C) == 0)
            continue;
        if (U16(e, 0x12) == 0) {
            spots_remove(self, D_08A62C9C[i]);
            continue;
        }
        t = S16(e, 0x1C) - 1;
        S16(e, 0x1C) = t;
        if (t <= 0) {
            spots_remove(self, D_08A62C9C[i]);
            continue;
        }
        t = S16(e, 0x1C);
        if ((t & 0x3F) != 0)
            continue;
        pos[0] = F32(e, 0);
        pos[1] = 20.0f + F32(e, 4);
        pos[2] = F32(e, 8);
        if (t < 900)
            k = 0.2f + 0.3f * ((f32)t / 900.0f);
        else
            k = 0.5f;
        func_09C6F4B8(k, pos, 0x15);
    }
}

/* new gathering point (copy of src, at pos when given); returns its index or -1 */
s32 spots_add(u8 *self, u8 *src, f32 *pos)
{
    u8 *e = SPOT(0);
    u32 h;
    s32 i, extra;

    spots_make_room(self);
    for (i = 0; i < 20; i++, e += 0x20) {
        if (U16(e, 0x10) != 0xFFFF)
            continue;
        if (pos == NULL) {
            F32(e, 0) = F32(src, 0);
            F32(e, 4) = F32(src, 4);
            F32(e, 8) = F32(src, 8);
        } else {
            F32(e, 0) = pos[0];
            F32(e, 4) = pos[1];
            F32(e, 8) = pos[2];
        }
        F32(e, 0xC) = F32(src, 0xC);
        U16(e, 0x10) = U16(src, 0x10);
        U16(e, 0x12) = U16(src, 0x12);
        U16(e, 0x14) = U16(src, 0x14);
        U16(e, 0x18) = U16(src, 0x18);
        e[0x1A] = src[0x1A];
        S16(e, 0x1C) = S16(src, 0x1C);
        if (U16(e, 0x14) == 2) {
            /* skills that give more gatherings */
            h = func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]);
            if (func_088D6A1C(h, 0xB0) != 0)
                U16(e, 0x12)++;
            extra = 0;
            if (func_088D6A54(h, 1) != 0)
                extra = (func_088900C4(D_08A631A0, 0) & 0xFFFF & 1) == 0;
            if (func_088D6A54(h, 2) != 0 && (func_088900C4(D_08A631A0, 0) & 0xFFFF & 3) == 0)
                extra = 1;
            if (extra)
                U16(e, 0x12)++;
        }
        spots_list_add(self, (s8)i);
        return i;
    }
    return -1;
}

void spots_remove(u8 *self, s32 idx)
{
    if (idx == -1)
        return;
    U16(SPOT(idx), 0x10) = 0xFFFF;
    spots_list_remove(self, (s8)idx);
}

/* gathering point where the hunter h stands */
u8 *spots_find(u8 *self, u8 *h)
{
    u8 *e = SPOT(0);
    s32 i;
    f32 dx, dz;

    for (i = 0; i < 20; i++, e += 0x20) {
        if (U16(e, 0x10) == 0xFFFF || U16(e, 0x18) != U16(h, 0x29A))
            continue;
        if (!(F32(h, 0x204) >= F32(e, 4) - 200.0f))
            continue;
        if (!(F32(h, 0x204) < F32(e, 4) + 100.0f))
            continue;
        dx = F32(h, 0x200) - F32(e, 0);
        dz = F32(h, 0x208) - F32(e, 8);
        if (!(F32(e, 0xC) * F32(e, 0xC) >= dx * dx + dz * dz))
            continue;
        return e;
    }
    return NULL;
}

u32 spots_kind(u8 *self, u8 *h)
{
    u8 *e = spots_find(self, h);

    if (e == NULL)
        return 0xFFFF;
    return U16(e, 0x14);
}

/* gathers at the point of h: item id, 0 (no point) or 0xFFFE (exhausted) */
u32 spots_gather(u8 *self, u8 *h)
{
    u8 *e = spots_find(self, h);
    u32 r;

    if (e == NULL)
        return 0;
    if (U16(e, 0x12) == 0)
        return 0xFFFE;
    r = spots_item(self, U16(e, 0x10) & 0x7FFF);
    if (U16(e, 0x12) != 0xFF)
        U16(e, 0x12)--;
    return r & 0xFFFF;
}

/* with 20 points active, removes the oldest one without the flag 1 */
void spots_make_room(u8 *self)
{
    s32 n, i;

    if (*(s8 *)(self + 0x21) < 20)
        return;
    n = *(s8 *)(self + 0x21) - 1;
    for (i = 0; i < n; i++)
        if ((D_09A4F060[D_08A62C9C[i] * 0x20 + 0x1E] & 1) == 0)
            break;
    spots_remove(D_08A62C7C, D_08A62C9C[i]);
}

void spots_list_remove_at(u8 *self, s32 i)
{
    i = (s8)i;
    for (; i < *(s8 *)(self + 0x21) - 1; i++)
        D_08A62C9C[i] = D_08A62C9C[i + 1];
    D_08A62C9C[i] = -1;
    self[0x21]--;
}

void spots_list_remove(u8 *self, s32 idx)
{
    s32 i;

    for (i = 0; i < *(s8 *)(self + 0x21); i++) {
        if ((s8)idx == D_08A62C9C[i]) {
            spots_list_remove_at(self, (s8)i);
            return;
        }
    }
}

/* change of area: monsters notified, temporary points removed */
void spots_area_change(u8 *self)
{
    s32 i;
    u8 *p;

    for (i = 0; i < 20; i++) {
        p = (i >= 0 && i < 20) ? (u8 *)U32(D_09C121B0, 0x1220 + i * 4) : NULL;
        if (p != NULL && *(s8 *)(p + 0x630) != -1)
            func_09AD7C88(p, *(s8 *)(p + 0x630));
    }
    for (i = *(s8 *)(self + 0x21) - 1; i >= 0; i--)
        if ((D_09A4F060[D_08A62C9C[i] * 0x20 + 0x1E] & 1) == 0)
            spots_remove(self, D_08A62C9C[i]);
}

void spots_list_add(u8 *self, u32 idx)
{
    D_08A62C9C[*(s8 *)(self + 0x21)] = idx;
    self[0x21]++;
}

/* random item of the gathering table id */
u32 spots_item(u8 *self, u32 id)
{
    return func_0887265C(D_09CD7EB0[id & 0xFFFF], 0);
}

/* --- monsters of the quest --------------------------------------------------------- */
void quest_reset_monsters(u8 *self, u32 id)
{
    u8 *m;

    if (quest_find_entry(self, id, 0, *(s8 *)(self + 0x20)) == NULL)
        return;
    m = quest_find_entry(self, id, 1, *(s8 *)(self + 0x20));
    for (; S16(m, 0) >= 0; m += 0x3C) {
        if ((S16(m, 0x2E) & 4) != 0) {
            S16(m, 0x2E) = 2;
            S16(m, 0x32) = -1;
        }
    }
}

/* when all the monsters of the list are done, moves to the next list (-2) */
void quest_check_monsters_done(u8 *self, u32 id)
{
    u8 *a, *m;
    s32 left = 0;

    a = quest_find_entry(self, id, 0, *(s8 *)(self + 0x20));
    if (a == NULL)
        return;
    m = quest_find_entry(self, id, 1, *(s8 *)(self + 0x20));
    for (; S16(m, 0) >= 0; m += 0x3C)
        if ((S16(m, 0x2E) & 5) == 0)
            left = 1;
    if (left == 0 && S16(m, 0) == -2) {
        quest_set_entry_offset(self, (u8 *)U32(self, 0x718), id, m + 0x3C, 1);
        quest_set_entry_offset(self, (u8 *)U32(self, 0x718), id, a + 0x10, 0);
    }
}

void quest_set_entry_offset(u8 *self, u8 *tbl, u32 id, u8 *p, s32 which)
{
    u32 t = *(u32 *)(tbl + *(s8 *)(self + 0x20) * 4);
    u32 *e;

    if (t == 0 || U16(D_089CC438, 0x6AF12) == 0)
        return;
    for (e = (u32 *)(t + U32(self, 0x50)); e[0] != 0; e += 4) {
        if (e[0] == (id & 0xFFFF)) {
            if (e[which + 2] + 1 < 2)
                return;
            e[which + 2] = (u32)p - U32(self, 0x50);
            return;
        }
    }
}

void quest_init_monsters(u8 *self)
{
    s32 n = 0;

    if (U32(self, 0x71C) != 0)
        quest_init_main_monsters(self, (u8 *)U32(self, 0x71C), &n);
    if (U32(self, 0x718) != 0)
        quest_init_sub_monsters(self, (u32 *)U32(self, 0x718), &n);
}

/* random size of a large monster (percent), limited by the rank of the quest */
u32 quest_random_size(u8 *self)
{
    const u8 *t = D_089A8148[D_089A8164[quest_rand(self) & 0xFFFF & 0x1F]];
    u32 v = t[quest_rand(self) & 0xFFFF & 0x1F];
    s32 i = *(s8 *)(U32(self, 0x4C) + 0x4A) * 2;

    if (v < D_08A62C80[i])
        v = D_08A62C80[i];
    if (D_08A62C80[i + 1] < (v & 0xFFFF))
        v = D_08A62C80[i + 1];
    return v & 0xFFFF;
}

void quest_monster_init(u8 *self, u8 *m, s32 *counter)
{
    S16(m, 0x2E) = 2;
    S16(m, 0x32) = -1;
    m[0x34] = 0xFF;
    U16(m, 0x2C) = *counter;
    m[0x35] = (D_09BB89B0[S16(m, 0)][0x5C] >> 2) & 1;
    (*counter)++;
}

/* 1 for the large monsters */
static u32 is_large(s32 id)
{
    switch ((s16)id) {
    case 0x01: case 0x06: case 0x08: case 0x0B: case 0x0E: case 0x0F: case 0x11: case 0x14:
    case 0x15: case 0x16: case 0x1A: case 0x1B: case 0x1C: case 0x1F: case 0x21: case 0x25:
    case 0x26: case 0x27: case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2C: case 0x2D:
    case 0x2E: case 0x2F: case 0x30: case 0x31: case 0x33: case 0x34: case 0x35: case 0x36:
    case 0x3B: case 0x3C: case 0x40: case 0x41: case 0x43: case 0x44: case 0x4B: case 0x4D:
    case 0x4E: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
    case 0x59:
        return 1;
    }
    return 0;
}

u32 monster_is_large(u8 *self, s32 id)
{
    return is_large(id);
}

u32 monster_is_large2(u8 *self, s32 id)
{
    return is_large(id);
}

/* main monsters: entries {.., .., n, offset}, monster lists of 0x3C bytes ended by -1 */
void quest_init_main_monsters(u8 *self, u8 *l, s32 *counter)
{
    s32 i;
    u8 *m;

    for (; U32(l, 8) != 0; l += 0x10) {
        if (U32(l, 0xC) == 0)
            continue;
        i = 0;
        for (m = (u8 *)(U32(l, 0xC) + U32(self, 0x50)); S16(m, 0) != -1; m += 0x3C) {
            quest_monster_init(self, m, counter);
            quest_wave_add(self, i, (u16 *)m);
            i++;
            if ((u8)monster_is_large(self, S16(m, 0)) == 1 && func_08851144(D_089CC438) == 0 &&
                func_08874124(D_08A62CB4, U16(D_089CC438, 0x6AF12)) == 1)
                S16(m, 0x30) = S16((u8 *)U32(self, 0x4C), 0x38) * (s32)(quest_random_size(self) & 0xFFFF) / 100;
            else
                S16(m, 0x30) = 100;
        }
    }
}

void quest_init_sub_monsters(u8 *self, u32 *t, s32 *counter)
{
    u8 *l, *m;

    for (; *t != 0; t++) {
        l = (u8 *)*t;
        if (U16(D_089CC438, 0x6AF12) != 0)
            l += U32(self, 0x50);
        for (; U32(l, 8) != 0; l += 0x10) {
            if (U32(l, 0xC) == 0)
                continue;
            for (m = (u8 *)(U32(l, 0xC) + U32(self, 0x50)); S16(m, 0) != -1; m += 0x3C) {
                quest_monster_init(self, m, counter);
                S16(m, 0x30) = 100;
            }
        }
    }
}

/* quest entry of the monster h */
u8 *quest_find_monster(u8 *self, u8 *h)
{
    s32 i;
    u8 *a, *m, *l;

    if (*(s8 *)(self + 0x20) >= 0) {
        for (i = 0; i <= *(s8 *)(self + 0x20); i++) {
            a = quest_find_entry(self, U16(h, 0x29A), 0, (s8)i);
            m = quest_find_entry(self, U16(h, 0x29A), 1, (s8)i);
            if (a == NULL)
                continue;
            for (; S16(m, 0) >= 0; m += 0x3C)
                if (U16(h, 0x1E6) == *(s8 *)(m + 0x34) && S16(h, 0x4AC) == S16(m, 0x2C))
                    return m;
        }
    }
    for (l = (u8 *)U32(self, 0x71C); U32(l, 8) != 0; l += 0x10) {
        if (U32(l, 0xC) == 0)
            continue;
        for (m = (u8 *)(U32(l, 0xC) + U32(self, 0x50)); S16(m, 0) != -1; m += 0x3C)
            if (S16(h, 0x4AC) == S16(m, 0x2C))
                return m;
    }
    return NULL;
}
