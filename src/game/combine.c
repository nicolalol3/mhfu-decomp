/* Item combinations (list D_089A47D0, indexed by the first item) and removal of items
 * from the item box of the house (g+0x3428). (Guessed names.) */
#include "menu.h"
#undef pouch_slot
#undef pouch_count
#undef combo_find
#undef combo_rate_of
#undef item_pairs_sort
#undef combo_rate_class

extern void *D_09A4F0C4;
extern const u8 D_089A47D0[];     /* combinations: s16 a, s16 b(=result), ..., 10 bytes each */
extern const u8 D_089A4E88[];     /* per item: count, first combination */
extern const u8 D_089A5B50[];
extern const u8 *D_089A5B34[];
u32 game_rand(void *a, int b);
s32 hunter_has_skill(void *hunter, int id);
void hunter_add_item(void *hunter, u16 id, int a, int b);
int gather_has_tool(void *p, void *hunter, u8 *e, u32 flag);
s32 game_box_size(void *g);

#define BOX(i) (D_089CC438 + 0x4A0 + 0x2F88 + (i) * 4)

static void *own_hunter(void)
{
    return (void *)player_get(D_09A4F0C4, D_089CC438[0x28]);
}

/* the combination of the items a and b (mode -1: without tool, 0: possible now, 1: any) */
u8 *combo_find(void *p, s32 a_arg, s32 b_arg, u32 flags, u32 x, s32 mode)
{
    s16 lo = a_arg, hi = b_arg;
    u8 *e;
    u32 n;

    if (hi < lo) {
        s16 t = lo;

        lo = hi;
        hi = t;
    }
    n = D_089A4E88[lo * 2];
    if (n == 0)
        return NULL;
    e = (u8 *)D_089A47D0 + D_089A4E88[lo * 2 + 1] * 10;
    for (; n != 0; n--, e += 10) {
        if (S16(e, 0) != hi)
            continue;
        if (flags & 1)
            return e;
        if (mode == -1) {
            if ((e[7] & 1) == 0)
                return e;
        } else if (mode == 0) {
            if ((u8)gather_has_tool(p, own_hunter(), e, x) == 1)
                return e;
        } else if (mode == 1) {
            return e;
        }
    }
    return NULL;
}

/* 1 if the combination e is known (flag in g+0x446C) and possible */
int combo_known(void *p, u8 *e, u32 flags, u32 x)
{
    if ((flags & 1) == 0 && (u8)gather_has_tool(p, own_hunter(), e, x) != 1)
        return 0;
    return (D_089CC438[0x446C + (e[6] >> 3)] & (1 << (e[6] & 7))) != 0;
}

int combo_known_ab(void *p, s32 a, s32 b)
{
    u8 *e = combo_find(p, a, b, 0, 0, 0);

    if (e == NULL)
        return 0;
    return (D_089CC438[0x446C + (e[6] >> 3)] & (1 << (e[6] & 7))) != 0;
}

s32 combo_rate(void *p, s32 a, s32 b, s32 mode);
s32 combo_rate_of(void *p, u8 *e, s32 mode);

/* combines a and b: returns the result (or -1) and the number made in out */
s32 combo_do(void *p, u8 *out, void *hunter, u32 a, u32 b, s32 mode)
{
    u8 *e = combo_find(p, a, b, 0, mode == 1, 0);
    s32 res;
    s8 rate;

    if (e == NULL)
        return -1;
    rate = combo_rate(p, a, b, mode);
    if ((s32)((u16)game_rand(D_08A631A0, 1) % 100) < rate) {
        res = S16(e, 2);
        if ((u8)hunter_has_skill(hunter, 0x9F) == 1) {
            *out = D_089A5B50[e[5]];
        } else {
            const u8 *t = D_089A5B34[e[5]];
            s32 r;

            *out = t[0];
            r = (u16)game_rand(D_08A631A0, 1) % 100;
            for (t++; *t <= r; t++) {
                (*out)++;
                r -= *t;
            }
        }
    } else {
        *out = 0;
        res = -1;
    }
    if (mode == 0) {
        hunter_add_item(hunter, a & 0xFFFF, -1, 0);
        hunter_add_item(hunter, b & 0xFFFF, -1, 0);
    }
    return res;
}

