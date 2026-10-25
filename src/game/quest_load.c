/* Quest (g_quest): events sent to the other players, loading of the quest file and of
 * the monsters of the area, life records of the hunts, sizes and helpers of the goals.
 * (Guessed names.) */
#include "quest.h"

extern void *g_net;
extern u8 *g_monsters;
extern u8 *g_monster_models;
extern void **g_loader;          /* file loader (C++ object) */
extern void *g_heap;
extern void *D_09A44C60;
extern u8 *g_sound;
extern void *g_camera;
extern const u8 *const *D_089A7CE8[];   /* quest id -> file id tables */
extern const u16 D_09BC8400[][3];        /* monster models: 3 file ids each */
extern const s32 D_089A8278[];
extern const s32 D_089ABD18[][2];        /* min / max size of each monster */
extern const u8 D_09CDE6C0[];            /* extra rewards: 14 bytes each, 0xFF = end */
extern const u32 D_09CDD850[];
extern const s8 D_089A7F64[], D_089A7F74[], D_089A7F78[], D_089A7F7C[];
extern const u8 D_089A8184[8];           /* monsters whose life is recorded */
extern const s16 D_089A818C[][8], D_089A81DC[][8];
extern const s8 D_089A8040[20], D_089A8054[20];

typedef s32 (*LdFunc0)(void *);
typedef s32 (*LdFunc1)(void *, u32);
typedef s32 (*LdFunc6)(void *, u32, void *, s32, s32, s32, s32);
#define LDCALL(type, off) ((type)((void **)*g_loader)[(off) / 4])

u16 game_map_id(u8 *g);
int party_has_kitchen_skill(u8 *p, u32 k);
void camera_mode_for_area(void *cam);
s32 hunter_has_skill(u32 h, int a);
s32 temp_alloc(void *a, int b, s32 c);
void temp_free(void *a, int b);
s32 temp_get(void *a, int b);
void func_088C2BE0(void *a, void *b, s32 c);
s32 func_088DDFE8(u8 *a, u32 b);
void func_088DDE20(u8 *a, u32 b);
void func_088DE0B8(u8 *a, u32 b, u32 c, s32 d);
void monster_models_load(u8 *a, u32 b, u32 c);
void monster_models_release(u8 *a, u32 b);
void func_09AF4050(u8 *p);
void func_09AB2E68(u8 *a, s32 i);
void func_09AB46A8(u8 *a, u32 b);
u8 *func_09AB2958(u8 *a, QuestMonster *m);
void func_09AB35F0(u8 *a);
void func_08880CF8(u8 *snd, s32 i);
void func_08880D64(u8 *snd, s32 a, u32 b, u32 c, u32 d, u32 k, s32 i);
u8 func_08880D94(u8 *snd, u16 *ids, s32 i);
void func_08880E50(u8 *snd, const s32 *t, u16 *a, u16 *b, u16 *c, u8 *k, s32 n, s32 f, u32 *sz);
void func_08881114(u8 *snd, s32 a, u32 b, u32 c, u32 d, u32 k, s32 i);

#define SMALL_END 0xFFFFFFFF        /* end of the lists of small monsters */

static void send_event(QuestEvent *ev)
{
    net_send(g_net, 6, ev);
}

/* time left, sent to the other players */
void quest_send_time(Quest *self)
{
    QuestEvent ev;

    if (G_TIME_STOP != 0)
        return;
    ev.type = 5;
    ev.zero = 0;
    ev.arg[1] = self->time_left;
    ev.arg[0] = self->time_left >> 16;
    send_event(&ev);
}

void quest_send_killed(Quest *self, QuestMonster *m, u8 *h)
{
    QuestEvent ev;

    ev.type = 7;
    ev.zero = 0;
    ev.arg[0] = m->index;
    ev.arg[1] = U16(h, E_AREA) | (u16)(m->count << 8);
    send_event(&ev);
}

