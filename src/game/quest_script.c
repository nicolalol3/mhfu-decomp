/* Quest (D_08A62C7C): the 2 goals (0x728, copies at 0x738), the quest script
 * (commands of 8 bytes: op, a, b, c), the rewards and the network events of the quest.
 * (Guessed names.) */
#include "menu.h"

typedef struct QEvent {
    u8 type, b1;
    u16 a, b, c, d, e;
} QEvent;

extern void *D_09A1DAEC;
extern u8 *D_09C121B0;
extern u8 *D_09A4F0D0;
extern void **D_089CC434;
extern u8 *D_08A62D8C;
extern void *D_089CBBE4;
extern const u16 D_089AC5A0[];
extern const s32 D_089A77C4[][2];
extern const s32 D_089A822C[];
extern const s32 D_08970DB4[];
extern const s8 D_089A7F64[], D_089A7F68[], D_089A7F84[];
extern const s16 D_089A8288[35];

const char *text_menu(void *g, u32 id);
u8 menu_msg_push_text(u8 *self, const char *s);
u8 menu_msg_push(u8 *self, u32 id, u32 x, u32 y);
u8 menu_msg_push_copy(u8 *self, const char *s);
int sprintf(char *s, const char *fmt, ...);
void func_088AB514(void *a, int b, void *c);
void func_08883988(void *snd, int a, int b, int c, int d, int e, int f, int g, int h);
s32 quest_state(u8 *self, s32 mode);
s32 quest_time(u8 *self, s32 mode);
u8 *quest_find_entry(u8 *self, u32 id, s32 which, s32 idx);
void quest_goal_reached(u8 *self);
void quest_set_target(u8 *self, u8 *h);
u32 quest_check_targets_done(u8 *self, u32 id);
void quest_kill_count(u8 *self, s32 id);
s32 quest_hunter_item_set(u8 *self, u8 *h, u32 id, s32 n);
int entity_is(u8 *o, u32 a, u32 b);
void game_set_flag(u8 *g, u32 f);
u8 *game_table_ptr(u8 *g, u32 i);
u32 func_088900C4(void *a, int b);
u8 func_088D6A54(u32 h, int a);
s32 func_088D6A1C(u32 h, int a);
void func_088966C0(void *a, int b);
void func_088709D4(u8 *self);
s32 func_08872B40(u8 *self);
s8 func_0887276C(u8 *self, u32 i);
u8 func_09AC88F0(u8 *p);
void func_08813594(void *cam, int a);
void func_08872444(u8 *self);
void func_088B87B0(void *a, int b);
void func_09AC9608(u8 *p);
void func_09AC8630(u8 *p, int a, int b);
void func_08873044(u8 *self, u32 i, s32 n, s32 m);

u32 quest_goal_done(u8 *self, s32 i);
void quest_goal_set(u8 *self, s32 i, u32 flag);
u32 quest_goals_check(u8 *self, s32 end);
void quest_end_fail(u8 *self, s32 alt);
void quest_end_clear(u8 *self, s32 alt);
s16 quest_goal_count(u8 *self, s32 id, u32 i);
s16 quest_goal_have(u8 *self, s32 id);
s32 quest_goal_left(u8 *self, s32 id);
s32 quest_goal_left_at(u8 *self, u32 i);
s32 quest_goal_item_ok(u8 *self, s16 *cmd);
s32 quest_goal_any_done(u8 *self);
s32 quest_points(u8 *self);
void quest_set_mon_flags(u8 *self, u8 *h);
void quest_goal_add(u8 *self, u32 *src);
void quest_monster_event(u8 *self, u8 *pkt);
s16 quest_monsters_hunted(u8 *self);
u32 quest_nop0(u8 *self);
void quest_nop(u8 *self, s32 a, s32 b, s32 c);

#define GOAL(i) (self + 0x728 + (i) * 8)

/* entry of the quest monster with index idx */
u8 *quest_find_monster_idx(u8 *self, s32 idx, u32 id)
{
    s32 i;
    u8 *a, *m, *l;

    if (*(s8 *)(self + 0x20) >= 0) {
        for (i = 0; i <= *(s8 *)(self + 0x20); i++) {
            a = quest_find_entry(self, id, 0, (s8)i);
            m = quest_find_entry(self, id, 1, (s8)i);
            if (a == NULL)
                continue;
            for (; S16(m, 0) >= 0; m += 0x3C)
                if ((s16)idx == S16(m, 0x2C))
                    return m;
        }
    }
    for (l = (u8 *)U32(self, 0x71C); U32(l, 8) != 0; l += 0x10) {
        if (U32(l, 0xC) == 0)
            continue;
        for (m = (u8 *)(U32(l, 0xC) + U32(self, 0x50)); S16(m, 0) != -1; m += 0x3C)
            if ((s16)idx == S16(m, 0x2C))
                return m;
    }
    return NULL;
}

