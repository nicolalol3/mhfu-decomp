/* The current quest (Quest, object D_08A62C7C): start of the quest, start area, targets,
 * waves of large monsters, end of the quest, bonuses and phase steps. (Guessed names.) */
#include "quest.h"
#include "vfpu.h"
#undef text_menu
#undef pouch_count
#undef quest_time
#undef quest_text

extern s32 D_08A5C464;              /* = QFILE->tables */
extern u8 D_08A62C8C[15];           /* random seeds received online */
extern u8 D_08A62C80[10];           /* min / max size for each size rank */
extern const u8 D_089A7EA4[10];
extern u8 *D_08A62CB4;
extern u8 *D_09C121B0;              /* monsters (20 slots at +0x1220) */
extern void *D_09A1DAEC;
extern void *D_08A62D90;
extern void **D_09A4F060;
extern u16 **D_09CD3CF0[];          /* start areas for each map (3 weighted lists) */
extern u16 *D_09CE470C[][2];
extern const u8 D_089A609C[0x172][16];   /* points of the areas: area, x, y, z */

u16 game_map_id(u8 *g);
int has_kitchen_skill(u8 *g, u32 k);
int game_rank(u8 *g);
int quest_flag_10(void);
u32 pouch_count(void *game, s16 id);
u16 func_088C722C(void *a, u32 b, u32 c);
void func_09A64E48(void *a, int b);
void func_08895EA0(void *a);
void *memcpy(void *d, const void *s, size_t n);

/* counts the quests done on each map */
void quest_count_kind(Quest *self)
{
    s32 k;
    u8 *c;

    switch (self->info->map) {
    case 1: k = 0xC; break;
    case 2: case 0x10: k = 0; break;
    case 3: case 0x11: k = 2; break;
    case 4: case 0x12: k = 3; break;
    case 5: case 0x13: k = 4; break;
    case 6: case 0x14: k = 1; break;
    case 0xB: case 0x15: k = 5; break;
    case 0xC: k = 0xD; break;
    case 0xD: case 0xE: case 0xF: k = 6; break;
    case 0x1A: k = 8; break;
    case 0x1B: k = 10; break;
    case 0x1C: k = 0xB; break;
    case 0x1D: k = 9; break;
    case 0x1E: case 0x1F: k = 7; break;
    default: return;
    }
    c = D_089CC438 + 0x6A53C + k;
    if (*c < 0xFF)
        (*c)++;
}

/* 1 when the hunter is not in the map of the quest (and the quest does not allow it) */
int quest_outside_map(Quest *self)
{
    u16 cur = G_AREA;

    if (game_map_id(D_089CC438) == cur)
        return 0;
    return (self->flags & QF_ANY_MAP) == 0;
}

/* data of the object id of the current map */
u8 *quest_map_object(Quest *self, u32 id)
{
    u32 *t = self->map_objects;
    u16 i = func_088C722C(D_09A4F060, G_MAP, id);

    if (i == 0xFFFF)
        return NULL;
    if (t[i] == 0)
        return NULL;
    return QOFF(self, t[i]);
}

/* moves pos to the last point of the current area within the radius in[3] */
void quest_snap_point(Quest *self, f32 *out, const f32 *in)
{
    f32 x = in[0], y = in[1], z = in[2];
    const u8 *e = D_089A609C[0];
    s32 n = 0;
    u32 i;

    for (i = 0; i < 0x172; i++, e += 16) {
        if (U16(e, 0) != G_AREA)
            continue;
        if (vfpu_dist3_sq(in, (const f32 *)(e + 4)) <= in[3] * in[3]) {
            out[0] = F32(e, 4);
            out[1] = F32(e, 8);
            out[2] = F32(e, 0xC);
            n++;
        }
    }
    if (n == 0) {
        out[0] = x;
        out[1] = y;
        out[2] = z;
    }
}

/* item table i of the quest file */
u8 *quest_table_entry(Quest *self, s32 i)
{
    return QOFF(self, *(s32 *)(D_08A5C464 + (u32)self->base + i * 4));
}

/* first group of large monsters: 0 = their kinds (s32, -1 = end), 1 = their QuestMonster */
u8 *quest_large_list(Quest *self, s32 which)
{
    QuestGroup *t = self->large;
    u32 off = 0;

    if (which == 0)
        off = t->list0;
    else if (which == 1)
        off = t->list1;
    if (G_QUEST_ID == 0)
        return (u8 *)off;
    if (off == 0)
        return NULL;
    return QOFF(self, off);
}

/* area id in the table of the phase idx: 0 = kinds of its small monsters, 1 = its
 * QuestMonster list (the offsets 0 and -1 are returned as they are) */
