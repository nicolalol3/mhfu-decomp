/* The current quest (object g_quest, 0x8A8 bytes) and the format of the quest files.
 * Shared by src/game/quest*.c. (Guessed names: the unknown fields keep their offset.) */
#ifndef QUEST_H
#define QUEST_H
#include "game.h"
/* the real functions (menu.h has aliases for the callers there) */
#undef quest_time
#undef quest_text

/* ---- quest file (loaded at g_quest_file; all the offsets are relative to its start) ---- */

typedef struct QuestCmd {       /* command of the quest script (QC_*) */
    s16 op;
    s16 arg[3];
} QuestCmd;

typedef struct QuestGoal {      /* goal of the quest (also in the file, QuestInfo.goal) */
    u32 flags;                  /* GOAL_* */
    u16 target;                 /* monster kind or item id */
    s16 count;                  /* how many (in the quest: how many are left) */
} QuestGoal;

typedef struct QuestStep {      /* sub-goal that moves the quest to the next phase */
    u32 kind;                   /* 1: hunt monster id, 2: have item id */
    u16 id;
    s8 count;
    s8 phase;                   /* phase reached when done */
} QuestStep;

typedef struct QuestInfo {      /* general data of the quest */
    u32 flags;                  /* 0x00 QF_* */
    s32 fee;                    /* 0x04 contract fee */
    s32 reward;                 /* 0x08 reward money */
    s32 penalty;                /* 0x0C money lost for each faint */
    s32 time;                   /* 0x10 time limit in frames (30 per second) */
    u32 texts;                  /* 0x14 offset of the texts (one table per language) */
    u8 x18[2];
    s8 stars;                   /* 0x1A */
    u8 x1b;
    s8 map;                     /* 0x1C */
    u8 x1d;
    u8 goal_mode;               /* 0x1E 1: main goal only, 2: both goals */
    u8 x1f;
    QuestGoal goal[2];          /* 0x20 */
} QuestInfo;

typedef struct QuestFile {      /* header of the quest file */
    u32 info;                   /* 0x00 offset of the QuestInfo */
    u32 x04;
    u32 sub_data;               /* 0x08 */
    u32 rewards;                /* 0x0C offset of the QuestRewardRule list */
    u32 script;                 /* 0x10 offset of the QuestCmd list */
    u32 large;                  /* 0x14 offset of the QuestGroup list of the large monsters */
    u32 phases;                 /* 0x18 offset of the area tables of each phase */
    u32 map_objects;            /* 0x1C */
    u32 target_pos;             /* 0x20 random start points of the targets */
    u32 tables;                 /* 0x24 item tables */
    QuestStep step[2];          /* 0x28 */
    s16 size_base;              /* 0x38 base size (percent) of the large monsters */
    u8 x3a[2];
    s32 hrp;                    /* 0x3C hunter rank points when cleared */
    s32 hrp_other;              /* 0x40 hunter rank points otherwise */
    s8 kind;                    /* 0x44 1: village, 2 and 5: guild (low / high?) */
    s8 start_mode;              /* 0x45 0: fixed start area, 1: random (skill), 2: random */
    s8 bonus_kind;              /* 0x46 bonus of the quest: 1 time, 2 hunting */
    s8 bonus_monster;           /* 0x47 */
    s8 bonus_count;             /* 0x48 */
    u8 x49;
    s8 size_rank;               /* 0x4A limits of the random sizes (D_08A62C80) */
    u8 size_class;              /* 0x4B default size class of the monsters */
} QuestFile;

/* entry of the area tables (QuestFile.phases) and of the large monsters (QuestFile.large):
 * the lists end with an entry where list0 == 0 (large) or id == 0 (areas) */
typedef struct QuestGroup {
    u32 id;                     /* area */
    u32 x4;
    u32 list0;                  /* offset: kinds of the small monsters (u32 each, -1 = end),
                                   or of the large monsters */
    u32 list1;                  /* offset: QuestMonster list */
} QuestGroup;

