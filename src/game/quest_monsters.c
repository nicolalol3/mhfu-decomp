/* Quest (D_08A62C7C): items given by the quest, items delivered by the hunters, gathering
 * points of the area (20 GatherSpot after the area object D_09A4F060) and the monsters
 * placed by the quest file. (Guessed names.) */
#include "quest.h"

extern u8 *D_09A4F060;             /* area: +4 = gathering points */
extern s8 D_08A62C9C[20];          /* active gathering points, in order of creation */
extern u8 D_08A62C80[];            /* min / max monster size for each size rank */
extern const s8 D_089A77BC[8];
extern const u8 *D_089A8148[];
extern const u8 D_089A8164[32];
extern const u8 D_09BB89B0[][0x1D0];   /* monster data */
extern const u32 D_09CD7EB0[];
extern u8 *D_09C121B0;
extern void *D_09A1DAEC;
extern u8 *D_08A62CB4;

u8 menu_msg_push(u8 *self, u32 id, u32 x, u32 y);
u8 hunter_felyne_skill(u32 h, int a);
s32 hunter_has_skill(u32 h, int a);
u32 hunter_add_item(u8 *h, s32 id, s32 n, s32 c);
u32 hunter_deliver(u8 *h, u32 id, s32 n);
u16 *pouch_slot(u8 *g, s32 i);
void func_09C6F4B8(f32 scale, f32 *pos, int a);
void func_09AD7C88(u8 *a, s32 b);
s32 game_flag_10(u8 *g);

#define SPOT(i)     ((GatherSpot *)(D_09A4F060 + 4) + (i))
#define SPOT_LIST   D_08A62C9C
#define IS_PLAYER(h) (U16(h, E_SLOT) == G_MY_SLOT)
#define DELIVERED_ID(h, i)  U16(h, E_DELIVERED + (i) * 4)
#define DELIVERED_N(h, i)   S16(h, E_DELIVERED + 2 + (i) * 4)

void quest_item_remove(Quest *self, u32 id, s32 n)
{
    quest_item_add(self, id, (s16)-(s16)n);
    self->items_new = 1;
}

/* gives to the hunter h the items of the quest (with the messages for the player) */
void quest_give_items(Quest *self, u8 *h)
{
    s32 given = 0, i;
    s8 k;
    u32 r;
    QuestItem *e;

    if (self->items_new == 0) {
        if (IS_PLAYER(h))
            menu_msg_push(D_089CBBEC, 0, 0, 0);
        return;
    }
    k = D_089A77BC[game_rand(D_08A631A0, 0) & 0xFFFF & 7];
    if (hunter_felyne_skill((u32)h, 3) == 1)
        k = 0;
    if (k == 2) {
        if (IS_PLAYER(h))
            menu_msg_push(D_089CBBEC, 0, 0, 0);
    } else if (k == 1) {
        r = hunter_add_item(h, 0xF6, 1, 0) & 0xFFFF;
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
        for (i = 0, e = self->items; i < 5; i++, e++) {
            if (e->id == 0)
                continue;
            r = hunter_add_item(h, e->id, e->count, 0) & 0xFFFF;
            if (r == 3) {
                if (IS_PLAYER(h))
                    menu_msg_push(D_089CBBEC, 1, 3, (s16)e->id);
                given = 1;
            } else if (r == 2 || r == 1 || r == 0) {
                if (IS_PLAYER(h))
                    menu_msg_push(D_089CBBEC, 1, 0, (s16)e->id);
                given = 1;
            }
        }
        if (!given && IS_PLAYER(h))
            menu_msg_push(D_089CBBEC, 0, 0, 0);
    }
    self->items_new = 0;
    for (i = 0; i < 5; i++) {
        self->items[i].id = 0;
        self->items[i].count = 0;
    }
    self->item_next = 0;
    self->items_new = 0;
}