/* time passes for the quest timers (0x10: time left, 0x678: second timer) */
void quest_timers(u8 *self, s32 n)
{
    if (D_089CC438[0x422] != 0)
        return;
    if ((s32)U32(self, 0x10) > 0) {
        U32(self, 0x10) -= n;
        if ((s32)U32(self, 0x10) < 0)
            U32(self, 0x10) = 0;
    }
    if ((s32)U32(self, 0x678) > 0) {
        U32(self, 0x678) -= n;
        if ((s32)U32(self, 0x678) < 0)
            U32(self, 0x678) = 0;
    }
}

void quest_nop(u8 *self, s32 a, s32 b, s32 c)
{
}

/* count left of the hunting goal of the monster id, -1 if none */
s32 quest_goal_left(u8 *self, s32 id)
{
    s32 i;

    for (i = 0; i < 2; i++)
        if ((U32(GOAL(i), 0) & 0x11) == 0x11 && S16(GOAL(i), 4) == (s16)id)
            return S16(GOAL(i), 6);
    return -1;
}

s32 quest_goal_left_at(u8 *self, u32 i)
{
    u8 *g = GOAL(i & 0xFFFF);

    if ((U32(g, 0) & 0x11) == 0x11)
        return S16(g, 6);
    return -1;
}

u32 quest_goal_done(u8 *self, s32 i)
{
    u32 f = U32(GOAL(i), 0);

    if ((f & 0x10) == 0)
        return 0;
    if ((f & 0x440) == 0x40)
        return 0;
    if ((f & 0x20) == 0)
        return 0;
    return 1;
}

void quest_goal_complete(u8 *self, u32 i, u32 flag)
{
    if (quest_state(self, 1) == 0)
        quest_goal_set(self, i & 0xFFFF, flag);
}

/* goal i completed (flag: kind of completion), with the event for the other players */
void quest_goal_set(u8 *self, s32 i, u32 flag)
{
    u8 *g;
    QEvent ev;

    if (quest_goal_done(self, i) != 0)
        return;
    g = GOAL(i);
    U32(g, 0) |= 0x20;
    if (flag == 0x200)
        func_088966C0(D_089CBBEC, 0x11);
    if (flag == 0x100)
        func_088966C0(D_089CBBEC, 0x12);
    quest_goals_check(self, 0);
    g = GOAL(i);
    if ((U32(g, 0) & 0x11) == 0x11)
        S16(g, 6) = 0;
    ev.b = flag >> 16;
    ev.d = i;
    ev.a = 0;
    ev.c = flag;
    ev.type = 0x14;
    if (U32(self, 0x24) == 0)
        ev.e = 0xFFFF;
    else
        ev.e = S16((u8 *)U32(self, 0x24), 0x4AC);
    func_088AB514(D_09A1DAEC, 6, &ev);
}

void quest_goals_clear(u8 *self)
{
    memset(self + 0x728, 0, 0x10);
    memset(self + 0x738, 0, 0x10);
}

/* goals of the quest file */
void quest_goals_load(u8 *self)
{
    u32 *s;
    s32 i;

    self[0x67C] = U8((u8 *)U32(self, 0x68), 0x1E);
    s = (u32 *)(U32(self, 0x68) + 0x20);
    for (i = 0; i < 2; i++, s += 2)
        if (s[0] != 0)
            quest_goal_add(self, s);
}

void quest_goal_add(u8 *self, u32 *src)
{
    s32 i;
    u8 *g;

    for (i = 0; i < 2; i++) {
        if ((U32(GOAL(i), 0) & 0x10) != 0)
            continue;
        g = GOAL(i);
        U32(g, 0) = src[0];
        U16(g, 4) = U16(src, 4);
        U16(g, 6) = U16(src, 6);
        U32(g, 0x10) = U32(g, 0);
        U16(g, 0x14) = U16(g, 4);
        U16(g, 0x16) = U16(g, 6);
        U32(g, 0x10) |= 0x10;
        U32(g, 0) |= 0x10;
        return;
    }
}