typedef struct QuestMonster {   /* monster placed by the quest (0x3C bytes) */
    s16 kind;                   /* 0x00 -1: end of the list, -2: end, next list follows */
    u8 x02[2];
    s8 count;                   /* 0x04 how many times it appears */
    s8 size_class;              /* 0x05 */
    u8 x06[2];
    u16 area;                   /* 0x08 */
    s8 not_target;              /* 0x0A 1: does not count for the goals */
    s8 random_pos;              /* 0x0B 1: random start point (QuestFile.target_pos) */
    u8 x0c[0x10];
    u32 angle;                  /* 0x1C */
    f32 pos[3];                 /* 0x20 */
    s16 index;                  /* 0x2C unique number in the quest */
    s16 state;                  /* 0x2E QM_* */
    s16 size;                   /* 0x30 percent */
    s16 life;                   /* 0x32 life saved when it left the area, -1 = full */
    s8 slot;                    /* 0x34 entity slot, -1 = not present */
    u8 large;                   /* 0x35 */
    u8 x36[6];
} QuestMonster;

typedef struct QuestRewardRule { /* rule of the rewards: cond and list of {weight, item, count} */
    u16 cond;
    u8 x2;
    u8 count;                   /* (cond 0x8000: carves) */
    u32 list;                   /* offset */
} QuestRewardRule;

/* gathering point (20 of them after the area object g_area) */
typedef struct GatherSpot {
    f32 pos[3];
    f32 radius;
    u16 table;                  /* 0x10 gathering table; 0xFFFF = free */
    u16 left;                   /* 0x12 gatherings left, 0xFF = no limit */
    u16 kind;                   /* 0x14 2: mining / bugs (skills) */
    u16 x16;
    u16 area;                   /* 0x18 */
    u8 flags;                   /* 0x1A bit 0: kept when the area changes */
    u8 x1b;
    s16 timer;                  /* 0x1C 0 = permanent */
    u8 x1e[2];
} GatherSpot;
_Static_assert(sizeof(GatherSpot) == 0x20, "GatherSpot");

/* ---- the quest object ---- */

typedef struct QuestItem {      /* item given by the quest */
    u16 id;
    s8 count;
    u8 pad;
} QuestItem;

typedef struct QuestWave {      /* large monsters that come one after the other (0x2C bytes) */
    QuestMonster *mon[5];       /* 0x00 */
    s8 kind[5];                 /* 0x14 */
    s8 count[5];                /* 0x19 */
    s8 n;                       /* 0x1E */
    s8 cur;                     /* 0x1F current monster */
    u8 seen[5];                 /* 0x20 players that sent the wave event (bit mask) */
    u8 spawned[5];              /* 0x25 */
    u8 pad[2];
} QuestWave;

typedef struct QuestEvent {     /* event sent to the other players (net_send kind 6) */
    u8 type;
    u8 b1;
    u16 zero;
    u16 arg[4];
} QuestEvent;

