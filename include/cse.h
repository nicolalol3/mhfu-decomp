/* Named offsets of the objects of the Capcom Sound Engine (used with U8/U16/U32 and [], so
 * the compiled code is the same as with the raw offsets). Shared by src/game/cse*.c and
 * src/game/sas_voice.c. (Guessed names.) */
#ifndef CSE_H
#define CSE_H

/* a voice of the engine (32 voices of 0x48 bytes) */
enum {
    CV_STATE = 0x0,             /* u32 1 starting, 2 playing, 3 to stop, 5 stopping, 7 ended */
    CV_AGE = 0x4,               /* u32 frames since the start */
    CV_LENGTH = 0x8,            /* u32 */
    CV_KEY = 0xC,               /* u8 */
    CV_SAS = 0x10,              /* u32 the sceSas voice (SV_*) */
    CV_ID = 0x14,               /* u32 id of the sound; 0x14-0x1F: the conditions of the voice */
    CV_FLAGS = 0x18,            /* u8 */
    CV_FLAGS2 = 0x19,           /* u8 */
    CV_X1A = 0x1A,              /* u8 */
    CV_PRIORITY = 0x1B,         /* u8 */
    CV_CHANNEL = 0x1C,          /* u8 (bit 7: global) */
    CV_X1D = 0x1D,              /* u8 */
    CV_ID1 = 0x1E,              /* u8 (0xFF: none) */
    CV_ID2 = 0x1F,              /* u8 (0xFF: none) */
    CV_PAN = 0x20,              /* u8 */
    CV_PAN2 = 0x21,             /* u8 */
    CV_VOL = 0x22,              /* u16 (0-0xFFF) */
    CV_VOL2 = 0x24,             /* u16 (0-0xFFF) */
    CV_PITCH = 0x26,            /* s16 */
    CV_PITCH2 = 0x28,           /* s16 */
    CV_X2A = 0x2A,              /* u8 */
    CV_BEND = 0x2C,             /* s16 pitch bend */
    CV_BEND_UP = 0x2E,          /* u16 range of the bend */
    CV_BEND_DOWN = 0x30,        /* u16 */
    CV_ENV = 0x32,              /* u16 envelope */
    CV_RATE = 0x34,             /* u32 */
    CV_LFO1 = 0x38,             /* the 2 LFOs {u32 phase, u16 speed, u16 depth} */
    CV_LFO1_SPEED = 0x3C,
    CV_LFO1_DEPTH = 0x3E,
    CV_LFO2 = 0x40,
    CV_LFO2_SPEED = 0x44,
    CV_LFO2_DEPTH = 0x46,
};

/* a voice of the sceSas driver (0x2C bytes from 0x09A0E3CC) */
enum {
    SV_FRAME = 0x0,             /* u32 frame of the key on */
    SV_STATE = 0x4,             /* u16 */
    SV_NUM = 0x6,               /* s16 number of the sceSas voice */
    SV_MASK = 0x8,              /* u32 1 << number */
    SV_FLAGS = 0xC,             /* u32 changes to send to sceSas */
    SV_VOL_L = 0x10,            /* s16 */
    SV_VOL_R = 0x12,            /* s16 */
    SV_PITCH = 0x14,            /* u16 (0x1000: original pitch) */
    SV_ADSR1 = 0x16,            /* u16 */
    SV_ADSR2 = 0x18,            /* u16 */
    SV_ENV_A = 0x20,            /* u16 */
    SV_ENV_B = 0x22,            /* u16 */
    SV_DATA = 0x24,             /* u32 address of the VAG data */
    SV_SIZE = 0x28,             /* u32 */
};

/* an ATRAC3 stream player (4 players of 0x204 bytes at 0x09A0D260) */
enum {
    AT_USED = 0x0,              /* u32 */
    AT_THREAD = 0x4,            /* u32 decode thread */
    AT_ID = 0x8,                /* s32 sceAtrac id (-1: none) */
    AT_DECODED = 0xC,           /* u32 results of the last decode */
    AT_END = 0x10,              /* u32 */
    AT_LEFT = 0x14,             /* u32 */
    AT_DATA_SET = 0x1C,         /* u32 */
    AT_CHANNEL = 0x24,          /* u32 audio channel */
    AT_BUF_SEL = 0x28,          /* u32 buffer being filled (0/1) */
    AT_MODE = 0x2C,             /* u32 */
    AT_LOOP_START = 0x34,       /* s32 (-1: none) */
    AT_LOOP_END = 0x38,         /* s32 (-1: none) */
    AT_X3C = 0x3C,              /* u32 */
    AT_X40 = 0x40,              /* u32 */
    AT_LOOP = 0x44,             /* u32 */
    AT_LOOP_ON = 0x48,          /* u32 */
    AT_LOOP_N = 0x4C,           /* u32 */
    AT_X50 = 0x50,              /* u32 */
    AT_STATE = 0x54,            /* u32 0 idle, 2 set, 3 playing, 4 ending, 5 released, 6 error */
    AT_ERR_WHY = 0x58,          /* u32 */
    AT_ERR_CODE = 0x5C,         /* u32 */
    AT_PAUSED = 0x60,           /* u32 */
    AT_WAKE = 0x64,             /* u32 */
    AT_VOLUME = 0x6C,           /* u32 (0-0x7F) */
    AT_VOLUME_OUT = 0x70,       /* u32 (0-0x8000) */
    AT_PAN = 0x74,              /* u32 */
    AT_FLAGS = 0x78,            /* u32 */
    AT_SAMPLES = 0x7C,          /* u32 */
    AT_X84 = 0x84,              /* u32 */
    AT_OUT_LEN = 0x88,          /* u32 samples of the output */
    AT_X8C = 0x8C,              /* u32 */
    AT_SIZE = 0x90,             /* u32 size of the data */
    AT_BUF_SIZE = 0x94,         /* u32 */
    AT_BUF = 0x98,              /* u32[2] the output buffers */
    AT_DATA = 0xA0,             /* u32 the data */
    AT_QUEUE_N = 0xA4,          /* u32 requests in the queue */
    AT_QUEUE_POS = 0xA8,        /* u32 */
    AT_QUEUE = 0xB0,            /* 16 requests {u32, u32} */
    AT_X130 = 0x130,            /* u32[4] */
    AT_LIMITER = 0x150,         /* f32[] parameters of the limiter */
    AT_LIM_GAIN = 0x17C,        /* f32 */
    AT_LIM_A = 0x188,
    AT_LIM_B = 0x18C,
    AT_LIM_C = 0x190,
    AT_LIM_D = 0x194,
    AT_LIM_E = 0x198,
    AT_LIM_F = 0x19C,
    AT_LIM_G = 0x1A0,
    AT_LIM_STATE1 = 0x1A4,      /* f32[8] */
    AT_LIM_STATE2 = 0x1C4,      /* f32[8] */
    AT_LIM_STATE3 = 0x1E4,      /* f32[] */
};

#endif
