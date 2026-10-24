/* Menu: list of the active skills of the equipment (entries of 0x12 bytes at self+0x684
 * and self+0xA50), multi-line texts. Same class as menu_box.c. (Guessed names.)
 *
 * Skill entry: u16 sort key, u8 flags, u8 skill id, s8 points[6] (weapon, 4 armor
 * pieces, ...), s8 total, u8 colors[7]. */
#include "menu.h"

extern void *D_08A62C54;          /* equipment data */
extern void *D_089CBBE8;          /* text converter */
extern const s32 D_0893AEB8[5];
extern u8 *const D_08978474[];      /* armor tables (0x28 bytes per piece) */

s32 equip_skill_points(void *a, void *eq, u8 skill);
s32 armor_set_points(void *a, void *hunter, u8 skill);
u8 func_088D6AC8(void *hunter, int skill);
u8 *func_0885CA50(void *a, void *eq);
u8 equip_rarity(void *a, int type, u16 id);
u8 table_314fc_b(void *a, u8 v);
u32 func_088D7DE8(void *hunter);
void func_088D897C(void *hunter);
s16 func_08828658(void *self, void *hunter);
s32 func_08890F70(void *font, const char *s);
int utf8_seq_len(void *self, u32 c_arg);
void equip_set_weapon(u8 *hunter, u8 *eq);
int skill_entry_flags(void *self, u8 *hunter, u8 *e);

/* fills the entry e (skill e[3]) with the points of each piece of the hunter;
 * with cmp != 0 colors the points compared with the list at self+0xA50.
 * Returns 0 if the skill has no points. */
int skill_entry_fill(u8 *self, u8 *hunter, u8 *e, int cmp)
{
    u8 *l;
    s32 n;
    u32 i;
    s8 t;

    if (e[3] == 1)
        return skill_entry_flags(self, hunter, e);
    e[4] = equip_skill_points(D_08A62C54, hunter + 0x4E8, e[3]);
    e[5] = equip_skill_points(D_08A62C54, hunter + 0x500, e[3]);
    e[6] = armor_set_points(D_08A62C54, hunter, e[3]);
    e[7] = equip_skill_points(D_08A62C54, hunter + 0x518, e[3]);
    e[8] = equip_skill_points(D_08A62C54, hunter + 0x524, e[3]);
    e[9] = equip_skill_points(D_08A62C54, hunter + 0x4F4, e[3]);
    if (e[9] == 0 && e[8] == 0 && e[7] == 0 && e[6] == 0 && e[4] == 0 && e[5] == 0)
        return 0;
    e[2] = 0x80;
    e[10] = e[9] + e[8] + e[7] + e[6] + e[4] + e[5];
    U16(e, 0) = 0;
    if (func_088D6AC8(hunter, e[3]) == 0) {
        e[17] = 0; e[16] = 0; e[15] = 0; e[14] = 0; e[13] = 0; e[12] = 0; e[11] = 0;
    } else {
        e[2] |= 1;
        U16(e, 0) -= 0x8000;
        e[17] = 3; e[16] = 3; e[15] = 3; e[14] = 3; e[13] = 3; e[12] = 3; e[11] = 3;
    }
    t = e[10];
    if (t < 0)
        U16(e, 0) += t * -0x80;
    else
        U16(e, 0) += t * 0x80 + 0x4000;
    U16(e, 0) += e[3];
    if (cmp != 0) {
        l = self + 0xA50;
        for (n = 54; n != 0; n--, l += 0x12) {
            if (l[2] == 0)
                break;
            if (e[3] == l[3]) {
                for (i = 0; i < 6; i++) {
                    if ((s8)l[4 + i] < (s8)e[4 + i])
                        e[11 + i] = 4;
                    else if ((s8)e[4 + i] < (s8)l[4 + i])
                        e[11 + i] = 2;
                }
                return 1;
            }
        }
        for (i = 0; i < 6; i++) {
            if ((s8)e[4 + i] > 0)
                e[11 + i] = 4;
            else if ((s8)e[4 + i] < 0)
                e[11 + i] = 2;
        }
    }
    return 1;
}

