/* Quest (D_08A62C7C): network events, loading of the quest file and of the monsters of the
 * area, records of the hunts, sizes and helpers of the goals. (Guessed names.) */
#include "menu.h"

typedef struct QEvent {
    u8 type, b1;
    u16 a, b, c, d;
} QEvent;

extern void *D_09A1DAEC;
extern u8 *D_09C121B0;
extern u8 *D_09A4F0D0;
extern void **D_089CC434;          /* file loader (C++ object) */
extern void *D_09A0B540;
extern void *D_09A44C60;
extern u8 *D_08A62D8C;
extern void *D_089CBBE4;
extern u8 D_08A5C440[];
extern const u8 *const *D_089A7CE8[];   /* quest id -> file id tables */
extern const u16 D_09BC8400[][3];        /* monster models: 3 file ids each */
extern const s32 D_089A8278[];
extern const s32 D_089ABD18[][2];        /* min / max size of each monster */
extern const u8 D_09CDE6C0[];            /* extra rewards: 14 bytes each, 0xFF = end */
extern const u32 D_09CDD850[];
extern const s8 D_089A7F64[], D_089A7F74[], D_089A7F78[], D_089A7F7C[];
extern const u8 D_089A8184[8];
extern const s16 D_089A818C[][8], D_089A81DC[][8];
extern const s8 D_089A8040[20], D_089A8054[20];

typedef s32 (*LdFunc0)(void *);
typedef s32 (*LdFunc1)(void *, u32);
typedef s32 (*LdFunc6)(void *, u32, void *, s32, s32, s32, s32);
#define LDCALL(type, off) ((type)((void **)*D_089CC434)[(off) / 4])

void func_088AB514(void *a, int b, void *c);
u8 *quest_table_entry(u8 *self, s32 i);
u8 *quest_find_entry(u8 *self, u32 id, s32 which, s32 idx);
u8 *quest_reward_list(u8 *self, s32 which);
u8 *quest_find_monster(u8 *self, u8 *h);
void quest_check_monsters_done(u8 *self, u32 id);
void quest_reset_monsters(u8 *self, u32 id);
void quest_target_killed(u8 *self, u8 *h);
u32 quest_rand(u8 *self);
u16 game_map_id(u8 *g);
int party_has_kitchen_skill(u8 *p, u32 k);
void camera_mode_for_area(void *cam);
u32 func_088900C4(void *a, int b);
s32 func_088D6A1C(u32 h, int a);
s32 func_08898694(void *a, int b, s32 c);
void func_08898748(void *a, int b);
s32 func_08898898(void *a, int b);
void func_088C2BE0(void *a, void *b, s32 c);
s32 func_088DDFE8(u8 *a, u32 b);
void func_088DDE20(u8 *a, u32 b);
void func_088DE0B8(u8 *a, u32 b, u32 c, s32 d);
void func_088DE22C(u8 *a, u32 b, u32 c);
void func_088DE3D4(u8 *a, u32 b);
void func_09AF4050(u8 *p);
void func_09AB2E68(u8 *a, s32 i);
void func_09AB46A8(u8 *a, u32 b);
u8 *func_09AB2958(u8 *a, u8 *m);
void func_09AB35F0(u8 *a);
void func_08880CF8(u8 *snd, s32 i);
void func_08880D64(u8 *snd, s32 a, u32 b, u32 c, u32 d, u32 k, s32 i);
u8 func_08880D94(u8 *snd, u16 *ids, s32 i);
void func_08880E50(u8 *snd, const s32 *t, u16 *a, u16 *b, u16 *c, u8 *k, s32 n, s32 f, u32 *sz);
void func_08881114(u8 *snd, s32 a, u32 b, u32 c, u32 d, u32 k, s32 i);