/* removes n items id from the box; returns how many could not be removed */
s32 box_remove(void *p, u32 id_arg, s32 n_arg)
{
    u16 id = id_arg;
    s16 n = n_arg;
    s16 count, i;

    if (n < 0 || id == 0 || D_0899D19B[id * 0x18] == 0xFF)
        return 0;
    count = game_box_size(D_089CC438);
    for (i = 0; i < count; i++) {
        u8 *b = BOX(i);

        if (U16(b, 0) != id)
            continue;
        if (S16(b, 2) < n) {
            n = n - S16(b, 2);
            S16(b, 2) = 0;
        } else {
            S16(b, 2) = S16(b, 2) - n;
            n = 0;
        }
        if (S16(b, 2) <= 0)
            U16(b, 0) = 0;
        if (n <= 0)
            return 0;
    }
    return n;
}

/* same, starting from the slot (0xFFFF: from the end of the box) */
s32 box_remove_from(void *p, u32 id_arg, s32 n_arg, u32 slot_arg)
{
    u16 id = id_arg, slot = slot_arg;
    s16 n = n_arg;
    s16 count, i;

    if (n < 0 || id == 0 || D_0899D19B[id * 0x18] == 0xFF)
        return 0;
    count = game_box_size(D_089CC438);
    if (slot == 0xFFFF) {
        for (i = count - 1; i >= 0; i--) {
            u8 *b = BOX(i);

            if (U16(b, 0) != id)
                continue;
            if (S16(b, 2) < n) {
                n = n - S16(b, 2);
                S16(b, 2) = 0;
            } else {
                S16(b, 2) = S16(b, 2) - n;
                n = 0;
            }
            if (S16(b, 2) <= 0)
                U16(b, 0) = 0;
            if (n <= 0)
                return 0;
        }
        return n;
    } else {
        u8 *b = BOX(slot);

        if (S16(b, 2) == 0 || U16(b, 0) != id)
            return (s16)box_remove_from(p, id_arg, n_arg, 0xFFFF);
        if (S16(b, 2) < n) {
            n = n - S16(b, 2);
            S16(b, 2) = 0;
        } else {
            S16(b, 2) = S16(b, 2) - n;
            n = 0;
        }
        if (S16(b, 2) <= 0)
            U16(b, 0) = 0;
        if (n <= 0)
            return n;
        return (s16)box_remove_from(p, id_arg, n, 0xFFFF);
    }
}

/* first slot of the box with the item id (0xFFFF = none) */
u32 box_find(void *p, u32 id_arg)
{
    u16 id = id_arg;
    u32 i;

    if (id == 0xFFFF)
        return 0xFFFF;
    for (i = 0; (s32)i < (s16)game_box_size(D_089CC438); i++) {
        if (U16(BOX(i), 0) == id && S16(BOX(i), 2) > 0)
            return i & 0xFFFF;
    }
    return 0xFFFF;
}

/* success rate of the combination of a and b */
s32 combo_rate(void *p, s32 a, s32 b, s32 mode)
{
    u8 *e = combo_find(p, a, b, mode == 1, 0, 0);

    if (e == NULL)
        return -1;
    return combo_rate_of(p, e, mode);
}

extern const u8 D_089A5B58[], D_089A5B6C[];
extern const u16 D_089A5B60[5];
extern const s16 D_089A5864[][2];  /* pairs of items of the combination list */
extern const u8 D_0899D198[];
s32 func_088D6AC8_(void *hunter, int skill) __asm__("func_088D6AC8");
s32 hunter_felyne_skill(void *hunter, int id);
s32 pouch_count(u8 *g, u32 id_arg);
u32 box_find(void *p, u32 id_arg);
u16 *pouch_slot(u8 *g, s32 i);
int game_test_flag_6a7b4(u8 *g, u32 i_arg);
void game_set_flag_6a7b4(u8 *g, u32 i_arg);