typedef struct Quest {
    s8 end_step;                /* 0x000 steps of the end of the quest (quest_update) */
    s8 end_sub;                 /* 0x001 */
    u8 x002[4];
    s8 result;                  /* 0x006 QS_* at the end */
    u8 x007;
    s8 timeup_label;            /* 0x008 script label when the time is over */
    s8 goal_label;              /* 0x009 script label when a goal is done */
    u8 x00a[2];
    s32 wait;                   /* 0x00C wait counter of the script */
    s32 time_left;              /* 0x010 frames; -1 when over */
    s32 reward;                 /* 0x014 reward money (lowered by each faint) */
    s32 penalty;                /* 0x018 */
    s16 pc;                     /* 0x01C script position, -1 = ended */
    s16 resume_pc;              /* 0x01E */
    s8 phase;                   /* 0x020 phase of the quest (which monsters appear) */
    s8 spot_count;              /* 0x021 active gathering points */
    u8 x022[2];
    u8 *last_target;            /* 0x024 monster that changed the goals last */
    u32 flags;                  /* 0x028 QF_* */
    u8 x02c[0x20];
    QuestFile *file;            /* 0x04C */
    u8 *base;                   /* 0x050 start of the file: base of all its offsets */
    QuestCmd *script;           /* 0x054 */
    u32 *texts;                 /* 0x058 */
    QuestRewardRule *rewards;   /* 0x05C */
    u32 *map_objects;           /* 0x060 */
    u32 x064;
    QuestInfo *info;            /* 0x068 */
    QuestItem items[5];         /* 0x06C */
    s8 item_next;               /* 0x080 slot replaced when the 5 are full */
    u8 items_new;               /* 0x081 */
    s8 area_phase;              /* 0x082 phase when the monsters of the area were loaded */
    s8 time_warnings;           /* 0x083 "N minutes left" messages still to show */
    u8 rank[4][2];              /* 0x084 hunter rank of each player: at the end, at the start */
    struct { u16 a; u8 b; u8 pad; } x08c[32];
    s32 net_value;              /* 0x10C */
    u32 players;                /* 0x110 masks of the players: bits 0-3 rank sent, 4-7 ...,
                                   8-11 quest ended */
    s16 target_life;            /* 0x114 life of the recorded target */
    u8 x116;
    s8 label2;                  /* 0x117 */
    s32 sync_timer;             /* 0x118 */
    u8 label2_go;               /* 0x11C */
    u8 seed[15];                /* 0x11D random seeds (shared online) */
    u8 x12c;
    s8 bonus_kind;              /* 0x12D 1: time, 2: hunting; 0 = lost */
    s8 bonus_monster;           /* 0x12E */
    s8 bonus_count;             /* 0x12F */
    s8 bonus_step;              /* 0x130 */
    s8 bonus_table;             /* 0x131 */
    u16 rand;                   /* 0x132 random generator of the quest */
    u16 size_max[90];           /* 0x134 largest monster of each kind hunted */
    u16 size_min[90];           /* 0x1E8 smallest */
    u16 kills[90];              /* 0x29C */
    u16 captures[90];           /* 0x350 */
    u16 count_start[90];        /* 0x404 monsters of each kind at the start */
    u16 count_end[90];          /* 0x4B8 and at the end */
    u32 counted[16];            /* 0x56C bit mask of the QuestMonster.index counted */
    s8 points_n;                /* 0x5AC */
    u8 x5ad[3];
    s32 points[2];              /* 0x5B0 points of the two hunters (treasure / training) */
    u32 item_mask[2];           /* 0x5B8 special items found (quest_mark_item) */
    s16 main_models;            /* 0x5C0 models of the main monsters loaded */
    u8 parts_broken[90];        /* 0x5C2 bit mask of the parts broken, for each kind */
    u8 parts_rewarded[90];      /* 0x61C */
    u8 x676[2];
    s32 timer2;                 /* 0x678 second timer */
    u8 goal_mode;               /* 0x67C 1: main goal only, 2: both goals */
    u8 x67d[3];
    s8 bonus_items;             /* 0x680 rewards added by the bonuses */
    u8 x681;
    s16 log_id;                 /* 0x682 */
    u8 log[0x88];               /* 0x684 best events of the quest (record_add_best) */
    u8 player_n;                /* 0x70C players that share the reward */
    u8 x70d[3];
    s32 msg_timer;              /* 0x710 */
    u16 file_id;                /* 0x714 file of the quest loaded */
    u8 load_step;               /* 0x716 */
    u8 x717;
    u32 *phases;                /* 0x718 */
    QuestGroup *large;          /* 0x71C */
    u8 *record_target;          /* 0x720 monster whose life is recorded */
    s8 timer2_count;            /* 0x724 */
    u8 x725[3];
    QuestGoal goal[2];          /* 0x728 */
    QuestGoal goal_orig[2];     /* 0x738 the goals as they were at the start */
    u8 x748[2];
    u8 record_kind;             /* 0x74A */
    u8 x74b;
    QuestStep step[2];          /* 0x74C */
    u8 slot[6];                 /* 0x75C bit 7: set, bits 0-6: value */
    u8 hunt_msg[5];             /* 0x762 "N left" messages already shown */
    u8 active;                  /* 0x767 */
    QuestWave wave[2];          /* 0x768 */
    u8 size_class;              /* 0x7C0 default size class */
    u8 carve_chance[11];        /* 0x7C1 chances of the carves (0-terminated) */
    u8 comrade_kills[90];       /* 0x7CC monsters hunted by the Felyne comrade */
    u8 x826[2];
    GatherSpot *gathered[20];   /* 0x828 gathering points used by the player */
    u16 gather_n[20];           /* 0x878 times each one was used */
    u32 gather_total;           /* 0x8A0 */
    u8 time_msg;                /* 0x8A4 */
    u8 x8a5[3];
} Quest;

