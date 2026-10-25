/* End of the quest (object g_result): size records, items sold at the end, timers of
 * the result screen. (Guessed names.) */
#include "quest.h"
#include "result.h"
#undef pouch_slot

extern u8 *g_result;
extern u8 *g_sound;
extern const u8 D_0899D19C[];      /* item table (+4: flags) */
extern const s32 D_0899D1A8[];     /* item table (+0xC: price), 6 words per item */

u32 game_record_max(u8 *g, u32 i);
u32 game_record_min(u8 *g, u32 i);
void game_update_record_max(u8 *g, u32 i, u32 v);
void game_update_record_min(u8 *g, u32 i, u32 v);
u32 game_best_record_max(u8 *g, u32 i, s32 a);
u32 game_best_record_min(u8 *g, u32 i, s32 a);
f32 game_size_percent(u8 *g, u32 i, u32 v);
void record_add_best(u8 *g, u8 *log, s32 kind, s32 i, u32 v);
u8 *emblem_find(void *menu, u32 i);
u32 game_table_byte(u8 *g, u32 i);
u16 *pouch_slot(u8 *g, s32 i);
u32 hunter_add_item(u32 h, s32 id, s32 n, s32 c);
u8 hunter_felyne_skill(u32 h, int a);
void func_08882C24(u8 *snd, int a, int b, int c);
u8 func_0887CA08(u8 *self);

void result_init_sounds(u8 *self);

static void add_record(u8 *self, u32 i, u32 v, u32 kind)
{
    s32 j;
    u8 *e;

    for (j = 0; j < 10; j++) {
        e = self + RES_RECORDS + j * 4;
        if (e[0] != 0)
            continue;
        e[0] = i;
        U16(e, 2) = v;
        e[1] = kind;
        U32(self, RES_RECORD_N)++;
        return;
    }
}

/* new size records of the monsters (the largest / smallest ever hunted) */
void result_size_records(u8 *self)
{
    u16 mx[90], mn[90];
    u8 own[90];
    Quest *q = QUEST;
    u8 *em;
    s32 i, j;
    u32 g, v, w;
    int ch_max, ch_min;

    for (i = 1; i < 90; i++) {
        mx[i] = (u16)game_record_max(g_game, i & 0xFFFF);
        mn[i] = (u16)game_record_min(g_game, i & 0xFFFF);
    }
    /* the variants count in the record of their base monster */
    for (i = 1; i < 90; i++) {
        g = game_table_byte(g_game, i & 0xFF) & 0xFF;
        if (g == (u32)(i & 0xFF)) {
            own[i] = 1;
            continue;
        }
        if (mx[g] < mx[i])
            mx[g] = mx[i];
        if (mn[g] == 0)
            mn[g] = mn[i];
        else if (mn[i] != 0 && mn[i] < mn[g])
            mn[g] = mn[i];
        own[i] = 0;
    }
    /* sizes of this quest */
    for (i = 1; i < 90; i++) {
        v = q->size_max[i];
        if (v != 0 && (u16)game_record_max(g_game, i & 0xFFFF) < v)
            game_update_record_max(g_game, i & 0xFFFF, v);
        w = q->size_min[i];
        if (w != 0 && (w < (u16)game_record_min(g_game, i & 0xFFFF) ||
                       (u16)game_record_min(g_game, i & 0xFFFF) == 0))
            game_update_record_min(g_game, i & 0xFFFF, q->size_min[i]);
    }
    U32(self, RES_RECORD_N) = 0;
    for (j = 0; j < 10; j++) {
        U16(self, RES_RECORDS + 2 + j * 4) = 0;
        self[RES_RECORDS + 1 + j * 4] = 0;
        self[RES_RECORDS + j * 4] = 0;
    }
    for (i = 1; i < 90; i++) {
        if (own[i] == 0)
            continue;
        ch_max = mx[i] != (u16)game_best_record_max(g_game, i & 0xFF, 0);
        v = mn[i];
        ch_min = mn[i] != (u16)game_best_record_min(g_game, i & 0xFF, 0);
        if (!ch_max && !ch_min)
            continue;
        if (ch_max && ch_min) {
            u16 a = (u16)game_best_record_max(g_game, i & 0xFF, 0);

            if (a == (u16)game_best_record_min(g_game, i & 0xFF, 0)) {
                add_record(self, i, mx[i], 0);
            } else {
                add_record(self, i, mx[i], 1);
                add_record(self, i, mn[i], 2);
            }
        } else if (ch_max) {
            add_record(self, i, mx[i], 1);
        } else {
            add_record(self, i, v, 2);
        }
        em = emblem_find(g_menu, i & 0xFF);
        if (ch_max) {
            u16 b = (u16)game_best_record_max(g_game, i & 0xFF, 0);

            if (S16(em, 4) <= (s32)b)
                record_add_best(g_game, QUEST->log, 8, i,
                                (u32)((game_size_percent(g_game, i & 0xFF, b) + 0.05f) * 10.0f));
        }
        if (ch_min) {
            u16 b = (u16)game_best_record_min(g_game, i & 0xFF, 0);

            if ((s32)b <= S16(em, 8))
                record_add_best(g_game, QUEST->log, 9, i,
                                (u32)((game_size_percent(g_game, i & 0xFF, b) + 0.05f) * 10.0f));
        }
    }
}