/* entry of the special skill 1: which pieces give it */
int skill_entry_flags(void *self, u8 *hunter, u8 *e)
{
    int any;

    e[4] = (s8)equip_skill_points(D_08A62C54, hunter + 0x4E8, e[3]) > 0;
    e[5] = (s8)equip_skill_points(D_08A62C54, hunter + 0x500, e[3]) > 0;
    e[6] = (s8)equip_skill_points(D_08A62C54, hunter + 0x50C, e[3]) > 0;
    e[7] = (s8)equip_skill_points(D_08A62C54, hunter + 0x518, e[3]) > 0;
    e[8] = (s8)equip_skill_points(D_08A62C54, hunter + 0x524, e[3]) > 0;
    e[9] = (s8)equip_skill_points(D_08A62C54, hunter + 0x4F4, e[3]) > 0;
    any = e[9] || e[8] || e[7] || e[6] || e[4] || e[5];
    if (any) {
        e[2] = 0x82;
        e[10] = 0;
        U16(e, 0) = 0;
    }
    return any;
}

/* copies a skill entry (as the original: key, flags, id, points, total, colors) */
static void skill_entry_copy(u8 *d, const u8 *s)
{
    int i;

    U16(d, 0) = U16(s, 0);
    d[2] = s[2];
    d[3] = s[3];
    for (i = 4; i < 0x12; i++)
        d[i] = s[i];
}

/* adds skill c to the sorted list at self+0x684 (if it has points) */
void skill_list_add(u8 *self, void *hunter, u32 c, void *other)
{
    u8 e[0x12];
    u8 *end, *pos, *q;
    s32 n;

    e[3] = c;
    if ((u8)skill_entry_fill(self, hunter, e, (int)other) != 1)
        return;
    end = self + 0x684;
    for (n = 54; end[2] != 0; end += 0x12)
        if (--n < 1)
            return;
    pos = self + 0x684;
    for (n = 54; pos[2] != 0; pos += 0x12) {
        if (U16(pos, 0) <= U16(e, 0)) {
            for (q = end; q > pos; q -= 0x12)
                skill_entry_copy(q, q - 0x12);
            break;
        }
        if (--n < 1)
            return;
    }
    skill_entry_copy(pos, e);
}

/* 1 if an armor piece of the hunter has a decoration slot of kind 1 */
int skill_armor_has_slot(void *self, u8 *hunter)
{
    const s32 *t = D_0893AEB8;
    int i, j;

    for (i = 0; i < 5; i++, t++) {
        const u8 *p = D_08978474[*t] + U16(hunter, *t * 0xC + 0x4F6) * 0x28 + 0x1E;

        for (j = 5; j > 0; j--, p += 2)
            if (*p == 1)
                return 1;
    }
    return 0;
}

/* recomputes the stats of the hunter after a change of equipment */
void skill_update_hunter(void *self, u8 *hunter)
{
    s16 v;

    equip_set_weapon(hunter, hunter + 0x4E8);
    func_088D897C(hunter);
    v = func_08828658(self, hunter);
    v = (s8)D_089CC438[0x6993C] + 100 + v;
    if (v < 151) {
        S16(hunter, 0x2E4) = v;
    } else {
        v = 150;
        S16(hunter, 0x2E4) = 150;
    }
    S16(hunter, 0x41E) = v;
    S16(hunter, 0x62E) = *(s16 *)(D_089CC438 + 0x69938) + 300;
    if (S16(hunter, 0x62E) < 75)
        S16(hunter, 0x62E) = 75;
    if (S16(hunter, 0x62E) > 450)
        S16(hunter, 0x62E) = 450;
    U8(hunter, 0x38A) = D_089CC438[0x6993A];
    U8(hunter, 0x3D4) = 0;
    (*(void (**)(void *))(*(u8 **)hunter + 0x5C))(hunter);
    U8(hunter, 0x38B) = D_089CC438[0x6993B];
    U8(hunter, 0x3D8) = 0;
    (*(void (**)(void *))(*(u8 **)hunter + 0x60))(hunter);
    (*(void (**)(void *))(*(u8 **)hunter + 0x64))(hunter);
}