u8 *quest_find_entry(Quest *self, u32 id, s32 which, s32 idx)
{
    u32 t;
    QuestGroup *p;
    s32 off = 0;

    if (G_QUEST_ID == 0)
        return NULL;
    t = self->phases[(s8)idx];
    if (t == 0)
        return NULL;
    p = (QuestGroup *)QOFF(self, t);
    for (;;) {
        if (p->id == 0)
            return NULL;
        if (p->id == (id & 0xFFFF))
            break;
        p++;
    }
    if (which == 0)
        off = p->list0;
    else if (which == 1)
        off = p->list1;
    if ((u32)(off + 1) < 2)
        return (u8 *)off;
    return QOFF(self, off);
}

u8 *quest_sub_data(Quest *self)
{
    if (G_QUEST_ID == 0)
        return NULL;
    return QOFF(self, self->file->sub_data);
}

void quest_ctor(Quest *self)
{
    memset(self, 0, sizeof(Quest));
    quest_load_init(self);
}

int quest_is_flag_800000(Quest *self)
{
    return (self->info->flags & 0x800000) != 0;
}

/* start of a quest: random seeds, counters and pointers into the quest file */
void quest_start(Quest *self)
{
    u8 *g;
    s32 i, v;
    s8 c;

    G_QUEST_STATE = QS_RUNNING;
    spots_init(self);
    quest_goals_clear(self);
    quest_clear_steps(self);
    if ((OPT_FLAGS & 1) == 0) {
        for (i = 0; i < 15; i++)
            self->seed[i] = game_rand(D_08A631A0, 0);
    } else {
        for (i = 0; i < 15; i++)
            self->seed[i] = D_08A62C8C[i];
    }
    for (i = 0; i < 10; i++)
        D_08A62C80[i] = D_089A7EA4[i];
    quest_rand_seed(self);
    quest_rand(self);
    for (i = 0; i < 32; i++) {
        self->x08c[i].a = 0;
        self->x08c[i].b = 0;
    }
    for (i = 0; i < 90; i++) {
        self->size_min[i] = 0;
        self->size_max[i] = 0;
        self->kills[i] = 0;
        self->captures[i] = 0;
    }
    for (i = 0; i < 4; i++)
        self->seed[i] = quest_rand(self);
    self->players = 0;
    self->net_value = 0;
    self->label2_go = 0;
    self->label2 = 0;
    self->goal_label = 0;
    for (i = 0; i < 8; i++)
        ((u8 *)self->rank)[i] = 0;
    self->points[0] = 0;
    self->points[1] = 0;
    self->points_n = 0;
    self->item_mask[0] = 0;
    self->item_mask[1] = 0;
    v = game_rank(D_089CC438);
    self->rank[G_MY_SLOT][1] = v;
    D_089CC438[0x24] = 0;
    D_089CC438[0x25] = 0;
    D_089CC438[0x26] = 0;
    D_089CC438[0x27] = 0;
    for (i = 0; i < 5; i++) {
        self->items[i].id = 0;
        self->items[i].count = 0;
    }
    self->item_next = 0;
    self->items_new = 0;
    self->x002[1] = 0;
    self->end_step = 0;
    self->end_sub = 0;
    self->phase = 0;
    self->area_phase = 0;
    self->last_target = NULL;
    self->time_left = 0;
    self->x002[2] = 0;
    self->main_models = 0;
    self->base = D_08A5C440;
    self->file = QFILE;
    self->info = (QuestInfo *)QOFF(self, QFILE->info);
    G_MAP = self->info->map;
    quest_pick_start_area(self, 0xFF);
    v = quest_time(self, 1);
    self->time_left = v;
    c = (v - 1) / 9000;
    self->time_warnings = c;
    if (c > 2)
        self->time_warnings = 2;
    self->script = (QuestCmd *)QOFF(self, QFILE->script);
    self->pc = 0;
    self->resume_pc = 0;
    self->texts = (u32 *)QOFF(self, self->info->texts);
    self->phases = (u32 *)QOFF(self, QFILE->phases);
    self->large = (QuestGroup *)QOFF(self, QFILE->large);
    self->map_objects = (u32 *)QOFF(self, QFILE->map_objects);
    self->rewards = (QuestRewardRule *)QOFF(self, QFILE->rewards);
    self->reward = self->info->reward;
    self->penalty = self->info->penalty;
    self->flags = self->info->flags;
    self->size_class = QFILE->size_class;
    self->x116 = QFILE->x49;
    quest_pick_size(self, (s8 *)&self->size_class);
    if ((u8)quest_is_flag_800000(self) == 1)
        D_089CC438[0x41A] = 1;
    else
        D_089CC438[0x41A] = 0;
    self->bonus_kind = QFILE->bonus_kind;
    self->bonus_monster = QFILE->bonus_monster;
    self->bonus_count = QFILE->bonus_count;
    self->bonus_step = 0;
    self->bonus_table = self->time_left / 9000;
    quest_goals_load(self);
    quest_load_steps(self);
    for (i = 0; i < 90; i++) {
        self->parts_broken[i] = 0;
        self->parts_rewarded[i] = 0;
    }
    quest_clear_waves(self);
    quest_init_monsters(self);
    func_08895EA0(D_089CBBEC);
    if (quest_flag_10() != 0) {
        g = D_089CC438;
        memcpy(g + 0x6ADFC, g + 0x43C8, 0x60);
        for (i = 0; i < 0x30; i++)
            U16(g, 0x43C8 + i * 2) = 0;
    }
    for (i = 0; i < 6; i++)
        quest_slot_clear(self, i);
    U32(D_08A62CB4, 0x104) = U32(D_089CC438, 0x696F0);
    U32(D_08A62CB4, 0x108) = U32(D_089CC438, 0x696E8);
    quest_count_monsters(self, self->count_start);
    self->active = 1;
    if (quest_comrade_allowed(QUEST) != 0)
        quest_bonus_clear(QUEST);
    self->x67d[0] = 0;
    self->x67d[1] = 0;
    self->x67d[2] = 0;
    self->bonus_items = 0;
    self->x681 = 0;
    self->log_id = 0;
    self->time_msg = 0;
    D_09C121B0[0x1278] = 0;
}

