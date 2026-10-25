/* Quest (g_quest): the 2 goals (Quest.goal, copies in Quest.goal_orig), the quest script
 * (QuestCmd, commands QC_*), the rewards and the events received from the other players.
 * (Guessed names.) */
#include "quest.h"
#undef text_menu
#undef pouch_count

extern void *g_net;
extern u8 *g_monsters;
extern u8 *g_monster_models;
extern void **g_loader;
extern u8 *g_sound;
extern void *g_camera;
extern const u16 D_089AC5A0[];      /* IDs of the quest messages */
extern const s32 D_089A77C4[][2];
extern const s32 D_089A822C[];
extern const s32 D_08970DB4[];
extern const s8 D_089A7F64[], D_089A7F68[], D_089A7F84[];   /* chances of the rewards */
extern const s16 D_089A8288[35];

const char *text_menu(void *g, u32 id);
u8 menu_msg_push_text(u8 *self, const char *s);
u8 menu_msg_push(u8 *self, u32 id, u32 x, u32 y);
u8 menu_msg_push_copy(u8 *self, const char *s);
int sprintf(char *s, const char *fmt, ...);
void sound_play(void *snd, int a, int b, int c, int d, int e, int f, int g, int h);
int entity_is(u8 *o, u32 a, u32 b);
void game_set_flag(u8 *g, u32 f);
u8 *game_table_ptr(u8 *g, u32 i);
u32 pouch_count(void *game, s16 id);
u8 hunter_felyne_skill(u32 h, int a);
s32 hunter_has_skill(u32 h, int a);
void help_popup(void *a, int b);
u8 func_09AC88F0(u8 *p);
void func_08813594(void *cam, int a);
void func_088B87B0(void *a, int b);
void func_09AC9608(u8 *p);
void func_09AC8630(u8 *p, int a, int b);

#define DELIVERED_ID(h, i)  U16(h, E_DELIVERED + (i) * 4)
#define DELIVERED_N(h, i)   S16(h, E_DELIVERED + 2 + (i) * 4)

/* entry of the quest monster with index idx */
QuestMonster *quest_find_monster_idx(Quest *self, s32 idx, u32 id)
{
    s32 i;
    u8 *a;
    QuestMonster *m;
    QuestGroup *l;

    if (self->phase >= 0) {
        for (i = 0; i <= self->phase; i++) {
            a = quest_find_entry(self, id, 0, (s8)i);
            m = (QuestMonster *)quest_find_entry(self, id, 1, (s8)i);
            if (a == NULL)
                continue;
            for (; m->kind >= 0; m++)
                if ((s16)idx == m->index)
                    return m;
        }
    }
    for (l = self->large; l->list0 != 0; l++) {
        if (l->list1 == 0)
            continue;
        for (m = (QuestMonster *)QOFF(self, l->list1); m->kind != -1; m++)
            if ((s16)idx == m->index)
                return m;
    }
    return NULL;
}

/* time passes for the quest timers */
void quest_timers(Quest *self, s32 n)
{
    if (G_TIME_STOP != 0)
        return;
    if (self->time_left > 0) {
        self->time_left -= n;
        if (self->time_left < 0)
            self->time_left = 0;
    }
    if (self->timer2 > 0) {
        self->timer2 -= n;
        if (self->timer2 < 0)
            self->timer2 = 0;
    }
}

void quest_nop(Quest *self, s32 a, s32 b, s32 c)
{
}

/* count left of the hunting goal of the monster id, -1 if none */
s32 quest_goal_left(Quest *self, s32 id)
{
    s32 i;

    for (i = 0; i < 2; i++)
        if ((self->goal[i].flags & (GOAL_HUNT | GOAL_ACTIVE)) == (GOAL_HUNT | GOAL_ACTIVE) &&
            (s16)self->goal[i].target == (s16)id)
            return self->goal[i].count;
    return -1;
}

s32 quest_goal_left_at(Quest *self, u32 i)
{
    QuestGoal *g = &self->goal[i & 0xFFFF];

    if ((g->flags & (GOAL_HUNT | GOAL_ACTIVE)) == (GOAL_HUNT | GOAL_ACTIVE))
        return g->count;
    return -1;
}

/* 1 when the goal i is done (and not failed) */
u32 quest_goal_done(Quest *self, s32 i)
{
    u32 f = self->goal[i].flags;

    if ((f & GOAL_ACTIVE) == 0)
        return 0;
    if ((f & (GOAL_FAIL_SEEN | GOAL_FAILED)) == GOAL_FAILED)
        return 0;
    if ((f & GOAL_DONE) == 0)
        return 0;
    return 1;
}

