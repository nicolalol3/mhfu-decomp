/* Fields of the general game state (D_089CC438) and of the system object (D_08A631A0)
 * used in many places. (Guessed names.) */
#ifndef GAME_H
#define GAME_H
#include "menu.h"

#define G_MY_SLOT           D_089CC438[0x28]        /* slot (0-3) of the local player */
#define G_QUEST_STATE       D_089CC438[0x2E]        /* QS_* */
#define G_PLAYER_HERE(i)    D_089CC438[(i) * 0x8C + 0x30]   /* != 0: player i is present */
#define G_REWARDS           (&U16(D_089CC438, 0x390))   /* 32 rewards {item, count} */
#define G_TIMER2_ON         D_089CC438[0x418]       /* second timer running */
#define G_TIME_STOP         D_089CC438[0x422]       /* != 0: the quest timers stop */
#define G_ARENA             D_089CC438[0x480]       /* != 0: arena / fixed start area */
#define G_AREA              U16(D_089CC438, 0x6AF0E)    /* current area of the map */
#define G_MAP               D_089CC438[0x6AF10]     /* map of the quest */
#define G_QUEST_ID          U16(D_089CC438, 0x6AF12)    /* id of the quest (0: none) */
#define GAME_FLAGS          (*(u32 *)(D_089CC438 + 0x6AF14))
#define OPT_FLAGS           (*(u32 *)(D_08A631A0 + 0xF7A57C))   /* bit 0: online */

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
u32 player_get(void *players, u8 slot);         /* D_09A4F0C4: entity of the player slot */
u32 game_rand(void *sys, int which);            /* D_08A631A0: 16-bit random numbers */
void net_send(void *net, int kind, void *data); /* D_09A1DAEC: event to the other players */

#endif