/* success rate (0..100) of the combination e: skills and combination books */
s32 combo_rate_of(void *p, u8 *e, s32 mode)
{
    void *h = own_hunter();
    u8 r;
    u32 i;

    if ((u8)gather_has_tool(p, h, e, mode == 1) != 1)
        return -1;
    if ((e[7] & 2) && (u8)hunter_has_skill(h, 0x45) == 1)
        return 100;
    if ((e[7] & 0x10) && (u8)hunter_has_skill(h, 0xC2) == 1)
        return 100;
    r = D_089A5B58[(s8)e[4]];
    switch ((u8)func_088D6AC8_(h, 0x3B)) {
    case 0x99:
        r += 15;
        break;
    case 0x9A:
        r += 25;
        break;
    case 0x9B:
        r += 45;
        break;
    case 0x9C:
        r -= 5;
        break;
    case 0x9D:
        r -= 10;
        break;
    case 0x9E:
        r -= 15;
        break;
    }
    if (hunter_felyne_skill(h, 0xF) != 0)
        r += 10;
    if (hunter_felyne_skill(h, 0x10) != 0)
        r += 5;
    if (mode == 0) {
        for (i = 0; i < 5; i++) {
            if (pouch_count(D_089CC438, D_089A5B60[i]) == 0)
                break;
        }
        r += D_089A5B6C[i];
    }
    if (mode == 1) {
        for (i = 0; i < 5; i++) {
            int has = pouch_count(D_089CC438, D_089A5B60[i]) != 0;

            if ((U32(D_089CC438, 0x6AF14) & 1) != 0 && (u16)box_find(p, D_089A5B60[i]) != 0xFFFF)
                has = 1;
            if (!has)
                break;
        }
        r += D_089A5B6C[i];
    }
    if (r >= 0x65)
        r = 100;
    return (s8)r;
}

/* marks the combination e as known */
void combo_set_known(void *p, u8 *e)
{
    D_089CC438[0x446C + (e[6] >> 3)] |= 1 << (e[6] & 7);
}

u8 combo_amount(void *p, u8 *e)
{
    return D_089A5B50[e[5]];
}

/* next combination of the list that can be done now (dir: -1 / +1, 0 = the current one) */
u8 *combo_next(void *p, u8 *idx, s32 dir_arg, s16 *a, s16 *b, u32 flags)
{
    s8 dir = dir_arg;
    s16 i = *idx;
    u8 *e;
    s32 n;

    if (dir == 0) {
        e = combo_find(p, D_089A5864[i][0], D_089A5864[i][1], flags, 0, 0);
        if (e != NULL) {
            *a = D_089A5864[i][0];
            *b = D_089A5864[i][1];
            return e;
        }
        dir = 1;
    }
    for (n = 0xAC; n != 0; n--) {
        i = i + dir;
        if (i >= 0xAC)
            i = 0;
        else if (i < 0)
            i = 0xAB;
        e = combo_find(p, D_089A5864[i][0], D_089A5864[i][1], flags, 0, 0);
        if (e != NULL) {
            *idx = i;
            *a = D_089A5864[i][0];
            *b = D_089A5864[i][1];
            return e;
        }
    }
    return NULL;
}

/* number of items id in the first slot of the box (999 = unlimited) */
s32 box_count_first(void *p, u32 id_arg)
{
    u16 id = id_arg;
    s16 count = game_box_size(D_089CC438);
    s32 i;

    for (i = 0; i < count; i++) {
        if (U16(BOX(i), 0) == id) {
            if (D_0899D19B[id * 0x18] == 0xFF)
                return 999;
            return S16(BOX(i), 2);
        }
    }
    return 0;
}

/* total number of items id in the box */
s32 box_count(void *p, u32 id_arg)
{
    u16 id = id_arg;
    s16 count = game_box_size(D_089CC438);
    s32 i, n = 0;

    for (i = 0; i < count; i++) {
        if (U16(BOX(i), 0) == id) {
            if (D_0899D19B[id * 0x18] == 0xFF)
                return 999;
            n += S16(BOX(i), 2);
        }
    }
    return n;
}

/* how many items id still fit in the box */
s32 box_room(void *p, u32 id_arg)
{
    u16 id = id_arg;
    s16 count;
    s32 i, n = 0;

    if (id == 0 || D_0899D19B[id * 0x18] == 0xFF)
        return 0;
    count = game_box_size(D_089CC438);
    for (i = 0; i < count; i++) {
        if (U16(BOX(i), 0) == id)
            n += 99 - S16(BOX(i), 2);
        else if (U16(BOX(i), 0) == 0)
            n += 99;
    }
    return n;
}

/* how many items id can be added before the box is full */
s32 box_room2(void *p, u32 id_arg)
{
    u16 id = id_arg;
    s32 count, max, room;
    u8 *b;

    if (id == 0)
        return -1;
    count = (s16)game_box_size(D_089CC438);
    max = D_0899D19B[id * 0x18];
    if (max != 0xFF)
        max = 99;
    room = max;
    for (b = D_089CC438 + 0x3428; count > 0; count--, b += 4) {
        if (U16(b, 0) == 0)
            return max;
        if (U16(b, 0) == id && max != 0xFF) {
            room -= max - S16(b, 2);
            if (room <= 0)
                return max;
        }
    }
    return max - room;
}