/* draws the lines of s; align = 0 left, 2 centered, 4 right (on the widest line) */
void text_draw_lines_block(void *self, s32 x_arg, s32 y_arg, const char *s, s32 align, s32 lh_arg)
{
    s16 x = x_arg;
    s16 y = y_arg;
    s16 lh = lh_arg;
    char buf[128];
    const char *p = s;
    s32 w = 0, n, len;

    while (*p != 0) {
        const char *nl = strchr(p, '\n');

        if (nl == NULL) {
            n = func_08890F70(D_08A631A8, p);
            if (w < n)
                w = n;
            break;
        }
        strncpy(buf, p, nl - p);
        buf[nl - p] = 0;
        n = func_08890F70(D_08A631A8, buf);
        if (w < n)
            w = n;
        p = nl + 1;
    }
    w = w * align / 4;
    p = s;
    for (;;) {
        n = 0;
        for (; *p != 0 && *p != '\n'; p += len) {
            n += utf8_seq_len(D_089CBBE8, *p);
            len = utf8_seq_len(D_089CBBE8, *p);
        }
        strncpy(buf, s, n);
        buf[n] = 0;
        func_088920A8(D_08A631A8, (s16)(x - (s16)w), y, buf);
        y += lh;
        if (*p == 0)
            break;
        s = p + 1;
        p = s;
    }
}

/* draws the lines of s, each one aligned on its own width */
void text_draw_lines(void *self, s32 x_arg, s32 y_arg, const char *s, s32 align, s32 lh_arg)
{
    s16 x = x_arg;
    s16 y = y_arg;
    s16 lh = lh_arg;
    char buf[128];
    const char *p = s;
    s32 n, len, w;

    for (;;) {
        n = 0;
        for (; *p != 0 && *p != '\n'; p += len) {
            n += utf8_seq_len(D_089CBBE8, *p);
            len = utf8_seq_len(D_089CBBE8, *p);
        }
        strncpy(buf, s, n);
        buf[n] = 0;
        w = func_08890F70(D_08A631A8, buf) * align / 4;
        func_088920A8(D_08A631A8, (s16)(x - (s16)w), y, buf);
        y += lh;
        if (*p == 0)
            break;
        s = p + 1;
        p = s;
    }
}

/* 1 if skill k can be shown for the weapon eq (list = the skills already active) */
int skill_applies_to_weapon(u8 *self, u8 *eq, u32 k_arg, const s8 *list)
{
    s8 k = k_arg;
    u8 cls = equip_rarity(D_08A62C54, eq[1], U16(eq, 2));
    u8 f = table_314fc_b(D_08A62C54, cls);
    s8 other;
    int i;

    switch (k) {
    case 0x1C: other = 0x1B; break;
    case 0x28: other = 0x27; break;
    case 0x14: other = 0x13; break;
    case 0x0B: other = 0x0A; break;
    case 0x07: other = 0x06; break;
    case 0x30:
        if ((s8)cls == 5 || (s8)cls == 1) {
            u32 w = U32(func_0885CA50(D_08A62C54, eq), 0x18);

            return ((w | func_088D7DE8(*(void **)(self + 0x530))) & 0xE00) != 0;
        }
        if ((s8)cls == 8)
            return 1;
        return (s8)cls == 2;
    case 0x2D:
    case 0x17:
        return (f & 1) != 0;
    case 0x16:
        return (s8)cls == 1 || (s8)cls == 5;
    case 0x24:
    case 0x12:
        return (f & 2) != 0;
    case 0:
        return 0;
    default:
        return 1;
    }
    for (i = 0; i < 5; i++)
        if (list[i] == other)
            return 0;
    return 1;
}