void quest_send_captured(Quest *self, QuestMonster *m, u8 *h)
{
    QuestEvent ev;

    ev.type = 8;
    ev.zero = 0;
    ev.arg[0] = m->index;
    ev.arg[1] = U16(h, E_AREA);
    send_event(&ev);
}

u32 quest_table_item(Quest *self, u32 i)
{
    return quest_table_rand((const u16 *)quest_table_entry(self, i & 0xFFFF), NULL);
}

/* loads the file of the quest id (0: not loaded yet, 1: done) */
s32 quest_load_file(Quest *self, u32 id, u8 *dst)
{
    u32 k, f;

    if (self->load_step == 1) {
        if (LDCALL(LdFunc0, 0x38)(g_loader) != 0)
            return 0;
        if (dst == NULL)
            dst = g_quest_file;
        func_088C2BE0(D_09A44C60, dst, temp_get(g_heap, 0x17));
        temp_free(g_heap, 0x17);
        self->load_step = 0;
        return 1;
    }
    if (self->load_step != 0)
        return 0;
    id &= 0xFFFF;
    if (id >= 60000)
        return 1;
    /* file of the quest: the ids are grouped by thousands */
    if (id < 10000)
        k = id / 1000;
    else if (id < 20000)
        k = (s32)(id - 10000) / 1000 + 5;
    else if (id < 30000)
        k = (s32)(id - 20000) / 1000 + 7;
    else if (id < 40000)
        k = 10;
    else if (id < 50000)
        k = 11;
    else
        k = 12;
    f = ((const u16 *)D_089A7CE8[k][(s32)id % 1000 / 100])[(s32)id % 100];
    if (self->file_id == f)
        return 1;
    LDCALL(LdFunc6, 0x34)(g_loader, f, (void *)temp_alloc(g_heap, 0x17, 0x6800), -1, 0, 0, 1);
    self->file_id = f;
    self->load_step++;
    return 0;
}

void quest_load_init(Quest *self)
{
    self->load_step = 0;
    self->file_id = 0xFFFF;
}

/* loads the models of the monsters of the waves (biggest file first) */
void quest_load_wave_models(Quest *self)
{
    u32 sz[2];
    u8 done[2] = {0xFF, 0xFF};
    s32 w, j, k, uniq;
    QuestWave *e;
    u32 v;

    for (w = 0, e = self->wave; w < 2; w++, e++) {
        sz[w] = 0;
        for (j = 0; j < e->n; j++) {
            v = LDCALL(LdFunc1, 0x18)(g_loader, ((u8)e->kind[j] + 0x17DF) & 0xFFFF);
            if (sz[w] < v)
                sz[w] = v;
        }
    }
    for (w = 0, e = self->wave; w < 2; w++, e++) {
        if (e->n <= 0)
            continue;
        uniq = 1;
        for (k = 0; k < w; k++) {
            if (done[k] == e->kind[0]) {
                uniq = 0;
                break;
            }
        }
        if (uniq) {
            func_088DE0B8(g_monster_models, (u8)e->kind[0], sz[w], w);
            done[w] = e->kind[0];
        }
    }
}

/* small monsters of the area: id 0x12 (in the map of the quest), 10 and 0x4A in some
 * areas, then the list of the area in the quest file */
static int area_has_10(Quest *self, u32 a)
{
    if ((self->flags & (QF_TRAINING | QF_TREASURE)) != 0)
        return 0;
    switch (a) {
    case 0x29: case 0xC2: case 0x36: case 0x94: case 0x10: case 0x97: case 1: case 0x6E:
    case 0x5C: case 0x65: case 0xCD: case 0xE3:
        return 1;
    }
    return 0;
}

static int area_has_4a(Quest *self, u32 a)
{
    if ((self->flags & QF_TREASURE) == 0)
        return 0;
    switch (a) {
    case 0x15: case 0x8C: case 0x97: case 0xA1: case 0x6E: case 0x62: case 0xF7:
        return 1;
    }
    return 0;
}