u32 quest_area_has_monster(u8 *self, u32 k);
void quest_monster_detach(u8 *self, u8 *p);
u16 quest_table_rand(const u16 *l, u16 *out);
u32 quest_record_slot(u8 *self, s32 id);
s32 quest_size_clamp(u8 *self, s32 a, s32 b);

static void send_event(QEvent *ev)
{
    func_088AB514(D_09A1DAEC, 6, ev);
}

/* time left, sent to the other players */
void quest_send_time(u8 *self)
{
    QEvent ev;

    if (D_089CC438[0x422] != 0)
        return;
    ev.type = 5;
    ev.a = 0;
    ev.c = U32(self, 0x10);
    ev.b = U32(self, 0x10) >> 16;
    send_event(&ev);
}

void quest_send_killed(u8 *self, u8 *m, u8 *h)
{
    QEvent ev;

    ev.type = 7;
    ev.a = 0;
    ev.b = S16(m, 0x2C);
    ev.c = U16(h, 0x29A) | (u16)(*(s8 *)(m + 4) << 8);
    send_event(&ev);
}

void quest_send_captured(u8 *self, u8 *m, u8 *h)
{
    QEvent ev;

    ev.type = 8;
    ev.a = 0;
    ev.b = U16(m, 0x2C);
    ev.c = U16(h, 0x29A);
    send_event(&ev);
}

u16 quest_table_item(u8 *self, u32 i)
{
    return quest_table_rand((const u16 *)quest_table_entry(self, i & 0xFFFF), NULL);
}

/* loads the file of the quest id (0: not loaded yet, 1: done) */
s32 quest_load_file(u8 *self, u32 id, u8 *dst)
{
    u32 k, f;

    if (self[0x716] == 1) {
        if (LDCALL(LdFunc0, 0x38)(D_089CC434) != 0)
            return 0;
        if (dst == NULL)
            dst = D_08A5C440;
        func_088C2BE0(D_09A44C60, dst, func_08898898(D_09A0B540, 0x17));
        func_08898748(D_09A0B540, 0x17);
        self[0x716] = 0;
        return 1;
    }
    if (self[0x716] != 0)
        return 0;
    id &= 0xFFFF;
    if (id >= 60000)
        return 1;
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
    if (U16(self, 0x714) == f)
        return 1;
    LDCALL(LdFunc6, 0x34)(D_089CC434, f, (void *)func_08898694(D_09A0B540, 0x17, 0x6800), -1, 0, 0, 1);
    U16(self, 0x714) = f;
    self[0x716]++;
    return 0;
}

void quest_load_init(u8 *self)
{
    self[0x716] = 0;
    U16(self, 0x714) = 0xFFFF;
}

/* loads the models of the monsters of the waves (biggest file first) */
void quest_load_wave_models(u8 *self)
{
    u32 sz[2];
    u8 done[2] = {0xFF, 0xFF};
    s32 w, j, k, uniq;
    u8 *e;
    u32 v;

    for (w = 0, e = self; w < 2; w++, e += 0x2C) {
        sz[w] = 0;
        for (j = 0; j < *(s8 *)(e + 0x786); j++) {
            v = LDCALL(LdFunc1, 0x18)(D_089CC434, (e[0x77C + j] + 0x17DF) & 0xFFFF);
            if (sz[w] < v)
                sz[w] = v;
        }
    }
    for (w = 0, e = self; w < 2; w++, e += 0x2C) {
        if (*(s8 *)(e + 0x786) <= 0)
            continue;
        uniq = 1;
        for (k = 0; k < w; k++) {
            if (done[k] == *(s8 *)(e + 0x77C)) {
                uniq = 0;
                break;
            }
        }
        if (uniq) {
            func_088DE0B8(D_09A4F0D0, e[0x77C], sz[w], w);
            done[w] = e[0x77C];
        }
    }
}

/* small monsters of the area: id 0x12 (in the map of the quest), 10 and 0x4A in some
 * areas, then the list of the area in the quest file */
