/* The camera (object g_camera, 0xDC8 bytes). The camera functions use VFPU instructions that
 * the test bench cannot run, so the fields are named offsets (used with U8/S16/U32/F32)
 * instead of a structure: this way the compiled code stays exactly the same.
 * (Guessed names.) */
#ifndef CAMERA_H
#define CAMERA_H

/* The camera starts with 6 "views" of 0x1A0 bytes (view 0 = the main one, 3 = close to
 * the player, 4 = special, 5 = cutscene); CAM_VIEW(cam, k) + VIEW_* gives a field of view k. */
#define CAM_VIEW(cam, k)    ((u8 *)(cam) + (k) * 0x1A0)
enum {
    VIEW_EYE = 0xB0,            /* f32[4] position of the eye */
    VIEW_TARGET = 0xC0,         /* f32[4] point looked at */
    VIEW_X130 = 0x130,
    VIEW_FOV = 0x138,           /* f32 field of view */
    VIEW_ACTIVE = 0x141         /* u8 */
};

/* shakes: 3 slots of 0x20 bytes from CAM_SHAKE */
enum {
    SHAKE_POS = 0x00,           /* f32[4] source of the shake */
    SHAKE_ACTIVE = 0x10,        /* u8 */
    SHAKE_TYPE = 0x11,          /* u8 bit 7: no fall-off with the distance */
    SHAKE_TIMER = 0x12,         /* s16 */
    SHAKE_RANGE = 0x14,         /* u8 index of the radius */
    SHAKE_SIZE = 0x20
};

enum {
    CAM_NEAR = 0x000,           /* f32 near plane */
    CAM_FAR = 0x004,            /* f32 far plane */
    CAM_ASPECT = 0x008,         /* f32 width / height */
    CAM_FOV = 0x00C,            /* f32 field of view (radians) */
    CAM_FOV_APPLIED = 0x010,
    CAM_X014 = 0x014,
    CAM_EYE_WANT = 0x020,       /* f32[4] where the eye is moving to */
    CAM_TARGET_WANT = 0x030,    /* f32[4] where the target is moving to */
    CAM_X0A0 = 0x0A0,
    CAM_FOV_WANT = 0x0A8,
    CAM_X142 = 0x142,
    CAM_VIEW3_FOV = 0x618,      /* = view 3 + VIEW_FOV */
    CAM_VIEW3_ACTIVE = 0x621,
    CAM_ZOOM_BASE = 0x680,
    CAM_ZOOM_SCALE = 0x688,
    CAM_ZOOM_STATE = 0x698,
    CAM_ZOOM_A = 0x699,
    CAM_ZOOM_ON = 0x69A,
    CAM_VIEW4_ACTIVE = 0x7C1,
    CAM_TRANS = 0x7E0,          /* transition between points: u8 state */
    CAM_TRANS_STEP = 0x810,     /* u8 (also: kinds of the points, from +count) */
    CAM_TRANS_TIME = 0x814,     /* s16 */
    CAM_TRANS_TIME2 = 0x816,    /* s16 */
    CAM_TRANS_COUNT = 0x834,    /* u32 points left */
    CAM_CUTSCENE_ON = 0x961,    /* = view 5 + VIEW_ACTIVE */
    CAM_CUTSCENE_TIMER = 0x964, /* s16 */
    CAM_X968 = 0x968,
    CAM_CUTSCENE_ID = 0x9BE,    /* u8 */
    CAM_CUTSCENE_STATE = 0x9BF, /* s8 */
    CAM_X9C0 = 0x9C0,
    CAM_CUTSCENE_FLAG = 0xA0A,
    CAM_XA0B = 0xA0B,
    CAM_STICK_X = 0xA70,        /* s16 input that moves the camera */
    CAM_PAD = 0xA72,
    CAM_XA74 = 0xA74,
    CAM_STICK2_X = 0xA76,
    CAM_STICK2_Y = 0xA78,
    CAM_MODE_ARG = 0xA7A,       /* u8 */
    CAM_MODE_TARGET = 0xA7C,    /* u32 */
    CAM_MODE = 0xA80,           /* u8 special mode (camera_set_mode) */
    CAM_XA81 = 0xA81,
    CAM_PLAYER = 0xA84,         /* u8 * entity of the local player */
    CAM_XA88 = 0xA88,
    CAM_XA8A = 0xA8A,
    CAM_VIEW_SEL = 0xA8D,       /* u8 view used for the wanted eye / target */
    CAM_ZONE = 0xA90,           /* u32 camera zone where the player is */
    CAM_XAA9 = 0xAA9,
    CAM_CULLING = 0xAAA,        /* u8 0: no visibility tests */
    CAM_AREA = 0xAAC,           /* u8 * camera data of the map area */
    CAM_SHAKE = 0xAB0,          /* 3 shakes (SHAKE_*) */
    CAM_XB10 = 0xB10,
    CAM_ZONE_LOCK = 0xB14,
    CAM_GRID_X = 0xB16,         /* u16 cell of the zone grid */
    CAM_GRID_Z = 0xB18,
    CAM_LIMITS = 0xB1C,         /* s16[4] then u32[4] (from the area data) */
    CAM_ROLL = 0xB34,
    CAM_XB3C = 0xB3C,
    CAM_EYE = 0xB40,            /* f32[4] current eye */
    CAM_TARGET = 0xB50,         /* f32[4] current target */
    CAM_UP = 0xB60,             /* f32[4] */
    CAM_SCREEN_SHIFT = 0xB70,   /* f32[2] */
    CAM_VIEW_MTX = 0xB80,       /* f32[16] view matrix */
    CAM_VIEW_MTX_S = 0xBC0,     /* f32[16] view matrix with the global scale */
    CAM_PROJ_MTX = 0xC00,       /* f32[16] projection */
    CAM_INV_VIEW_S = 0xC40,
    CAM_INV_VIEW = 0xC80,
    CAM_BILLBOARD = 0xCC0,      /* f32[16] rotation towards the camera */
    CAM_INV_PROJ = 0xD00,
    CAM_PLANES = 0xD50,         /* f32[4][4] side planes of the view */
    CAM_DEPTH_NEAR = 0xD90,
    CAM_DEPTH_FAR = 0xD94,
    CAM_SCREEN_SCALE = 0xDA0,   /* f32[3] */
    CAM_SCREEN_CENTER = 0xDAC,  /* f32[3] */
    CAM_DISTANCE = 0xDB8,       /* f32 eye - target */
    CAM_XDC0 = 0xDC0,
    CAM_ACTIVE_VIEW = 0xDC4     /* s8 view used this frame (-1: the main one) */
};

#endif