/* quest abandoned */
void quest_fail(Quest *self)
{
    if (quest_state(self, 1) != 0)
        return;
    G_QUEST_STATE = QS_ABANDONED;
    self->result = QS_ABANDONED;
    self->pc = -1;
    net_send(D_09A1DAEC, 0xC, NULL);
}

/* start area of the quest (who = 0xFF: area of the game, else area of that hunter) */
void quest_pick_start_area(Quest *self, u32 who)
{
    u8 *g = D_089CC438;
    u16 **t = D_09CD3CF0[self->info->map];
    s8 c;
    u16 *l, *p;
    s32 sum, r, acc;
    u8 seed;

    c = g[0x480] == 0 ? self->file->start_mode : 0;
    if (c == 0) {
        if ((who & 0xFFFF) != 0xFF)
            goto hunter;
        if (g[0x480] == 0)
            U16(g, 0x6AF0E) = *D_09CE470C[g[0x6AF10]][0];
        else
            U16(g, 0x6AF0E) = U16(g, 0x484);
        return;
    }
    c = g[0x480] == 0 ? self->file->start_mode : 0;
    if (c == 1)
        l = has_kitchen_skill(g, 0x1F) != 0 ? t[1] : t[0];
    else
        l = t[2];
    /* weighted lists: {weight, area} ... 0xFF */
    sum = 0;
    for (p = l; *p != 0xFF; p += 2)
        sum += *p;
    who &= 0xFFFF;
    seed = who == 0xFF ? self->seed[G_MY_SLOT] : self->seed[who];
    r = (s32)seed % sum;
    acc = 0;
    p = l;
    while (*p != 0xFF) {
        acc += *p;
        if (r < acc) {
            p++;
            break;
        }
        p += 2;
    }
    if (who == 0xFF) {
        G_AREA = *p;
        return;
    }
hunter:
    {
        u16 a = G_AREA;
        U16(player_get(D_09A4F0C4, G_MY_SLOT), E_AREA) = a;
    }
}

void quest_check_end(Quest *self)
{
    if (quest_state(self, 1) == 0)
        func_09A64E48(D_08A62D90, 0);
}

/* 1 while the quest is finishing (QS_CLEARING, QS_CLEARED), -1 when it failed (mode 1) */
s32 quest_state(Quest *self, s32 mode)
{
    u8 s = G_QUEST_STATE;

    if ((u8)(s - QS_CLEARING) < 2)
        return 1;
    if (mode == 1 && (u8)(s - QS_FAILED) < 3)
        return -1;
    return 0;
}

/* --- targets, waves and end of the quest --------------------------------------------- */
extern const u16 D_089AC5A0[];      /* IDs of the quest messages */
extern const u16 D_089A7EB0[];
extern u8 *D_09A4F0D0;
extern u8 *D_08A62D8C;
extern void **D_089CC434;
extern void *D_09B23820;
extern const u8 *D_089A800C[];
extern const u16 D_09CE4808[], D_09CE5008[], D_09CE5220[];