/* sets the count of a delivery item of the hunter h (0: no goal, 1: set, 2: goal reached) */
s32 quest_hunter_item_set(Quest *self, u8 *h, u32 id, s32 n)
{
    s16 max = 0, i, v, cur;

    for (i = 0; i < 2; i++) {
        v = quest_goal_count(QUEST, id, i & 0xFFFF);
        if (v >= 0) {
            v = v - quest_goal_have(QUEST, id);
            if (max < v)
                max = v;
        }
    }
    if (max <= 0)
        return 0;
    cur = 0;
    for (i = 0; i < 4; i++)
        if (DELIVERED_ID(h, i) == (id & 0xFFFF))
            cur = DELIVERED_N(h, i);
    if (cur != 0) {
        for (i = 0; i < 4; i++) {
            if (DELIVERED_ID(h, i) == (id & 0xFFFF)) {
                DELIVERED_N(h, i) = (s8)n;
                return (s16)n < max ? 1 : 2;
            }
        }
        return 1;
    }
    for (i = 0; i < 4; i++) {
        if (DELIVERED_ID(h, i) == 0) {
            DELIVERED_ID(h, i) = id;
            DELIVERED_N(h, i) = (s8)n;
            return (s16)n < max ? 0 : 2;
        }
    }
    return 0;
}

/* count of a delivery item: who 1 = player, 0 = all the hunters, 2.. = hunter who-2 */
s32 quest_hunter_item_get(Quest *self, u32 id, s32 who)
{
    s16 sum = 0;
    s32 p, i;
    u8 *h;

    if (who == 0) {
        for (p = 0; p < 4; p++) {
            h = (u8 *)player_get(D_09A4F0C4, p);
            if (h == NULL)
                continue;
            for (i = 0; i < 4; i++)
                if (DELIVERED_ID(h, i) == (id & 0xFFFF))
                    sum += DELIVERED_N(h, i);
        }
        return sum;
    }
    if (who == 1)
        h = (u8 *)player_get(D_09A4F0C4, G_MY_SLOT);
    else
        h = (u8 *)((u32 (*)(void *, s32))player_get)(D_09A4F0C4, who - 2);
    for (i = 0; i < 4; i++)
        if (DELIVERED_ID(h, i) == (id & 0xFFFF))
            return DELIVERED_N(h, i);
    return 0;
}

void quest_send_item(Quest *self, u32 id)
{
    QuestEvent ev;

    ev.arg[0] = id;
    ev.type = 9;
    ev.zero = 0;
    ev.arg[1] = quest_hunter_item_get(self, id, 1);
    net_send(D_09A1DAEC, 6, &ev);
    menu_msg_push(D_089CBBEC, 1, 8, (s16)id);
}

/* delivery goals: the hunter h puts the items of the pouch in the delivery box */
u32 quest_check_deliver(Quest *self, u8 *h)
{
    u32 r = 0;
    QuestGoal *g;
    s32 i, s;
    u16 *p, *q;

    if (quest_state(self, 1) != 0)
        return 0;
    for (i = 0, g = self->goal; i < 2; i++, g++) {
        if (((g->flags & GOAL_DONE) | (GOAL_ACTIVE | GOAL_DELIVER)) != (GOAL_ACTIVE | GOAL_DELIVER))
            continue;
        if (g->target == 0)
            continue;
        if (g->target == 0x3A0 && (self->flags & QF_TREASURE) != 0 && quest_points(self) < 2000)
            continue;
        for (s = 0; s < 0x18; s++) {
            u16 id = g->target;

            if (id != *pouch_slot(D_089CC438, s))
                continue;
            p = pouch_slot(D_089CC438, s);
            q = pouch_slot(D_089CC438, s);
            if ((hunter_deliver(h, p[0], (s16)q[1]) & 0xFFFF) != 0) {
                r = 1;
                quest_send_item(self, g->target);
            }
        }
    }
    return r;
}

/* --- gathering points ------------------------------------------------------------- */
void spots_init(Quest *self)
{
    s32 i;

    self->spot_count = 0;
    for (i = 0; i < 20; i++) {
        SPOT(i)->table = 0xFFFF;
        SPOT_LIST[i] = -1;
    }
}