/* the offsets above must stay the ones of the game */
#define QCHECK(t, f, off) _Static_assert(__builtin_offsetof(t, f) == (off), #t "." #f)
QCHECK(QuestInfo, goal, 0x20);
QCHECK(QuestFile, size_class, 0x4B);
QCHECK(QuestMonster, slot, 0x34);
_Static_assert(sizeof(QuestMonster) == 0x3C, "QuestMonster");
_Static_assert(sizeof(QuestWave) == 0x2C, "QuestWave");
QCHECK(Quest, last_target, 0x24);
QCHECK(Quest, file, 0x4C);
QCHECK(Quest, items, 0x6C);
QCHECK(Quest, x08c, 0x8C);
QCHECK(Quest, net_value, 0x10C);
QCHECK(Quest, seed, 0x11D);
QCHECK(Quest, bonus_kind, 0x12D);
QCHECK(Quest, size_max, 0x134);
QCHECK(Quest, counted, 0x56C);
QCHECK(Quest, points, 0x5B0);
QCHECK(Quest, main_models, 0x5C0);
QCHECK(Quest, timer2, 0x678);
QCHECK(Quest, log, 0x684);
QCHECK(Quest, player_n, 0x70C);
QCHECK(Quest, phases, 0x718);
QCHECK(Quest, goal, 0x728);
QCHECK(Quest, step, 0x74C);
QCHECK(Quest, wave, 0x768);
QCHECK(Quest, size_class, 0x7C0);
QCHECK(Quest, comrade_kills, 0x7CC);
QCHECK(Quest, gathered, 0x828);
QCHECK(Quest, time_msg, 0x8A4);
_Static_assert(sizeof(Quest) == 0x8A8, "Quest");

extern u8 g_quest_file[];
#define QUEST       ((Quest *)g_quest)
#define QFILE       ((QuestFile *)g_quest_file)   /* the loaded quest file */
#define QOFF(q, off) ((u8 *)((u32)(off) + (u32)(q)->base))   /* offset in the file -> pointer */

/* QuestGoal.flags */
enum {
    GOAL_HUNT = 0x1,            /* hunt or capture count monsters target */
    GOAL_DELIVER = 0x2,         /* deliver count items target */
    GOAL_SPECIAL = 0x4,         /* with GOAL_BREAK or GOAL_REPEL */
    GOAL_ACTIVE = 0x10,
    GOAL_DONE = 0x20,
    GOAL_FAILED = 0x40,         /* (a monster to capture was killed) */
    GOAL_PLAYER_ONLY = 0x80,    /* only the items of the local player count */
    GOAL_CAPTURE = 0x100,       /* must be captured */
    GOAL_FAIL_SEEN = 0x400,
    GOAL_BREAK = 0x4000,        /* break the part count of monster target */
    GOAL_REPEL = 0x8000,        /* deal count * 100 damage to the target */
    GOAL_HALF_TIME = 0x10000    /* (repel) only after half of the time */
};

/* how a goal was completed (quest_goal_set) */
enum {
    GOAL_BY_DELIVERY = 0x2,
    GOAL_BY_CAPTURE = 0x100,
    GOAL_BY_KILL = 0x200,
    GOAL_BY_BREAK = 0x4000,
    GOAL_BY_REPEL = 0x8000
};

/* QuestMonster.state */
enum {
    QM_DONE = 0x1,              /* hunted the number of times */
    QM_WAITING = 0x2,
    QM_RESPAWN = 0x4,           /* killed, will come back */
    QM_CAPTURED = 0x8
};

/* Quest.flags (copy of QuestInfo.flags); the meaning of most bits is not known yet */
enum {
    QF_TREASURE = 0x10,         /* treasure hunt (points) (guess) */
    QF_KEEP_LIFE = 0x80,        /* the life of the target is kept between quests */
    QF_ANY_MAP = 0x1000,
    QF_FIXED_SIZE = 0x4000,
    QF_KEEP_SIZE = 0x8000,
    QF_NO_HUNT_MSG = 0x400000,  /* (also no rewards for the captures) */
    QF_TRAINING = 0x1000000     /* training school (guess) */
};