const char *text_menu(void *g, u32 id);
u8 menu_msg_push_text(u8 *self, const char *s);
u8 menu_msg_push(u8 *self, u32 id, u32 x, u32 y);
void sound_play(void *snd, int a, int b, int c, int d, int e, int f, int g, int h);
u8 game_table_byte(u8 *g, u32 i);
u16 func_09ACBF40(u8 *h);
void func_09AC7668(u8 *h);
void func_09AB46A8(u8 *a, u32 b);
void func_088DE3D4(u8 *a, u32 b);
void func_09AB2B40(u8 *a, u32 b);
void func_088B87B0(void *a, int b);
u8 *func_09A8A4B0(void *a);

/* time limit (mode 1) or time left */
s32 quest_time(Quest *self, s32 mode)
{
    s32 t;

    if (mode == 1)
        return self->info->time;
    t = self->time_left;
    if (t < 0)
        t = 0;
    return t;
}

/* 1 when the monster h is a target still to be hunted */
int quest_is_target(Quest *self, u8 *h)
{
    QuestMonster *e;

    if (quest_state(self, 0) != 0)
        return 0;
    e = quest_find_monster(self, h);
    if (e == NULL)
        return 0;
    if ((e->state & QM_DONE) != 0)
        return quest_wave_is_last(self, h) == 0;
    return e->not_target != 1;
}

void quest_clear_waves(Quest *self)
{
    s32 i, j;
    QuestWave *w;

    for (i = 0; i < 2; i++) {
        w = &self->wave[i];
        for (j = 0; j < 5; j++) {
            w->kind[j] = 0;
            w->seen[j] = 0;
            w->spawned[j] = 0;
            w->mon[j] = NULL;
            w->count[j] = 0;
        }
        w->n = 0;
        w->cur = 0;
    }
}

/* adds a monster to the wave i */
void quest_wave_add(Quest *self, s32 i, QuestMonster *m)
{
    QuestWave *w = &self->wave[i];

    w->mon[w->n] = m;
    w->count[w->n] = m->count;
    w->kind[w->n] = (s8)m->kind;
    w->n++;
}

/* 1 when h is the last monster of its wave */
int quest_wave_is_last(Quest *self, u8 *h)
{
    s32 i;
    QuestWave *w = self->wave;

    for (i = 0; i < 2; i++, w++) {
        if (w->kind[w->cur] == h[E_KIND]) {
            w = &self->wave[i];
            return !(w->cur < w->n - 1);
        }
    }
    return 0;
}

int quest_is_target2(Quest *self, u8 *h)
{
    QuestMonster *e;

    if (quest_state(self, 0) != 0)
        return 0;
    e = quest_find_monster(self, h);
    if (e == NULL || (e->state & QM_DONE) != 0 || e->not_target == 1)
        return 0;
    return 1;
}

/* spawns the next monster of each wave when its slot is free */
void quest_spawn_waves(Quest *self)
{
    s32 i;
    QuestWave *w = self->wave;
    u32 used, a, b;

    for (i = 0; i < 2; i++, w++) {
        used = (i >= 0 && i < 20) ? U32(D_09C121B0, 0x1220 + i * 4) : 0;
        if (used != 0)
            continue;
        if (w->n <= 0)
            continue;
        if (!(w->cur < w->n))
            continue;
        if (w->spawned[w->cur] != 0)
            continue;
        a = (u8)w->kind[w->cur];
        b = ((u8 *)w->kind)[w->cur - 1];    /* the previous monster of the wave */
        func_09AB46A8(D_09C121B0, b);
        func_088DE3D4(D_09A4F0D0, b);
        func_09AB2B40(D_09C121B0, a);
        w->spawned[w->cur] = 1;
    }
}

void quest_send_wave(Quest *self, u32 i)
{
    QuestEvent ev;

    ev.type = 0x15;
    ev.zero = 0;
    ev.arg[0] = (u8)i;
    ev.arg[1] = self->wave[(u8)i].cur;
    net_send(D_09A1DAEC, 6, &ev);
}