static int area_has_10(u8 *self, u32 a)
{
    if ((U32(self, 0x28) & 0x1000010) != 0)
        return 0;
    switch (a) {
    case 0x29: case 0xC2: case 0x36: case 0x94: case 0x10: case 0x97: case 1: case 0x6E:
    case 0x5C: case 0x65: case 0xCD: case 0xE3:
        return 1;
    }
    return 0;
}

static int area_has_4a(u8 *self, u32 a)
{
    if ((U32(self, 0x28) & 0x10) == 0)
        return 0;
    switch (a) {
    case 0x15: case 0x8C: case 0x97: case 0xA1: case 0x6E: case 0x62: case 0xF7:
        return 1;
    }
    return 0;
}

static void load_monster(u32 k)
{
    u8 *m = D_09A4F0D0;

    if (m[0x214] != 0 && func_088DDFE8(D_09A4F0D0, k) != 0)
        func_088DDE20(m, k);
    else
        func_088DE0B8(m, k, 0, -1);
}

void quest_load_area_monsters(u8 *self)
{
    u32 a = U16(D_089CC438, 0x6AF0E);
    u32 *l;
    s32 i;

    quest_check_monsters_done(D_08A62C7C, a);
    if (a == game_map_id(D_089CC438))
        load_monster(0x12);
    if (area_has_10(self, a))
        load_monster(10);
    if (area_has_4a(self, a))
        load_monster(0x4A);
    l = (u32 *)quest_find_entry(self, a, 0, *(s8 *)(self + 0x20));
    self[0x82] = self[0x20];
    if (l == NULL)
        return;
    for (i = 0; i < 4; i++, l++)
        if (*l != 0xFFFFFFFF)
            load_monster(*l & 0xFF);
}

/* slots of the small monsters of the area a (after the large ones of the waves) */
void quest_place_area_monsters(u8 *self, u32 a)
{
    u8 *q = D_08A62C7C;
    u32 n = *(s8 *)(q + 0x786) > 0;
    u32 *l;
    s32 i;

    if (*(s8 *)(q + 0x7B2) > 0 && *(s8 *)(q + 0x77C) != *(s8 *)(q + 0x7A8))
        n++;
    if ((a & 0xFFFF) == game_map_id(D_089CC438))
        func_088DE22C(D_09A4F0D0, 0x12, n++);
    if (area_has_10(self, a & 0xFFFF))
        func_088DE22C(D_09A4F0D0, 10, n++);
    if (area_has_4a(self, a & 0xFFFF))
        func_088DE22C(D_09A4F0D0, 0x4A, n++);
    l = (u32 *)quest_find_entry(self, a, 0, *(s8 *)(self + 0x20));
    if (l == NULL)
        return;
    for (i = 0; i < 4; i++, l++)
        if (*l != 0xFFFFFFFF)
            func_088DE22C(D_09A4F0D0, *l & 0xFF, n++);
}

/* 1 when the small monster k belongs to the current area */
u32 quest_area_has_monster(u8 *self, u32 k)
{
    u32 *l;

    k &= 0xFF;
    if (k == 0x12 || k == 10 || k == 0x4A)
        return 1;
    l = (u32 *)quest_find_entry(self, U16(D_089CC438, 0x6AF0E), 0, *(s8 *)(self + 0x82));
    if (l == NULL)
        return 0;
    for (; *l != 0xFFFFFFFF; l++)
        if ((*l & 0xFF) == k)
            return 1;
    return 0;
}