/* item data of the pouch slot i */
const u8 *pouch_item_data(void *p, s32 i)
{
    return D_0899D198 + pouch_slot(D_089CC438, i)[0] * 0x18;
}

void item_set_seen(void *p, u32 id)
{
    game_set_flag_6a7b4(D_089CC438 + 0x4A0, id);
}

int item_seen(void *p, u32 id)
{
    return game_test_flag_6a7b4(D_089CC438 + 0x4A0, id);
}

extern void *D_08A62C50;
extern const u8 *D_08978474[];
extern const u8 D_08978488[], D_089A5597[], D_08959FCC[], D_08959FDF[], D_08960B9C[];
extern const u8 D_089314FC[];
s32 pouch2_count(void *unused, u32 id_arg);
u16 weapon_base_id(void *p, u8 *e);
const char *text_table3(void *g, u32 id);
const char *text_table5(void *g, u32 id);
const char *text_table7(void *g, u32 id);
const char *text_table9(void *g, u32 id);
const char *text_table11(void *g, u32 id);
const char *text_table13(void *g, u32 id);
const char *text_table15(void *g, u32 id);

/* shell sort of (u16 id, s16 n) pairs: by id, then the biggest n first */
void item_pairs_sort(u16 *a, u32 n)
{
    u32 h = 1;
    s32 i, j;

    if (n >= 2) {
        do
            h = h * 3 + 1;
        while (h < n);
    }
    for (;;) {
        h = (s32)h / 3;
        if ((s32)h < 1)
            return;
        if (h >= n)
            continue;
        for (i = h; (u32)i < n; i++) {
            u16 id = a[i * 2];
            s16 v = a[i * 2 + 1];

            for (j = i - h; j >= 0; j -= h) {
                if (!(id < a[j * 2] || (id == a[j * 2] && (s16)a[j * 2 + 1] < v)))
                    break;
                a[(j + h) * 2] = a[j * 2];
                a[(j + h) * 2 + 1] = a[j * 2 + 1];
            }
            a[(j + h) * 2] = id;
            a[(j + h) * 2 + 1] = v;
        }
    }
}

/* index of the first combination rate class not above v */
u32 combo_rate_class(s32 v_arg)
{
    s8 v = v_arg;
    u32 i;

    for (i = 0; i < 6; i++) {
        if (!(v < D_089A5B58[i]))
            break;
    }
    return i;
}

/* list of the items of ids that the hunter has (in the pouch or in the box) */
s32 items_owned(void *p, const u16 *ids, s32 n, s16 *out)
{
    s32 i, k = 0;

    for (i = 0; i < n; i++) {
        s16 a = pouch2_count(D_08A62C50, ids[i]);
        s32 b = box_count(D_08A62C50, ids[i]);

        if (a + b > 0) {
            k++;
            out[0] = ids[i];
            out[1] = a;
            out[2] = b;
            out += 3;
        }
    }
    return k;
}

/* name of an equipment piece of the kind k */
const char *equip_name(void *p, u32 k, u32 id)
{
    switch ((u8)k) {
    case 0:
        return text_table15(D_089CC438, id);
    case 1:
        return text_table7(D_089CC438, id);
    case 2:
        return text_table9(D_089CC438, id);
    case 3:
        return text_table11(D_089CC438, id);
    case 4:
        return text_table13(D_089CC438, id);
    case 5:
        return text_table3(D_089CC438, id);
    case 6:
        return text_table5(D_089CC438, id);
    }
    return NULL;
}

/* armor piece i of the kind k */
const u8 *armor_data(void *p, u32 k, s32 i)
{
    return D_08978474[k & 0xFF] + (s16)i * 0x28;
}

u8 item_rarity(void *p, u32 i)
{
    return D_08978488[i & 0xFFFF];
}

u8 item_rarity_of(void *a, u32 b)
{
    return D_08978488[weapon_base_id(a, (u8 *)b)];
}

/* rarity of the equipment id of the kind k (5 = bowgun, 6 = other weapons) */
u8 equip_rarity(void *p, u32 k, u32 id_arg)
{
    u16 id = id_arg;

    if (id >= 0x5DD)
        return D_089A5597[id];
    if ((u8)k == 5)
        return D_08978488[*(const u16 *)(D_08959FCC + id * 0x18)];
    if ((u8)k == 6)
        return D_08978488[*(const u16 *)(D_08960B9C + id * 0x1C)];
    return 0;
}