void quest_goal_complete(Quest *self, u32 i, u32 how)
{
    if (quest_state(self, 1) == 0)
        quest_goal_set(self, i & 0xFFFF, how);
}

/* goal i completed (how: GOAL_BY_*), with the event for the other players */
void quest_goal_set(Quest *self, s32 i, u32 how)
{
    QuestGoal *g;
    QuestEvent ev;

    if (quest_goal_done(self, i) != 0)
        return;
    g = &self->goal[i];
    g->flags |= GOAL_DONE;
    if (how == GOAL_BY_KILL)
        help_popup(g_menu, 0x11);
    if (how == GOAL_BY_CAPTURE)
        help_popup(g_menu, 0x12);
    quest_goals_check(self, 0);
    g = &self->goal[i];
    if ((g->flags & (GOAL_HUNT | GOAL_ACTIVE)) == (GOAL_HUNT | GOAL_ACTIVE))
        g->count = 0;
    ev.arg[0] = how >> 16;
    ev.arg[2] = i;
    ev.zero = 0;
    ev.arg[1] = how;
    ev.type = 0x14;
    if (self->last_target == NULL)
        ev.arg[3] = 0xFFFF;
    else
        ev.arg[3] = S16(self->last_target, E_QINDEX);
    net_send(g_net, 6, &ev);
}

void quest_goals_clear(Quest *self)
{
    memset(self->goal, 0, sizeof(self->goal));
    memset(self->goal_orig, 0, sizeof(self->goal_orig));
}

/* goals of the quest file */
void quest_goals_load(Quest *self)
{
    QuestGoal *s;
    s32 i;

    self->goal_mode = self->info->goal_mode;
    s = self->info->goal;
    for (i = 0; i < 2; i++, s++)
        if (s->flags != 0)
            quest_goal_add(self, s);
}

void quest_goal_add(Quest *self, QuestGoal *src)
{
    s32 i;
    QuestGoal *g;

    for (i = 0; i < 2; i++) {
        if ((self->goal[i].flags & GOAL_ACTIVE) != 0)
            continue;
        g = &self->goal[i];
        g->flags = src->flags;
        g->target = src->target;
        g->count = src->count;
        self->goal_orig[i].flags = g->flags;
        self->goal_orig[i].target = g->target;
        self->goal_orig[i].count = g->count;
        self->goal_orig[i].flags |= GOAL_ACTIVE;
        g->flags |= GOAL_ACTIVE;
        return;
    }
}

/* per-frame check of the goals: deliveries, parts broken, damage of the repel goals */
void quest_goals_update(Quest *self)
{
    QuestGoal *g = self->goal;
    u32 i, f;
    s16 v;
    s32 half;

    for (i = 0; (s32)i < 2; i++) {
        f = g->flags;
        if (((f & GOAL_DONE) | GOAL_ACTIVE) != GOAL_ACTIVE) {
            g++;
            continue;
        }
        if ((f & GOAL_DELIVER) == GOAL_DELIVER) {
            v = quest_goal_count(self, g->target, i & 0xFFFF);
            v = v - quest_goal_have(self, g->target);
            g->count = v;
            if (v <= 0)
                quest_goal_complete(self, i & 0xFFFF, GOAL_BY_DELIVERY);
        }
        if ((g->flags & (GOAL_BREAK | GOAL_SPECIAL)) == (GOAL_BREAK | GOAL_SPECIAL) &&
            (self->parts_broken[g->target] & (1 << (u16)g->count)) != 0)
            quest_goal_complete(self, i & 0xFFFF, GOAL_BY_BREAK);
        f = g->flags;
        if ((f & (GOAL_REPEL | GOAL_SPECIAL)) != (GOAL_REPEL | GOAL_SPECIAL) ||
            self->record_target == NULL) {
            g++;
            continue;
        }
        if ((f & GOAL_HALF_TIME) != 0) {
            half = quest_time(self, 1) / 2;
            if (half < quest_time(self, 0))
                continue;       /* (the original does not move to the next goal here) */
        }
        if ((s32)self->target_life - S16(self->record_target, E_LIFE) >= g->count * 100) {
            if ((g->flags & GOAL_HALF_TIME) == GOAL_HALF_TIME && self->result != QS_FAILED &&
                self->time_msg == 0) {
                menu_msg_push_text(g_menu, text_menu(g_game, D_089AC5A0[11]));
                self->time_msg = 1;
            }
            quest_goal_complete(self, i & 0xFFFF, GOAL_BY_REPEL);
        }
        g++;
    }
}