/* per-frame check of the goals: deliveries (2), monster parts (0x4004), times (0x8004) */
void quest_goals_update(u8 *self)
{
    u8 *g = GOAL(0);
    u32 i, f;
    s16 v;
    s32 half;

    for (i = 0; (s32)i < 2; i++) {
        f = U32(g, 0);
        if (((f & 0x20) | 0x10) != 0x10) {
            g += 8;
            continue;
        }
        if ((f & 2) == 2) {
            v = quest_goal_count(self, U16(g, 4), i & 0xFFFF);
            v = v - quest_goal_have(self, U16(g, 4));
            S16(g, 6) = v;
            if (v <= 0)
                quest_goal_complete(self, i & 0xFFFF, 2);
        }
        if ((U32(g, 0) & 0x4004) == 0x4004 &&
            (self[0x5C2 + U16(g, 4)] & (1 << U16(g, 6))) != 0)
            quest_goal_complete(self, i & 0xFFFF, 0x4000);
        f = U32(g, 0);
        if ((f & 0x8004) != 0x8004 || U32(self, 0x720) == 0) {
            g += 8;
            continue;
        }
        if ((f & 0x10000) != 0) {
            half = quest_time(self, 1) / 2;
            if (half < quest_time(self, 0))
                continue;       /* (the original does not move to the next goal here) */
        }
        if ((s32)S16(self, 0x114) - S16((u8 *)U32(self, 0x720), 0x2E4) >= S16(g, 6) * 100) {
            if ((U32(g, 0) & 0x10000) == 0x10000 && *(s8 *)(self + 6) != 5 && self[0x8A4] == 0) {
                menu_msg_push_text(D_089CBBEC, text_menu(D_089CC438, D_089AC5A0[11]));
                self[0x8A4] = 1;
            }
            quest_goal_complete(self, i & 0xFFFF, 0x8000);
        }
        g += 8;
    }
}

/* count to deliver for the goal i (-1 when it is not a delivery of id) */
s16 quest_goal_count(u8 *self, s32 id, u32 i)
{
    u8 *g = self + 0x738 + (i & 0xFFFF) * 8;

    if ((U32(g, 0) & 0x12) == 0x12 && S16(g, 4) == (s16)id)
        return S16(g, 6);
    return -1;
}

/* capture goals of the monster id */
u32 quest_capture_done(u8 *self, u32 id)
{
    u32 r = 0, i;
    u8 *g = GOAL(0);

    for (i = 0; (s32)i < 2; i++, g += 8) {
        if ((U32(g, 0) & 0x60) != 0 || (U32(g, 0) & 0x11) != 0x11 || U16(g, 4) != (id & 0xFFFF))
            continue;
        S16(g, 6)--;
        if (S16(g, 6) <= 0) {
            quest_goal_complete(self, i & 0xFFFF, 0x100);
            r = 1;
        } else {
            func_08873044(self, id & 0xFF, S16(g, 6), S16(self, 0x73E + i * 8));
        }
    }
    return r;
}

s32 quest_goal_any_done(u8 *self)
{
    s32 i;

    for (i = 0; i < 2; i++)
        if ((U32(GOAL(i), 0) & 0x100) != 0 && (U32(GOAL(i), 0) & 0x31) == 0x31)
            return 1;
    return 0;
}

/* items id delivered: of the player only (flag 0x80) or of all the hunters */
s16 quest_goal_have(u8 *self, s32 id)
{
    s32 i, p, k;
    s16 n = 0;
    u8 *h;

    for (i = 0; i < 2; i++)
        if ((U32(GOAL(i), 0) & 0x12) == 0x12 && U16(GOAL(i), 4) == (id & 0xFFFF))
            break;
    if (i >= 2)
        return 0;
    if ((U32(GOAL(i), 0) & 0x80) != 0) {
        h = (u8 *)func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]);
        for (k = 0; k < 4; k++)
            if (U16(h, 0x542 + k * 4) == (id & 0xFFFF))
                return S16(h, 0x544 + k * 4);
        return 0;
    }
    for (p = 0; p < 4; p++) {
        h = (u8 *)func_088DCEBC(D_09A4F0C4, p);
        if (h == NULL)
            continue;
        for (k = 0; k < 4; k++)
            if (U16(h, 0x542 + k * 4) == (id & 0xFFFF))
                n += S16(h, 0x544 + k * 4);
    }
    return n;
}

/* 1 when the main goals are completed (and with end != 0, the quest is cleared) */
u32 quest_goals_check(u8 *self, s32 end)
{
    u32 done = 0, f;
    s32 i;

    for (i = 0; i < 2; i++) {
        f = U32(GOAL(i), 0);
        if ((f & 0x10) == 0)
            continue;
        if ((f & 0x20) != 0)
            done |= 1 << i;
        if ((f & 0x440) == 0x40) {
            U32(GOAL(i), 0) |= 0x400;
            if (end != 0)
                quest_end_fail(self, 0);
            return 0;
        }
    }
    if (self[0x67C] == 2) {
        if ((done & 3) != 3)
            return 0;
    } else if (self[0x67C] == 1) {
        if ((done & 1) != 1)
            return 0;
    } else {
        return 0;
    }
    if (end != 0)
        quest_end_clear(self, 0);
    return 1;
}

/* jumps the script to the label -2 (failed) or 0x25 */
void quest_end_fail(u8 *self, s32 alt)
{
    s16 *p = (s16 *)U32(self, 0x54);
    s16 n = 0, op = alt == 0 ? -2 : 0x25;

    for (; *p != op; p += 4)
        n++;
    S16(self, 0x1C) = n;
    D_089CC438[0x2E] = 5;
}