/* change of area: the monsters of the old area are removed */
void quest_leave_area(u8 *self)
{
    s32 i;
    u8 *p, *e;
    u32 *l;

    for (i = 0; i < 20; i++) {
        p = (i >= 0 && i < 20) ? (u8 *)U32(D_09C121B0, 0x1220 + i * 4) : NULL;
        if (p == NULL || quest_area_has_monster(self, p[0x1E8]) == 0)
            continue;
        e = quest_find_monster(self, p);
        if (e != NULL && S16(e, 0x2E) != 1) {
            if (p[0x1E8] == 0x17)
                func_09AF4050(p);
            if (p[0x1D4] < 2 && p[0x298] != 5) {
                S16(e, 0x32) = S16(p, 0x2E4);
            } else {
                quest_target_killed(self, p);
                if ((S16(e, 0x2E) & 4) != 0) {
                    S16(e, 0x2E) = 2;
                    S16(e, 0x32) = -1;
                }
            }
            e[0x34] = 0xFF;
        }
        quest_monster_detach(self, p);
        func_09AB2E68(D_09C121B0, i);
    }
    quest_reset_monsters(self, U16(D_089CC438, 0x6AF0E));
    func_09AB46A8(D_09C121B0, 0x12);
    func_09AB46A8(D_09C121B0, 10);
    func_09AB46A8(D_09C121B0, 0x4A);
    func_088DE3D4(D_09A4F0D0, 0x12);
    func_088DE3D4(D_09A4F0D0, 10);
    func_088DE3D4(D_09A4F0D0, 0x4A);
    l = (u32 *)quest_find_entry(self, U16(D_089CC438, 0x6AF0E), 0, *(s8 *)(self + 0x82));
    if (l == NULL)
        return;
    for (; *l != 0xFFFFFFFF; l++) {
        func_09AB46A8(D_09C121B0, *l & 0xFF);
        func_088DE3D4(D_09A4F0D0, *(u8 *)l);
    }
}

/* the hunters that were riding / attached to the monster 0x12 are released */
void quest_monster_detach(u8 *self, u8 *p)
{
    s32 i;
    u8 *h;

    if (*(s8 *)(p + 0x1E8) != 0x12)
        return;
    for (i = 0; i < 4; i++) {
        h = (u8 *)func_088DCEBC(D_09A4F0C4, i);
        if (h != NULL && (U32(h, 4) & 1) != 0 && U32(h, 0x560) == (u32)p)
            U32(h, 0x560) = 0;
    }
}

/* spawns the monsters of the list 1; the sizes of monsters of the same kind are spread */
void quest_spawn_sub_monsters(u8 *self)
{
    u8 *l = quest_reward_list(self, 1);
    u32 psz = 0, pk = 0, first = 0;
    s32 pv = 0, d;
    u8 *p = NULL;
    s16 v;

    if (l != NULL) {
        while (S16(l, 0) >= 0) {
            p = func_09AB2958(D_09C121B0, l);
            if (p != NULL && *(s8 *)(l + 0x35) != 0) {
                U32(self, 0x720) = (u32)p;
                if (D_09BC8400[p[0x1E8]][0] == D_09BC8400[pk][0]) {
                    v = S16(p, 0x5F4);
                    d = v - pv;
                    if (d < 0)
                        d = -d;
                    if (d < 200) {
                        if (psz < U16(p, 0x274)) {
                            S16(p, 0x5F4) = v - 200;
                            if (S16(p, 0x5F4) < D_089ABD18[pk][0])
                                S16(p, 0x5F4) += 200;
                        } else if (U16(p, 0x274) < psz) {
                            S16(p, 0x5F4) = v + 200;
                            if (D_089ABD18[pk][1] < S16(p, 0x5F4))
                                S16(p, 0x5F4) -= 200;
                        } else {
                            S16(p, 0x5F4) = v - 200;
                            if (!(S16(p, 0x5F4) < D_089ABD18[pk][0]))
                                break;
                            S16(p, 0x5F4) += 200;
                            S16(p, 0x5F4) += 200;
                            if (D_089ABD18[pk][1] < S16(p, 0x5F4))
                                S16(p, 0x5F4) -= 200;
                        }
                    }
                }
            }
            if (first == 0 && p != NULL) {
                pv = S16(p, 0x5F4);
                pk = p[0x1E8];
                psz = U16(p, 0x274);
                first++;
            }
            l += 0x3C;
        }
    }
    func_09AB35F0(D_09C121B0);
}