static void load_monster(u32 k)
{
    u8 *m = g_monster_models;

    if (m[0x214] != 0 && func_088DDFE8(g_monster_models, k) != 0)
        func_088DDE20(m, k);
    else
        func_088DE0B8(m, k, 0, -1);
}

void quest_load_area_monsters(Quest *self)
{
    u32 a = G_AREA;
    u32 *l;
    s32 i;

    quest_check_monsters_done(QUEST, a);
    if (a == game_map_id(g_game))
        load_monster(0x12);
    if (area_has_10(self, a))
        load_monster(10);
    if (area_has_4a(self, a))
        load_monster(0x4A);
    l = (u32 *)quest_find_entry(self, a, 0, self->phase);
    self->area_phase = self->phase;
    if (l == NULL)
        return;
    for (i = 0; i < 4; i++, l++)
        if (*l != SMALL_END)
            load_monster(*l & 0xFF);
}

/* slots of the small monsters of the area a (after the large ones of the waves) */
void quest_place_area_monsters(Quest *self, u32 a)
{
    Quest *q = QUEST;
    u32 n = q->wave[0].n > 0;
    u32 *l;
    s32 i;

    if (q->wave[1].n > 0 && q->wave[0].kind[0] != q->wave[1].kind[0])
        n++;
    if ((a & 0xFFFF) == game_map_id(g_game))
        monster_models_load(g_monster_models, 0x12, n++);
    if (area_has_10(self, a & 0xFFFF))
        monster_models_load(g_monster_models, 10, n++);
    if (area_has_4a(self, a & 0xFFFF))
        monster_models_load(g_monster_models, 0x4A, n++);
    l = (u32 *)quest_find_entry(self, a, 0, self->phase);
    if (l == NULL)
        return;
    for (i = 0; i < 4; i++, l++)
        if (*l != SMALL_END)
            monster_models_load(g_monster_models, *l & 0xFF, n++);
}

/* 1 when the small monster k belongs to the current area */
u32 quest_area_has_monster(Quest *self, u32 k)
{
    u32 *l;

    k &= 0xFF;
    if (k == 0x12 || k == 10 || k == 0x4A)
        return 1;
    l = (u32 *)quest_find_entry(self, G_AREA, 0, self->area_phase);
    if (l == NULL)
        return 0;
    for (; *l != SMALL_END; l++)
        if ((*l & 0xFF) == k)
            return 1;
    return 0;
}

/* change of area: the monsters of the old area are removed */
void quest_leave_area(Quest *self)
{
    s32 i;
    u8 *p;
    QuestMonster *e;
    u32 *l;

    for (i = 0; i < 20; i++) {
        p = (i >= 0 && i < 20) ? (u8 *)U32(g_monsters, 0x1220 + i * 4) : NULL;
        if (p == NULL || quest_area_has_monster(self, p[E_KIND]) == 0)
            continue;
        e = quest_find_monster(self, p);
        if (e != NULL && e->state != QM_DONE) {
            if (p[E_KIND] == 0x17)
                func_09AF4050(p);
            if (p[0x1D4] < 2 && p[0x298] != 5) {
                e->life = S16(p, E_LIFE);
            } else {
                quest_target_killed(self, p);
                if ((e->state & QM_RESPAWN) != 0) {
                    e->state = QM_WAITING;
                    e->life = -1;
                }
            }
            e->slot = -1;
        }
        quest_monster_detach(self, p);
        func_09AB2E68(g_monsters, i);
    }
    quest_reset_monsters(self, G_AREA);
    func_09AB46A8(g_monsters, 0x12);
    func_09AB46A8(g_monsters, 10);
    func_09AB46A8(g_monsters, 0x4A);
    monster_models_release(g_monster_models, 0x12);
    monster_models_release(g_monster_models, 10);
    monster_models_release(g_monster_models, 0x4A);
    l = (u32 *)quest_find_entry(self, G_AREA, 0, self->area_phase);
    if (l == NULL)
        return;
    for (; *l != SMALL_END; l++) {
        func_09AB46A8(g_monsters, *l & 0xFF);
        monster_models_release(g_monster_models, *(u8 *)l);
    }
}

