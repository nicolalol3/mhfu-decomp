/* Shared declarations of the menu code (src/game/menu_*.c). */
#ifndef MENU_H
#define MENU_H
#include "libc.h"

#define F32(p, off) (*(f32 *)((u8 *)(p) + (off)))
#define U32(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define S16(p, off) (*(s16 *)((u8 *)(p) + (off)))
#define U16(p, off) (*(u16 *)((u8 *)(p) + (off)))
#define U8(p, off)  (*(u8 *)((u8 *)(p) + (off)))
#define S8(p, off)  (*(s8 *)((u8 *)(p) + (off)))

/* global game data */
extern u8 *D_089CC438;            /* general game state */
extern void *D_09A4F0C4;
extern void *D_089CBBEC;          /* menu instance */
extern void *D_08A62C7C;
extern void *D_08A631A8;          /* on-screen text writer */
extern u8 *D_08A631A0;
extern void *D_08A62C50;
extern const u32 D_08938A7C[];
extern const f32 D_0893887C[][2];
extern const u16 D_089AC834[];    /* IDs of the menu texts */
extern const u16 D_089AC588[];
extern const char D_08938478[];   /* "%s" */
extern const char D_08938540[];   /* formato "%d/%d" */
extern const char D_08938548[];
extern const s8 D_0893AF2C[];
extern const u8 D_0899D19B[];     /* item table (0x18 bytes per item) */
extern u8 D_0893AFDC[], D_0893AFF0[], D_0893B004[], D_0893B0E0[];
extern u8 D_089AC5D8[];

/* text positions: x, y, line spacing */
extern s16 D_08938488, D_0893848A; extern s32 D_0893848C;
extern s16 D_0893849C, D_0893849E; extern s32 D_089384A0;
extern s16 D_089384D8, D_089384DA;
extern s16 D_089384EC; extern s32 D_089384F0;
extern s16 D_089384A4, D_089384A8, D_089384B0, D_089384B2;
extern u8 D_089384AC; extern s32 D_089384B4;   /* larghezza carattere (byte) */
extern s16 D_089384B8, D_089384C0;
extern s16 D_0893B164, D_0893B166; extern s32 D_0893B168;
extern const s16 D_089384F4[7], D_08938504[7][2], D_08938520[7], D_08938530[7];
extern u8 D_0893847C[], D_08938490[], D_089384CC[], D_089384E0[];

void func_0890BEC0(void *arr, void *ctor, void *dtor, int size, int count);
void func_0890BE54(void *arr, void *dtor, int size, int count);
void func_0890BFA4(void *p);
void *memset(void *p, int c, size_t n);
void func_08832838(void *self);
#define menu_free_data func_08832838
u32 player_get(void *a, u8 b);
void func_0883D274(void *self);
#define hud_notice_clear func_0883D274
u32 func_0886A35C(void *a, int b);
#define quest_time func_0886A35C
void func_08829674(void *self);
#define menu_get_mode func_08829674
void func_088306DC(void *self);
#define menu_reset_messages func_088306DC
void func_0882FA24(void *layout, const void *src, int a, int b, int c);
#define layout_build func_0882FA24
int func_08829928(void *self);
#define menu_item_giftable func_08829928
u16 *func_08856884(void *game, int idx);
#define pouch_slot func_08856884
int func_08829A48(void *self, u16 id);
#define menu_item_for_quest func_08829A48
void func_0882F580(void *self, void *layout, int a, int b, int alpha, int c);
void func_08832E34(void *self, int x, int y, int a, int b, int c);
#define gfx_draw_count_box func_08832E34
void func_0882F9A8(void *self, const void *layout, int a, int b, int alpha, int c);
#define layout_draw_from func_0882F9A8
void func_08829B58(void *self, int a);
#define menu_draw_give func_08829B58
void func_08890DE8(void *font, int w, int h);
void func_08890E38(void *font, int color);
const char *func_0885151C(void *game, u16 id);
#define text_menu func_0885151C
const char *func_08851528(void *game, u32 id);
#define text_item_name func_08851528
void func_088920A8(void *font, int x, int y, const char *fmt, ...);
void func_08891474(void *font, int x, int y, const char *fmt, ...);
void func_088915B0(void *font, int x, int y, int color, const char *fmt, ...);
void menu_draw_item_list(void *self, int sel, u32 color, u32 flags, int d);
void func_0882F4A4(void *self, int x, int y, int w, int h, u32 flags);
#define gfx_fill_rect_pulse func_0882F4A4
void func_0882FC50(void *self, const void *layout, const char *text, int alpha);
#define layout_draw func_0882FC50
int func_088286E4(void *self, int idx, u16 id);
#define menu_slot_has_item func_088286E4
int func_0885B5B8(void *a, int b, int c, int d, int e, int f);
#define combo_find func_0885B5B8
s32 func_0885BE08(void *a, u32 b, int c);
#define combo_rate_of func_0885BE08
u32 func_0885C718(int a);
#define combo_rate_class func_0885C718
const char *func_08825104(void *self, u32 a);
#define menu_item_category func_08825104
s32 func_08828F3C(void *self, u16 *entry);
#define menu_combo_room func_08828F3C
u32 func_088568AC(void *game, s16 id);
#define pouch_count func_088568AC