/* sounds / models of the main monsters of the quest (up to 4) */
void quest_load_main_models(u8 *self)
{
    s32 *l = (s32 *)quest_reward_list(self, 0);
    u8 k[4] = {0, 0, 0, 0};
    u32 sz[4] = {0, 0, 0, 0};
    u16 a[4], b[4], c[4];
    s32 i, j, n = 0;
    u8 r;
    u8 *w;
    u32 v;

    for (i = 0; i < 4; i++)
        a[i] = b[i] = c[i] = 0xFFFF;
    if (l == NULL)
        return;
    for (i = 0; i < 4; i++, l++) {
        if (*l == -1)
            break;
        func_08880CF8(D_08A62D8C, i);
        a[i] = D_09BC8400[*l][0];
        b[i] = D_09BC8400[*l][1];
        c[i] = D_09BC8400[*l][2];
        k[i] = *l;
        func_08880D64(D_08A62D8C, (s16)D_089A8278[i], a[i], b[i], c[i], k[i], i);
        n++;
    }
    if (n <= 0)
        return;
    if (k[0] == k[1])
        n--;
    func_08880D94(D_08A62D8C, a, 0);
    r = func_08880D94(D_08A62D8C, b, 1);
    func_08880D94(D_08A62D8C, c, 2);
    for (i = 0, w = self; i < n; i++, w += 0x2C) {
        for (j = 0; j < *(s8 *)(w + 0x786); j++) {
            v = LDCALL(LdFunc1, 0x18)(D_089CC434, D_09BC8400[*(s8 *)(w + 0x77C + j)][1]);
            if (sz[i] < v)
                sz[i] = v;
        }
        if (i == 1 && r != 0)
            sz[i] = 0;
    }
    func_08880E50(D_08A62D8C, D_089A8278, a, b, c, k, n, 0, sz);
    S16(self, 0x5C0) = n;
}

/* model of the next monster of the wave w */
void quest_load_wave_model(u8 *self, u32 w)
{
    u8 *e;
    u32 id, a, b, c;

    w &= 0xFF;
    e = self + w * 0x2C;
    id = e[0x77C + *(s8 *)(e + 0x787)];
    func_08880CF8(D_08A62D8C, w);
    a = D_09BC8400[id][0];
    b = D_09BC8400[id][1];
    c = D_09BC8400[id][2];
    func_08880D64(D_08A62D8C, (s16)D_089A8278[w], a, b, c, id, w);
    func_08881114(D_08A62D8C, D_089A8278[w], a, b, c, id, w);
}

/* models of the monsters already present in the area (slots after 0x5C0) */
void quest_load_area_models(u8 *self)
{
    u8 k[4] = {0, 0, 0, 0};
    u16 a[4], b[4], c[4];
    s32 i, n = 0;
    u32 v;
    u8 *e;

    for (i = 0; i < 4; i++)
        a[i] = b[i] = c[i] = 0xFFFF;
    for (i = S16(self, 0x5C0); i < 4; i++) {
        func_08880CF8(D_08A62D8C, i);
        e = D_09A4F0D0 + i * 0xC;
        v = (U16(e, 0x6C) & 3) != 0 ? U16(e, 0x6E) : 0xFFFF;
        if (v == 0xFFFF)
            continue;
        k[i] = v - 0x17DF;
        a[i] = D_09BC8400[k[i]][0];
        b[i] = D_09BC8400[k[i]][1];
        c[i] = D_09BC8400[k[i]][2];
        func_08880D64(D_08A62D8C, (s16)D_089A8278[i], a[i], b[i], c[i], k[i], i);
        n++;
    }
    if (n <= 0)
        return;
    func_08880D94(D_08A62D8C, a, 0);
    func_08880D94(D_08A62D8C, b, 1);
    func_08880D94(D_08A62D8C, c, 2);
    func_08880E50(D_08A62D8C, D_089A8278, a, b, c, k, S16(self, 0x5C0) + n, 1, NULL);
}

