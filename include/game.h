/* Fields of the general game state (g_game) and of the system object (g_sys)
 * used in many places. (Guessed names.) */
#ifndef GAME_H
#define GAME_H
#include "menu.h"


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
