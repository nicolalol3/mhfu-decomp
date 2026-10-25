/* Fields of the general game state (g_game) and of the system object (g_sys)
 * used in many places. (Guessed names.) */
#ifndef GAME_H
#define GAME_H
#include "menu.h"

#define G_MY_SLOT           g_game[0x28]        /* slot (0-3) of the local player */
#define G_QUEST_STATE       g_game[0x2E]        /* QS_* */
#define G_PLAYER_HERE(i)    g_game[(i) * 0x8C + 0x30]   /* != 0: player i is present */
#define G_REWARDS           (&U16(g_game, 0x390))   /* 32 rewards {item, count} */
#define G_TIMER2_ON         g_game[0x418]       /* second timer running */
#define G_TIME_STOP         g_game[0x422]       /* != 0: the quest timers stop */
#define G_ARENA             g_game[0x480]       /* != 0: arena / fixed start area */
#define G_AREA              U16(g_game, 0x6AF0E)    /* current area of the map */
#define G_MAP               g_game[0x6AF10]     /* map of the quest */
#define G_QUEST_ID          U16(g_game, 0x6AF12)    /* id of the quest (0: none) */
#define GAME_FLAGS          (*(u32 *)(g_game + 0x6AF14))
#define OPT_FLAGS           (*(u32 *)(g_sys + 0xF7A57C))   /* bit 0: online */

/* states of the quest (G_QUEST_STATE) */
enum {
    QS_NONE = 0,
    QS_RUNNING = 2,
    QS_CLEARING = 3,        /* goals done, end of the quest in progress */
    QS_CLEARED = 4,
    QS_FAILED = 5,
    QS_RETURN = 6,          /* back to the village (command 0x1F) */
    QS_ABANDONED = 7        /* quest_fail */
};

/* functions of the game not decompiled yet (names from what they do) */
u32 player_get(void *players, u8 slot);         /* g_players: entity of the player slot */
u32 game_rand(void *sys, int which);            /* g_sys: 16-bit random numbers */
void net_send(void *net, int kind, void *data); /* g_net: event to the other players */

#endif