/* jumps the script to the label -1 (cleared) or 0x26 */
void quest_end_clear(u8 *self, s32 alt)
{
    s16 *p = (s16 *)U32(self, 0x54);
    u8 st = D_089CC438[0x2E];
    s16 n = 0, op;

    if (st != 2 && st != 1 && st != 0)
        return;
    op = alt == 0 ? -1 : 0x26;
    for (; *p != op; p += 4)
        n++;
    S16(self, 0x1C) = n;
    D_089CC438[0x2E] = 3;
}

/* script position of the label n (command 0x1A) */
static s16 find_label(u8 *self, s32 n)
{
    s16 *p = (s16 *)U32(self, 0x54);
    s16 i = 0;

    for (; p[0] != 0x1A || p[1] != n; p += 4)
        i++;
    return i + 1;
}

/* position after the first command op that follows pc */
static s16 skip_to(s16 *cmd, s16 pc, s32 op)
{
    for (;;) {
        cmd += 4;
        pc++;
        if (cmd[0] == op)
            return pc + 1;
    }
}

static void send_event(u32 type)
{
    QEvent ev;

    ev.type = type;
    ev.a = 0;
    ev.b = 0;
    ev.c = 0;
    func_088AB514(D_09A1DAEC, 6, &ev);
}

static void push_msg(u32 i)
{
    menu_msg_push_text(D_089CBBEC, text_menu(D_089CC438, D_089AC5A0[i]));
}

/* time of the hunt of the target and of its parts */
static void target_time(u8 *self)
{
    u8 *m = (u8 *)U32(self, 0x720);

    if (m != NULL && (u8)func_0887276C(self, m[0x1E8]) != 0xFF) {
        S16(self, 0x114) = S16(m, 0x2E4);
        self[0x5C2 + m[0x1E8]] = U16(m, 0x774);
        quest_set_mon_flags(self, m);
    }
}