/* the hunters that were riding / attached to the monster 0x12 are released */
void quest_monster_detach(Quest *self, u8 *p)
{
    s32 i;
    u8 *h;

    if (*(s8 *)(p + E_KIND) != 0x12)
        return;
    for (i = 0; i < 4; i++) {
        h = (u8 *)player_get(g_players, i);
        if (h != NULL && (U32(h, 4) & 1) != 0 && U32(h, 0x560) == (u32)p)
            U32(h, 0x560) = 0;
    }
}

/* spawns the large monsters of the first group; the sizes of the monsters of the same
 * kind are spread apart */
void quest_spawn_sub_monsters(Quest *self)
{
    QuestMonster *l = (QuestMonster *)quest_large_list(self, 1);
    u32 psz = 0, pk = 0, first = 0;
    s32 pv = 0, d;
    u8 *p = NULL;
    s16 v;

    if (l != NULL) {
        while (l->kind >= 0) {
            p = func_09AB2958(g_monsters, l);
            if (p != NULL && l->large != 0) {
                self->record_target = p;
                if (D_09BC8400[p[E_KIND]][0] == D_09BC8400[pk][0]) {
                    v = S16(p, E_SIZE_SHOWN);
                    d = v - pv;
                    if (d < 0)
                        d = -d;
                    if (d < 200) {
                        if (psz < U16(p, E_SIZE)) {
                            S16(p, E_SIZE_SHOWN) = v - 200;
                            if (S16(p, E_SIZE_SHOWN) < D_089ABD18[pk][0])
                                S16(p, E_SIZE_SHOWN) += 200;
                        } else if (U16(p, E_SIZE) < psz) {
                            S16(p, E_SIZE_SHOWN) = v + 200;
                            if (D_089ABD18[pk][1] < S16(p, E_SIZE_SHOWN))
                                S16(p, E_SIZE_SHOWN) -= 200;
                        } else {
                            S16(p, E_SIZE_SHOWN) = v - 200;
                            if (!(S16(p, E_SIZE_SHOWN) < D_089ABD18[pk][0]))
                                break;
                            S16(p, E_SIZE_SHOWN) += 200;
                            S16(p, E_SIZE_SHOWN) += 200;
                            if (D_089ABD18[pk][1] < S16(p, E_SIZE_SHOWN))
                                S16(p, E_SIZE_SHOWN) -= 200;
                        }
                    }
                }
            }
            if (first == 0 && p != NULL) {
                pv = S16(p, E_SIZE_SHOWN);
                pk = p[E_KIND];
                psz = U16(p, E_SIZE);
                first++;
            }
            l++;
        }
    }
    func_09AB35F0(g_monsters);
}