/* count to deliver for the goal i (-1 when it is not a delivery of id) */
s16 quest_goal_count(Quest *self, s32 id, u32 i)
{
    QuestGoal *g = &self->goal_orig[i & 0xFFFF];

    if ((g->flags & (GOAL_DELIVER | GOAL_ACTIVE)) == (GOAL_DELIVER | GOAL_ACTIVE) &&
        (s16)g->target == (s16)id)
        return g->count;
    return -1;
}

/* capture goals of the monster id */
u32 quest_capture_done(Quest *self, u32 id)
{
    u32 r = 0, i;
    QuestGoal *g = self->goal;

    for (i = 0; (s32)i < 2; i++, g++) {
        if ((g->flags & (GOAL_DONE | GOAL_FAILED)) != 0 ||
            (g->flags & (GOAL_HUNT | GOAL_ACTIVE)) != (GOAL_HUNT | GOAL_ACTIVE) ||
            g->target != (id & 0xFFFF))
            continue;
        g->count--;
        if (g->count <= 0) {
            quest_goal_complete(self, i & 0xFFFF, GOAL_BY_CAPTURE);
            r = 1;
        } else {
            quest_hunt_message(self, id & 0xFF, g->count, self->goal_orig[i].count);
        }
    }
    return r;
}

/* 1 when a goal "capture" is done */
s32 quest_goal_any_done(Quest *self)
{
    s32 i;

    for (i = 0; i < 2; i++)
        if ((self->goal[i].flags & GOAL_CAPTURE) != 0 &&
            (self->goal[i].flags & (GOAL_DONE | GOAL_ACTIVE | GOAL_HUNT)) ==
                (GOAL_DONE | GOAL_ACTIVE | GOAL_HUNT))
            return 1;
    return 0;
}

/* items id delivered: of the player only (GOAL_PLAYER_ONLY) or of all the hunters */
s16 quest_goal_have(Quest *self, s32 id)
{
    s32 i, p, k;
    s16 n = 0;
    u8 *h;

    for (i = 0; i < 2; i++)
        if ((self->goal[i].flags & (GOAL_DELIVER | GOAL_ACTIVE)) == (GOAL_DELIVER | GOAL_ACTIVE) &&
            self->goal[i].target == (id & 0xFFFF))
            break;
    if (i >= 2)
        return 0;
    if ((self->goal[i].flags & GOAL_PLAYER_ONLY) != 0) {
        h = (u8 *)player_get(g_players, G_MY_SLOT);
        for (k = 0; k < 4; k++)
            if (DELIVERED_ID(h, k) == (id & 0xFFFF))
                return DELIVERED_N(h, k);
        return 0;
    }
    for (p = 0; p < 4; p++) {
        h = (u8 *)player_get(g_players, p);
        if (h == NULL)
            continue;
        for (k = 0; k < 4; k++)
            if (DELIVERED_ID(h, k) == (id & 0xFFFF))
                n += DELIVERED_N(h, k);
    }
    return n;
}

/* 1 when the main goals are completed (and with end != 0, the quest is cleared) */
u32 quest_goals_check(Quest *self, s32 end)
{
    u32 done = 0, f;
    s32 i;

    for (i = 0; i < 2; i++) {
        f = self->goal[i].flags;
        if ((f & GOAL_ACTIVE) == 0)
            continue;
        if ((f & GOAL_DONE) != 0)
            done |= 1 << i;
        if ((f & (GOAL_FAIL_SEEN | GOAL_FAILED)) == GOAL_FAILED) {
            self->goal[i].flags |= GOAL_FAIL_SEEN;
            if (end != 0)
                quest_end_fail(self, 0);
            return 0;
        }
    }
    if (self->goal_mode == 2) {
        if ((done & 3) != 3)
            return 0;
    } else if (self->goal_mode == 1) {
        if ((done & 1) != 1)
            return 0;
    } else {
        return 0;
    }
    if (end != 0)
        quest_end_clear(self, 0);
    return 1;
}

/* jumps the script to the label QC_FAIL (or QC_FAIL2) */
void quest_end_fail(Quest *self, s32 alt)
{
    QuestCmd *p = self->script;
    s16 n = 0, op = alt == 0 ? QC_FAIL : QC_FAIL2;

    for (; p->op != op; p++)
        n++;
    self->pc = n;
    G_QUEST_STATE = QS_FAILED;
}

/* jumps the script to the label QC_CLEAR (or QC_CLEAR2) */
void quest_end_clear(Quest *self, s32 alt)
{
    QuestCmd *p = self->script;
    u8 st = G_QUEST_STATE;
    s16 n = 0, op;

    if (st != QS_RUNNING && st != 1 && st != QS_NONE)
        return;
    op = alt == 0 ? QC_CLEAR : QC_CLEAR2;
    for (; p->op != op; p++)
        n++;
    self->pc = n;
    G_QUEST_STATE = QS_CLEARING;
}