/* runs the quest script from the command pc; returns the new pc */
s32 quest_script_run(u8 *self, u8 *h, s32 pc_arg)
{
    s32 pc = pc_arg;    /* (kept at 32 bits until it changes, like the original) */
    s16 *c;
    char buf[0x80];
    s32 t, v;

    if (D_089CC438[0x2E] == 2 && (s32)U32(self, 0x10) >= 0) {
        if (*(s8 *)(self + 0x83) > 0) {
            t = D_089A77C4[*(s8 *)(self + 0x83)][0];
            if (!(t < (s32)U32(self, 0x10))) {
                sprintf(buf, text_menu(D_089CC438, D_089AC5A0[4]), t / 1800);
                buf[0x7F] = 0;
                menu_msg_push_copy(D_089CBBEC, buf);
                self[0x83]--;
            }
        }
        quest_timers(self, 1);
        if ((s32)U32(self, 0x10) > 0) {
            if ((s32)U32(self, 0x10) % 9000 == 0)
                func_088709D4(self);
        } else {
            U32(self, 0x10) = 0;
            func_088709D4(self);
            U32(self, 0x10) = -1;
            pc = find_label(self, *(s8 *)(self + 8));
        }
    }
    for (;;) {
        if (*(s8 *)(self + 9) != 0 && quest_goal_any_done(self) != 0) {
            pc = find_label(self, *(s8 *)(self + 9));
            self[9] = 0;
        }
        if (*(s8 *)(self + 0x117) != 0 && *(s8 *)(self + 0x11C) != 0) {
            pc = find_label(self, *(s8 *)(self + 0x117));
            self[0x117] = 0;
        }
        c = (s16 *)(U32(self, 0x54) + (s16)pc * 8);
        switch (c[0]) {
        case 0x39:
            if (quest_goals_check(self, 1) == 0 && func_08872B40(self) != 0)
                goto out;
            pc = (s16)(pc + 1);
            continue;
        case 0x37:
            if (*(s8 *)(self + 0x12D) == c[1])
                pc = (s16)(pc + 1);
            else
                pc = skip_to(c, (s16)pc, 0x38);
            continue;
        case 0x35:
            if (quest_points(self) < c[1])
                pc = skip_to(c, (s16)pc, 0x36);
            else
                pc = (s16)(pc + 1);
            continue;
        case 0x34:
            quest_goal_complete(self, 0, 0);
            pc = (s16)(pc + 1);
            continue;
        case 0x33:
            goto out;
        case 0x32:
        case 0x31:
            pc = (s16)(pc + 1);
            continue;
        case 0x30:
            if (c[2] < (s16)quest_goal_left(self, (u16)c[1]))
                pc = (s16)(pc + 1);
            else
                pc = find_label(self, c[3]);
            continue;
        case 0x2F:
            self[8] = c[1];
            pc = (s16)(pc + 1);
            goto out;
        case 0x1A:
            pc = (s16)(pc + 1);
            continue;
        case 0x27:
            goto out;
        case 0x1C:
            pc = find_label(self, c[1]);
            continue;
        case 0x2A: {
            s32 i;
            u8 *p = NULL;

            for (i = 0; i < 20; i++) {
                p = (i >= 0 && i < 20) ? (u8 *)U32(D_09C121B0, 0x1220 + i * 4) : NULL;
                if (p != NULL)
                    break;
            }
            if (func_09AC88F0(p) == 1) {
                quest_goal_complete(self, 0, 0x200);
                pc = (s16)(pc + 1);
            } else {
                pc = skip_to(c, (s16)pc, 0x2B);
            }
            continue;
        }
        case 0x28: {
            u8 *m = (u8 *)U32(self, 0x720);

            if (S16(self, 0x114) - c[1] < S16(m, 0x2E4)) {
                pc = skip_to(c, (s16)pc, 0x29);
            } else {
                S16(self, 0x114) = S16(m, 0x2E4);
                pc = (s16)(pc + 1);
            }
            continue;
        }
        case 0x26:
            target_time(self);
            self[6] = 3;
            D_089CC438[0x2E] = 3;
            pc = (s16)(pc + 1);
            U32(self, 0x110) |= 1 << (D_089CC438[0x28] + 8);
            send_event(0xB);
            continue;
        case 0x25:
            D_089CC438[0x2E] = 5;
            self[6] = 5;
            pc = (s16)(pc + 1);
            push_msg(8);
            send_event(10);
            continue;
        case 0x2C:
            self[0x117] = c[1];
            pc = (s16)(pc + 1);
            goto out;
        case 0x23:
            self[9] = c[1];
            pc = (s16)(pc + 1);
            goto out;
        case 0x21:
            if (quest_goal_item_ok(self, c) == 0)
                goto out;
            pc = (s16)(pc + 1);
            continue;
        case 0x20:
            self[0x20] = c[1];
            quest_goal_reached(self);
            pc = (s16)(pc + 1);
            continue;
        case 0x1B:
            self[8] = c[1];
            v = quest_time(self, 1);
            sprintf(buf, text_menu(D_089CC438, D_089AC5A0[7]), v / 1800);
            buf[0x7F] = 0;
            menu_msg_push_copy(D_089CBBEC, buf);
            pc = (s16)(pc + 1);
            goto out;
        case 0x1F:
            D_089CC438[0x2E] = 6;
            self[6] = 6;
            pc = -1;
            push_msg(6);
            if ((U32(D_089CC438, 0x6AF14) & 4) == 0)
                menu_msg_push(D_089CBBEC, 0, 0xF, 0);
            goto out;
        case -2:
            pc = (s16)(pc + 1);
            D_089CC438[0x2E] = 5;
            self[6] = 5;
            send_event(4);
            continue;
        case 0x18:
            if ((u8)entity_is(h, 4, 0) == 0)
                goto out;
            pc = (s16)(pc + 1);
            continue;
        case 0x1E:
            pc = -1;
            self[0]++;
            self[1] = 0;
            goto out;
        case 0x2D:
            if ((U32(self, 0x110) & 0xF00) == 0xF00 ||
                D_089CC438[D_089CC438[0x28] * 0x8C + 0x30] == 0)
                pc = (s16)(pc + 1);
            goto out;
        case -1:
            target_time(self);
            D_089CC438[0xC] = 0x81;
            self[6] = 3;
            D_089CC438[0x2E] = 3;
            pc = (s16)(pc + 1);
            U32(self, 0x110) |= 1 << (D_089CC438[0x28] + 8);
            send_event(2);
            continue;
        case 4:
            pc = (s16)(pc + 1);
            U32(self, 0x10) += (u16)c[1];
            goto out;
        case 0xC:
            if (quest_nop0(self) == 0) {
                pc = S16(self, 0x1E);
                goto out;
            }
            if (U16(h, 0x29A) != c[1])
                goto out;
            pc = (s16)(pc + 1);
            continue;
        case 0x24:
            if (c[2] < (s16)quest_goal_left_at(self, (u8)c[1]))
                goto out;
            pc = (s16)(pc + 1);
            continue;
        case 6:
            if (quest_goals_check(self, 0) == 0)
                pc = skip_to(c, (s16)pc, 7);
            else
                pc = (s16)(pc + 1);
            continue;
        case 5:
            pc = (s16)(pc + 1);
            continue;
        case 0xB:
            push_msg(c[1]);
            pc = (s16)(pc + 1);
            goto out;
        case 0xA:
            U32(self, 0xC)--;
            if ((s32)U32(self, 0xC) <= 0)
                pc = (s16)(pc + 1);
            goto out;
        case 9:
            pc = (s16)(pc + 1);
            U32(self, 0xC) = c[1];
            goto out;
        case 3:
            if (quest_goals_check(self, 1) == 0)
                goto out;
            pc = (s16)(pc + 1);
            continue;
        case 2:
            quest_nop(self, c[1], c[2], c[3]);
            pc = (s16)(pc + 1);
            continue;
        case 1:
            pc = (s16)(pc + 1);
            self[0x67C] = c[1];
            continue;
        default:
            goto out;
        }
    }
out:
    if (D_089CC438[0xC] == 0x80) {
        D_089CC438[0xC] = 0;
        func_08813594(D_089CBBE4, 0);
    }
    return pc;
}