u8 bowgun_byte13(void *p, u8 *e)
{
    if (U16(e, 2) == 0)
        return 0;
    if (e[1] == 5)
        return D_08959FDF[U16(e, 2) * 0x18];
    return 0;
}

u8 table_314fc_a(void *p, u32 i)
{
    return D_089314FC[(i & 0xFF) * 2];
}

u8 table_314fc_b(void *p, u32 i)
{
    return D_089314FC[(i & 0xFF) * 2 + 1];
}

extern const u8 D_08960BA4[], D_08960B9F[], D_08959FDA[], D_08959FDC[];
extern const u8 *D_08979448[];
extern s16 D_089CC004;
u8 *deco_data(void *p, u32 id);

/* attack of the weapon e (kind 6), with its upgrades and sharpness bonus */
s32 weapon_attack(void *p, u8 *e)
{
    s16 base, v;
    u8 k;

    if (e[1] != 6)
        return -1;
    base = *(const s16 *)(D_08960BA4 + U16(e, 2) * 0x1C);
    k = equip_rarity(p, e[1], U16(e, 2));
    if (k == 10)
        return base;
    v = base + *(const s16 *)(D_08979448[D_08960B9F[U16(e, 2) * 0x1C]] + (U16(e, 4) & 0xF) * 0x18);
    if (U16(e, 4) & 0x10)
        v += D_089CC004;
    if (U16(e, 4) & 0x20) {
        if (k == 1)
            v += base / 10;
        else
            v += base / 15;
    }
    return v;
}

s8 deco_skill_points(void *p, u32 d, u32 skill);

/* skill points of the skill given by the equipment e (armor piece and decorations) */
s8 equip_skill_points(void *p, u8 *e, u32 skill_arg)
{
    u8 skill = skill_arg;
    u16 id = U16(e, 2);
    s8 n = 0;
    int i;

    if (id == 0)
        return 0;
    switch (e[1]) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4: {
        const u8 *a = D_08978474[e[1]] + id * 0x28;

        for (i = 0; i < 5; i++) {
            if (a[0x1E + i * 2] == skill)
                n += (s8)a[0x1F + i * 2];
        }
    }
        /* fall through */
    case 5:
    case 6:
        for (i = 0; i < 3; i++) {
            if (U16(e, 6 + i * 2) != 0)
                n += deco_skill_points(p, U16(e, 6 + i * 2), skill_arg);
        }
        break;
    }
    return n;
}

/* skill points of a decoration */
s8 deco_skill_points(void *p, u32 d, u32 skill_arg)
{
    u8 skill = skill_arg;
    u8 *x = deco_data(p, d);
    s8 n = 0;
    int i;

    if (x == NULL)
        return 0;
    for (i = 0; i < 4; i++) {
        if (x[8 + i * 2] == skill)
            n += (s8)x[9 + i * 2];
    }
    return n;
}

/* points of the armor set: points of the chest times the number of pieces with the skill */
s8 armor_set_points(void *p, u8 *h, u32 skill)
{
    s8 a = equip_skill_points(p, h + 0x50C, skill);
    s8 c;

    if (a == 0)
        return 0;
    c = 1;
    if (equip_skill_points(p, h + 0x500, 1) != 0)
        c++;
    if (equip_skill_points(p, h + 0x50C, 1) != 0)
        c++;
    if (equip_skill_points(p, h + 0x518, 1) != 0)
        c++;
    if (equip_skill_points(p, h + 0x524, 1) != 0)
        c++;
    if (equip_skill_points(p, h + 0x4F4, 1) != 0)
        c++;
    if (equip_skill_points(p, h + 0x4E8, 1) != 0)
        c++;
    return (s8)(a * c);
}

/* bowgun ammo: number of shots of the ammo kind v (0 if the bowgun cannot use it) */
u8 bowgun_ammo_a(void *p, u32 kind, u32 id, u32 v)
{
    if ((u16)kind != 5)
        return 0;
    if ((u16)v != D_08959FDA[(u16)id * 0x18])
        return 0;
    return D_08959FDA[(u16)id * 0x18 + 1];
}

u8 bowgun_ammo_b(void *p, u32 kind, u32 id, u32 v)
{
    if ((u16)kind != 5)
        return 0;
    if ((u16)v != D_08959FDC[(u16)id * 0x18])
        return 0;
    return D_08959FDC[(u16)id * 0x18 + 1];
}

/* base id of the weapon e */
u16 weapon_base_id(void *p, u8 *e)
{
    if (e[1] == 5)
        return *(const u16 *)(D_08959FCC + U16(e, 2) * 0x18);
    if (e[1] == 6)
        return *(const u16 *)(D_08960B9C + U16(e, 2) * 0x1C);
    return 0;
}