/* script position after the label n */
static s16 find_label(Quest *self, s32 n)
{
    QuestCmd *p = self->script;
    s16 i = 0;

    for (; p->op != QC_LABEL || p->arg[0] != n; p++)
        i++;
    return i + 1;
}

/* position after the first command op that follows pc */
static s16 skip_to(QuestCmd *cmd, s16 pc, s32 op)
{
    for (;;) {
        cmd++;
        pc++;
        if (cmd->op == op)
            return pc + 1;
    }
}

static void send_event(u32 type)
{
    QuestEvent ev;

    ev.type = type;
    ev.zero = 0;
    ev.arg[0] = 0;
    ev.arg[1] = 0;
    net_send(g_net, 6, &ev);
}

static void push_msg(u32 i)
{
    menu_msg_push_text(g_menu, text_menu(g_game, D_089AC5A0[i]));
}

/* life of the recorded target and of its parts at the end of the quest */
static void target_time(Quest *self)
{
    u8 *m = self->record_target;

    if (m != NULL && (u8)quest_record_slot(self, m[E_KIND]) != 0xFF) {
        self->target_life = S16(m, E_LIFE);
        self->parts_broken[m[E_KIND]] = U16(m, E_PARTS);
        quest_set_mon_flags(self, m);
    }
}