extern const s8 D_08978E38[];      /* shells of the gunlances (4 per weapon) */
s32 hunter_has_skill(void *hunter, int skill);
s32 func_0885B3A0(void *a, s32 id, int b, int c);
void hunter_add_item(void *hunter, u16 id, int a, int b);
void func_088D8900(void *hunter);
s32 combo_known(void *a, void *b, int c, int d);

/* 1 if the gunlance wd has a shell of the kind of skill k (0x16, 0x17, 0x18) */
int skill_gunlance_shell(void *self, void *hunter, u8 *wd, u32 k_arg)
{
    u8 k = k_arg;
    const s8 *p;
    int n = 3;
    int i;

    if (wd[0xB] == 4)
        n++;
    else if (wd[0xB] == 2) {
        if ((u8)hunter_has_skill(hunter, 0xAA) == 0)
            n--;
    } else if ((u8)hunter_has_skill(hunter, 0xAA) == 1)
        n++;
    p = D_08978E38 + wd[0x10] * 4;
    for (i = 0; i < n; i++, p++) {
        switch (*p) {
        case 0: case 1: case 2: case 3: case 4:
            if (k == 0x16)
                return 1;
            break;
        case 5: case 6: case 7: case 8: case 9:
            if (k == 0x18)
                return 1;
            break;
        case 10: case 11: case 12: case 13: case 14:
            if (k == 0x17)
                return 1;
            break;
        }
    }
    return 0;
}

/* affinity bonus of the skill 9 of the hunter */
s32 skill_affinity_bonus(void *self, void *hunter)
{
    switch (func_088D6AC8(hunter, 9)) {
    case 0x11: return 20;
    case 0x12: return 30;
    case 0x13: return 50;
    case 0x14: return -10;
    case 0x15: return -20;
    case 0x16: return -30;
    default: return 0;
    }
}

/* 1 if slot s contains the item id (and the item exists) */
int menu_slot_has_item(void *self, s32 s_arg, s32 id_arg)
{
    s8 s = s_arg;
    s16 id = id_arg;

    if (s == -1)
        return 0;
    if (S16(func_08856884(D_089CC438, s), 2) == 0)
        return 0;
    if (id != *(s16 *)func_08856884(D_089CC438, s))
        return 0;
    return (u8)func_0885B3A0(D_08A62C50, id, 0, 0) != 0;
}

/* selects the combination of the items of slots a and b (0 if none) */
int menu_slot_combine(u8 *self, s32 a_arg, s32 b_arg)
{
    s8 a = a_arg;
    s8 b = b_arg;
    s16 ib;
    u8 *e;

    if (a == -1 || b == a)
        return 0;
    if (S16(func_08856884(D_089CC438, a), 2) == 0)
        return 0;
    ib = *(s16 *)func_08856884(D_089CC438, b);
    e = (u8 *)func_0885B5B8(D_08A62C50, ib, *(s16 *)func_08856884(D_089CC438, a), 0, 0, 0);
    U32(self, 0x5A8) = (u32)e;
    if (e == NULL)
        return 0;
    if (combo_known(D_08A62C50, e, 0, 0) == 0)
        U16(self, 0x5B4) = 0xFFFF;
    else
        U16(self, 0x5B4) = U16(*(u8 **)(self + 0x5A8), 2);
    return 1;
}

/* gives back the two selected items, if the selection was cancelled */
void menu_slot_restore(u8 *self)
{
    int i;

    if ((U32(self, 0x528) & 8) == 0)
        return;
    for (i = 0; i < 2; i++)
        if (U16(self, 0x5AC + i * 2) != 0xFFFF)
            hunter_add_item(*(void **)(self + 0x530), U16(self, 0x5AC + i * 2), 1, 0);
    if ((*(u32 *)(D_089CC438 + 0x6AF14) & 1) != 0)
        func_088D8900(*(void **)(self + 0x530));
    U32(self, 0x528) &= ~8;
}