/* a target monster h appears: position and area from the quest data */
s32 quest_place_target(Quest *self, u8 *h)
{
    QuestMonster *e = quest_find_monster(self, h);
    u8 lim;

    if (e == NULL)
        return 0;
    if ((e->state & QM_DONE) != 0)
        return 0;
    e->state = QM_WAITING;
    if ((u8)monster_is_large2(self, h[E_KIND]) == 1) {
        U16(h, E_SIZE) += 5;
        lim = game_table_byte(D_089CC438, h[E_KIND]);
        if (D_089A7EB0[lim] < U16(h, E_SIZE))
            U16(h, E_SIZE) = D_089A7EB0[lim];
        U16(h, E_SIZE_SHOWN) = func_09ACBF40(h);
        if ((self->flags & QF_KEEP_SIZE) == 0)
            h[E_SIZE_CLASS] = quest_size_clamp(self, *(s8 *)(h + E_SIZE_CLASS), 1);
    }
    e->life = S16(h, E_LIFE);
    if (e->random_pos != 1 || (u8)quest_pick_target_pos(self, h) != 1) {
        F32(h, E_POS) = e->pos[0];
        F32(h, E_POS + 4) = e->pos[1];
        F32(h, E_POS + 8) = e->pos[2];
        U32(h, E_ANGLE) = e->angle;
    }
    U16(h, E_AREA) = e->area;
    ((void (*)(u8 *))((void **)U32(h, 0))[0xF4 / 4])(h);
    func_09AC7668(h);
    return e->count;
}

/* random start point of the target in its area (list of {area, n, offset}) */
int quest_pick_target_pos(Quest *self, u8 *h)
{
    u8 *base = self->base;
    u16 *l = (u16 *)(self->file->target_pos + (u32)base);
    u8 *st = (u8 *)D_09A4F060;
    u32 id, f, r;
    u32 *p;

    for (;; l += 4) {
        id = l[0];
        if (id == 0xFFFF)
            return 0;
        f = D_09CE4808[U32(st, 0x284)] != 0xFFFF;
        if (f != D_09CE5008[id] && D_09CE5220[id] != 0xFFFF)
            id = D_09CE5220[id];
        if (U16(h, E_AREA) == id)
            break;
    }
    r = game_rand(D_08A631A0, 0) & 0xFFFF;
    p = (u32 *)(U32(l, 4) + (u32)base + (s32)r % (s32)l[1] * 0x10);
    U32(h, E_ANGLE) = p[0];
    U32(h, E_POS) = p[1];
    U32(h, E_POS + 4) = p[2];
    U32(h, E_POS + 8) = p[3];
    return 1;
}

/* records the largest / smallest size of the kind of h */
static void record_size(Quest *self, u8 *h)
{
    u32 i;

    i = h[E_KIND];
    if (self->size_max[i] < U16(h, E_SIZE))
        self->size_max[i] = U16(h, E_SIZE);
    i = h[E_KIND];
    if (self->size_min[i] == 0 || U16(h, E_SIZE) < self->size_min[i])
        self->size_min[i] = U16(h, E_SIZE);
}

/* target h killed */
void quest_target_killed(Quest *self, u8 *h)
{
    QuestMonster *e = quest_find_monster(self, h);
    u32 i;

    if (e == NULL)
        return;
    if ((e->state & (QM_DONE | QM_RESPAWN)) != 0) {
        quest_set_target(self, h);
        return;
    }
    e->count--;
    quest_hunt_done(self, h);
    quest_kill_count(self, h[E_KIND]);
    i = h[E_KIND];
    self->kills[i]++;
    i = h[E_KIND];
    if (self->kills[i] >= 10000)
        self->kills[i] = 9999;
    if ((u8)monster_is_large(self, h[E_KIND]) == 1)
        record_size(self, h);
    if (e->count > 0) {
        e->state = QM_RESPAWN;
    } else {
        e->slot = -1;
        e->state = QM_DONE;
    }
    quest_set_target(self, h);
    quest_check_targets_done(self, h[E_KIND]);
    if (h[0x6DB] != 0)
        quest_send_killed(self, e, h);
}

/* the target h left the area: its life is kept */
void quest_target_left(Quest *self, u8 *h)
{
    QuestMonster *e = quest_find_monster(self, h);

    if (e != NULL && (e->state & QM_DONE) == 0) {
        e->life = U16(h, E_LIFE);
        e->slot = -1;
    }
}

/* target h captured */
void quest_target_captured(Quest *self, u8 *h)
{
    QuestMonster *e = quest_find_monster(self, h);
    u32 i;

    if (e == NULL)
        return;
    if ((e->state & QM_CAPTURED) != 0) {
        quest_set_target(self, h);
        return;
    }
    i = h[E_KIND];
    self->captures[i]++;
    i = h[E_KIND];
    if (self->captures[i] >= 10000)
        self->captures[i] = 9999;
    if ((u8)monster_is_large(self, h[E_KIND]) == 1)
        record_size(self, h);
    quest_set_target(self, h);
    quest_capture_done(self, h[E_KIND]);
    if ((e->state & (QM_DONE | QM_RESPAWN)) == 0) {
        quest_send_captured(self, e, h);
        e->count--;
        quest_hunt_done(self, h);
        if (e->count > 0) {
            e->state = QM_RESPAWN;
        } else {
            e->slot = -1;
            e->state = QM_DONE;
        }
    }
    e->state |= QM_CAPTURED;
}

