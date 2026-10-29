/* Named offsets of the sound objects (used with U8/U16/U32/F32, so the compiled code is the
 * same as with the raw offsets). Shared by src/game/sound*.c. (Guessed names.) */
#ifndef SOUND_H
#define SOUND_H

/* the sound bank manager (D_08A62D84) */
enum {
    SB_MEM = 0x0,               /* u32 memory of the sound data */
    SB_MEM_SIZE = 0x4,          /* u32 */
    SB_SELECTED = 0x8,          /* s32 bank set selected (-1: none) */
    SB_MODE = 0x10,             /* s32 output mode */
    SB_PAUSED = 0x14,           /* u8 */
    /* the channels: 0x14 bytes each, at SLOT(self, i) */
    SB_CH_DATA = 0x18,          /* u32 data of the channel */
    SB_CH_SIZE = 0x1C,          /* u32 */
    SB_CH_ADDR = 0x20,          /* u32 address in the sound memory */
    SB_CH_A = 0x24,             /* u16 sound loaded (0xFFFF: none) */
    SB_CH_B = 0x26,             /* u16 */
    SB_CH_C = 0x28,             /* u16 */
    SB_CH_FLAGS = 0x2A,         /* u16 bit 0: playing */
    SB_PARAMS_SET = 0x158,      /* u8 */
    SB_PARAMS = 0x15C,          /* u32[6] output parameters given to the library */
    SB_PARAM_B = 0x160,
    SB_PARAM_C = 0x164,
    SB_VOLUME = 0x168,          /* the 4th parameter */
    SB_PARAM_E = 0x16C,
    SB_PARAM_F = 0x170,
    SB_FADE_FROM = 0x174,       /* s16 volume fade */
    SB_FADE_TO = 0x176,         /* s16 */
    SB_FADE_LEN = 0x178,        /* s16 frames */
    SB_FADE_LEFT = 0x17A,       /* s16 frames left */
};

/* the sound system g_sound */
enum {
    SND_RQ_ARG = 0x8B2C,        /* arguments of the request: change {a, c, b, ch} or load
                                   {a[4], b[4], k, ch, n} or music {id, ..} */
    SND_RQ_C = 0x8B2E,          /* s16 (change) */
    SND_RQ_B = 0x8B30,          /* s16 (change) */
    SND_RQ_CH = 0x8B32,         /* s16 channel (change) */
    SND_RQ_LOAD_B = 0x8B34,     /* u16[4] (load) */
    SND_RQ_LOAD_K = 0x8B3C,     /* u16 (load) */
    SND_RQ_LOAD_CH = 0x8B3E,    /* s16 channel (load) */
    SND_RQ_LOAD_N = 0x8B40,     /* u32 pairs (load) */
    SND_RQ_LOAD_DST = 0x8B44,   /* u32[] (load) */
    SND_RQ_MUSIC = 0x8B54,      /* u16[] (music 3) */
    SND_CH_A = 0x8B64,          /* u16[4] sounds asked for each channel */
    SND_CH_C = 0x8B6C,          /* u16[4] */
    SND_CH_B = 0x8B74,          /* u16[4] */
    SND_CH_NUM = 0x8B7C,        /* s16[4] */
    SND_CH_EXTRA = 0x8B84,      /* u8[4] */
    SND_MON_A = 0x8B88,         /* s32 sounds of the monsters */
    SND_MON_B = 0x8B8C,         /* s32 */
    SND_RQ_ONE = 0x8B90,        /* u32 (one) */
    SND_REQ_KIND = 0x8B94,      /* u32 request in progress (0: none) */
    SND_REQ_STEP = 0x8B98,      /* u32 */
    SND_MUSIC_SEL = 0x308BC0,   /* s32 music set selected */
    SND_DATA = 0x308BC4,        /* u32 sound data */
    SND_VOL_A = 0x5623D0,       /* u32 volume of the effects */
    SND_VOL_B = 0x5623D4,       /* u32 volume of the voices */
    SND_LISTENER = 0x5623F0,    /* f32[3] position of the listener */
    SND_PRX_THREAD = 0x562424,  /* thread that loads the modules */
    SND_PRX_MOD = 0x562428,     /* u32[3] modules 0x300-0x302 */
    SND_MUSIC_FLAG = 0x562434,  /* u32 */
    SND_PITCH = 0x562438,       /* u32 pitch of the next 3D sound */
    SND_LOADED_A = 0x56243C,    /* u16[4] sounds loaded on each channel */
    SND_LOADED_C = 0x562444,    /* u16[4] */
    SND_LOADED_B = 0x56244C,    /* u16[4] */
    SND_LOADED_CH = 0x562454,   /* s16[4] */
    SND_LOADED_EXTRA = 0x56245C, /* u8[4] */
    SND_DIST = 0x562460,        /* f32 distance of the last 3D sound */
    SND_RANGE = 0x56246C,       /* f32 */
};

/* the music controller (D_08A62D90) */
enum {
    MC_TIMER = 0x0,             /* s32 frames left before the next change */
    MC_AREA_KIND = 0x4,         /* s32 kind of the area (>= 4: no music change) */
    MC_MUSIC = 0x8,             /* u16 music of the area (0xFFFF: none) */
    MC_EVENT = 0xA,             /* u16 music of the event / jingle */
    MC_NEXT = 0xC,              /* u16 */
    MC_VOLUME = 0x10,           /* u32 volume of the area music */
    MC_FLAGS = 0x14,            /* u32 bit 1: jingle playing, bit 8 */
    MC_X18 = 0x18,              /* u8[3] */
    MC_X1C = 0x1C,              /* u16[3] */
    MC_X24 = 0x24,              /* u32 */
    MC_X2A = 0x2A,              /* u16 */
};

/* the music player (D_08A62D94): 2 stream players of 0x18 bytes, then 21 entries of the
 * stream data (0x10 bytes each, offsets from self + i * 0x10) */
enum {
    MP_STREAM = 0x0,            /* u32 handle of the stream (0: none) */
    MP_ID = 0x4,                /* s32 entry played */
    MP_STATE = 0x8,             /* s32 state of the stream (3: playing) */
    MP_VOLUME = 0xC,            /* u16 */
    MP_XE = 0xE,                /* u16 */
    MP_FADE_FROM = 0x10,        /* s16 */
    MP_FADE_TO = 0x12,          /* s16 */
    MP_FADE_LEN = 0x14,         /* s16 */
    MP_FADE_LEFT = 0x16,        /* s16 */
    MP_X18 = 0x18,              /* u32 (of the object) */
    ME_ADDR = 0x30,             /* u32 data of the entry */
    ME_SIZE = 0x34,             /* u32 */
    ME_LOADED = 0x38,           /* u32 size loaded (0: none) */
    ME_ID = 0x3C,               /* u16 music loaded (0xFFFF: none) */
};

#endif