/* timers of the points (with the sparkle effect every 64 frames) */
void spots_update(Quest *self)
{
    s32 i;
    s16 t;
    GatherSpot *e;
    f32 pos[3], k;

    for (i = 0; i < self->spot_count; i++) {
        e = SPOT(SPOT_LIST[i]);
        if (e->table == 0xFFFF)
            continue;
        if (e->timer == 0)
            continue;
        if (e->left == 0) {
            spots_remove(self, SPOT_LIST[i]);
            continue;
        }
        t = e->timer - 1;
        e->timer = t;
        if (t <= 0) {
            spots_remove(self, SPOT_LIST[i]);
            continue;
        }
        t = e->timer;
        if ((t & 0x3F) != 0)
            continue;
        pos[0] = e->pos[0];
        pos[1] = 20.0f + e->pos[1];
        pos[2] = e->pos[2];
        if (t < 900)
            k = 0.2f + 0.3f * ((f32)t / 900.0f);
        else
            k = 0.5f;
        func_09C6F4B8(k, pos, 0x15);
    }
}

/* new gathering point (copy of src, at pos when given); returns its index or -1 */
s32 spots_add(Quest *self, GatherSpot *src, f32 *pos)
{
    GatherSpot *e = SPOT(0);
    u32 h;
    s32 i, extra;

    spots_make_room(self);
    for (i = 0; i < 20; i++, e++) {
        if (e->table != 0xFFFF)
            continue;
        if (pos == NULL) {
            e->pos[0] = src->pos[0];
            e->pos[1] = src->pos[1];
            e->pos[2] = src->pos[2];
        } else {
            e->pos[0] = pos[0];
            e->pos[1] = pos[1];
            e->pos[2] = pos[2];
        }
        e->radius = src->radius;
        e->table = src->table;
        e->left = src->left;
        e->kind = src->kind;
        e->area = src->area;
        e->flags = src->flags;
        e->timer = src->timer;
        if (e->kind == 2) {
            /* skills that give more gatherings */
            h = player_get(D_09A4F0C4, G_MY_SLOT);
            if (hunter_has_skill(h, 0xB0) != 0)
                e->left++;
            extra = 0;
            if (hunter_felyne_skill(h, 1) != 0)
                extra = (game_rand(D_08A631A0, 0) & 0xFFFF & 1) == 0;
            if (hunter_felyne_skill(h, 2) != 0 && (game_rand(D_08A631A0, 0) & 0xFFFF & 3) == 0)
                extra = 1;
            if (extra)
                e->left++;
        }
        spots_list_add(self, (s8)i);
        return i;
    }
    return -1;
}

void spots_remove(Quest *self, s32 idx)
{
    if (idx == -1)
        return;
    SPOT(idx)->table = 0xFFFF;
    spots_list_remove(self, (s8)idx);
}

/* gathering point where the hunter h stands */
GatherSpot *spots_find(Quest *self, u8 *h)
{
    GatherSpot *e = SPOT(0);
    s32 i;
    f32 dx, dz;

    for (i = 0; i < 20; i++, e++) {
        if (e->table == 0xFFFF || e->area != U16(h, E_AREA))
            continue;
        if (!(F32(h, E_POS + 4) >= e->pos[1] - 200.0f))
            continue;
        if (!(F32(h, E_POS + 4) < e->pos[1] + 100.0f))
            continue;
        dx = F32(h, E_POS) - e->pos[0];
        dz = F32(h, E_POS + 8) - e->pos[2];
        if (!(e->radius * e->radius >= dx * dx + dz * dz))
            continue;
        return e;
    }
    return NULL;
}

u32 spots_kind(Quest *self, u8 *h)
{
    GatherSpot *e = spots_find(self, h);

    if (e == NULL)
        return 0xFFFF;
    return e->kind;
}