void quest_set_target(Quest *self, u8 *h)
{
    if (quest_goal_counts(self, h[E_KIND]) != 0)
        self->last_target = h;
}

/* hunting goals of the kind id after a kill: 1 when one of them is now complete */
u32 quest_check_targets_done(Quest *self, u32 id)
{
    QuestGoal *g = self->goal;
    u32 r = 0, f;
    s32 i;

    for (i = 0; i < 2; i++, g++) {
        f = g->flags;
        if ((f & (GOAL_DONE | GOAL_FAILED)) != 0)
            continue;
        if ((f & (GOAL_HUNT | GOAL_ACTIVE)) == (GOAL_HUNT | GOAL_ACTIVE) &&
            g->target == (id & 0xFFFF)) {
            if ((f & GOAL_CAPTURE) != 0) {
                g->flags = f | GOAL_FAILED;
                return 0;
            }
            g->count--;
            if (g->count <= 0) {
                quest_goal_complete(self, i & 0xFFFF, GOAL_BY_KILL);
                r = 1;
            } else {
                quest_hunt_message(self, id & 0xFF, g->count, self->goal_orig[i].count);
            }
        }
        if ((g->flags & (GOAL_REPEL | GOAL_ACTIVE | GOAL_SPECIAL)) ==
                (GOAL_REPEL | GOAL_ACTIVE | GOAL_SPECIAL) &&
            g->target == (id & 0xFFFF)) {
            quest_goal_complete(self, i & 0xFFFF, GOAL_BY_REPEL);
            r = 1;
        }
    }
    return r;
}

/* a hunter fainted: the reward is lowered; 1 when nothing is left (the quest fails),
 * 2 when there was no reward */
s32 quest_faint(Quest *self)
{
    s32 t;

    if (self->reward <= 0)
        return 2;
    t = self->reward - self->penalty;
    self->reward = t;
    if (t > 0)
        return 0;
    if (quest_state(self, 1) == 0)
        quest_end_fail(self, 0);
    if (self->reward <= 0)
        self->reward = 0;
    return 1;
}

/* marks in Quest.players the players that are not present (bit shift + i) */
static void mark_absent(Quest *self, s32 shift)
{
    s32 i;

    for (i = 0; i < 4; i++)
        if (G_PLAYER_HERE((u8)i) == 0)
            self->players |= 1 << (i + shift);
}

static void push_msg(u32 i)
{
    menu_msg_push_text(D_089CBBEC, text_menu(D_089CC438, D_089AC5A0[i]));
}

