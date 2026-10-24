/* Quest (D_08A62C7C): flags of the 6 slots at 0x75C, hunting messages, counts of the
 * monsters, bonus rewards (gathering in the area, Felyne comrade) and unlock checks.
 * (Guessed names.) */
#include "menu.h"

extern void *D_09A0B540;
extern const u16 D_089AC5A0[];
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
u16 quest_goal_target(u8 *self, s32 i);
s16 quest_goal_count(u8 *self, s32 id, u32 i);
s16 quest_goal_have(u8 *self, s32 id);
void quest_pick_size(u8 *self, s8 *p);
s32 quest_state(u8 *self, s32 mode);
u32 func_08870AC4(u8 *self, u32 i);
u32 spots_item(u8 *self, u32 id);
u8 *friend_marked(u8 *g);
u32 func_088900C4(void *a, int b);
s32 func_08898694(void *a, int b, s32 c);
void func_08898748(void *a, int b);
int game_test_flag(u8 *g, u32 f);
int game_test_flag2(u8 *g, u32 f);
void game_set_flag(u8 *g, u32 f);
int game_has_progress(u8 *g, u32 f);
s32 flag_id_at(u8 *g, s32 i);

void quest_count_monsters_main(u8 *self, u8 *l, u16 *out);
void quest_count_monsters_sub(u8 *self, u32 *t, u16 *out);
u32 quest_unlock_check(u8 *self, s32 i);

/* the 6 bytes at 0x75C: bit 7 = set, bits 0-6 = value */
void quest_slot_set(u8 *self, s32 i, u32 v)
{
    u8 *p = self + i + 0x75C;

    *p = (*p & 0x7F) | 0x80;
    *p = (*p & 0x80) | (v & 0x7F);
}

int quest_slot_is_set(u8 *self, s32 i)
{
    return *(s8 *)(self + i + 0x75C) < 0;
}

void quest_slot_clear(u8 *self, s32 i)
{
    u8 *p = self + i + 0x75C;

    *p &= 0x7F;
    *p = (*p & 0x80) | 0x7F;
}

u8 quest_slot_get(u8 *self, s32 i)
{
    return self[i + 0x75C];
}

void quest_slots_copy(u8 *self, const u8 *s)
{
    s32 i;

    for (i = 0; i < 6; i++)
        self[0x75C + i] = s[i];
}

s32 quest_slot_find(u8 *self, u32 v)
{
    s32 i;

    for (i = 0; i < 6; i++)
        if ((self[0x75C + i] & 0x7F) == v)
            return i;
    return -1;
}

/* 1 when all the deliveries of the goals are done */
int quest_deliveries_done(u8 *self)
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

/* size of the monster h, from its quest entry (or the default of the quest) */
void quest_monster_size(u8 *self, u8 *h)
{
    u8 *w, *m;
    s32 i;

    if (U16(h, 0x1E6) < 2) {
        w = self + U16(h, 0x1E6) * 0x2C;
        for (i = 0; i < 5; i++) {
            m = (u8 *)U32(w, 0x768 + i * 4);
            if (m != NULL && S16(h, 0x4AC) == S16(m, 0x2C)) {
                h[0x4BC] = m[5];
                quest_pick_size(self, (s8 *)(h + 0x4BC));
                return;
            }
        }
    }
    h[0x4BC] = self[0x7C0];
}

/* "N left" messages of the hunting goals (left: monsters left, max: monsters to hunt) */
void quest_hunt_message(u8 *self, u32 id, s32 left, s32 max)
{
    s32 few, k;

    if ((U32(self, 0x28) & 0x400000) != 0)
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
        if (max > 2 && left < 3 && self[0x763] == 0) {
            self[0x763] = 1;
            menu_msg_push_text(D_089CBBEC, text_menu(D_089CC438, D_089AC5A0[D_089A853C[k][1]]));
        }
        if (max > 1 && left < 2 && self[0x762] == 0) {
            self[0x762] = 1;
            menu_msg_push_text(D_089CBBEC, text_menu(D_089CC438, D_089AC5A0[D_089A853C[k][0]]));
        }
    } else {
        if (max > 20 && left < 21 && self[0x766] == 0) {
            self[0x766] = 1;
            menu_msg_push_text(D_089CBBEC, text_menu(D_089CC438, D_089AC5A0[D_089A853C[k][4]]));
        }
        if (max > 10 && left < 11 && self[0x765] == 0) {
            self[0x765] = 1;
            menu_msg_push_text(D_089CBBEC, text_menu(D_089CC438, D_089AC5A0[D_089A853C[k][3]]));
        }
        if (max > 5 && left < 6 && self[0x764] == 0) {
            self[0x764] = 1;
            menu_msg_push_text(D_089CBBEC, text_menu(D_089CC438, D_089AC5A0[D_089A853C[k][2]]));
        }
    }
}

/* the Felyne comrade can come on this quest */
u32 quest_comrade_allowed(u8 *self)
{
    s8 m;

    if (friend_marked(D_089CC438) == NULL)
        return 0;
    if ((*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) != 0 || D_089CC438[0x480] != 0)
        return 0;
    if ((U32(self, 0x28) & 0x10) != 0 || (U32(self, 0x28) & 0x1000000) != 0)
        return 0;
    m = self[0x77C];
    if (m == 7 || m == 0x32 || m == 0x37 || m == 0x3A)
        return 0;
    return 1;
}