/* gathers at the point of h: item id, 0 (no point) or 0xFFFE (exhausted) */
u32 spots_gather(Quest *self, u8 *h)
{
    GatherSpot *e = spots_find(self, h);
    u32 r;

    if (e == NULL)
        return 0;
    if (e->left == 0)
        return 0xFFFE;
    r = spots_item(self, e->table & 0x7FFF);
    if (e->left != 0xFF)
        e->left--;
    return r & 0xFFFF;
}

/* with 20 points active, removes the oldest one that is not kept */
void spots_make_room(Quest *self)
{
    s32 n, i;

    if (self->spot_count < 20)
        return;
    n = self->spot_count - 1;
    for (i = 0; i < n; i++)
        if ((SPOT(SPOT_LIST[i])->flags & 1) == 0)
            break;
    spots_remove(QUEST, SPOT_LIST[i]);
}

void spots_list_remove_at(Quest *self, s32 i)
{
    i = (s8)i;
    for (; i < self->spot_count - 1; i++)
        SPOT_LIST[i] = SPOT_LIST[i + 1];
    SPOT_LIST[i] = -1;
    self->spot_count--;
}

void spots_list_remove(Quest *self, s32 idx)
{
    s32 i;

    for (i = 0; i < self->spot_count; i++) {
        if ((s8)idx == SPOT_LIST[i]) {
            spots_list_remove_at(self, (s8)i);
            return;
        }
    }
}

/* change of area: monsters notified, temporary points removed */
void spots_area_change(Quest *self)
{
    s32 i;
    u8 *p;

    for (i = 0; i < 20; i++) {
        p = (i >= 0 && i < 20) ? (u8 *)U32(D_09C121B0, 0x1220 + i * 4) : NULL;
        if (p != NULL && *(s8 *)(p + 0x630) != -1)
            func_09AD7C88(p, *(s8 *)(p + 0x630));
    }
    for (i = self->spot_count - 1; i >= 0; i--)
        if ((SPOT(SPOT_LIST[i])->flags & 1) == 0)
            spots_remove(self, SPOT_LIST[i]);
}

void spots_list_add(Quest *self, u32 idx)
{
    SPOT_LIST[self->spot_count] = idx;
    self->spot_count++;
}

/* random item of the gathering table id */
u32 spots_item(Quest *self, u32 id)
{
    return quest_table_rand((const u16 *)D_09CD7EB0[id & 0xFFFF], NULL);
}

/* --- monsters of the quest --------------------------------------------------------- */

/* the monsters of the area id that will come back are reset */
void quest_reset_monsters(Quest *self, u32 id)
{
    QuestMonster *m;

    if (quest_find_entry(self, id, 0, self->phase) == NULL)
        return;
    m = (QuestMonster *)quest_find_entry(self, id, 1, self->phase);
    for (; m->kind >= 0; m++) {
        if ((m->state & QM_RESPAWN) != 0) {
            m->state = QM_WAITING;
            m->life = -1;
        }
    }
}

/* when all the monsters of the list are done, moves to the next list (-2) */
void quest_check_monsters_done(Quest *self, u32 id)
{
    u8 *a;
    QuestMonster *m;
    s32 left = 0;

    a = quest_find_entry(self, id, 0, self->phase);
    if (a == NULL)
        return;
    m = (QuestMonster *)quest_find_entry(self, id, 1, self->phase);
    for (; m->kind >= 0; m++)
        if ((m->state & (QM_DONE | QM_RESPAWN)) == 0)
            left = 1;
    if (left == 0 && m->kind == -2) {
        quest_set_entry_offset(self, self->phases, id, (u8 *)(m + 1), 1);
        quest_set_entry_offset(self, self->phases, id, a + 0x10, 0);
    }
}

/* changes the list "which" of the area id in the table of the current phase */
void quest_set_entry_offset(Quest *self, u32 *tbl, u32 id, u8 *p, s32 which)
{
    u32 t = tbl[self->phase];
    QuestGroup *e;

    if (t == 0 || G_QUEST_ID == 0)
        return;
    for (e = (QuestGroup *)QOFF(self, t); e->id != 0; e++) {
        if (e->id == (id & 0xFFFF)) {
            if ((&e->list0)[which] + 1 < 2)
                return;
            (&e->list0)[which] = (u32)p - (u32)self->base;
            return;
        }
    }
}

