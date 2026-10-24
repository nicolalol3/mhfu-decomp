/* Quest (D_08A62C7C): the 6 slots of Quest.slot, "N left" messages of the hunts, counts of
 * the monsters, bonus rewards (gathering in the area, Felyne comrade) and unlock checks.
 * (Guessed names.) */
#include "quest.h"

extern void *D_09A0B540;
extern const u16 D_089AC5A0[];      /* IDs of the quest messages */
extern const s32 D_089A853C[][5];
extern const u8 D_0899D19C[];      /* item table (+4: flags) */
extern const u16 D_09CD8F7C[][26];
extern const u8 D_09BB89B0[][0x1D0];
extern const u32 D_0893162C[6], D_08931644[6];
extern const u16 D_089A8624[38][3];
extern const u16 D_089A857C[28][3];
extern const s16 D_089A8564[];

const char *text_menu(void *g, u32 id);
u8 menu_msg_push_text(u8 *self, const char *s);
void *memcpy(void *d, const void *s, size_t n);
u8 *friend_marked(u8 *g);
s32 temp_alloc(void *a, int b, s32 c);
void temp_free(void *a, int b);
int game_test_flag(u8 *g, u32 f);
int game_test_flag2(u8 *g, u32 f);
void game_set_flag(u8 *g, u32 f);
int game_has_progress(u8 *g, u32 f);
s32 flag_id_at(u8 *g, s32 i);

/* Quest.slot[i]: bit 7 = set, bits 0-6 = value */
void quest_slot_set(Quest *self, s32 i, u32 v)
{
    u8 *p = self->slot + i;

    *p = (*p & 0x7F) | 0x80;
    *p = (*p & 0x80) | (v & 0x7F);
}

int quest_slot_is_set(Quest *self, s32 i)
{
    return *(s8 *)(self->slot + i) < 0;
}

void quest_slot_clear(Quest *self, s32 i)
{
    u8 *p = self->slot + i;

    *p &= 0x7F;
    *p = (*p & 0x80) | 0x7F;
}

u8 quest_slot_get(Quest *self, s32 i)
{
    return self->slot[i];
}

void quest_slots_copy(Quest *self, const u8 *s)
{
    s32 i;

    for (i = 0; i < 6; i++)
        self->slot[i] = s[i];
}

s32 quest_slot_find(Quest *self, u32 v)
{
    s32 i;

    for (i = 0; i < 6; i++)
        if ((self->slot[i] & 0x7F) == v)
            return i;
    return -1;
}

/* 1 when all the deliveries of the goals are done */
int quest_deliveries_done(Quest *self)
{
    s32 left = 0, d;
    u32 i;
    u16 id;
    s16 n;

    for (i = 0; (s32)i < 2; i++) {
        id = quest_goal_target(self, i);
        n = quest_goal_count(self, id, i & 0xFFFF);
        if (n >= 0) {
            d = n - quest_goal_have(self, id);
            if (left < d)
                left = d;
        }
    }
    return left <= 0;
}

/* size class of the monster h, from its quest entry (or the default of the quest) */
void quest_monster_size(Quest *self, u8 *h)
{
    QuestWave *w;
    QuestMonster *m;
    s32 i;

    if (U16(h, E_SLOT) < 2) {
        w = &self->wave[U16(h, E_SLOT)];
        for (i = 0; i < 5; i++) {
            m = w->mon[i];
            if (m != NULL && S16(h, E_QINDEX) == m->index) {
                h[E_SIZE_CLASS] = m->size_class;
                quest_pick_size(self, (s8 *)(h + E_SIZE_CLASS));
                return;
            }
        }
    }
    h[E_SIZE_CLASS] = self->size_class;
}

/* "N left" messages of the hunting goals (left: monsters left, max: monsters to hunt) */
void quest_hunt_message(Quest *self, u32 id, s32 left, s32 max)
{
    s32 few, k;

    if ((self->flags & QF_NO_HUNT_MSG) != 0)
        return;
    switch ((u8)id) {
    case 0x50: case 0x4F: case 0x57: case 0x56: case 0x43: case 0x30:
        few = 1;
        k = 0;
        break;
    case 0x49: case 0x42: case 0x39: case 0x38: case 0x18: case 0x17: case 0x13: case 9:
        few = 0;
        k = 0;
        break;
    case 0x46: case 0x45: case 0x3F: case 0x3E: case 0x3D: case 0x23: case 0x22: case 0x1E:
    case 0x19: case 0x10: case 0xD: case 0xC: case 5: case 4: case 3:
        few = 0;
        k = 1;
        break;
    default:
        few = 1;
        k = 1;
        break;
    }
    if (few) {
        /* "2 left", "1 left" */
        if (max > 2 && left < 3 && self->hunt_msg[1] == 0) {
            self->hunt_msg[1] = 1;
            menu_msg_push_text(D_089CBBEC, text_menu(D_089CC438, D_089AC5A0[D_089A853C[k][1]]));
        }
        if (max > 1 && left < 2 && self->hunt_msg[0] == 0) {
            self->hunt_msg[0] = 1;
            menu_msg_push_text(D_089CBBEC, text_menu(D_089CC438, D_089AC5A0[D_089A853C[k][0]]));
        }
    } else {
        /* "20 left", "10 left", "5 left" */
        if (max > 20 && left < 21 && self->hunt_msg[4] == 0) {
            self->hunt_msg[4] = 1;
            menu_msg_push_text(D_089CBBEC, text_menu(D_089CC438, D_089AC5A0[D_089A853C[k][4]]));
        }
        if (max > 10 && left < 11 && self->hunt_msg[3] == 0) {
            self->hunt_msg[3] = 1;
            menu_msg_push_text(D_089CBBEC, text_menu(D_089CC438, D_089AC5A0[D_089A853C[k][3]]));
        }
        if (max > 5 && left < 6 && self->hunt_msg[2] == 0) {
            self->hunt_msg[2] = 1;
            menu_msg_push_text(D_089CBBEC, text_menu(D_089CC438, D_089AC5A0[D_089A853C[k][2]]));
        }
    }
}

