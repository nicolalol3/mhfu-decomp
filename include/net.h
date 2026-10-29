/* Named offsets of the network objects (used with U8/U16/U32/F32 and [], so the compiled
 * code is the same as with the raw offsets). (Guessed names.) */
#ifndef NET_H
#define NET_H

/* the session of the data exchange (D_09A1D5D0, src/game/netsync.c), 0x158 bytes */
enum {
    NS_TICKS = 0x0,             /* s16 seconds of the start */
    NS_COUNT_A = 0x2,           /* s16 */
    NS_COUNT_B = 0x4,           /* s16 */
    NS_SOCK = 0x8,              /* s32[4] stream socket of each player */
    NS_BCAST = 0x18,            /* s32 broadcast socket */
    NS_ACTIVE = 0x1C,           /* u8 */
    NS_MODE = 0x20,             /* u8 1: ad hoc, 2: infrastructure */
    NS_STATE = 0x21,            /* u8 state of ns_update */
    NS_PLAYERS = 0x22,          /* u8 */
    NS_OWN = 0x23,              /* u8 index of this player */
    NS_ADDR = 0x24,             /* u8[4][0x10] addresses */
    NS_READY = 0x64,            /* u32 bit i: the player i said it is ready */
    NS_PSTATE = 0x68,           /* u8[4] state of each player (1 start .. 6 gone) */
    NS_FULL_TIME = 0x6C,        /* f32 last packet with the player states */
    NS_LAST = 0x70,             /* s64 system time of the last frame */
    NS_TIME = 0x78,             /* f32 seconds since the start */
    NS_SECOND = 0x7C,           /* f32 time of the last second */
    NS_MINUTE = 0x80,           /* f32 time of the last clear of the counters */
    NS_SENT = 0x8C,             /* u32 bytes sent in this second */
    NS_RECV = 0x90,             /* u32 bytes received in this second */
    NS_SENT_LAST = 0x94,        /* u32 */
    NS_RECV_LAST = 0x98,        /* u32 */
    NS_SENT_ALL = 0x9C,         /* u32 */
    NS_RECV_ALL = 0xA0,         /* u32 */
    NS_BCAST_SENT = 0xA4,       /* u32 */
    NS_BCAST_RECV = 0xA8,       /* u32 */
    NS_PACKETS = 0xAC,          /* u32[4] packets received from each player */
    NS_STREAM_LOST = 0xBC,      /* u32[4] */
    NS_INTERVAL = 0xCC,         /* f32 time between two sends */
    NS_SEND_TIME = 0xD0,        /* f32 last send */
    NS_MAX_BYTES = 0xD4,        /* s32 bytes per second */
    NS_STAT = 0xD8,             /* u16 state of a socket (pdp_status) */
    NS_STAT_REFRESH = 0xDA,     /* u16 */
    NS_STAT_FREE = 0xDC,        /* u16 free space to send */
    NS_HEARD = 0xE0,            /* f32[4] last packet of each player */
    NS_WAIT_TIME = 0xF0,        /* f32[4] start of the wait of each player */
    NS_FREE_LAST = 0x100,       /* u16[4] */
    NS_FULL_COUNT = 0x108,      /* u16[4] seconds with the stream full */
    NS_SEQ_SEEN = 0x110,        /* u8[4] */
    NS_SEQ = 0x114,             /* u8[4] sequence of each player */
    NS_STAMP = 0x118,           /* u16[4] */
    NS_ERR_RECV = 0x120,        /* s32 */
    NS_ERR_SEND = 0x124,        /* s32 */
    NS_ERR_X = 0x128,           /* s32 */
    NS_ERR_VALUE = 0x12C,       /* u32 */
    NS_SILENCE = 0x130,         /* f32 silence before a player is dropped */
    NS_TIMEOUT = 0x134,         /* f32 */
    NS_WAIT_SECS = 0x138,       /* u8[4] wait of each player */
    NS_FULL_MAX = 0x13C,        /* s32 seconds with the stream full before the drop */
    NS_CB_MSG = 0x140,          /* callback of the game messages and its argument */
    NS_CB_MSG_ARG = 0x144,
    NS_CB_JOIN = 0x148,         /* callback of a player that joined */
    NS_CB_JOIN_ARG = 0x14C,
    NS_CB_LEFT = 0x150,         /* callback of a player that left */
    NS_CB_LEFT_ARG = 0x154,
};

/* a slot of the lobby players (4 slots of 200 bytes from D_09A1FDE0) */
enum {
    LP_ENTRY = 0xB8,            /* the entry of the player: id (MAC) [8], name, info */
    LP_INFO = 0xD8,             /* info of the player (0xA4 bytes) */
    LP_SEEN = 0x17E,            /* u8 a packet was received */
    LP_SILENT = 0x17F,          /* u8 no packet since the last check */
};

/* a task of the lobby (scan, connection, join, choice of a slot) */
enum {
    LT_COUNTER = 0xC,           /* s32 frames to wait */
    LT_ACTIVE = 0x20,           /* u8 */
    LT_STATE = 0x21,            /* u8 0 running, 1 done, 2 failed */
    LT_STEP = 0x22,             /* u8 */
    LT_ANSWER = 0x23,           /* u8 1 yes, 2 no */
    LT_ERROR = 0x26,            /* u16 error code */
};

#endif