/* extra rewards (carves, captures, broken parts) added after the 16 normal ones */
void quest_extra_rewards(u8 *self)
{
    const u8 *e = D_09CDE6C0;
    s32 n = 16, off = 0x40, j;
    u32 id, bit, li, h;
    const s8 *ch;
    u8 *p;

    for (; e[0] != 0xFF; e += 14) {
        id = e[0];
        if (e[1] == 0x80) {
            if (U16(self, 0x350 + id * 2) == 0 || (U32(self, 0x28) & 0x400000) != 0)
                continue;
        } else {
            p = self + id;
            bit = 1 << e[1];
            if ((p[0x5C2] & bit) == 0 || (p[0x61C] & bit) != 0)
                continue;
            p[0x61C] |= bit;
        }
        li = U16(e, 2 + *(s8 *)(U32(self, 0x4C) + 0x44) * 2);
        if (li == 0xFFFF)
            continue;
        if (e[1] == 0x80) {
            h = func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]);
            if (func_088D6A1C(h, 0xCE) != 0)
                ch = D_089A7F7C;
            else if (func_088D6A1C(h, 0xCD) != 0)
                ch = D_089A7F78;
            else
                ch = D_089A7F74;
        } else {
            ch = D_089A7F64;
        }
        for (j = 0; n < 32; j++) {
            s8 c = ch[j];

            if (!((s32)(func_088900C4(D_08A631A0, 0) & 0xFFFF & 0x1F) < c))
                break;
            {
                u16 v = quest_table_rand((const u16 *)D_09CDD850[li], &U16(D_089CC438, off + 0x392));

                n++;
                U16(D_089CC438, off + 0x390) = v;
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
    r = ((s32)(func_088900C4(D_08A631A0, 0) & 0xFFFF) % (s32)(sum & 0xFFFF)) & 0xFFFF;
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

/* slot of the monster in the table of the hunting records (0xFF: not recorded) */
u32 quest_record_slot(u8 *self, s32 id)
{
    u32 i;

    if ((U32(D_08A62C7C, 0x28) & 0x80) == 0)
        return 0xFF;
    for (i = 0; i < 8; i = (u8)(i + 1))
        if (D_089A8184[i] == (u8)id)
            return i;
    return 0xFF;
}

/* rank of the quest for the records: 0-1 (village), 2-4 (guild, by stars) */
static s32 record_rank(u8 *self)
{
    s8 st = *(s8 *)(U32(self, 0x68) + 0x1A);
    u32 a = U16(D_089CC438, 0x6AF12);

    if (a >= 10000 && a < 20000)
        return st < 7 ? 0 : 1;
    if (st < 6)
        return 2;
    if (st < 9)
        return 3;
    return 4;
}

/* sets the life of the recorded monster h from the record (or the default value) */
void quest_record_apply(u8 *self, u8 *h)
{
    u32 k = quest_record_slot(self, h[0x1E8]) & 0xFF;
    s32 r = record_rank(self);
    u8 *rec;

    if ((*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) != 0)
        rec = D_089CC438 + 0x388;
    else
        rec = D_089CC438 + r * 0x20 + k * 4 + 0x6A54A;
    if (S16(rec, 0) == 0)
        S16(rec, 0) = D_089A818C[r][k];
    S16(h, 0x41E) = D_089A818C[r][k];
    S16(h, 0x2E4) = S16(rec, 0);
    if (S16(h, 0x2E4) < D_089A81DC[r][k])
        S16(h, 0x2E4) = D_089A81DC[r][k];
    if (D_089CC438[0x480] != 0)
        return;
    U16(h, 0x774) = rec[2];
    self[0x5C2 + h[0x1E8]] = U16(h, 0x774);
    self[0x61C + h[0x1E8]] = rec[3];
    S16(self, 0x114) = S16(h, 0x2E4);
    self[0x74A] = h[0x1E8];
}

/* saves the state of the recorded monster (life and broken parts) */
void quest_record_save(u8 *self)
{
    u32 k = quest_record_slot(self, self[0x74A]);
    u8 *rec, *m;

    if ((k & 0xFF) == 0xFF)
        return;
    rec = D_089CC438 + record_rank(self) * 0x20 + (k & 0xFF) * 4 + 0x6A54A;
    m = (u8 *)U32(self, 0x720);
    self[0x5C2 + self[0x74A]] = U16(m, 0x774);
    rec[2] = self[0x5C2 + self[0x74A]];
    rec[3] = self[0x61C + self[0x74A]];
    S16(rec, 0) = S16((u8 *)U32(self, 0x720), 0x2E4);
    S16(self, 0x114) = S16((u8 *)U32(self, 0x720), 0x2E4);
    if (S16(rec, 0) <= 0) {
        S16(rec, 0) = 0;
        rec[2] = 0;
        rec[3] = 0;
    }
}

/* the hunter h starts the second timer (18000 frames) */
void quest_start_timer2(u8 *self, u8 *h)
{
    QEvent ev;

    D_089CC438[0x418] = 1;
    U32(self, 0x678) = 18000;
    if (U16(h, 0x1E6) != D_089CC438[0x28])
        return;
    camera_mode_for_area(D_089CBBE4);
    ev.type = 0x13;
    ev.a = 0;
    ev.b = 0;
    ev.c = 0;
    send_event(&ev);
}

/* monsters left in the two waves */
s32 quest_waves_left(u8 *self)
{
    s16 n = 0;
    s32 i;
    u8 *w = self;

    for (i = 0; i < 2; i++, w += 0x2C)
        if (*(s8 *)(w + 0x786) > 0)
            n += *(s8 *)(U32(w, 0x768) + 4);
    return n;
}

void quest_timer2_update(u8 *self)
{
    QEvent ev;

    if (D_089CC438[0x418] != 1 || (s32)U32(self, 0x678) > 0)
        return;
    if (U32(self, 0x678) != 0) {
        D_089CC438[0x418] = 0;
        return;
    }
    D_089CC438[0x418] = 0;
    self[0x724]++;
    ev.type = 0x12;
    ev.a = 0;
    ev.b = *(s8 *)(self + 0x724);
    ev.c = 0;
    send_event(&ev);
}

/* random size class of the large monster */
void quest_pick_size(u8 *self, s8 *p)
{
    const s8 *t;

    if ((U32(self, 0x28) & 0x4000) != 0)
        return;
    t = party_has_kitchen_skill(D_089CC438, 0x1D) == 0 ? D_089A8040 : D_089A8054;
    *p = quest_size_clamp(self, *p, t[(s32)(quest_rand(self) & 0xFFFF) % 20]);
}

/* adds b to a, staying in the group of 8 of a (0-7, 8-15, 16-23, 24-31) */
s32 quest_size_clamp(u8 *self, s32 a, s32 b)
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

/* kind of the goal i: 2 hunt, 1 delivery, 3 timed, 0 other */
u32 quest_goal_kind(u8 *self, s32 i)
{
    u32 f = U32(self, 0x738 + i * 8);

    if ((f & 1) != 0)
        return 2;
    if ((f & 2) != 0)
        return 1;
    if ((f & 0x18004) == 0x18004)
        return 3;
    return 0;
}

u16 quest_goal_target(u8 *self, s32 i)
{
    return U16(self, 0x73C + i * 8);
}

s32 quest_goal_amount(u8 *self, s32 i)
{
    return S16(self, 0x73E + i * 8);
}

s32 quest_goal_left_i(u8 *self, s32 i)
{
    return S16(self, 0x72E + i * 8);
}