/* the Felyne comrade can come on this quest */
u32 quest_comrade_allowed(Quest *self)
{
    s8 m;

    if (friend_marked(D_089CC438) == NULL)
        return 0;
    if ((OPT_FLAGS & 1) != 0 || G_ARENA != 0)
        return 0;
    if ((self->flags & QF_TREASURE) != 0 || (self->flags & QF_TRAINING) != 0)
        return 0;
    m = self->wave[0].kind[0];
    if (m == 7 || m == 0x32 || m == 0x37 || m == 0x3A)
        return 0;
    return 1;
}

/* counts the monsters of each kind in the quest (each QuestMonster.index once) */
void quest_count_monsters(Quest *self, u16 *out)
{
    s32 i;

    for (i = 0; i < 16; i++)
        self->counted[i] = 0;
    for (i = 0; i < 90; i++)
        out[i] = 0;
    quest_count_monsters_main(self, self->large, out);
    quest_count_monsters_sub(self, self->phases, out);
}

static void count_list(Quest *self, QuestMonster *m, u16 *out)
{
    s32 idx;
    u32 *w;

    for (; m->kind != -1; m++) {
        idx = m->index;
        w = &self->counted[idx / 32];
        if ((*w & (1u << (idx & 31))) != 0)
            continue;
        out[m->kind] += m->count;
        idx = m->index;
        w = &self->counted[idx / 32];
        *w |= 1u << (idx & 31);
    }
}

void quest_count_monsters_main(Quest *self, QuestGroup *l, u16 *out)
{
    for (; l->list0 != 0; l++)
        if (l->list1 != 0)
            count_list(self, (QuestMonster *)QOFF(self, l->list1), out);
}

void quest_count_monsters_sub(Quest *self, u32 *t, u16 *out)
{
    QuestGroup *l;

    for (; *t != 0; t++) {
        l = (QuestGroup *)*t;
        if (G_QUEST_ID != 0)
            l = (QuestGroup *)((u8 *)l + (u32)self->base);
        for (; l->list0 != 0; l++)
            if (l->list1 != 0)
                count_list(self, (QuestMonster *)QOFF(self, l->list1), out);
    }
}

void quest_bonus_clear(Quest *self)
{
    s32 i;

    for (i = 0; i < 20; i++) {
        self->gathered[i] = NULL;
        self->gather_n[i] = 0;
    }
    self->gather_total = 0;
    for (i = 0; i < 90; i++)
        self->comrade_kills[i] = 0;
}

void quest_rewards_clear(void)
{
    s32 i;

    for (i = 0; i < 64; i++)
        G_REWARDS[i] = 0;
}

/* shuffles the n items and copies at most 8 of them to the reward list at dst */
static s32 shuffle_copy(u16 *buf, s32 n, u16 *dst)
{
    s32 i, a, b;
    u16 t;

    if (n > 0) {
        for (i = 0; i < n; i++) {
            a = (s32)(game_rand(D_08A631A0, 0) & 0xFFFF) % n;
            b = (s32)(game_rand(D_08A631A0, 0) & 0xFFFF) % n;
            t = buf[a];
            buf[a] = buf[b];
            buf[b] = t;
        }
    }
    if (n > 8)
        n = 8;
    for (i = 0; i < n; i++) {
        dst[i * 2] = buf[i];
        dst[i * 2 + 1] = 1;
    }
    return n;
}