extern const u8 D_0894E628[], D_0894932C[], D_0893B818[];   /* forge recipes (0x1A bytes) */
int village_min_rank(u8 *g, u32 n_arg);

/* can the recipe idx (kind 0..2) be forged? 2: all materials, 1: known, 0: no */
s32 recipe_check(void *p, u32 kind_arg, s32 idx_arg, s32 mode)
{
    u16 kind = kind_arg;
    s16 idx = idx_arg;
    const u8 *r;
    u8 ok = 0, total = 0, known = 0;
    s32 a = 0, b = 0;
    s32 i;

    if (kind == 2)
        r = D_0894E628 + idx * 0x1A;
    else if (kind == 1)
        r = D_0894932C + idx * 0x1A;
    else if (kind == 0)
        r = D_0893B818 + idx * 0x1A;
    else
        return 0;
    for (i = 0; i < 4; i++) {
        u16 id = *(const u16 *)(r + 4 + i * 4);
        s16 n = *(const s16 *)(r + 6 + i * 4);
        int seen;

        if (id == 0 || n == 0) {
            ok |= 1 << i;
            continue;
        }
        seen = (u8)item_seen(D_08A62C50, id) == 1;
        if (mode == 2) {
            a = 0;
            b = box_count(D_08A62C50, id);
        } else if (mode == 1) {
            a = (s16)pouch2_count(D_08A62C50, id);
            b = 0;
        } else if (mode == 0) {
            a = (s16)pouch2_count(D_08A62C50, id);
            b = box_count(D_08A62C50, id);
        }
        if (r[0x14 + i] != 0) {
            total++;
            if (seen)
                known++;
        }
        if (!(a + b < n))
            ok |= 1 << i;
    }
    if (r[1] == 2 && (u8)village_min_rank(D_089CC438, 0xE) == 0)
        return 0;
    if (ok == 0xF)
        return 2;
    if (total != 0 && total == known)
        return 1;
    return r[1] != 0;
}

/* upgrade flags of the weapons of the box (slots of 0xC bytes at g+0x548) */
int slot_is_weapon(void *p, s32 i)
{
    return D_089CC438[0x549 + (s16)i * 0xC] == 6;
}

u32 slot_flag(void *p, s32 i, u32 mask)
{
    u8 *g = D_089CC438 + 0x4A0;

    if ((u8)slot_is_weapon(p, i) == 0)
        return 0xFF;
    return (U16(g + (s16)i * 0xC, 0xAC) & mask) != 0;
}

u32 slot_level(void *p, s32 i)
{
    u8 *g = D_089CC438;

    if ((u8)slot_is_weapon(p, i) == 0)
        return 0;
    return U16(g + (s16)i * 0xC, 0x54C) & 0xF;
}

void slot_level_up(void *p, s32 i)
{
    u8 *s = D_089CC438 + 0x4A0;
    u16 v, l;

    if ((u8)slot_is_weapon(p, i) == 0)
        return;
    s += (s16)i * 0xC;
    v = U16(s, 0xAC);
    l = v & 0xF;
    l = (l < 4) ? l + 1 : 4;
    U16(s, 0xAC) = (v & 0xF0) | l;
}

void slot_set_flag10(void *p, s32 i, s32 clear)
{
    u8 *s = D_089CC438 + 0x4A0;

    if ((u8)slot_is_weapon(p, i) == 0)
        return;
    s += (s16)i * 0xC;
    if (clear == 0) {
        U16(s, 0xAC) |= 0x10;
        U16(s, 0xAC) &= 0xFFDF;
    } else {
        U16(s, 0xAC) &= 0xFFEF;
    }
}

void slot_set_flag20(void *p, s32 i, s32 clear)
{
    u8 *s = D_089CC438 + 0x4A0;

    if ((u8)slot_is_weapon(p, i) == 0)
        return;
    s += (s16)i * 0xC;
    if (clear == 0) {
        U16(s, 0xAC) |= 0x20;
        U16(s, 0xAC) &= 0xFFEF;
        U16(s, 0xAC) &= 0xFF7F;
    } else {
        U16(s, 0xAC) &= 0xFFDF;
    }
}

void slot_set_flag40(void *p, s32 i, s32 clear)
{
    u8 *s = D_089CC438 + 0x4A0;

    if ((u8)slot_is_weapon(p, i) == 0)
        return;
    s += (s16)i * 0xC;
    if (clear == 0)
        U16(s, 0xAC) |= 0x40;
    else
        U16(s, 0xAC) &= 0xFFBF;
}