/* runs the quest script from the command pc; returns the new pc */
s32 quest_script_run(Quest *self, u8 *h, s32 pc_arg)
{
    s32 pc = pc_arg;    /* (kept at 32 bits until it changes, like the original) */
    QuestCmd *c;
    char buf[0x80];
    s32 t, v;

    if (G_QUEST_STATE == QS_RUNNING && self->time_left >= 0) {
        /* "N minutes left" messages */
        if (self->time_warnings > 0) {
            t = D_089A77C4[self->time_warnings][0];
            if (!(t < self->time_left)) {
                sprintf(buf, text_menu(g_game, D_089AC5A0[4]), t / 1800);
                buf[0x7F] = 0;
                menu_msg_push_copy(g_menu, buf);
                self->time_warnings--;
            }
        }
        quest_timers(self, 1);
        if (self->time_left > 0) {
            if (self->time_left % 9000 == 0)
                quest_send_time(self);
        } else {
            self->time_left = 0;
            quest_send_time(self);
            self->time_left = -1;
            pc = find_label(self, self->timeup_label);
        }
    }
    for (;;) {
        if (self->goal_label != 0 && quest_goal_any_done(self) != 0) {
            pc = find_label(self, self->goal_label);
            self->goal_label = 0;
        }
        if (self->label2 != 0 && (s8)self->label2_go != 0) {
            pc = find_label(self, self->label2);
            self->label2 = 0;
        }
        c = &self->script[(s16)pc];
        switch (c->op) {
        case QC_WAIT_GOALS_WAVES:
            if (quest_goals_check(self, 1) == 0 && quest_waves_left(self) != 0)
                goto out;
            pc = (s16)(pc + 1);
            continue;
        case QC_IF_BONUS:
            if (self->bonus_kind == c->arg[0])
                pc = (s16)(pc + 1);
            else
                pc = skip_to(c, (s16)pc, QC_END_BONUS);
            continue;
        case QC_IF_POINTS:
            if (quest_points(self) < c->arg[0])
                pc = skip_to(c, (s16)pc, QC_END_POINTS);
            else
                pc = (s16)(pc + 1);
            continue;
        case QC_COMPLETE_GOAL:
            quest_goal_complete(self, 0, 0);
            pc = (s16)(pc + 1);
            continue;
        case QC_STOP2:
            goto out;
        case QC_NOP32:
        case QC_NOP31:
            pc = (s16)(pc + 1);
            continue;
        case QC_IF_HUNT_LEFT:
            if (c->arg[1] < (s16)quest_goal_left(self, (u16)c->arg[0]))
                pc = (s16)(pc + 1);
            else
                pc = find_label(self, c->arg[2]);
            continue;
        case QC_TIME_UP_QUIET:
            self->timeup_label = c->arg[0];
            pc = (s16)(pc + 1);
            goto out;
        case QC_LABEL:
            pc = (s16)(pc + 1);
            continue;
        case QC_STOP:
            goto out;
        case QC_GOTO:
            pc = find_label(self, c->arg[0]);
            continue;
        case QC_IF_MONSTER: {
            s32 i;
            u8 *p = NULL;

            for (i = 0; i < 20; i++) {
                p = (i >= 0 && i < 20) ? (u8 *)U32(g_monsters, 0x1220 + i * 4) : NULL;
                if (p != NULL)
                    break;
            }
            if (func_09AC88F0(p) == 1) {
                quest_goal_complete(self, 0, GOAL_BY_KILL);
                pc = (s16)(pc + 1);
            } else {
                pc = skip_to(c, (s16)pc, QC_END_MONSTER);
            }
            continue;
        }
        case QC_IF_TARGET_HIT: {
            u8 *m = self->record_target;

            if (self->target_life - c->arg[0] < S16(m, E_LIFE)) {
                pc = skip_to(c, (s16)pc, QC_END_TARGET_HIT);
            } else {
                self->target_life = S16(m, E_LIFE);
                pc = (s16)(pc + 1);
            }
            continue;
        }
        case QC_CLEAR2:
            target_time(self);
            self->result = QS_CLEARING;
            G_QUEST_STATE = QS_CLEARING;
            pc = (s16)(pc + 1);
            self->players |= 1 << (G_MY_SLOT + 8);
            send_event(0xB);
            continue;
        case QC_FAIL2:
            G_QUEST_STATE = QS_FAILED;
            self->result = QS_FAILED;
            pc = (s16)(pc + 1);
            push_msg(8);
            send_event(10);
            continue;
        case QC_LABEL2:
            self->label2 = c->arg[0];
            pc = (s16)(pc + 1);
            goto out;
        case QC_GOAL_LABEL:
            self->goal_label = c->arg[0];
            pc = (s16)(pc + 1);
            goto out;
        case QC_WAIT_ITEM:
            if (quest_goal_item_ok(self, c) == 0)
                goto out;
            pc = (s16)(pc + 1);
            continue;
        case QC_SET_PHASE:
            self->phase = c->arg[0];
            quest_goal_reached(self);
            pc = (s16)(pc + 1);
            continue;
        case QC_TIME_UP:
            self->timeup_label = c->arg[0];
            v = quest_time(self, 1);
            sprintf(buf, text_menu(g_game, D_089AC5A0[7]), v / 1800);
            buf[0x7F] = 0;
            menu_msg_push_copy(g_menu, buf);
            pc = (s16)(pc + 1);
            goto out;
        case QC_RETURN:
            G_QUEST_STATE = QS_RETURN;
            self->result = QS_RETURN;
            pc = -1;
            push_msg(6);
            if ((GAME_FLAGS & 4) == 0)
                menu_msg_push(g_menu, 0, 0xF, 0);
            goto out;
        case QC_FAIL:
            pc = (s16)(pc + 1);
            G_QUEST_STATE = QS_FAILED;
            self->result = QS_FAILED;
            send_event(4);
            continue;
        case QC_WAIT_ENTITY:
            if ((u8)entity_is(h, 4, 0) == 0)
                goto out;
            pc = (s16)(pc + 1);
            continue;
        case QC_END_STEP:
            pc = -1;
            self->end_step++;
            self->end_sub = 0;
            goto out;
        case QC_WAIT_PLAYERS:
            if ((self->players & 0xF00) == 0xF00 || G_PLAYER_HERE(G_MY_SLOT) == 0)
                pc = (s16)(pc + 1);
            goto out;
        case QC_CLEAR:
            target_time(self);
            g_game[0xC] = 0x81;
            self->result = QS_CLEARING;
            G_QUEST_STATE = QS_CLEARING;
            pc = (s16)(pc + 1);
            self->players |= 1 << (G_MY_SLOT + 8);
            send_event(2);
            continue;
        case QC_ADD_TIME:
            pc = (s16)(pc + 1);
            self->time_left += (u16)c->arg[0];
            goto out;
        case QC_WAIT_AREA:
            if (quest_nop0(self) == 0) {
                pc = self->resume_pc;
                goto out;
            }
            if (U16(h, E_AREA) != c->arg[0])
                goto out;
            pc = (s16)(pc + 1);
            continue;
        case QC_WAIT_GOAL_LEFT:
            if (c->arg[1] < (s16)quest_goal_left_at(self, (u8)c->arg[0]))
                goto out;
            pc = (s16)(pc + 1);
            continue;
        case QC_IF_GOALS:
            if (quest_goals_check(self, 0) == 0)
                pc = skip_to(c, (s16)pc, QC_END_IF_GOALS);
            else
                pc = (s16)(pc + 1);
            continue;
        case QC_END_IF:
            pc = (s16)(pc + 1);
            continue;
        case QC_MESSAGE:
            push_msg(c->arg[0]);
            pc = (s16)(pc + 1);
            goto out;
        case QC_WAIT:
            self->wait--;
            if (self->wait <= 0)
                pc = (s16)(pc + 1);
            goto out;
        case QC_SET_WAIT:
            pc = (s16)(pc + 1);
            self->wait = c->arg[0];
            goto out;
        case QC_WAIT_GOALS:
            if (quest_goals_check(self, 1) == 0)
                goto out;
            pc = (s16)(pc + 1);
            continue;
        case QC_NOP2:
            quest_nop(self, c->arg[0], c->arg[1], c->arg[2]);
            pc = (s16)(pc + 1);
            continue;
        case QC_GOAL_MODE:
            pc = (s16)(pc + 1);
            self->goal_mode = c->arg[0];
            continue;
        default:
            goto out;
        }
    }
out:
    if (g_game[0xC] == 0x80) {
        g_game[0xC] = 0;
        func_08813594(g_camera, 0);
    }
    return pc;
}