void result_init_timers(u8 *self)
{
    U16(self, RES_STEP_TIMER) = 0x5A;
    U16(self, RES_TIMER) = 1;
    U16(self, RES_XAC) = 900;
    result_init_sounds(self);
}

/* the items to sell at the end of the quest (flag 2) are taken from the pouch;
 * returns their total price */
s32 result_sell_items(u8 *self)
{
    u32 h = player_get(g_players, G_MY_SLOT);
    u16 *p, *o;
    s32 i, sum = 0;

    self[RES_HAS_SOLD] = 0;
    for (i = 0; i < 60; i++)
        U16(self, RES_SOLD + i * 2) = 0;
    p = pouch_slot(g_game, 0);
    U32(self, RES_SOLD_N) = 0;
    o = &U16(self, RES_SOLD);
    for (i = 0; i < 24; i++, p += 2) {
        if (p[0] == 0 || (D_0899D19C[p[0] * 0x18] & 2) == 0)
            continue;
        o[0] = p[0];
        o[1] = p[1];
        hunter_add_item(h, p[0], (s16)-p[1], 0);
        self[RES_HAS_SOLD] = 1;
        o += 2;
        U32(self, RES_SOLD_N)++;
    }
    o = &U16(self, RES_SOLD);
    for (i = 0; i < (s32)U32(self, RES_SOLD_N); i++, o += 2) {
        if (o[0] == 0)
            return sum;
        sum += (s16)o[1] * D_0899D1A8[o[0] * 6];
    }
    return sum;
}

void result_init_sounds(u8 *self)
{
    func_08882C24(g_sound, 0, 4, 0x46);
    self[RES_SND1] = 1;
    self[RES_SND2] = 0;
    self[RES_SND3] = 4;
}

/* reward money of the quest (doubled by a skill, plus the contract fee back) */
void result_init_money(u8 *self)
{
    Quest *q = QUEST;
    u32 h;

    U16(self, RES_STEP) = 0;
    U16(self, RES_XA6) = 0;
    U16(self, RES_TIMER) = 3600;
    U32(g_result, RES_VALUE) = 0;
    U16(self, RES_STEP_TIMER) = 10;
    if (quest_state(QUEST, 1) != 1)
        return;
    h = player_get(g_players, G_MY_SLOT);
    if (hunter_felyne_skill(h, 0x31) == 0)
        U32(g_result, RES_VALUE) = q->reward;
    else
        U32(g_result, RES_VALUE) = q->reward << 1;
    if ((OPT_FLAGS & 1) == 0 || (u8)func_0887CA08(self) == 1)
        U32(g_result, RES_VALUE) += QUEST->info->fee * 2;
}