extern const s32 D_089A5B80[4];
extern const u8 D_089779D4[], D_08959FDE[], D_08960BA8[];
u32 func_0885CB98(void *p, u32 kind, u32 c);
u32 armor_max_level(void *p, u8 *e);

void slot_set_flag80(void *p, s32 i, s32 clear)
{
    u8 *s = D_089CC438 + 0x4A0;

    if ((u8)slot_is_weapon(p, i) == 0)
        return;
    s += (s16)i * 0xC;
    if (clear == 0) {
        U16(s, 0xAC) |= 0x80;
        U16(s, 0xAC) &= 0xFFDF;
    } else {
        U16(s, 0xAC) &= 0xFF7F;
    }
}

/* price of the upgrade level lvl of a weapon (kind 6) */
s32 weapon_upgrade_price(void *p, u32 kind, u32 c, s32 lvl)
{
    if ((u8)kind != 6)
        return 0;
    if (lvl < 0 || lvl >= 4)
        return 0;
    return (func_0885CB98(p, kind, c) / 10) * D_089A5B80[lvl];
}

/* number of decorations of the equipment e */
s32 deco_count(void *p, u8 *e)
{
    s32 i, n = 0;

    for (i = 0; i < 3; i++) {
        if (S16(e, 6 + i * 2) != 0)
            n++;
    }
    return n;
}

s8 equip_slots(void *p, u8 *e);

/* free decoration slots of the equipment e */
s32 deco_free_slots(void *p, u8 *e)
{
    s32 s, used = 0, i;

    if (e[1] >= 7)
        return 0;
    s = equip_slots(p, e);
    if (s <= 0)
        return 0;
    if (s >= 4)
        s = 3;
    for (i = 0; i < s; i++) {
        if (U16(e, 6 + i * 2) != 0) {
            s8 c = *(const s8 *)deco_data(p, U16(e, 6 + i * 2));

            if (c == 0 || c >= 4)
                c = 1;
            used += c;
        }
    }
    if (s < used)
        return 0;
    return s - used;
}

/* data of the decoration item id (NULL if the item is not a decoration) */
u8 *deco_data(void *p, u32 id)
{
    u16 k = D_0899D1AE[(id & 0xFFFF) * 12];

    if (k == 0)
        return NULL;
    return (u8 *)D_089779D4 + k * 0x10;
}

/* 1 if the decoration d can go in the equipment e */
int deco_fits(void *p, u8 *e, u32 d)
{
    u8 *x = deco_data(p, d);
    u8 k;

    if (x == NULL)
        return 0;
    k = e[1];
    if (k == 6)
        return (x[1] & 2) != 0;
    if (k == 5)
        return (x[1] & 1) != 0;
    if (k >= 5)
        return 0;
    return (x[1] & armor_data(p, k, S16(e, 2))[4] & 0xC) != 0;
}

/* the decorations of e, one entry per slot used */
void deco_list(void *p, u8 *e, u16 *out)
{
    s32 i, j;

    out[0] = 0;
    out[1] = 0;
    out[2] = 0;
    for (i = 0; i < 3; i++) {
        if (U16(e, 6 + i * 2) != 0) {
            u8 *d = deco_data(p, U16(e, 6 + i * 2));

            for (j = 0; j < d[0]; j++)
                *out++ = U16(e, 6 + i * 2);
        }
    }
}

/* defense of the armor piece e at its upgrade level */
u32 armor_defense(void *p, u8 *e)
{
    u16 id = U16(e, 2);
    const u8 *a;
    u8 lv, m;

    if (id == 0)
        return 0;
    a = D_08978474[e[1]] + id * 0x28;
    lv = e[4];
    m = armor_max_level(p, e);
    if (m < lv)
        lv = m;
    return (u16)(a[0xC] + lv * 2);
}

/* number of decoration slots of the equipment e */
s8 equip_slots(void *p, u8 *e)
{
    u8 k = e[1];

    if ((u32)(k - 5) < 2) {
        if (k == 5)
            return D_08959FDE[U16(e, 2) * 0x18];
        return D_08960BA8[U16(e, 2) * 0x1C];
    }
    return D_08978474[k][U16(e, 2) * 0x28 + 0x12];
}