extern const s16 D_08938AFC[][3], D_08939140[][3];
extern f32 D_09CDEF40[][8];        /* screen rectangles (overlay data) */
u32 combo_amount(void *a, void *e);
s32 game_test_bit62(void *game, u8 i);
s32 game_count_bits62(void *game);

/* 1 if both items of the combination are chosen */
int menu_combo_ready(u8 *self)
{
    if (S16(self, 0x5AC) == -1)
        return 0;
    if (S16(self, 0x5AE) == -1)
        return 0;
    return 1;
}

/* 1 if slot s is one of the two chosen slots */
int menu_combo_has_slot(u8 *self, u32 s_arg)
{
    u16 s = s_arg;

    if (s == self[0x5B0])
        return 1;
    if (s == self[0x5B1])
        return 1;
    return 0;
}

/* how many of item it (id, count) can still be carried:
 * -1 not this item, 0 none, 1 fewer than the result, 2 enough */
int menu_combo_room(u8 *self, u16 *it)
{
    u8 *e = *(u8 **)(self + 0x5A8);
    s32 room;

    if (e == NULL)
        return -1;
    if (it[0] != S16(e, 2))
        return -1;
    if ((u8)combo_known(D_08A62C50, e, 0, 0) == 0)
        return -1;
    room = D_0899D19B[it[0] * 0x18] - (s16)it[1];
    if (room <= 0)
        return 0;
    if (room < (u8)combo_amount(D_08A62C50, *(void **)(self + 0x5A8)))
        return 1;
    return 2;
}

/* position (x, y, z) of entry id of the menu layout */
void menu_layout_pos(void *self, f32 *x, f32 *y, f32 *z, u32 id_arg)
{
    u16 id = id_arg;
    const s16 *p = D_08938AFC[id];

    if (D_089CC438[0x6AF10] == 15 && (u32)(id - 0x80) < 2)
        p = D_08939140[id - 0x80];
    *x = p[0];
    *y = p[1];
    *z = p[2];
}

/* screen position of entry id: scaled layout position, or the center of the
 * rectangle id when the layout has no z (returns 0 in this case) */
int menu_layout_screen_pos(void *self, f32 *x, f32 *y, u32 id)
{
    f32 z;

    menu_layout_pos(self, x, y, &z, id);
    if (z == 0.0f) {
        f32 *r = D_09CDEF40[(u16)id];

        *x = r[0] + 0.5f * r[4];
        *y = r[1] + 0.5f * r[5];
        return 0;
    }
    {
        u8 g = D_089CC438[0x6AF10];

        *x = 0x1.af286cp-8f * *x * D_0893887C[g][0];
        *y = 0x1.af286cp-8f * *y * D_0893887C[g][1];
    }
    return 1;
}

/* next hunter of the list in direction dir (-1 if none) */
s32 menu_next_hunter(void *self, s32 cur_arg, s32 dir_arg)
{
    s8 cur = cur_arg;
    s8 dir = dir_arg;
    s32 v;
    int n;

    if (game_count_bits62(D_089CC438) == 0)
        return -1;
    if (cur < 0) {
        v = 0;
    } else {
        cur += dir;
        v = cur;
        if (v > 61) {
            v = 0;
            cur = 0;
        }
        if (cur < 0)
            v = 61;
    }
    for (n = 62; n > 0; n--) {
        s8 c;

        if (game_test_bit62(D_089CC438, v & 0xFF) != 0)
            return v;
        c = v;
        if (dir < 0) {
            if (c < 1)
                v = 61;
            else
                v = (s8)(c - 1);
        } else {
            v = (s8)(c + 1);
            if (v > 61)
                v = 0;
        }
    }
    return -1;
}