/* commands of the quest script (QuestCmd.op); "skip to X" = go after the next command X */
enum {
    QC_CLEAR = -1,              /* label: the quest is cleared */
    QC_FAIL = -2,               /* label: the quest is failed */
    QC_GOAL_MODE = 1,           /* arg0: Quest.goal_mode */
    QC_NOP2 = 2,
    QC_WAIT_GOALS = 3,          /* wait until the goals are done (then clear) */
    QC_ADD_TIME = 4,            /* arg0: frames; stop */
    QC_END_IF = 5,
    QC_IF_GOALS = 6,            /* goals done, else skip to QC_END_IF_GOALS */
    QC_END_IF_GOALS = 7,
    QC_SET_WAIT = 9,            /* arg0: frames; stop */
    QC_WAIT = 0xA,              /* wait the frames of QC_SET_WAIT */
    QC_MESSAGE = 0xB,           /* arg0: message; stop */
    QC_WAIT_AREA = 0xC,         /* wait until the hunter is in the area arg0 */
    QC_WAIT_ENTITY = 0x18,
    QC_LABEL = 0x1A,            /* arg0: label number */
    QC_TIME_UP = 0x1B,          /* arg0: label when the time is over (with message); stop */
    QC_GOTO = 0x1C,             /* arg0: label */
    QC_END_STEP = 0x1E,         /* script ended: next end step */
    QC_RETURN = 0x1F,           /* back to the village */
    QC_SET_PHASE = 0x20,        /* arg0: Quest.phase */
    QC_WAIT_ITEM = 0x21,        /* wait until arg1 items arg0 are in the pouch */
    QC_GOAL_LABEL = 0x23,       /* arg0: label when a goal is done; stop */
    QC_WAIT_GOAL_LEFT = 0x24,   /* wait until goal arg0 has at most arg1 left */
    QC_FAIL2 = 0x25,
    QC_CLEAR2 = 0x26,
    QC_STOP = 0x27,
    QC_IF_TARGET_HIT = 0x28,    /* the target lost arg0 life, else skip to QC_END_TARGET_HIT */
    QC_END_TARGET_HIT = 0x29,
    QC_IF_MONSTER = 0x2A,       /* else skip to QC_END_MONSTER */
    QC_END_MONSTER = 0x2B,
    QC_LABEL2 = 0x2C,           /* arg0: Quest.label2; stop */
    QC_WAIT_PLAYERS = 0x2D,     /* wait for the other players */
    QC_TIME_UP_QUIET = 0x2F,    /* arg0: label when the time is over; stop */
    QC_IF_HUNT_LEFT = 0x30,     /* goal of monster arg0 has more than arg1 left, else goto arg2 */
    QC_NOP31 = 0x31,
    QC_NOP32 = 0x32,
    QC_STOP2 = 0x33,
    QC_COMPLETE_GOAL = 0x34,
    QC_IF_POINTS = 0x35,        /* points >= arg0, else skip to QC_END_POINTS */
    QC_END_POINTS = 0x36,
    QC_IF_BONUS = 0x37,         /* bonus kind == arg0, else skip to QC_END_BONUS */
    QC_END_BONUS = 0x38,
    QC_WAIT_GOALS_WAVES = 0x39  /* wait for the goals (or for the end of the waves) */
};

/* fields of the entities (hunters and monsters), for U8/U16/S16/F32(e, ...) */
enum {
    E_SLOT = 0x1E6,             /* u16 player slot / entity slot */
    E_KIND = 0x1E8,             /* u8 monster kind */
    E_ANGLE = 0x1F4,
    E_POS = 0x200,              /* f32 x, y, z */
    E_SIZE = 0x274,             /* u16 */
    E_AREA = 0x29A,             /* u16 */
    E_LIFE = 0x2E4,             /* s16 */
    E_QINDEX = 0x4AC,           /* s16 QuestMonster.index */
    E_SIZE_CLASS = 0x4BC,       /* s8 */
    E_DELIVERED = 0x542,        /* 4 x {u16 item, s16 count} */
    E_SIZE_SHOWN = 0x5F4,       /* s16 */
    E_PARTS = 0x774             /* u16 parts broken */
};