/* sounds / models of the large monsters of the quest (up to 4) */
void quest_load_main_models(Quest *self)
{
    s32 *l = (s32 *)quest_large_list(self, 0);
    u8 k[4] = {0, 0, 0, 0};
    u32 sz[4] = {0, 0, 0, 0};
    u16 a[4], b[4], c[4];
    s32 i, j, n = 0;
    u8 r;
    QuestWave *w;
    u32 v;

    for (i = 0; i < 4; i++)
        a[i] = b[i] = c[i] = 0xFFFF;
    if (l == NULL)
        return;
    for (i = 0; i < 4; i++, l++) {
        if (*l == -1)
            break;
        func_08880CF8(g_sound, i);
        a[i] = D_09BC8400[*l][0];
        b[i] = D_09BC8400[*l][1];
        c[i] = D_09BC8400[*l][2];
        k[i] = *l;
        func_08880D64(g_sound, (s16)D_089A8278[i], a[i], b[i], c[i], k[i], i);
        n++;
    }
    if (n <= 0)
        return;
    if (k[0] == k[1])
        n--;
    func_08880D94(g_sound, a, 0);
    r = func_08880D94(g_sound, b, 1);
    func_08880D94(g_sound, c, 2);
    for (i = 0, w = self->wave; i < n; i++, w++) {
        for (j = 0; j < w->n; j++) {
            v = LDCALL(LdFunc1, 0x18)(g_loader, D_09BC8400[w->kind[j]][1]);
            if (sz[i] < v)
                sz[i] = v;
        }
        if (i == 1 && r != 0)
            sz[i] = 0;
    }
    func_08880E50(g_sound, D_089A8278, a, b, c, k, n, 0, sz);
    self->main_models = n;
}

/* model of the next monster of the wave w */
void quest_load_wave_model(Quest *self, u32 w)
{
    QuestWave *e;
    u32 id, a, b, c;

    w &= 0xFF;
    e = &self->wave[w];
    id = (u8)e->kind[e->cur];
    func_08880CF8(g_sound, w);
    a = D_09BC8400[id][0];
    b = D_09BC8400[id][1];
    c = D_09BC8400[id][2];
    func_08880D64(g_sound, (s16)D_089A8278[w], a, b, c, id, w);
    func_08881114(g_sound, D_089A8278[w], a, b, c, id, w);
}

/* models of the monsters already present in the area (slots after Quest.main_models) */
void quest_load_area_models(Quest *self)
{
    u8 k[4] = {0, 0, 0, 0};
    u16 a[4], b[4], c[4];
    s32 i, n = 0;
    u32 v;
    u8 *e;

    for (i = 0; i < 4; i++)
        a[i] = b[i] = c[i] = 0xFFFF;
    for (i = self->main_models; i < 4; i++) {
        func_08880CF8(g_sound, i);
        e = g_monster_models + i * 0xC;
        v = (U16(e, 0x6C) & 3) != 0 ? U16(e, 0x6E) : 0xFFFF;
        if (v == 0xFFFF)
            continue;
        k[i] = v - 0x17DF;
        a[i] = D_09BC8400[k[i]][0];
        b[i] = D_09BC8400[k[i]][1];
        c[i] = D_09BC8400[k[i]][2];
        func_08880D64(g_sound, (s16)D_089A8278[i], a[i], b[i], c[i], k[i], i);
        n++;
    }
    if (n <= 0)
        return;
    func_08880D94(g_sound, a, 0);
    func_08880D94(g_sound, b, 1);
    func_08880D94(g_sound, c, 2);
    func_08880E50(g_sound, D_089A8278, a, b, c, k, self->main_models + n, 1, NULL);
}

/* extra rewards (captures and broken parts) added after the 16 normal ones */
void quest_extra_rewards(Quest *self)
{
    const u8 *e = D_09CDE6C0;
    s32 n = 16, off = 0x40, j;
    u32 id, bit, li, h;
    const s8 *ch;

    for (; e[0] != 0xFF; e += 14) {
        id = e[0];
        if (e[1] == 0x80) {
            /* captures */
            if (self->captures[id] == 0 || (self->flags & QF_NO_HUNT_MSG) != 0)
                continue;
        } else {
            /* the part e[1] is broken and not rewarded yet */
            bit = 1 << e[1];
            if ((self->parts_broken[id] & bit) == 0 || (self->parts_rewarded[id] & bit) != 0)
                continue;
            self->parts_rewarded[id] |= bit;
        }
        li = U16(e, 2 + self->file->kind * 2);
        if (li == 0xFFFF)
            continue;
        if (e[1] == 0x80) {
            h = player_get(g_players, G_MY_SLOT);
            if (hunter_has_skill(h, 0xCE) != 0)
                ch = D_089A7F7C;
            else if (hunter_has_skill(h, 0xCD) != 0)
                ch = D_089A7F78;
            else
                ch = D_089A7F74;
        } else {
            ch = D_089A7F64;
        }
        for (j = 0; n < 32; j++) {
            s8 c = ch[j];

            if (!((s32)(game_rand(g_sys, 0) & 0xFFFF & 0x1F) < c))
                break;
            {
                u16 v = quest_table_rand((const u16 *)D_09CDD850[li], &U16(g_game, off + 0x392));

                n++;
                U16(g_game, off + 0x390) = v;
                off += 4;
            }
        }
    }
}