/* counts the monsters of each kind in the quest (each index once) */
void quest_count_monsters(u8 *self, u16 *out)
{
    s32 i;

    for (i = 0; i < 16; i++)
        U32(self, 0x56C + i * 4) = 0;
    for (i = 0; i < 90; i++)
        out[i] = 0;
    quest_count_monsters_main(self, (u8 *)U32(self, 0x71C), out);
    quest_count_monsters_sub(self, (u32 *)U32(self, 0x718), out);
}

static void count_list(u8 *self, u8 *m, u16 *out)
{
    s32 idx;
    u32 *w;

    for (; S16(m, 0) != -1; m += 0x3C) {
        idx = S16(m, 0x2C);
        w = (u32 *)(self + 0x56C + (idx / 32) * 4);
        if ((*w & (1u << (idx & 31))) != 0)
            continue;
        out[S16(m, 0)] += *(s8 *)(m + 4);
        idx = S16(m, 0x2C);
        w = (u32 *)(self + 0x56C + (idx / 32) * 4);
        *w |= 1u << (idx & 31);
    }
}

void quest_count_monsters_main(u8 *self, u8 *l, u16 *out)
{
    for (; U32(l, 8) != 0; l += 0x10)
        if (U32(l, 0xC) != 0)
            count_list(self, (u8 *)(U32(l, 0xC) + U32(self, 0x50)), out);
}

void quest_count_monsters_sub(u8 *self, u32 *t, u16 *out)
{
    u8 *l;

    for (; *t != 0; t++) {
        l = (u8 *)*t;
        if (U16(D_089CC438, 0x6AF12) != 0)
            l += U32(self, 0x50);
        for (; U32(l, 8) != 0; l += 0x10)
            if (U32(l, 0xC) != 0)
                count_list(self, (u8 *)(U32(l, 0xC) + U32(self, 0x50)), out);
    }
}

void quest_bonus_clear(u8 *self)
{
    s32 i;

    for (i = 0; i < 20; i++) {
        U32(self, 0x828 + i * 4) = 0;
        U16(self, 0x878 + i * 2) = 0;
    }
    U32(self, 0x8A0) = 0;
    for (i = 0; i < 90; i++)
        self[0x7CC + i] = 0;
}

void quest_rewards_clear(void)
{
    s32 i;

    for (i = 0; i < 64; i++)
        U16(D_089CC438, 0x390 + i * 2) = 0;
}

/* shuffles the n items and copies at most 8 of them to the reward list at dst */
static s32 shuffle_copy(u16 *buf, s32 n, u16 *dst)
{
    s32 i, a, b;
    u16 t;

    if (n > 0) {
        for (i = 0; i < n; i++) {
            a = (s32)(func_088900C4(D_08A631A0, 0) & 0xFFFF) % n;
            b = (s32)(func_088900C4(D_08A631A0, 0) & 0xFFFF) % n;
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

/* bonus rewards of the gatherings done by the hunter (tables at 0x828, counts at 0x878) */
void quest_gather_rewards(u8 *self)
{
    u16 *buf, *p;
    s32 n = 0, i;
    u8 *e;
    u32 it;

    if (U32(self, 0x8A0) == 0)
        return;
    buf = (u16 *)func_08898694(D_09A0B540, 0x19, U32(self, 0x8A0) * 2);
    p = buf;
    for (i = 0; i < 20; ) {
        e = (u8 *)U32(self, 0x828 + i * 4);
        if (e == NULL)
            break;
        it = func_08870AC4(self, U16(e, 0x10) & 0x7FFF) & 0xFFFF;
        if (it != 0 && (D_0899D19C[it * 0x18] & 6) == 0) {
            *p++ = it;
            n++;
        }
        if (U16(self, 0x878 + i * 2) != 0)
            U16(self, 0x878 + i * 2)--;
        if (U16(self, 0x878 + i * 2) == 0)
            i++;
    }
    n = shuffle_copy(buf, n, &U16(D_089CC438, 0x390));
    func_08898748(D_09A0B540, 0x19);
    self[0x680] += (s8)n;
}

/* bonus rewards brought by the Felyne comrade (monsters hunted, by comrade level) */
void quest_comrade_rewards(u8 *self)
{
    u16 *buf = (u16 *)func_08898694(D_09A0B540, 0x19, 0xB4);
    u16 *p = buf;
    s32 k = *(s8 *)(U32(D_08A62C7C, 0x4C) + 0x44);
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
        if (i <= 0 || self[0x7CC + i] == 0)
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
        if ((func_088900C4(D_08A631A0, 0) & 0xFFFF) % 100 < ch) {
            *p++ = it;
            n++;
        }
    }
    n = shuffle_copy(buf, n, &U16(D_089CC438, 0x3B0));
    func_08898748(D_09A0B540, 0x19);
    self[0x680] += (s8)n;
}

/* 1 when the monster can appear in the quest id (the unlocking flag is set) */
u32 quest_monster_unlocked(u8 *self, u32 id)
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

u32 quest_is_special(u8 *self, s32 id)
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
    u8 *h = (u8 *)func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]);

    memcpy(D_089CC438 + 0x43C8, h + 0x568, 0x60);
}

u32 quest_any_unlock(u8 *self)
{
    s32 i;

    for (i = 0; i < 0x3D; i++)
        if (quest_unlock_check(self, i) != 0)
            return 1;
    return 0;
}

/* 1 when the unlock i becomes available now */
u32 quest_unlock_check(u8 *self, s32 i)
{
    s32 f;

    if (U16(D_089CC438, 0x6AF12) == 0xC22 && quest_state(D_08A62C7C, 1) == 1)
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