/* quest.c */
void quest_count_kind(Quest *self);
int quest_outside_map(Quest *self);
u8 *quest_map_object(Quest *self, u32 id);
void quest_snap_point(Quest *self, f32 *out, const f32 *in);
u8 *quest_table_entry(Quest *self, s32 i);
u8 *quest_large_list(Quest *self, s32 which);
u8 *quest_find_entry(Quest *self, u32 id, s32 which, s32 idx);
u8 *quest_sub_data(Quest *self);
void quest_ctor(Quest *self);
int quest_is_flag_800000(Quest *self);
void quest_start(Quest *self);
void quest_fail(Quest *self);
void quest_pick_start_area(Quest *self, u32 who);
void quest_check_end(Quest *self);
s32 quest_state(Quest *self, s32 mode);
s32 quest_time(Quest *self, s32 mode);
int quest_is_target(Quest *self, u8 *h);
void quest_clear_waves(Quest *self);
void quest_wave_add(Quest *self, s32 i, QuestMonster *m);
int quest_wave_is_last(Quest *self, u8 *h);
int quest_is_target2(Quest *self, u8 *h);
void quest_spawn_waves(Quest *self);
void quest_send_wave(Quest *self, u32 i);
s32 quest_place_target(Quest *self, u8 *h);
int quest_pick_target_pos(Quest *self, u8 *h);
void quest_target_killed(Quest *self, u8 *h);
void quest_target_left(Quest *self, u8 *h);
void quest_target_captured(Quest *self, u8 *h);
void quest_set_target(Quest *self, u8 *h);
u32 quest_check_targets_done(Quest *self, u32 id);
s32 quest_faint(Quest *self);
s32 quest_update(Quest *self);
void quest_rand_seed(Quest *self);
u32 quest_rand(Quest *self);
void quest_bonus_check(Quest *self);
void quest_hunt_done(Quest *self, u8 *h);
void quest_goal_reached(Quest *self);
void quest_check_deliveries(Quest *self);
void quest_kill_count(Quest *self, s32 id);
void quest_clear_steps(Quest *self);
void quest_load_steps(Quest *self);
u8 *quest_text(Quest *self, s32 i);
s32 quest_large_kinds(Quest *self, u8 *out);
s32 quest_item_count(Quest *self, u32 id);
s32 quest_item_add(Quest *self, u32 id, s32 n);

/* quest_monsters.c */
void quest_item_remove(Quest *self, u32 id, s32 n);
void quest_give_items(Quest *self, u8 *h);
s32 quest_hunter_item_set(Quest *self, u8 *h, u32 id, s32 n);
s32 quest_hunter_item_get(Quest *self, u32 id, s32 who);
void quest_send_item(Quest *self, u32 id);
u32 quest_check_deliver(Quest *self, u8 *h);
void spots_init(Quest *self);
void spots_update(Quest *self);
s32 spots_add(Quest *self, GatherSpot *src, f32 *pos);
void spots_remove(Quest *self, s32 idx);
GatherSpot *spots_find(Quest *self, u8 *h);
u32 spots_kind(Quest *self, u8 *h);
u32 spots_gather(Quest *self, u8 *h);
void spots_make_room(Quest *self);
void spots_list_remove_at(Quest *self, s32 i);
void spots_list_remove(Quest *self, s32 idx);
void spots_area_change(Quest *self);
void spots_list_add(Quest *self, u32 idx);
u32 spots_item(Quest *self, u32 id);
void quest_reset_monsters(Quest *self, u32 id);
void quest_check_monsters_done(Quest *self, u32 id);
void quest_set_entry_offset(Quest *self, u32 *tbl, u32 id, u8 *p, s32 which);
void quest_init_monsters(Quest *self);
u32 quest_random_size(Quest *self);
void quest_monster_init(Quest *self, QuestMonster *m, s32 *counter);
u32 monster_is_large(Quest *self, s32 id);
u32 monster_is_large2(Quest *self, s32 id);
void quest_init_main_monsters(Quest *self, QuestGroup *l, s32 *counter);
void quest_init_sub_monsters(Quest *self, u32 *t, s32 *counter);
QuestMonster *quest_find_monster(Quest *self, u8 *h);