/* weighted random item of a list {weight, item[, count]}..., 0xFFFF = end */
u16 quest_table_rand(const u16 *l, u16 *out)
{
    s32 st = out == NULL ? 2 : 3;
    const u16 *p;
    u32 sum = 0, r, acc = 0;

    for (p = l; *p != 0xFFFF; p += st)
        sum = (sum + *p) & 0xFFFF;
    r = ((s32)(game_rand(g_sys, 0) & 0xFFFF) % (s32)(sum & 0xFFFF)) & 0xFFFF;
    for (p = l; *p != 0xFFFF; p += st) {
        acc = (acc + *p) & 0xFFFF;
        if ((s32)r < (s32)acc) {
            if (out != NULL)
                *out = p[2];
            return p[1];
        }
    }
    if (out != NULL && *(s16 *)out != 0)
        *out = 0;
    return 0xFFFF;
}

/* slot of the monster in the table of the life records (0xFF: not recorded) */
u32 quest_record_slot(Quest *self, s32 id)
{
    u32 i;

    if ((QUEST->flags & QF_KEEP_LIFE) == 0)
        return 0xFF;
    for (i = 0; i < 8; i = (u8)(i + 1))
        if (D_089A8184[i] == (u8)id)
            return i;
    return 0xFF;
}

/* rank of the quest for the records: 0-1 (village), 2-4 (guild, by stars) */
static s32 record_rank(Quest *self)
{
    s8 st = self->info->stars;
    u32 a = G_QUEST_ID;

    if (a >= 10000 && a < 20000)
        return st < 7 ? 0 : 1;
    if (st < 6)
        return 2;
    if (st < 9)
        return 3;
    return 4;
}

/* sets the life of the recorded monster h from the record (or the default value) */
void quest_record_apply(Quest *self, u8 *h)
{
    u32 k = quest_record_slot(self, h[E_KIND]) & 0xFF;
    s32 r = record_rank(self);
    u8 *rec;

    if ((OPT_FLAGS & 1) != 0)
        rec = g_game + 0x388;
    else
        rec = g_game + r * 0x20 + k * 4 + 0x6A54A;
    if (S16(rec, 0) == 0)
        S16(rec, 0) = D_089A818C[r][k];
    S16(h, 0x41E) = D_089A818C[r][k];
    S16(h, E_LIFE) = S16(rec, 0);
    if (S16(h, E_LIFE) < D_089A81DC[r][k])
        S16(h, E_LIFE) = D_089A81DC[r][k];
    if (G_ARENA != 0)
        return;
    U16(h, E_PARTS) = rec[2];
    self->parts_broken[h[E_KIND]] = U16(h, E_PARTS);
    self->parts_rewarded[h[E_KIND]] = rec[3];
    self->target_life = S16(h, E_LIFE);
    self->record_kind = h[E_KIND];
}