/* per-frame update of the quest: 0, 1 (finished), -1, -2 */
s32 quest_update(Quest *self)
{
    u32 h = player_get(D_09A4F0C4, G_MY_SLOT);
    QuestEvent ev;
    u32 v;

    mark_absent(self, 8);
    spots_update(self);
    if (G_QUEST_ID == 0)
        return 0;
    if (self->result == QS_ABANDONED)
        return -2;
    if (self->result == 8)
        return -2;
    if (self->points_n != 0) {
        ev.type = 0xF;
        ev.zero = 0;
        v = self->points[self->points_n - 1];
        ev.arg[1] = v;
        ev.arg[0] = v >> 16;
        net_send(D_09A1DAEC, 6, &ev);
        self->points_n = 0;
        ev.type = 0x10;
        ev.zero = 0;
        ev.arg[0] = self->item_mask[0];
        ev.arg[1] = self->item_mask[0] >> 16;
        ev.arg[2] = self->item_mask[1];
        net_send(D_09A1DAEC, 6, &ev);
    }
    switch (self->end_step) {
    case 3:
        return 0;
    case 2:
        /* online: waits for the other players */
        switch (self->end_sub) {
        case 3:
            return 0;
        case 2:
            if (self->sync_timer > 0)
                self->sync_timer--;
            mark_absent(self, 4);
            if (((u8 *)&self->players)[0] != 0xFF && self->sync_timer > 0)
                return 0;
            self->end_sub++;
            G_QUEST_STATE = QS_CLEARED;
            self->reward = self->reward / (s32)self->player_n;
            quest_count_monsters(self, self->count_end);
            return 0;
        case 1:
            if (self->sync_timer > 0)
                self->sync_timer--;
            mark_absent(self, 0);
            if ((self->players & 0xF) != 0xF && self->sync_timer > 0)
                return 0;
            self->end_sub++;
            ev.type = 0xD;
            ev.zero = 0;
            ev.arg[1] = self->net_value;
            ev.arg[0] = self->net_value >> 16;
            net_send(D_09A1DAEC, 6, &ev);
            self->players |= 1 << (G_MY_SLOT + 4);
            self->sync_timer = 1800;
            return 0;
        case 0:
            self->end_sub = 1;
            self->result = QS_CLEARED;
            self->rank[G_MY_SLOT][0] = game_rank(D_089CC438);
            ev.type = 0xC;
            ev.zero = 0;
            ev.arg[0] = self->rank[G_MY_SLOT][0];
            ev.arg[1] = 0;
            net_send(D_09A1DAEC, 6, &ev);
            self->players |= 1 << G_MY_SLOT;
            self->sync_timer = 5400;
            return 0;
        }
        return 0;
    case 1:
        /* end messages */
        switch (self->end_sub) {
        case 2:
            if (--self->msg_timer > 0)
                return 0;
            self->end_step++;
            self->end_sub = 0;
            push_msg(2);
            if ((GAME_FLAGS & 4) == 0)
                menu_msg_push(D_089CBBEC, 0, 0xF, 0);
            self->active = 0;
            return 0;
        case 1:
            if (--self->msg_timer > 0)
                return 0;
            self->end_sub++;
            if ((self->flags & 0x80000) == 0 ||
                ((self->flags & 0x200000) != 0 && quest_waves_left(self) == 0)) {
                self->msg_timer = 1800;
                push_msg(1);
            } else {
                self->msg_timer = 600;
                push_msg(0);
                if ((self->flags & QF_TREASURE) != 0)
                    push_msg(10);
            }
            return 0;
        case 0:
            push_msg((self->flags & QF_TREASURE) == 0 ? 0xC : 0x1A);
            self->end_sub++;
            self->msg_timer = 150;
            return 0;
        }
        return 0;
    case 0:
        if (self->result == QS_CLEARED)
            return 1;
        if (self->result == QS_RETURN)
            return -1;
        if (self->pc == -1)
            return 1;
        quest_goals_update(self);
        quest_goals_check(self, 1);
        quest_timer2_update(self);
        self->pc = quest_script_run(self, (u8 *)h, self->pc);
        quest_bonus_check(self);
        quest_check_deliveries(self);
        quest_spawn_waves(self);
        return 0;
    }
    return 0;
}

/* seed of the quest random generator, from the 15 seed bytes */
void quest_rand_seed(Quest *self)
{
    s32 i;

    self->rand = 0;
    for (i = 0; i < 7; i++)
        self->rand += (u16)(self->seed[i * 2] + (u16)(self->seed[i * 2 + 1] << 8));
    if (self->rand == 0)
        self->rand = 0x1C3;
}

u32 quest_rand(Quest *self)
{
    u32 v = self->rand;

    if (v == 0)
        v = 1;
    v = (v * 0xB0) % 0xFF53;
    self->rand = v;
    return v & 0xFFFF;
}

static void bonus_failed(void)
{
    push_msg(9);
    sound_play(D_08A62D8C, 1, 0x77, 0, 0, 0, 0, 0, 0);
}

/* time bonus (kind 1): lost after the time or by chance */
void quest_bonus_check(Quest *self)
{
    const u8 *t = D_089A800C[self->bonus_table];
    const u8 *p;
    QuestEvent ev;

    if (self->bonus_kind == 0 || self->bonus_kind != 1)
        return;
    p = t + self->bonus_step * 2;
    if (self->time_left > p[0] * 1800)
        return;
    self->bonus_step++;
    if ((game_rand(D_08A631A0, 0) & 0x7F) >= p[1])
        return;
    self->bonus_kind = 0;
    bonus_failed();
    ev.type = 0xE;
    ev.zero = 0;
    ev.arg[0] = 0;
    ev.arg[1] = 0;
    net_send(D_09A1DAEC, 6, &ev);
}

/* hunting bonus (kind 2): lost after hunting the wrong monsters */
void quest_hunt_done(Quest *self, u8 *h)
{
    int bad;

    if (self->bonus_kind == 0 || self->bonus_kind != 2)
        return;
    if (self->bonus_monster != 0) {
        if (self->bonus_monster != h[E_KIND])
            return;
        if (--self->bonus_count <= 0) {
            self->bonus_kind = 0;
            bonus_failed();
        }
        return;
    }
    bad = 0;
    if (self->wave[0].n != 0 && self->wave[1].n == 0) {
        bad = 1;
    } else if (self->wave[0].n != 0) {
        if (self->wave[1].n != 0 &&
            self->wave[0].count[0] != self->wave[0].mon[0]->count &&
            self->wave[1].count[0] != self->wave[1].mon[0]->count)
            bad = 1;
    }
    if (self->bonus_count > 0)
        self->bonus_count--;
    if (self->bonus_count <= 0 && bad) {
        self->bonus_kind = 0;
        bonus_failed();
    }
}