/* quest_script.c */
QuestMonster *quest_find_monster_idx(Quest *self, s32 idx, u32 id);
void quest_timers(Quest *self, s32 n);
void quest_nop(Quest *self, s32 a, s32 b, s32 c);
s32 quest_goal_left(Quest *self, s32 id);
s32 quest_goal_left_at(Quest *self, u32 i);
u32 quest_goal_done(Quest *self, s32 i);
void quest_goal_complete(Quest *self, u32 i, u32 how);
void quest_goal_set(Quest *self, s32 i, u32 how);
void quest_goals_clear(Quest *self);
void quest_goals_load(Quest *self);
void quest_goal_add(Quest *self, QuestGoal *src);
void quest_goals_update(Quest *self);
s16 quest_goal_count(Quest *self, s32 id, u32 i);
u32 quest_capture_done(Quest *self, u32 id);
s32 quest_goal_any_done(Quest *self);
s16 quest_goal_have(Quest *self, s32 id);
u32 quest_goals_check(Quest *self, s32 end);
void quest_end_fail(Quest *self, s32 alt);
void quest_end_clear(Quest *self, s32 alt);
s32 quest_script_run(Quest *self, u8 *h, s32 pc_arg);
void quest_set_mon_flags(Quest *self, u8 *h);
s32 quest_points(Quest *self);
void quest_mark_item(Quest *self, u32 id);
void quest_make_rewards(Quest *self);
u32 quest_nop0(Quest *self);
s32 quest_goal_item_ok(Quest *self, QuestCmd *c);
s16 quest_monsters_hunted(Quest *self);
u32 quest_goal_counts(Quest *self, s32 id);
void quest_event(Quest *self, u32 who, QuestEvent *pkt);
void quest_monster_event(Quest *self, QuestEvent *pkt);

/* quest_load.c */
void quest_send_time(Quest *self);
void quest_send_killed(Quest *self, QuestMonster *m, u8 *h);
void quest_send_captured(Quest *self, QuestMonster *m, u8 *h);
u32 quest_table_item(Quest *self, u32 i);
s32 quest_load_file(Quest *self, u32 id, u8 *dst);
void quest_load_init(Quest *self);
void quest_load_wave_models(Quest *self);
void quest_load_area_monsters(Quest *self);
void quest_place_area_monsters(Quest *self, u32 a);
u32 quest_area_has_monster(Quest *self, u32 k);
void quest_leave_area(Quest *self);
void quest_monster_detach(Quest *self, u8 *p);
void quest_spawn_sub_monsters(Quest *self);
void quest_load_main_models(Quest *self);
void quest_load_wave_model(Quest *self, u32 w);
void quest_load_area_models(Quest *self);
void quest_extra_rewards(Quest *self);
u16 quest_table_rand(const u16 *l, u16 *out);
u32 quest_record_slot(Quest *self, s32 id);
void quest_record_apply(Quest *self, u8 *h);
void quest_record_save(Quest *self);
void quest_start_timer2(Quest *self, u8 *h);
s32 quest_waves_left(Quest *self);
void quest_timer2_update(Quest *self);
void quest_pick_size(Quest *self, s8 *p);
s32 quest_size_clamp(Quest *self, s32 a, s32 b);
u32 quest_goal_kind(Quest *self, s32 i);
u16 quest_goal_target(Quest *self, s32 i);
s32 quest_goal_amount(Quest *self, s32 i);
s32 quest_goal_left_i(Quest *self, s32 i);

/* quest_bonus.c */
void quest_slot_set(Quest *self, s32 i, u32 v);
int quest_slot_is_set(Quest *self, s32 i);
void quest_slot_clear(Quest *self, s32 i);
u8 quest_slot_get(Quest *self, s32 i);
void quest_slots_copy(Quest *self, const u8 *s);
s32 quest_slot_find(Quest *self, u32 v);
int quest_deliveries_done(Quest *self);
void quest_monster_size(Quest *self, u8 *h);
void quest_hunt_message(Quest *self, u32 id, s32 left, s32 max);
u32 quest_comrade_allowed(Quest *self);
void quest_count_monsters(Quest *self, u16 *out);
void quest_count_monsters_main(Quest *self, QuestGroup *l, u16 *out);
void quest_count_monsters_sub(Quest *self, u32 *t, u16 *out);
void quest_bonus_clear(Quest *self);
void quest_rewards_clear(void);
void quest_gather_rewards(Quest *self);
void quest_comrade_rewards(Quest *self);
u32 quest_monster_unlocked(Quest *self, u32 id);
u32 quest_is_special(Quest *self, s32 id);
void quest_save_deliveries(void);
u32 quest_any_unlock(Quest *self);
u32 quest_unlock_check(Quest *self, s32 i);

#endif
