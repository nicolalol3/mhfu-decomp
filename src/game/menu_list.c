/* Menu di gioco: liste di oggetti (8 per pagina) e scheda informazioni.
 * Stessa classe di menu_box.c. (Nomi ipotetici.) */
#include "menu.h"

extern const u16 D_089AC870[];
extern const s16 D_08938554[];
extern const char D_0893855C[];        /* "%d" (quantità) */
extern const u8 D_0899D19B[];
extern const s16 D_08938560[][3];      /* posizione (x, y) e flag di alcune scritte */
extern const u8 D_08930BDC[];          /* tabella da 0xE byte per voce */
extern const u16 D_089AC648[], D_089AC65C[];
extern const u8 D_0893B0CC[];

int func_08828EEC(void *self);
int func_0885B3A0(void *a, int id, int b, int c);
int func_08828F14(void *self, u16 idx);
u32 func_08852F58(void *game);
u32 func_08852FA0(void *game, u8 idx);
s32 func_08850520(void *game, u8 id);
void func_08890E40(void *font, int a);
const char *func_08851674(void *game, int idx);
int func_0886D644(void *a, u8 id);
s32 func_0885028C(void *game, u8 id, u8 *out);
s32 func_08850338(void *game, u8 id, u8 *out);
f32 func_08850238(void *game, u8 a, u16 b);
u64 func_08905294(f32 x);    /* float -> double (formato del gioco) */
void menu_draw_rating_icon(void *self, const u8 *entry, int x, int y, int v);
void func_0882AF70(void *self, int x, int y, int w, int h, int a);
void func_0882FE34(void *self, int x, int y, u8 a, u8 b);

void menu_draw_list_hint(void *self, u32 idx);

/* lista di 8 oggetti (pagina della voce sel) con quantità e colori */
void menu_draw_items_page(void *self, s16 *list, int sel, u32 color, u32 flags, int d)
{
    char title[32];
    u8 layout[0x28];
    int page = sel / 8;
    u32 idx = page * 8;
    u16 *e;
    s16 y;
    int i;

    sprintf(title, D_0893854C, func_0885151C(D_089CC438, D_089AC870[flags & 1]));
    func_0882FA24(layout, D_0893B0E0, (int)title, 0, D_08938554[d]);
    if (color == 0) {
        sel = -1;
    } else {
        sel = sel - page * 8;
        U32(layout, 0x18) = color;
    }
    func_0882F580(self, layout, (int)title, sel, 0xFF, 0);
    if (flags & 0x10)
        func_0882FB38(self, layout, page & 0xFFFF, 3, 0, 0);
    e = (u16 *)(list + page * 16);
    y = S16(layout, 14);
    for (i = 8; i > 0; i--, e += 2, idx++) {
        int col = 0, full = 2;

        if (flags & 0xC) {
            if ((u8)func_08828EEC(self) == 0) {
                if ((flags & 8) == 0) {
                    if ((u8)func_0885B3A0(D_08A62C50, (s16)e[0], 0, 0) == 0) {
                        col = 10;
                        full = 13;
                    } else if ((u8)func_08828F14(self, idx & 0xFFFF) == 1) {
                        col = 5;
                    }
                } else if ((u8)func_08828F14(self, idx & 0xFFFF) == 1) {
                    col = 5;
                } else if (func_0885B5B8(D_08A62C50, S16(self, 0x5AC), (s16)e[0], 0, 0, 0) == 0) {
                    col = 10;
                    full = 13;
                }
            } else {
                s16 r = (s16)func_08828F3C(self, e);

                if (r < 0) {
                    if ((u8)func_08828F14(self, idx & 0xFFFF) == 1) {
                        col = 5;
                    } else {
                        col = 10;
                        full = 13;
                    }
                } else if (r == 1) {
                    col = 6;
                } else if (r == 0) {
                    col = 2;
                }
            }
        }
        func_08890E38(D_08A631A8, col);
        func_08890DFC(D_08A631A8, S16(layout, 12), y);
        if (e[0] == 0) {
            func_0889134C(D_08A631A8, D_08938478, func_0885151C(D_089CC438, *(u16 *)0x089AC876));
        } else {
            u8 max;

            func_0889134C(D_08A631A8, D_08938478, func_08851528(D_089CC438, e[0]));
            max = D_0899D19B[e[0] * 0x18];
            if (max > 1) {
                func_08890DFC(D_08A631A8, (s16)(S16(layout, 12) + 0x72), y);
                if (max == 0xFF) {
                    func_0889134C(D_08A631A8, D_08938478, func_0885151C(D_089CC438, *(u16 *)0x089AC878));
                } else {
                    if ((s16)max <= (s16)e[1])
                        func_08890E38(D_08A631A8, full);
                    func_0889134C(D_08A631A8, D_0893855C, (s16)e[1]);
                }
            }
        }
        y += S16(layout, 16);
    }
    if (flags & 0x40)
        menu_draw_list_hint(self, 1);
}