/* flags of the game for some special monsters hunted without damage at 0x2E4 */
void quest_set_mon_flags(u8 *self, u8 *h)
{
    s8 k, m;

    if (S16(h, 0x2E4) != 0)
        return;
    k = *(s8 *)(U32(D_08A62C7C, 0x4C) + 0x44);
    m = h[0x1E8];
    if (k == 5) {
        if (m == 0x24)
            game_set_flag(D_089CC438, 0xD9);
        else if (m == 2)
            game_set_flag(D_089CC438, 0xD8);
        else if (m == 0x3B)
            game_set_flag(D_089CC438, 0xD7);
        else if (m == 0x41)
            game_set_flag(D_089CC438, 0xD6);
        else if (m == 0x36)
            game_set_flag(D_089CC438, 0xD5);
    } else if (k == 2) {
        if (m == 0x24)
            game_set_flag(D_089CC438, 0xD4);
        else if (m == 2)
            game_set_flag(D_089CC438, 0xD3);
    }
}

s32 quest_points(u8 *self)
{
    return U32(self, 0x5B0) + U32(self, 0x5B4);
}

/* marks in the 64-bit mask 0x5B8 the special item id */
void quest_mark_item(u8 *self, u32 id)
{
    s16 t[35];
    s32 i;

    for (i = 0; i < 35; i++)
        t[i] = D_089A8288[i];
    for (i = 0; i < 35; i++) {
        if ((u16)t[i] == (id & 0xFFFF)) {
            if (i < 32)
                U32(self, 0x5B8) |= 1u << i;
            else
                U32(self, 0x5BC) |= 1u << (i - 32);
            return;
        }
    }
}

/* rewards of the quest: up to 16 + 16 items in D_089CC438 + 0x390 */
void quest_make_rewards(u8 *self)
{
    u32 h = func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]);
    u8 timed = 0;
    s32 i, j, n = 0, out = 0, k;
    u8 *rule, *l, *e;
    const s8 *ch;
    u32 c, sum, r, acc;
    u16 a, b;

    U32(D_089CC438, 0x410) = 0;
    U32(D_089CC438, 0x414) = 0;
    for (i = 0; i < 0x40; i++)
        U16(D_089CC438, 0x390 + i * 2) = 0;
    for (rule = (u8 *)U32(self, 0x5C); U16(rule, 0) != 0xFFFF; rule += 8) {
        ch = D_089A7F64;
        c = U16(rule, 0);
        if (c >= 0xF7 && c < 0x10C) {
            if ((s32)(quest_monsters_hunted(self) & 0xFFFF) < (s32)(U16(rule, 0) - 0xF7))
                goto next;
        } else if (c >= 0x24 && c <= 0x35) {
            s32 t1 = quest_time(D_08A62C7C, 1);
            s32 t0 = quest_time(D_08A62C7C, 0);

            if (timed != 0 || !((t1 - t0) / 1800 < D_089A822C[U16(rule, 0) - 0x24]))
                goto next;
            timed = 1;
        } else if (c >= 0x10C && c <= 0x119) {
            u32 area = U16(D_089CC438, 0x6AF12);

            if (quest_points(self) < D_08970DB4[U16(rule, 0) + area * 14])
                goto next;
        } else if (c == 1) {
            if (quest_monsters_hunted(self) <= 0)
                goto next;
            ch = D_089A7F84;
        } else if (c == 0x8000) {
            if ((U32(D_089CC438, 0x6AF14) & 4) != 0) {
                ch = D_089A7F68;
            } else {
                /* chances of the carves: 0x20 for each carve, then the skill value */
                u32 cnt = rule[3];
                s8 v;

                if (func_088D6A54(h, 0x13) != 0)
                    cnt = (u8)(cnt + 1);
                else if (func_088D6A54(h, 0x14) != 0 && (func_088900C4(D_08A631A0, 1) & 0xFFFF & 1) != 0)
                    cnt = (u8)(cnt + 1);
                k = 0;
                for (; k < (s32)cnt; k++)
                    self[0x7C1 + k] = 0x20;
                if (func_088D6A1C(h, 0x90) != 0)
                    v = 0x1A;
                else if (func_088D6A1C(h, 0x92) != 0)
                    v = 0x10;
                else if (func_088D6A1C(h, 0x91) != 0)
                    v = 0x1D;
                else if (func_088D6A1C(h, 0x93) != 0)
                    v = 8;
                else
                    v = 0x16;
                for (; k < 10; k++)
                    self[0x7C1 + k] = v;
                self[0x7C1 + k] = 0;
                ch = (const s8 *)(self + 0x7C1);
            }
        }
        /* rolls: one more item while rand % 32 < chance[i] */
        l = (u8 *)(U32(rule, 4) + U32(self, 0x50));
        for (j = 0;; j++) {
            s8 cj = ch[j];

            if (!((s32)(func_088900C4(D_08A631A0, 0) & 0xFFFF & 0x1F) < cj))
                break;
            sum = 0;
            for (e = l; U16(e, 0) != 0xFFFF; e += 6)
                sum = (sum + U16(e, 0)) & 0xFFFF;
            r = ((s32)(func_088900C4(D_08A631A0, 0) & 0xFFFF) % (s32)(sum & 0xFFFF)) & 0xFFFF;
            if (U16(rule, 0) == 0x8000 && j == 0)
                r = 0;
            acc = 0;
            e = l;
            while (U16(e, 0) != 0xFFFF) {
                acc = (acc + U16(e, 0)) & 0xFFFF;
                if ((s32)r < (s32)acc) {
                    e += 2;
                    break;
                }
                e += 6;
            }
            if (U16(e, 0) != 0) {
                n++;
                U16(D_089CC438, 0x390 + out) = U16(e, 0);
                U16(D_089CC438, 0x392 + out) = U16(e, 2);
                out += 4;
            }
            if (n >= 16)
                break;
        }
next:
        if (n >= 16)
            break;
    }
    func_08872444(self);
    /* sorts the two lists of 16 by item */
    for (k = 0; k < 2; k++) {
        for (i = k * 16; i < k * 16 + 15; i++) {
            u16 *p = &U16(D_089CC438, 0x390 + i * 4);

            for (j = i + 1; j < k * 16 + 16; j++) {
                u16 *q = &U16(D_089CC438, 0x390 + j * 4);

                if (q[0] != 0 && q[0] < p[0]) {
                    a = p[0];
                    b = p[1];
                    p[0] = q[0];
                    p[1] = q[1];
                    q[0] = a;
                    q[1] = b;
                }
            }
        }
    }
}

