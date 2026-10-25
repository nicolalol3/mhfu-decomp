/* The object of the result screens at the end of a quest (g_result). Named offsets, used
 * with U8/S16/U16/U32 like the camera (include/camera.h). (Guessed names.) */
#ifndef RESULT_H
#define RESULT_H

enum {
    RES_SOLD_TOTAL = 0x10,      /* u32 price of the items sold at the end */
    RES_VALUE = 0x14,           /* u32 money / points still to count on the screen */
    RES_VALUE_START = 0x18,     /* u32 */
    RES_HAS_SOLD = 0x1E,        /* u8 */
    RES_RECORD_SHOWN = 0x1F,    /* u8 record shown now */
    RES_X20 = 0x20,
    RES_SOLD_N = 0x24,          /* u32 */
    RES_SOLD = 0x28,            /* u16[60]: {item, count} of the items sold */
    RES_STEP = 0xA4,            /* s16 step of the screen (the lines appear one by one) */
    RES_XA6 = 0xA6,
    RES_STEP_TIMER = 0xA8,      /* s16 frames before the next step */
    RES_TIMER = 0xAA,           /* s16 frames before the screen closes */
    RES_XAC = 0xAC,
    RES_SOUND_ON = 0xAE,        /* s16 the counting sound is playing */
    RES_POINTS_A = 0xB0,        /* u32 points of the hunters (training) */
    RES_POINTS_B = 0xB4,
    RES_XB8 = 0xB8,
    RES_XBC = 0xBC,
    RES_RANK = 0xC0,            /* u8 */
    RES_SND1 = 0xC1,            /* u8 */
    RES_SND2 = 0xC2,
    RES_SND3 = 0xC3,
    RES_SOUND = 0xC8,           /* u32 handle of the counting sound */
    RES_RECORD_N = 0xCC,        /* u32 */
    RES_RECORDS = 0xD0,         /* 10 x {u8 monster, u8 kind, u16 size} new size records */
    RES_MASK_LO = 0xF8,         /* u32 */
    RES_MASK_HI = 0xFC,
    RES_X104 = 0x104,
    RES_X108 = 0x108,
    RES_POINTS_TOTAL = 0x120    /* u32 */
};

#endif