/* flags of the game for some special monsters killed (life 0) */
void quest_set_mon_flags(Quest *self, u8 *h)
{
    s8 k, m;

    if (S16(h, E_LIFE) != 0)
        return;
    k = QUEST->file->kind;
    m = h[E_KIND];
    if (k == 5) {
        if (m == 0x24)
            game_set_flag(g_game, 0xD9);
        else if (m == 2)
            game_set_flag(g_game, 0xD8);
        else if (m == 0x3B)
            game_set_flag(g_game, 0xD7);
        else if (m == 0x41)
            game_set_flag(g_game, 0xD6);
        else if (m == 0x36)
            game_set_flag(g_game, 0xD5);
    } else if (k == 2) {
        if (m == 0x24)
            game_set_flag(g_game, 0xD4);
        else if (m == 2)
            game_set_flag(g_game, 0xD3);
    }
}

s32 quest_points(Quest *self)
{
    return self->points[0] + self->points[1];
}

/* marks in Quest.item_mask the special item id */
void quest_mark_item(Quest *self, u32 id)
{
    s16 t[35];
    s32 i;

    for (i = 0; i < 35; i++)
        t[i] = D_089A8288[i];
    for (i = 0; i < 35; i++) {
        if ((u16)t[i] == (id & 0xFFFF)) {
            if (i < 32)
                self->item_mask[0] |= 1u << i;
            else
                self->item_mask[1] |= 1u << (i - 32);
            return;
        }
    }
}