/* scritta di aiuto accanto alla lista (riquadro opzionale) */
void menu_draw_list_hint(void *self, u32 idx_arg)
{
    u8 idx = idx_arg;
    const s16 *p = D_08938560[idx];

    if ((u16)p[2] & 1)
        func_0882DCA8(self, p[0] - 0xC, p[1] - 8, 0x50, 0x20, 0xFF);
    func_0882A9DC(self, p[0], p[1], 0x3A);
    func_08890DE8(D_08A631A8, 14, 14);
    func_088915B0(D_08A631A8, (s16)(p[0] + 0x16), (s16)(p[1] + 1), 0, D_08938478,
                  func_0885151C(D_089CC438, *(u16 *)0x089ACA00));
}

/* scheda con le informazioni di una voce (0x5D9 = voce, <0 = nessuna) */
void menu_draw_info_card(void *self)
{
    char title[32], b1[16], b2[16];
    Layout l;
    const u8 *ent;
    s8 n;

    sprintf(title, D_0893854C, func_0885151C(D_089CC438, *(u16 *)0x089AC60A));
    func_0882FA24(&l, D_0893B0CC, (int)title, 0, 0);
    func_0882F580(self, &l, (int)title, -1, 0xFF, 0);
    n = S8(self, 0x5D9);
    if (n < 0) {
        func_088915B0(D_08A631A8, (s16)(l.x + (s16)((int)((u16)l.w - 0x70) >> 1)), (s16)(l.ty + 0x50), 0,
                      D_08938478, func_0885151C(D_089CC438, *(u16 *)0x089AC638));
    } else {
        u16 cnt;
        s16 v;
        u8 kind;

        ent = D_08930BDC + n * 0xE;
        cnt = (u16)func_08852F58(D_089CC438);
        if (cnt > 1)
            func_0882FB38(self, &l, ((u16)func_08852FA0(D_089CC438, U8(self, 0x5D9)) & 0xFF) - 1, cnt, 0, 0);
        func_088920A8(D_08A631A8, l.tx, l.ty, func_0885151C(D_089CC438, *(u16 *)0x089AC63A),
                      func_0885151C(D_089CC438, D_089AC65C[ent[3]]));
        func_088920A8(D_08A631A8, l.tx, (s16)(l.ty + 0x10), func_0885151C(D_089CC438, *(u16 *)0x089AC63C),
                      func_0885151C(D_089CC438, D_089AC648[ent[0xC]]));
        v = (s16)func_08850520(D_089CC438, ent[3]);
        func_088920A8(D_08A631A8, l.tx, (s16)(l.ty + 0x20), func_0885151C(D_089CC438, *(u16 *)0x089AC63E), v);
        func_08890E40(D_08A631A8, 14);
        func_08891474(D_08A631A8, l.tx, (s16)(l.ty + 0x50), D_08938478, func_08851674(D_089CC438, S8(self, 0x5D9)));
        if ((s8)func_0886D644(D_08A62C7C, ent[3]) == 1) {
            const char *s1, *s2;

            if (v == 0) {
                s1 = s2 = func_0885151C(D_089CC438, *(u16 *)0x089AC644);
            } else {
                f32 f1, f2;
                s16 r;

                r = (s16)func_0885028C(D_089CC438, ent[3], &kind);
                f1 = func_08850238(D_089CC438, kind, r);
                menu_draw_rating_icon(self, ent, (s16)(l.tx + 0x4D), (s16)(l.ty + 0x30), r);
                r = (s16)func_08850338(D_089CC438, ent[3], &kind);
                f2 = func_08850238(D_089CC438, kind, r);
                menu_draw_rating_icon(self, ent, (s16)(l.tx + 0x4D), (s16)(l.ty + 0x3E), r);
                sprintf(b1, func_0885151C(D_089CC438, *(u16 *)0x089AC642), func_08905294(f1));
                sprintf(b2, func_0885151C(D_089CC438, *(u16 *)0x089AC642), func_08905294(f2));
                s1 = b1;
                s2 = b2;
            }
            func_088920A8(D_08A631A8, l.tx, (s16)(l.ty + 0x30), func_0885151C(D_089CC438, *(u16 *)0x089AC640));
            func_088920A8(D_08A631A8, (s16)(l.tx + 0x62), (s16)(l.ty + 0x30), s1);
            func_088920A8(D_08A631A8, (s16)(l.tx + 0x62), (s16)(l.ty + 0x3E), s2);
        }
        func_0882AF70(self, l.x + 0x9F, l.y + 0x25, 0x58, 0x40, 0xAF);
        func_0882FE34(self, l.x + 0xB3, l.y + 0x2C, ent[1], ent[2]);
    }
}