/* a phase step is reached: event and "goal" flags of the monsters cleared */
void quest_goal_reached(Quest *self)
{
    QuestEvent ev;
    u8 *m = D_09A4F0D0;
    s32 i;

    ev.zero = 0;
    ev.type = 6;
    ev.arg[0] = self->phase;
    ev.arg[1] = 0;
    net_send(D_09A1DAEC, 6, &ev);
    U32(m, 0x210) = U32(m, 0x204);
    U32(m, 0x20C) = U32(m, 0x208);
    for (i = 0x15; i <= 0x2A; i++)
        U16(m, i * 0xC) &= 0xFFFD;
    func_088B87B0(D_089CC434, 1);
}

/* item steps (kind 2): enough items in the pouch */
void quest_check_deliveries(Quest *self)
{
    QuestStep *g = self->step;
    s32 i;

    for (i = 0; i < 2; i++, g++) {
        if (g->kind != 2)
            continue;
        if ((s32)pouch_count(D_089CC438, g->id) < g->count)
            continue;
        if (self->phase < g->phase) {
            self->phase = g->phase;
            quest_goal_reached(self);
        }
        g->kind = 0;
    }
}

/* hunting steps (kind 1): monster id hunted */
void quest_kill_count(Quest *self, s32 id)
{
    QuestStep *g = self->step;
    s32 i;

    for (i = 0; i < 2; i++, g++) {
        if (g->kind != 1 || (s16)g->id != (s16)id)
            continue;
        if (--g->count > 0)
            continue;
        if (self->phase < g->phase) {
            self->phase = g->phase;
            quest_goal_reached(self);
        }
        g->kind = 0;
    }
}

void quest_clear_steps(Quest *self)
{
    self->step[0].kind = 0;
    self->step[0].id = 0;
    self->step[0].count = 0;
    self->step[1].kind = 0;
    self->step[1].id = 0;
    self->step[1].count = 0;
}

void quest_load_steps(Quest *self)
{
    QuestStep *d = self->step;
    QuestStep *s = self->file->step;
    s32 i;

    for (i = 0; i < 2; i++, d++, s++) {
        if (s->kind == 0)
            continue;
        d->kind = s->kind;
        d->id = s->id;
        d->count = s->count;
        d->phase = i + 1;
    }
}

/* text i of the quest file in the current language */
u8 *quest_text(Quest *self, s32 i)
{
    if ((GAME_FLAGS & 1) != 0)
        return func_09A8A4B0(D_09B23820);
    return QOFF(self, *(s32 *)(QOFF(self, self->texts[LANG]) + i * 4));
}

/* copies the kinds of the large monsters (until -1), returns the count */
s32 quest_large_kinds(Quest *self, u8 *out)
{
    s32 n = 0;
    s32 *a = (s32 *)quest_large_list(self, 0);
    u8 *b = quest_large_list(self, 1);

    if (a == NULL || b == NULL)
        return 0;
    while (*a != -1) {
        *out++ = *a++;
        n++;
    }
    return n;
}

/* the 5 slots of items given by the quest (id, count) */
s32 quest_item_count(Quest *self, u32 id)
{
    s16 i;

    for (i = 0; i < 5; i++)
        if (self->items[i].id == (id & 0xFFFF))
            return self->items[i].count;
    return 0;
}

/* 0: new slot, 1: added, 2 / 3: limited to the maximum, 4: none left, 5: replaced a slot */
s32 quest_item_add(Quest *self, u32 id, s32 n)
{
    s16 i;
    s8 max, v;
    QuestItem *e;

    if ((s16)quest_item_count(self, id) == 0) {
        for (i = 0; i < 5; i++) {
            if (self->items[i].id == 0 && (s16)n > 0) {
                e = &self->items[i];
                e->id = id;
                e->count = n;
                return 0;
            }
        }
        e = &self->items[self->item_next];
        e->id = id;
        self->items[self->item_next].count = n;
        self->item_next++;
        if (self->item_next >= 5)
            self->item_next = 0;
        return 5;
    }
    for (i = 0; i < 5; i++) {
        if (self->items[i].id != (id & 0xFFFF))
            continue;
        max = D_0899D19B[(id & 0xFFFF) * 0x18];
        e = &self->items[i];
        if ((s16)n > 0 && e->count >= max) {
            e->count = max;
            return 3;
        }
        v = e->count + (s8)n;
        e->count = v;
        if (v <= 0) {
            e->id = 0;
            e->count = 0;
            return 4;
        }
        if (max < v) {
            e->count = max;
            return 2;
        }
        return 1;
    }
    return 0;
}