/* rewards of the quest: up to 16 + 16 items in G_REWARDS */
void quest_make_rewards(Quest *self)
{
    u32 h = player_get(g_players, G_MY_SLOT);
    u8 timed = 0;
    s32 i, j, n = 0, out = 0, k;
    QuestRewardRule *rule;
    u8 *l, *e;
    const s8 *ch;
    u32 c, sum, r, acc;
    u16 a, b;

    U32(g_game, 0x410) = 0;
    U32(g_game, 0x414) = 0;
    for (i = 0; i < 0x40; i++)
        G_REWARDS[i] = 0;
    for (rule = self->rewards; rule->cond != 0xFFFF; rule++) {
        ch = D_089A7F64;
        c = rule->cond;
        if (c >= 0xF7 && c < 0x10C) {
            /* monsters hunted */
            if ((s32)(quest_monsters_hunted(self) & 0xFFFF) < (s32)(rule->cond - 0xF7))
                goto next;
        } else if (c >= 0x24 && c <= 0x35) {
            /* time used */
            s32 t1 = quest_time(QUEST, 1);
            s32 t0 = quest_time(QUEST, 0);

            if (timed != 0 || !((t1 - t0) / 1800 < D_089A822C[rule->cond - 0x24]))
                goto next;
            timed = 1;
        } else if (c >= 0x10C && c <= 0x119) {
            /* points */
            u32 area = G_QUEST_ID;

            if (quest_points(self) < D_08970DB4[rule->cond + area * 14])
                goto next;
        } else if (c == 1) {
            if (quest_monsters_hunted(self) <= 0)
                goto next;
            ch = D_089A7F84;
        } else if (c == 0x8000) {
            if ((GAME_FLAGS & 4) != 0) {
                ch = D_089A7F68;
            } else {
                /* chances of the carves: 0x20 for each carve, then the skill value */
                u32 cnt = rule->count;
                s8 v;

                if (hunter_felyne_skill(h, 0x13) != 0)
                    cnt = (u8)(cnt + 1);
                else if (hunter_felyne_skill(h, 0x14) != 0 && (game_rand(g_sys, 1) & 0xFFFF & 1) != 0)
                    cnt = (u8)(cnt + 1);
                k = 0;
                for (; k < (s32)cnt; k++)
                    self->carve_chance[k] = 0x20;
                if (hunter_has_skill(h, 0x90) != 0)
                    v = 0x1A;
                else if (hunter_has_skill(h, 0x92) != 0)
                    v = 0x10;
                else if (hunter_has_skill(h, 0x91) != 0)
                    v = 0x1D;
                else if (hunter_has_skill(h, 0x93) != 0)
                    v = 8;
                else
                    v = 0x16;
                for (; k < 10; k++)
                    self->carve_chance[k] = v;
                self->carve_chance[k] = 0;
                ch = (const s8 *)self->carve_chance;
            }
        }
        /* rolls: one more item while rand % 32 < chance[j]; list of {weight, item, count} */
        l = QOFF(self, rule->list);
        for (j = 0;; j++) {
            s8 cj = ch[j];

            if (!((s32)(game_rand(g_sys, 0) & 0xFFFF & 0x1F) < cj))
                break;
            sum = 0;
            for (e = l; U16(e, 0) != 0xFFFF; e += 6)
                sum = (sum + U16(e, 0)) & 0xFFFF;
            r = ((s32)(game_rand(g_sys, 0) & 0xFFFF) % (s32)(sum & 0xFFFF)) & 0xFFFF;
            if (rule->cond == 0x8000 && j == 0)
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
                U16(g_game, 0x390 + out) = U16(e, 0);
                U16(g_game, 0x392 + out) = U16(e, 2);
                out += 4;
            }
            if (n >= 16)
                break;
        }
next:
        if (n >= 16)
            break;
    }
    quest_extra_rewards(self);
    /* sorts the two lists of 16 by item */
    for (k = 0; k < 2; k++) {
        for (i = k * 16; i < k * 16 + 15; i++) {
            u16 *p = &G_REWARDS[i * 2];

            for (j = i + 1; j < k * 16 + 16; j++) {
                u16 *q = &G_REWARDS[j * 2];

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

u32 quest_nop0(Quest *self)
{
    return 0;
}

/* item goal of the command: at least arg1 items arg0 in the pouch */
s32 quest_goal_item_ok(Quest *self, QuestCmd *c)
{
    s16 n = c->arg[1];

    return n <= (s32)pouch_count(g_game, (u16)c->arg[0]);
}

/* monsters hunted of the kinds listed in the game table of the first large monster */
s16 quest_monsters_hunted(Quest *self)
{
    u8 *base = self->base;
    QuestGroup *large = (QuestGroup *)(base + self->file->large);
    u8 *p = game_table_ptr(g_game, *(u8 *)(base + large->list1));
    s16 n = 0;

    for (; *p != 0xFF; p++)
        n += (s16)self->kills[*p];
    return n;
}

/* 1 when the goal of the monster id counts for the end of the quest */
u32 quest_goal_counts(Quest *self, s32 id)
{
    s32 i;
    u32 f;

    for (i = 0; i < 2; i++) {
        f = self->goal[i].flags;
        if ((f & (GOAL_HUNT | GOAL_ACTIVE)) != (GOAL_HUNT | GOAL_ACTIVE) &&
            (f & (GOAL_SPECIAL | GOAL_ACTIVE)) != (GOAL_SPECIAL | GOAL_ACTIVE))
            continue;
        if ((s16)self->goal[i].target != (s16)id)
            continue;
        if (self->goal_mode == 2)
            return 1;
        if (self->goal_mode == 1 && i == 0)
            return 1;
    }
    return 0;
}

/* quest events received from the other players (network) */
void quest_event(Quest *self, u32 who, QuestEvent *pkt)
{
    s32 i;
    u8 *p;

    switch (pkt->type) {
    case 2:
        self->players |= 1 << ((who & 0xFF) + 8);
        quest_end_clear(self, 0);
        return;
    case 4:
        if (G_QUEST_STATE != QS_FAILED && G_QUEST_STATE != QS_RETURN &&
            G_QUEST_STATE != QS_ABANDONED)
            quest_end_fail(self, 0);
        break;
    case 5: {
        s32 v = ((s16)pkt->arg[0] << 16) | pkt->arg[1];

        if (v < self->time_left)
            self->time_left = v;
        break;
    }
    case 6:
        if (self->phase < (s16)pkt->arg[0]) {
            u8 *m = g_monster_models;

            self->phase = (s16)pkt->arg[0];
            U32(m, 0x210) = U32(m, 0x204);
            U32(m, 0x20C) = U32(m, 0x208);
            for (i = 0x15; i <= 0x2A; i++)
                U16(m, i * 0xC) &= 0xFFFD;
            func_088B87B0(g_loader, 1);
        }
        break;
    case 7:
        quest_monster_event(self, pkt);
        break;
    case 9:
        p = (u8 *)player_get(g_players, who & 0xFF);
        if (p != NULL)
            quest_hunter_item_set(self, p, pkt->arg[0], (s16)pkt->arg[1]);
        break;
    case 10:
        if (G_QUEST_STATE != QS_FAILED && G_QUEST_STATE != QS_RETURN &&
            G_QUEST_STATE != QS_ABANDONED)
            quest_end_fail(self, 1);
        break;
    case 0xB:
        self->players |= 1 << ((who & 0xFF) + 8);
        quest_end_clear(self, 1);
        break;
    case 0xC:
        self->players |= 1 << (who & 0xFF);
        self->rank[who & 0xFF][0] = (s16)pkt->arg[0];
        break;
    case 0xD:
        self->players |= 1 << ((who & 0xFF) + 4);
        self->net_value |= (s16)pkt->arg[0] << 16;
        self->net_value |= (s32)(s16)pkt->arg[1];
        break;
    case 0xE:
        if (self->bonus_kind != 0) {
            self->bonus_kind = 0;
            push_msg(9);
            sound_play(g_sound, 1, 0x77, 0, 0, 0, 0, 0, 0);
        }
        break;
    case 0xF:
        ((s32 *)self->points)[who & 0xFF] = ((s16)pkt->arg[0] << 16) | pkt->arg[1];
        break;
    case 0x10:
        self->item_mask[0] |= pkt->arg[0] | (pkt->arg[1] << 16);
        self->item_mask[1] |= pkt->arg[2];
        break;
    case 0x12:
        if (self->timer2_count < (s16)pkt->arg[0]) {
            self->timer2_count = (s16)pkt->arg[0];
            self->timer2 = -1;
        }
        break;
    case 0x13:
        G_TIMER2_ON = 1;
        self->timer2 = 18000;
        break;
    case 0x14: {
        u32 how = ((s16)pkt->arg[0] << 16) | pkt->arg[1];

        if ((s16)pkt->arg[3] != -1) {
            self->last_target = NULL;
            for (i = 0; i < 20; i++) {
                p = (i >= 0 && i < 20) ? (u8 *)U32(g_monsters, 0x1220 + i * 4) : NULL;
                if (p != NULL && S16(p, E_QINDEX) == (s16)pkt->arg[3]) {
                    self->last_target = p;
                    break;
                }
            }
        }
        quest_goal_set(self, pkt->arg[2], how);
        break;
    }
    case 0x15:
        self->wave[(s16)pkt->arg[0]].seen[(s16)pkt->arg[1]] |= (u8)(1 << (who & 0xFF));
        break;
    }
}

/* a monster of the quest left / was killed on the machine of another player */
void quest_monster_event(Quest *self, QuestEvent *pkt)
{
    u32 n = (pkt->arg[1] >> 8) & 0xFF;
    QuestMonster *m = quest_find_monster_idx(self, (s16)pkt->arg[0], pkt->arg[1] & 0xFF);
    u8 *p;
    s32 i;
    s16 f;

    if (m == NULL)
        return;
    f = m->state;
    if ((f & (QM_WAITING | QM_RESPAWN)) == 0)
        return;
    for (i = 0; i < 20; i++) {
        p = (i >= 0 && i < 20) ? (u8 *)U32(g_monsters, 0x1220 + i * 4) : NULL;
        if (p == NULL || U16(p, E_SLOT) != m->slot || S16(p, E_QINDEX) != (u8)m->index)
            continue;
        if (p[E_KIND] != m->kind)
            return;
        if ((U32(p, 4) & 1) == 0)
            return;
        if ((f & QM_RESPAWN) == 0) {
            if (p[0x1D4] >= 2)
                return;
            if (p[0x298] == 5)
                return;
        } else if (!((s32)(n & 0xFFFF) < m->count)) {
            return;
        }
        if (p[0x1D4] >= 2 || p[0x298] == 5)
            func_09AC9608(p);
        func_09AC8630(p, 5, 0);
        p[0x230] = 0xFF;
        m->state = QM_WAITING;
        S16(p, E_LIFE) = 0;
        quest_set_target(self, p);
        return;
    }
    if ((s32)(n & 0xFFFF) < m->count) {
        quest_check_targets_done(self, (u16)m->kind);
        quest_kill_count(self, (u16)m->kind);
        m->count--;
        if (m->count <= 0) {
            m->state = QM_DONE;
            m->slot = -1;
        } else {
            m->state = QM_WAITING;
        }
    }
}