u32 quest_nop0(u8 *self)
{
    return 0;
}

/* item goal of the command: enough items c[1] in the pouch (c[2]) */
s32 quest_goal_item_ok(u8 *self, s16 *c)
{
    s16 n = c[2];

    return n <= (s32)func_088568AC(D_089CC438, (u16)c[1]);
}

/* monsters hunted of the kinds listed in the table of the quest */
s16 quest_monsters_hunted(u8 *self)
{
    u8 *base = (u8 *)U32(self, 0x50);
    u8 *p = game_table_ptr(D_089CC438,
                           *(u8 *)(base + U32(base + U32((u8 *)U32(self, 0x4C), 0x14), 0xC)));
    s16 n = 0;

    for (; *p != 0xFF; p++)
        n += S16(self, 0x29C + *p * 2);
    return n;
}

/* 1 when the goal of the monster id counts for the end of the quest */
u32 quest_goal_counts(u8 *self, s32 id)
{
    s32 i;
    u32 f;

    for (i = 0; i < 2; i++) {
        f = U32(GOAL(i), 0);
        if ((f & 0x11) != 0x11 && (f & 0x14) != 0x14)
            continue;
        if (S16(GOAL(i), 4) != (s16)id)
            continue;
        if (self[0x67C] == 2)
            return 1;
        if (self[0x67C] == 1 && i == 0)
            return 1;
    }
    return 0;
}