/* saves the state of the recorded monster (life and broken parts) */
void quest_record_save(Quest *self)
{
    u32 k = quest_record_slot(self, self->record_kind);
    u8 *rec, *m;

    if ((k & 0xFF) == 0xFF)
        return;
    rec = g_game + record_rank(self) * 0x20 + (k & 0xFF) * 4 + 0x6A54A;
    m = self->record_target;
    self->parts_broken[self->record_kind] = U16(m, E_PARTS);
    rec[2] = self->parts_broken[self->record_kind];
    rec[3] = self->parts_rewarded[self->record_kind];
    S16(rec, 0) = S16(self->record_target, E_LIFE);
    self->target_life = S16(self->record_target, E_LIFE);
    if (S16(rec, 0) <= 0) {
        S16(rec, 0) = 0;
        rec[2] = 0;
        rec[3] = 0;
    }
}

/* the hunter h starts the second timer (18000 frames) */
void quest_start_timer2(Quest *self, u8 *h)
{
    QuestEvent ev;

    G_TIMER2_ON = 1;
    self->timer2 = 18000;
    if (U16(h, E_SLOT) != G_MY_SLOT)
        return;
    camera_mode_for_area(g_camera);
    ev.type = 0x13;
    ev.zero = 0;
    ev.arg[0] = 0;
    ev.arg[1] = 0;
    send_event(&ev);
}

/* monsters left in the two waves */
s32 quest_waves_left(Quest *self)
{
    s16 n = 0;
    s32 i;
    QuestWave *w = self->wave;

    for (i = 0; i < 2; i++, w++)
        if (w->n > 0)
            n += w->mon[0]->count;
    return n;
}

void quest_timer2_update(Quest *self)
{
    QuestEvent ev;

    if (G_TIMER2_ON != 1 || self->timer2 > 0)
        return;
    if (self->timer2 != 0) {
        G_TIMER2_ON = 0;
        return;
    }
    G_TIMER2_ON = 0;
    self->timer2_count++;
    ev.type = 0x12;
    ev.zero = 0;
    ev.arg[0] = self->timer2_count;
    ev.arg[1] = 0;
    send_event(&ev);
}

/* random size class of the large monster */
void quest_pick_size(Quest *self, s8 *p)
{
    const s8 *t;

    if ((self->flags & QF_FIXED_SIZE) != 0)
        return;
    t = party_has_kitchen_skill(g_game, 0x1D) == 0 ? D_089A8040 : D_089A8054;
    *p = quest_size_clamp(self, *p, t[(s32)(quest_rand(self) & 0xFFFF) % 20]);
}

/* adds b to a, staying in the group of 8 of a (0-7, 8-15, 16-23, 24-31) */
s32 quest_size_clamp(Quest *self, s32 a, s32 b)
{
    s8 lo, hi, v;

    a = (s8)a;
    if (a >= 0 && a < 8) {
        lo = 0;
        hi = 7;
    } else if (a >= 8 && a < 16) {
        lo = 8;
        hi = 0xF;
    } else if (a >= 16 && a < 24) {
        lo = 0x10;
        hi = 0x17;
    } else {
        lo = 0x18;
        hi = 0x1F;
    }
    v = a + b;
    if (v < lo)
        v = lo;
    if (hi < v)
        v = hi;
    return v;
}

/* kind of the goal i: 2 hunt, 1 delivery, 3 repel at half time, 0 other */
u32 quest_goal_kind(Quest *self, s32 i)
{
    u32 f = self->goal_orig[i].flags;

    if ((f & GOAL_HUNT) != 0)
        return 2;
    if ((f & GOAL_DELIVER) != 0)
        return 1;
    if ((f & (GOAL_HALF_TIME | GOAL_REPEL | GOAL_SPECIAL)) ==
        (GOAL_HALF_TIME | GOAL_REPEL | GOAL_SPECIAL))
        return 3;
    return 0;
}

u16 quest_goal_target(Quest *self, s32 i)
{
    return self->goal_orig[i].target;
}

s32 quest_goal_amount(Quest *self, s32 i)
{
    return self->goal_orig[i].count;
}

s32 quest_goal_left_i(Quest *self, s32 i)
{
    return self->goal[i].count;
}
