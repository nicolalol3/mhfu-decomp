/* Named offsets of the font object g_font (used with U8/U16/U32 and F(), so the compiled
 * code is the same as with the raw offsets). Shared by src/game/font*.c. (Guessed names.) */
#ifndef FONT_H
#define FONT_H

enum {
    FONT_SYS = 0x4,             /* u32 the system font is open */
    FONT_HANDLE = 0x8,          /* u32 sceFont handle */
    FONT_PIXELS = 0x14,         /* u32 pixels of the glyph being drawn */
    FONT_SYS_W = 0x5C,          /* u16 size of the system glyphs */
    FONT_SYS_H = 0x5E,          /* u16 */
    FONT_GLYPH_W = 0x114,       /* u8 size of a glyph (even) */
    FONT_GLYPH_H = 0x115,       /* u8 */
    FONT_PAD_X = 0x116,         /* u8 space around the glyph */
    FONT_PAD_Y = 0x117,         /* u8 */
    FONT_CELL_W = 0x118,        /* u8 cell of the glyph cache */
    FONT_CELL_H = 0x119,        /* u8 */
    FONT_COLS = 0x11A,          /* u8 cells in a row of a 256x256 texture */
    FONT_ROWS = 0x11B,          /* u8 */
    FONT_CELLS = 0x11C,         /* u16 cells of a texture */
    FONT_CELLS_ALL = 0x11E,     /* u16 cells of the 3 textures */
    FONT_X = 0x120,             /* s16 position of the next text */
    FONT_Y = 0x122,             /* s16 */
    FONT_Z = 0x124,             /* u16 depth */
    FONT_DRAW_X = 0x126,        /* s16 position of the entry being drawn */
    FONT_DRAW_Y = 0x128,        /* s16 */
    FONT_DRAW_Z = 0x12A,        /* u16 */
    FONT_SIZE_W = 0x12C,        /* u8 size of the characters */
    FONT_SIZE_H = 0x12D,        /* u8 */
    FONT_COLOR = 0x12E,         /* u8 color index */
    FONT_LAYER = 0x12F,         /* s8 layer of the next texts (0-5) */
    FONT_FIXED = 0x130,         /* u8 != 0: fixed width */
    FONT_TAGS = 0x131,          /* s8 */
    FONT_LAYER_N = 0x134,       /* u8[6] texts in each layer */
    FONT_ICON_N = 0x13A,        /* u8[6] icons in each layer */
    FONT_LAYER_BUF = 0x140,     /* u32[6] texts of each layer (0x600 bytes each at 0x228) */
    FONT_CACHE_NEXT = 0x158,    /* u16 next slot of the glyph cache */
    FONT_CACHE_N = 0x15A,       /* u16 */
    FONT_ON = 0x15C,            /* u8 */
    FONT_DRAW_LAYER = 0x15D,    /* s8 layer being drawn */
    FONT_BLINK = 0x15E,         /* u8 frame counter of the blinking colors */
    FONT_LINE_H = 0x15F,        /* u8 height of a line */
    FONT_TEXT_POS = 0x160,      /* u16 used part of the text buffer */
    FONT_CACHE_OLD = 0x162,     /* s16 */
    FONT_PALETTE = 0x164,       /* u32 default color */
    FONT_COLORS = 0x168,        /* u32[] colors */
    FONT_LAYERS = 0x228,        /* the 6 layers of 0x600 bytes */
    FONT_SLOT_OF = 0x2628,      /* u16[] cache slot of each character (0xFFFF: none) */
    FONT_SLOT_CHAR = 0x22608,   /* u16[] character of each slot */
    FONT_SLOT_AGE = 0x22968,    /* u8[] */
    FONT_ICONS = 0x22B18,       /* icons {x, y, id} */
    FONT_TEXT = 0x22CF8,        /* the text buffer */
};

#endif