/* quest events received from the other players (network) */
void quest_event(u8 *self, u32 who, u8 *pkt)
{
    s32 i;
    u8 *p;

    switch (pkt[0]) {
    case 2:
        U32(self, 0x110) |= 1 << ((who & 0xFF) + 8);
        quest_end_clear(self, 0);
        return;
    case 4:
        if (D_089CC438[0x2E] != 5 && D_089CC438[0x2E] != 6 && D_089CC438[0x2E] != 7)
            quest_end_fail(self, 0);
        break;
    case 5: {
        s32 v = (S16(pkt, 4) << 16) | U16(pkt, 6);

        if (v < (s32)U32(self, 0x10))
            U32(self, 0x10) = v;
        break;
    }
    case 6:
        if (*(s8 *)(self + 0x20) < S16(pkt, 4)) {
            u8 *m = D_09A4F0D0;

            self[0x20] = S16(pkt, 4);
            U32(m, 0x210) = U32(m, 0x204);
            U32(m, 0x20C) = U32(m, 0x208);
            for (i = 0x15; i <= 0x2A; i++)
                U16(m, i * 0xC) &= 0xFFFD;
            func_088B87B0(D_089CC434, 1);
        }
        break;
    case 7:
        quest_monster_event(self, pkt);
        break;
    case 9:
        p = (u8 *)func_088DCEBC(D_09A4F0C4, who & 0xFF);
        if (p != NULL)
            quest_hunter_item_set(self, p, U16(pkt, 4), S16(pkt, 6));
        break;
    case 10:
        if (D_089CC438[0x2E] != 5 && D_089CC438[0x2E] != 6 && D_089CC438[0x2E] != 7)
            quest_end_fail(self, 1);
        break;
    case 0xB:
        U32(self, 0x110) |= 1 << ((who & 0xFF) + 8);
        quest_end_clear(self, 1);
        break;
    case 0xC:
        U32(self, 0x110) |= 1 << (who & 0xFF);
        self[(who & 0xFF) * 2 + 0x84] = S16(pkt, 4);
        break;
    case 0xD:
        U32(self, 0x110) |= 1 << ((who & 0xFF) + 4);
        U32(self, 0x10C) |= S16(pkt, 4) << 16;
        U32(self, 0x10C) |= (s32)S16(pkt, 6);
        break;
    case 0xE:
        if (*(s8 *)(self + 0x12D) != 0) {
            self[0x12D] = 0;
            push_msg(9);
            func_08883988(D_08A62D8C, 1, 0x77, 0, 0, 0, 0, 0, 0);
        }
        break;
    case 0xF:
        U32(self, 0x5B0 + (who & 0xFF) * 4) = (S16(pkt, 4) << 16) | U16(pkt, 6);
        break;
    case 0x10:
        U32(self, 0x5B8) |= U16(pkt, 4) | (U16(pkt, 6) << 16);
        U32(self, 0x5BC) |= U16(pkt, 8);
        break;
    case 0x12:
        if (*(s8 *)(self + 0x724) < S16(pkt, 4)) {
            self[0x724] = S16(pkt, 4);
            U32(self, 0x678) = -1;
        }
        break;
    case 0x13:
        D_089CC438[0x418] = 1;
        U32(self, 0x678) = 18000;
        break;
    case 0x14: {
        u32 flag = (S16(pkt, 4) << 16) | U16(pkt, 6);

        if (S16(pkt, 0xA) != -1) {
            U32(self, 0x24) = 0;
            for (i = 0; i < 20; i++) {
                p = (i >= 0 && i < 20) ? (u8 *)U32(D_09C121B0, 0x1220 + i * 4) : NULL;
                if (p != NULL && S16(p, 0x4AC) == S16(pkt, 0xA)) {
                    U32(self, 0x24) = (u32)p;
                    break;
                }
            }
        }
        quest_goal_set(self, U16(pkt, 8), flag);
        break;
    }
    case 0x15:
        self[0x788 + S16(pkt, 4) * 0x2C + S16(pkt, 6)] |= (u8)(1 << (who & 0xFF));
        break;
    }
}

/* a monster of the quest left / was killed on the machine of another player */
void quest_monster_event(u8 *self, u8 *pkt)
{
    u32 n = (U16(pkt, 6) >> 8) & 0xFF;
    u8 *m = quest_find_monster_idx(self, S16(pkt, 4), U16(pkt, 6) & 0xFF);
    u8 *p;
    s32 i;
    s16 f;

    if (m == NULL)
        return;
    f = S16(m, 0x2E);
    if ((f & 6) == 0)
        return;
    for (i = 0; i < 20; i++) {
        p = (i >= 0 && i < 20) ? (u8 *)U32(D_09C121B0, 0x1220 + i * 4) : NULL;
        if (p == NULL || U16(p, 0x1E6) != *(s8 *)(m + 0x34) || S16(p, 0x4AC) != m[0x2C])
            continue;
        if (p[0x1E8] != S16(m, 0))
            return;
        if ((U32(p, 4) & 1) == 0)
            return;
        if ((f & 4) == 0) {
            if (p[0x1D4] >= 2)
                return;
            if (p[0x298] == 5)
                return;
        } else if (!((s32)(n & 0xFFFF) < *(s8 *)(m + 4))) {
            return;
        }
        if (p[0x1D4] >= 2 || p[0x298] == 5)
            func_09AC9608(p);
        func_09AC8630(p, 5, 0);
        p[0x230] = 0xFF;
        S16(m, 0x2E) = 2;
        S16(p, 0x2E4) = 0;
        quest_set_target(self, p);
        return;
    }
    if ((s32)(n & 0xFFFF) < *(s8 *)(m + 4)) {
        quest_check_targets_done(self, U16(m, 0));
        quest_kill_count(self, U16(m, 0));
        m[4]--;
        if (*(s8 *)(m + 4) <= 0) {
            S16(m, 0x2E) = 1;
            m[0x34] = 0xFF;
        } else {
            S16(m, 0x2E) = 2;
        }
    }
}