#define LANG (*(s32 *)(D_08A631A0 + 0xF7A574))   /* lingua corrente (0..6) */

/* other menu functions and data (src/game/menu_draw.c) */
typedef struct Layout {     /* text box (0x28 bytes) */
    s16 x, y, w, h;         /* 0x00 */
    u8 cw, b9, ba, bb;      /* 0x08 larghezza carattere */
    s16 tx, ty, lh, e;      /* 0x0C text position, line spacing */
    void *ptr;              /* 0x14 */
    u32 color;              /* 0x18 */
    s16 f[6];               /* 0x1C */
} Layout;

int sprintf(char *buf, const char *fmt, ...);
extern void *D_09A44C5C, *D_09B22D70;
extern const Layout D_0893B040;
extern const u8 D_0893B0F4[20];
extern u8 D_0893B108[], D_0893B11C[], D_0893B130[], D_0893AF8C[], D_0893AFB4[], D_0893B068[];
extern u8 D_0893B144[], D_0893B158[], D_0893B16C[], D_0893B180[], D_0893B194[], D_0893B1A8[];
extern const char D_0893854C[];
extern const u32 D_0893AA94[], D_0893AAB0[];
extern u8 D_089AC600[], D_089AC7CC[];
extern const u16 D_089AC7C4[], D_089ACC04[], D_089ACC44[], D_089ACC70[], D_089B54E0[], D_089B5558[];
extern const u16 *const D_0893AF34[];
extern const s16 D_0893AF54[4][2];
extern const u8 D_089779D4[], D_089779DC[];
extern const u8 D_0899D198[];        /* item table (0x18 bytes per item) */
extern const u16 D_0899D1AE[];

int func_08857C38(void *game);
#define friend_marked func_08857C38
void func_0882DCB0(void *self, int x, int y, int w, int h, int alpha, int c);
#define gfx_draw_frame func_0882DCB0
void func_08833290(void *self, int x, int y, int a, int b);
void menu_draw_info_card(void *self);
void menu_draw_trade_info(void *self);
void menu_draw_status(void *self, u8 *hunter, s32 page);
void menu_draw_equipment(void *self, u8 *hunter, int col);
u8 *menu_get_equip_slot(void *self, u8 *out, u32 slot, u8 *hunter);
void func_08830078(void *self, u32 a, int b, int c, int d, int e, int f);
#define equip_draw_name_box func_08830078
int func_08890E48(void *font, const char *s);
void func_0882A9DC(void *self, u16 x, u16 y, int id);
#define menu_draw_sprite func_0882A9DC
void func_0882FB38(void *self, void *layout, u16 a, u16 b, int c, int d);
#define layout_draw_pages func_0882FB38
const char *func_0886BEA4(void *a, int i);
#define quest_text func_0886BEA4
void func_0882D6F0(void *self, u16 x, u16 y, u16 w, int a, int r, int g, int b);
#define gfx_fill_rect func_0882D6F0
void func_08890DFC(void *font, int x, int y);
u8 *func_088298E4(void *self);
#define menu_quest_data func_088298E4
void menu_draw_stars(void *self, int a);
void menu_draw_number(void *self, int x, int y, u32 v);
void menu_draw_time(void *self, int x, int y, s32 which);
void func_08849D7C(void *self);
#define hud_draw_quest_info func_08849D7C
void func_0883488C(void *self, int dim);
#define menu_draw_main_page func_0883488C
int func_088239FC(void *self);
#define menu_special_mode func_088239FC
int func_0882A864(void *self, int a);
#define menu_input_ready func_0882A864
int func_09A6E5A8(void *p);
int func_0884FA5C(void *game, int a);
#define game_quest_locked func_0884FA5C
const char *func_09A63198(void *a, u16 id);
const char *func_09A7B790(void *a, u16 id);
void func_0882DCA8(void *self, u16 x, u16 y, int w, int h, int alpha);
#define gfx_draw_subwindow func_0882DCA8
void func_0882DCA0(void *self, u16 x, u16 y, int w, int h, int alpha);
#define gfx_draw_window func_0882DCA0
void func_0882E2C0(void *self, u16 id, int x, int y, int w, int h, int a, int alpha);
#define gfx_draw_item_icon_alpha func_0882E2C0
s32 func_088303A0(void *self, u8 v);
#define equip_rarity_text_color func_088303A0
void func_08892004(void *font, const char *fmt, ...);
void func_0889134C(void *font, const char *fmt, ...);
const char *func_08851534(void *game, u32 id);
#define text_item_info func_08851534
void func_0885C5DC(s16 *list, int count);
#define item_pairs_sort func_0885C5DC
void menu_draw_items_page(void *self, s16 *list, int sel, u32 color, u32 flags, int d);
void menu_draw_desc(void *self, u32 kind, u32 id, int arg);

#endif