/* bonus rewards of the gatherings done by the player (points in Quest.gathered) */
void quest_gather_rewards(Quest *self)
{
    u16 *buf, *p;
    s32 n = 0, i;
    GatherSpot *e;
    u32 it;

    if (self->gather_total == 0)
        return;
    buf = (u16 *)temp_alloc(D_09A0B540, 0x19, self->gather_total * 2);
    p = buf;
    for (i = 0; i < 20; ) {
        e = self->gathered[i];
        if (e == NULL)
            break;
        it = quest_table_item(self, e->table & 0x7FFF) & 0xFFFF;
        if (it != 0 && (D_0899D19C[it * 0x18] & 6) == 0) {
            *p++ = it;
            n++;
        }
        if (self->gather_n[i] != 0)
            self->gather_n[i]--;
        if (self->gather_n[i] == 0)
            i++;
    }
    n = shuffle_copy(buf, n, G_REWARDS);
    temp_free(D_09A0B540, 0x19);
    self->bonus_items += (s8)n;
}

/* bonus rewards brought by the Felyne comrade (monsters hunted, by comrade level) */
void quest_comrade_rewards(Quest *self)
{
    u16 *buf = (u16 *)temp_alloc(D_09A0B540, 0x19, 0xB4);
    u16 *p = buf;
    s32 k = QUEST->file->kind;
    s32 lv, i, n = 0;
    s16 pts;
    u32 id, it, ch;

    if (k == 1)
        k = 0;
    else if (k == 2)
        k = 1;
    else if (k == 5)
        k = 2;
    pts = S16(friend_marked(D_089CC438), 0x2A);
    if (pts < 1000)
        lv = 0;
    else if (pts < 2500)
        lv = 1;
    else if (pts < 4500)
        lv = 2;
    else if (pts < 6000)
        lv = 3;
    else if (pts < 7000)
        lv = 4;
    else
        lv = 5;
    for (i = 0; i < 90; i++) {
        if (i <= 0 || self->comrade_kills[i] == 0)
            continue;
        if (i == 2)
            id = k == 2 ? 0x3EF : k == 1 ? 0x430 : 0x3EF;
        else if (i == 0x47)
            id = k == 2 ? 0x3F1 : k == 1 ? 0x432 : 0x417;
        else if (i == 0x24)
            id = k == 2 ? 0x3F0 : k == 1 ? 0x431 : 0x3F0;
        else
            id = D_09CD8F7C[i][k];
        if ((id & 0xFFFF) == 0xFFFF)
            continue;
        it = spots_item(self, id & 0x7FFF) & 0xFFFF;
        if (it == 0 || (D_0899D19C[it * 0x18] & 6) != 0)
            continue;
        ch = ((D_09BB89B0[i][0x5C] >> 2) & 1) != 0 ? D_0893162C[lv] : D_08931644[lv];
        if ((game_rand(D_08A631A0, 0) & 0xFFFF) % 100 < ch) {
            *p++ = it;
            n++;
        }
    }
    n = shuffle_copy(buf, n, &G_REWARDS[0x10]);
    temp_free(D_09A0B540, 0x19);
    self->bonus_items += (s8)n;
}

/* 1 when the monster can appear in the quest id (the unlocking flag is set) */
u32 quest_monster_unlocked(Quest *self, u32 id)
{
    s32 i;

    for (i = 0; i < 38; i++) {
        if (D_089A8624[i][2] != id)
            continue;
        if (D_089A8624[i][0] == 0x164)
            return 1;
        return game_test_flag(D_089CC438, D_089A8624[i][0]) != 0;
    }
    return 1;
}

u32 quest_is_special(Quest *self, s32 id)
{
    const s16 *p;

    for (p = D_089A8564; *p != -1; p++)
        if (*p == (s16)id)
            return 1;
    return 0;
}

/* copies the delivered items of the player to the game state */
void quest_save_deliveries(void)
{
    u8 *h = (u8 *)player_get(D_09A4F0C4, G_MY_SLOT);

    memcpy(D_089CC438 + 0x43C8, h + 0x568, 0x60);
}

u32 quest_any_unlock(Quest *self)
{
    s32 i;

    for (i = 0; i < 0x3D; i++)
        if (quest_unlock_check(self, i) != 0)
            return 1;
    return 0;
}

/* 1 when the unlock i becomes available now */
u32 quest_unlock_check(Quest *self, s32 i)
{
    s32 f;

    if (G_QUEST_ID == 0xC22 && quest_state(QUEST, 1) == 1)
        game_set_flag(D_089CC438, 0x164);
    f = flag_id_at(D_089CC438, (s8)i);
    if (f < 0)
        return 0;
    if (game_test_flag2(D_089CC438, f) != 0)
        return 0;
    if (f >= 0x2C && f < 0x33)
        return game_test_flag(D_089CC438, f + 0xB8) != 0;
    if (f == 0x33)
        return game_test_flag(D_089CC438, 0xED) != 0;
    if (f >= 0xB && f < 0x39) {
        for (i = 0; i < 38; i++)
            if (f == D_089A8624[i][1] && game_test_flag(D_089CC438, D_089A8624[i][0]) != 0)
                return 1;
        return 0;
    }
    if (f >= 0 && f < 0xB) {
        for (i = 0; i < 28; i++)
            if (f == D_089A857C[i][1] && game_has_progress(D_089CC438, D_089A857C[i][0]) != 0)
                return 1;
    }
    return 0;
}