void quest_init_monsters(Quest *self)
{
    s32 n = 0;

    if (self->large != NULL)
        quest_init_main_monsters(self, self->large, &n);
    if (self->phases != NULL)
        quest_init_sub_monsters(self, self->phases, &n);
}

/* random size of a large monster (percent), limited by the size rank of the quest */
u32 quest_random_size(Quest *self)
{
    const u8 *t = D_089A8148[D_089A8164[quest_rand(self) & 0xFFFF & 0x1F]];
    u32 v = t[quest_rand(self) & 0xFFFF & 0x1F];
    s32 i = self->file->size_rank * 2;

    if (v < D_08A62C80[i])
        v = D_08A62C80[i];
    if (D_08A62C80[i + 1] < (v & 0xFFFF))
        v = D_08A62C80[i + 1];
    return v & 0xFFFF;
}

void quest_monster_init(Quest *self, QuestMonster *m, s32 *counter)
{
    m->state = QM_WAITING;
    m->life = -1;
    m->slot = -1;
    m->index = *counter;
    m->large = (D_09BB89B0[m->kind][0x5C] >> 2) & 1;
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

u32 monster_is_large(Quest *self, s32 id)
{
    return is_large(id);
}

u32 monster_is_large2(Quest *self, s32 id)
{
    return is_large(id);
}

/* large monsters: each group gives a wave; a random size when the size can change */
void quest_init_main_monsters(Quest *self, QuestGroup *l, s32 *counter)
{
    s32 i;
    QuestMonster *m;

    for (; l->list0 != 0; l++) {
        if (l->list1 == 0)
            continue;
        i = 0;
        for (m = (QuestMonster *)QOFF(self, l->list1); m->kind != -1; m++) {
            quest_monster_init(self, m, counter);
            quest_wave_add(self, i, m);
            i++;
            if ((u8)monster_is_large(self, m->kind) == 1 && game_flag_10(D_089CC438) == 0 &&
                quest_monster_unlocked((Quest *)D_08A62CB4, G_QUEST_ID) == 1)
                m->size = self->file->size_base * (s32)(quest_random_size(self) & 0xFFFF) / 100;
            else
                m->size = 100;
        }
    }
}

/* monsters of the areas, for each phase */
void quest_init_sub_monsters(Quest *self, u32 *t, s32 *counter)
{
    QuestGroup *l;
    QuestMonster *m;

    for (; *t != 0; t++) {
        l = (QuestGroup *)*t;
        if (G_QUEST_ID != 0)
            l = (QuestGroup *)((u8 *)l + (u32)self->base);
        for (; l->list0 != 0; l++) {
            if (l->list1 == 0)
                continue;
            for (m = (QuestMonster *)QOFF(self, l->list1); m->kind != -1; m++) {
                quest_monster_init(self, m, counter);
                m->size = 100;
            }
        }
    }
}

/* quest entry of the monster h */
QuestMonster *quest_find_monster(Quest *self, u8 *h)
{
    s32 i;
    u8 *a;
    QuestMonster *m;
    QuestGroup *l;

    if (self->phase >= 0) {
        for (i = 0; i <= self->phase; i++) {
            a = quest_find_entry(self, U16(h, E_AREA), 0, (s8)i);
            m = (QuestMonster *)quest_find_entry(self, U16(h, E_AREA), 1, (s8)i);
            if (a == NULL)
                continue;
            for (; m->kind >= 0; m++)
                if (U16(h, E_SLOT) == m->slot && S16(h, E_QINDEX) == m->index)
                    return m;
        }
    }
    for (l = self->large; l->list0 != 0; l++) {
        if (l->list1 == 0)
            continue;
        for (m = (QuestMonster *)QOFF(self, l->list1); m->kind != -1; m++)
            if (S16(h, E_QINDEX) == m->index)
                return m;
    }
    return NULL;
}