extern void *D_08A62C54;
extern const u8 D_08960BA0[], D_08959FD0[];
extern const f32 D_08979720[][2];
u32 armor_upgrade_cost(void *p, u8 *e);
u32 deco_value(void *p, u8 *e);

/* sell price of the armor piece e (half of its price and upgrades, plus the decorations) */
u32 armor_sell_price(void *p, u8 *e)
{
    u8 c[12];
    u32 v;
    u8 m;

    if (U16(e, 2) == 0)
        return 0;
    v = *(const u32 *)(D_08978474[e[1]] + U16(e, 2) * 0x28 + 8);
    m = armor_max_level(p, e);
    if (m != 0) {
        c[0] = e[0];
        c[1] = e[1];
        U16(c, 2) = U16(e, 2);
        U16(c, 6) = U16(e, 6);
        U16(c, 8) = U16(e, 8);
        U16(c, 0xA) = U16(e, 0xA);
        U16(c, 4) = 0;
        while (U16(c, 4) < U16(e, 4)) {
            v += armor_upgrade_cost(p, c);
            U16(c, 4)++;
            if (m < U16(c, 4))
                break;
        }
    }
    return (v >> 1) + deco_value(p, e);
}

/* sell price of the weapon e */
u32 weapon_sell_price(void *p, u8 *e)
{
    u16 id = U16(e, 2);
    u32 v;
    u8 r;

    if (id == 0)
        return 0;
    r = equip_rarity(D_08A62C54, e[1], id);
    if (r == 1 || r == 5) {
        s32 lv = U16(e, 4) & 0xF, i;

        v = *(const u32 *)(D_08960BA0 + U16(e, 2) * 0x1C);
        for (i = 0; i < lv; i++)
            v += weapon_upgrade_price(p, e[1], U16(e, 2), i);
    } else if (e[1] == 6) {
        v = *(const u32 *)(D_08960BA0 + U16(e, 2) * 0x1C);
    } else if (e[1] == 5) {
        v = *(const u32 *)(D_08959FD0 + U16(e, 2) * 0x18);
    } else {
        v = 0;
    }
    return (v >> 1) + deco_value(p, e);
}

/* value of the decorations of e */
u32 deco_value(void *p, u8 *e)
{
    u32 v = 0;
    s32 i;

    if (S16(e, 2) == 0)
        return 0;
    for (i = 0; i < 3; i++) {
        if (U16(e, 6 + i * 2) != 0)
            v += *(const u32 *)&D_0899D198[U16(e, 6 + i * 2) * 0x18 + 0x10];
    }
    return v;
}

/* highest upgrade level of the armor piece e (with the village rank) */
u32 armor_max_level(void *p, u8 *e)
{
    const u8 *a;
    u8 m = 0;
    s32 i;

    if (e[1] >= 5)
        return 0;
    a = D_08978474[e[1]] + U16(e, 2) * 0x28;
    if (a[0x14] > 0)
        m = a[0x14];
    for (i = 0x15; i <= 0x19; i++) {
        if (m < a[i])
            m = a[i];
    }
    if ((u8)village_min_rank(D_089CC438, 0xE) == 1 && m < a[0x1A])
        m = a[0x1A];
    if (m != 0)
        m--;
    if (m >= 0x40)
        m = 0x3F;
    return m;
}

/* class of the next upgrade of the armor piece e (-1: no more upgrades) */
s32 armor_upgrade_class(void *p, u8 *e)
{
    u8 l = U16(e, 4) + 1;
    const u8 *a;
    u8 m;

    if (e[1] >= 5)
        return 0;
    a = D_08978474[e[1]] + U16(e, 2) * 0x28;
    m = armor_max_level(p, e);
    if (m < l || m == 0)
        return -1;
    if (l < a[0x14])
        return 0;
    if (l < a[0x15])
        return 1;
    if (l < a[0x16])
        return 2;
    if (l < a[0x17])
        return 3;
    if (l < a[0x18])
        return 4;
    if ((u8)village_min_rank(D_089CC438, 0xE) == 0)
        return 5;
    if (l < a[0x19])
        return 5;
    return 6;
}

/* price of the next upgrade of the armor piece e */
u32 armor_upgrade_cost(void *p, u8 *e)
{
    f32 v;
    s8 k;

    if (e[1] >= 5)
        return 0;
    v = (f32)*(const u32 *)(D_08978474[e[1]] + U16(e, 2) * 0x28 + 8);
    k = armor_upgrade_class(p, e);
    if (k < 0)
        return 0;
    return (u32)(v * D_08979720[k][0] + D_08979720[k][1]);
}
