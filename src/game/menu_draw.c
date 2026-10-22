/* In-game menu: window drawing (descriptions, lists, pages).
 * Same class as menu_box.c. (Guessed names.) */
#include "menu.h"

#define GAME_FLAGS (*(u32 *)(D_089CC438 + 0x6AF14))
#define OPT_FLAGS  (*(u32 *)(D_08A631A0 + 0xF7A57C))

void menu_draw_page_list(void *self, s32 dim);

/* draws the main part of the menu according to the mode (0x57E/0x57F) */
void menu_draw_main(void *self)
{
    u8 layout[0x28];

    if (U8(self, 0x57E) == 1) {
        s8 m = U8(self, 0x57F);

        if (m == 2) {
            if (func_08857C38(D_089CC438) == 0) {
                func_08890E38(D_08A631A8, 0);
                func_08890DE8(D_08A631A8, 14, 14);
                func_0882DCB0(self, 0x10, 2, 0xE0, 0xBE, 0xFF, 0x1D5);
                func_0882DCB0(self, 0x10, 0xC0, 0xE0, 0x4D, 0xFF, 0x1D5);
                func_08891474(D_08A631A8, 0x22, 0x5C, func_0885151C(D_089CC438, *(u16 *)0x089AD118));
            } else {
                func_08833290(self, 0x10, 2, func_08857C38(D_089CC438), 0);
            }
        } else if (m == 1) {
            menu_draw_info_card(self);
        } else if (m == 0) {
            menu_draw_trade_info(self);
        }
    } else if (U8(self, 0x57E) == 0) {
        menu_draw_page_list(self, 1);
        func_0882FA24(layout, D_0893AFB4, 0, 0, 0);
        func_0882F9A8(self, D_0893AFB4, 0, U8(self, 0x57F), 0xFF, 0);
    }
}

/* information window (pages 0..3) */
void menu_draw_info(void *self)
{
    char buf[64];
    Layout l;
    u32 page;
    u8 *p;

    sprintf(buf, D_0893854C, func_0885151C(D_089CC438, D_089AC7C4[0]));
    if ((U32(self, 0x528) & 0x40) == 0) {
        func_0882F580(self, (void *)&D_0893B040, (int)buf, -1, 0xFF, 0);
        func_088915B0(D_08A631A8, 0xFC, 0x86, 0, D_08938478, func_0885151C(D_089CC438, D_089AC7C4[1]));
        return;
    }
    l = D_0893B040;
    page = U8(self, 0x57F);
    if (GAME_FLAGS & 1)
        page = (page + 1) & 0xFF;
    if (page == 0)
        sprintf(buf, D_0893854C, func_0885151C(D_089CC438, D_089AC7C4[2]));
    l.ptr = D_089AC7CC + page * 0x1A;
    func_0882F580(self, &l, (int)buf, -1, 0xFF, 0);
    func_0882FB38(self, &l, U8(self, 0x57F), U8(self, 0x581), 0, 0);
    func_088915B0(D_08A631A8, l.tx, (s16)(l.ty + l.lh - 2), 0, D_08938478, func_0886BEA4(D_08A62C7C, 0));
    func_0882D6F0(self, l.tx, l.ty + l.lh * 2 - 3, l.w - 0x18, 1, 0x80, 0x80, 0x80);
    if (page == 3) {
        func_08891474(D_08A631A8, (s16)(l.tx + l.cw * 4), (s16)(l.ty + l.lh * 2), D_08938478,
                      func_0886BEA4(D_08A62C7C, 5));
        func_08891474(D_08A631A8, l.tx, (s16)(l.ty + l.lh * 4), D_08938478, func_0886BEA4(D_08A62C7C, 3));
    } else if (page == 2) {
        func_08890DFC(D_08A631A8, (s16)(l.tx + l.cw * 6), (s16)(l.ty + l.lh * 2));
        p = func_088298E4(self);
        menu_draw_stars(self, (s8)p[0x1A]);
        func_08891474(D_08A631A8, l.tx, (s16)(l.ty + l.lh * 5), D_08938478, func_0886BEA4(D_08A62C7C, 1));
        func_08891474(D_08A631A8, l.tx, (s16)(l.ty + l.lh * 9), D_08938478, func_0886BEA4(D_08A62C7C, 2));
    } else if (page == 1) {
        int x = (s16)(l.tx + l.cw * 5);

        p = func_088298E4(self);
        menu_draw_number(self, x, (s16)(l.ty + l.lh * 2), U32(p, 8));
        p = func_088298E4(self);
        menu_draw_number(self, x, (s16)(l.ty + l.lh * 3), U32(p, 4));
        menu_draw_time(self, x, (s16)(l.ty + l.lh * 4), 1);
        p = func_088298E4(self);
        func_08891474(D_08A631A8, x, (s16)(l.ty + l.lh * 5), D_08938478,
                      func_0885151C(D_089CC438, D_089ACC04[(s8)p[0x1C]]));
        func_08891474(D_08A631A8, l.tx, (s16)(l.ty + l.lh * 7), D_08938478, func_0886BEA4(D_08A62C7C, 4));
        p = func_088298E4(self);
        func_08891474(D_08A631A8, l.tx, (s16)(l.ty + l.lh * 10), D_08938478,
                      func_0885151C(D_089CC438, D_089ACC44[p[0x1D]]));
    } else if (page == 0) {
        func_08849D7C(self);
    }
}

void menu_draw_item_box(void *self)
{
    menu_draw_status(self, (u8 *)U32(self, 0x530), U8(self, 0x57F));
}

void menu_draw_equip_box(void *self)
{
    u8 buf[12];

    func_0882F580(self, D_0893B068, (int)func_0885151C(D_089CC438, *(u16 *)0x089AC860), U8(self, 0x57F), 0xFF, 0);
    menu_draw_equipment(self, (u8 *)U32(self, 0x530), 1);
    func_08830078(self, (u32)menu_get_equip_slot(self, buf, U8(self, 0x57F), (u8 *)U32(self, 0x530)), 8, 0x28,
                  U8(self, 0x582) | 0x100, 0, 1);
}

/* confirmation window with the name of the selected item */
void menu_draw_confirm(void *self)
{
    u8 layout[0x28];
    char buf[64];
    u32 lw[5];
    u8 *l = (u8 *)lw;
    u16 id;
    s8 m;

    if (U8(self, 0x52F) < 3)
        return;
    menu_draw_item_list(self, U8(self, 0x586), D_0893AA94[U8(self, 0x52F)], 0x10, 0);
    if (U8(self, 0x592) == 0) {
        const char *s;
        int n, t, w;
        int i;

        for (i = 0; i < 20; i++)
            l[i] = D_0893B0F4[i];
        n = func_08890E48(D_08A631A8, func_0885151C(D_089CC438, *(u16 *)0x089AC84C));
        *(s16 *)(l + 0xC) = 0xF0 - ((n * l[8]) >> 2);
        t = (n * l[8]) >> 1;
        w = ((t + 0x20) / 16) * 16;
        *(s16 *)(l + 4) = w;
        *(s16 *)(l + 0) = 0xF0 - ((w & 0xFFFF) >> 1);
        s = func_0885151C(D_089CC438, *(u16 *)0x089AC84C);
        func_0882FC50(self, l, s, 0xFF);
        U32((u8 *)U32(self, 0x530), 0x614) &= ~1;
    } else {
        m = U8(self, 0x593);
        if (m < 2) {
            func_0882FC50(self, D_0893B108, func_08851528(D_089CC438, U16(self, 0x590)), 0xFF);
        } else {
            const char *fmt = func_0885151C(D_089CC438, *(u16 *)0x089AC84E);

            sprintf(buf, fmt, func_08851528(D_089CC438, U16(self, 0x590)), (int)m);
            func_0882FC50(self, D_0893B11C, buf, 0xFF);
        }
    }
    id = U16(self, 0x58E);
    menu_draw_desc(self, 1, id, -1);
    m = U8(self, 0x52F);
    if (m == 5) {
        func_0882F9A8(self, D_0893B130, 0, U8(self, 0x537), 0xFF, 0);
        return;
    }
    if (m != 3) {
        if (m != 4)
            return;
        if (U8(self, 0x5DA) == 0) {
            func_0882A9DC(self, 0x10E, 0x50, 0x35);
            func_088915B0(D_08A631A8, 0x120, 0x52, 0, func_0885151C(D_089CC438, *(u16 *)0x089AC9FE));
        }
    }
    func_0882FA24(layout, D_0893B130, 0, 0, 0);
    U32(layout, 0x14) = (u32)D_089AC600;
    U32(layout, 0x18) = D_0893AAB0[U8(self, 0x52F)];
    func_0882F580(self, layout, 0, U8(self, 0x58C), 0xFF, 0);
}

/* list of the entries of one page (6 per page); dim = draw in grey */
void menu_draw_page_list(void *self, s32 dim_arg)
{
    s8 dim = dim_arg;
    char title[32];
    u8 layout[0x28];
    char col[8];
    s16 page, y;
    const u16 *ids;
    int i;

    if (GAME_FLAGS & 1) {
        func_0883488C(self);
        return;
    }
    page = (s16)(U8(self, 0x57D) / 6);
    sprintf(title, D_0893854C, func_0885151C(D_089CC438, *(u16 *)0x089AC850));
    func_0882FA24(layout, D_0893AF8C, (int)title, 0, 0);
    if (dim != 0)
        U32(layout, 0x18) = 0x80808080;
    func_0882F580(self, layout, (int)title, U8(self, 0x57D) - (s16)(page * 6), 0xFF, 0);
    func_0882FB38(self, layout, page, 2, 0, dim);
    for (i = 0; i < 6; i++)
        col[i] = 0;
    if (page == 0) {
        u8 *p = (u8 *)U32(self, 0x530);

        if ((s8)func_088239FC(self) == 0 || (s8)func_0882A864(self, 0) == 0 || (s8)func_09A6E5A8(p) == 1)
            col[1] = 10;
        if (p[0x1E0] != 0)
            col[3] = 10;
        if ((GAME_FLAGS & 0x100) == 0)
            col[4] = 10;
        if ((s8)func_0884FA5C(D_089CC438, 0) == 1) {
            col[5] = 10;
            col[4] = 10;
        }
        if (OPT_FLAGS & 1)
            col[5] = 10;
    } else {
        if ((s8)func_0884FA5C(D_089CC438, 0) == 1)
            col[4] = 10;
        col[5] = 10;
    }
    ids = D_089B54E0 + (s16)(page * 6);
    y = S16(layout, 14);
    for (i = 0; i < 6; i++) {
        func_088915B0(D_08A631A8, S16(layout, 12), y, col[i], func_09A63198(D_09A44C5C, ids[i]));
        y += S16(layout, 16);
    }
}

/* help buttons at the bottom right */
void menu_draw_help_keys(void *self)
{
    int x = 0x168, w = 0x70;

    func_08890DE8(D_08A631A8, 14, 14);
    func_08890E38(D_08A631A8, 0);
    func_088920A8(D_08A631A8, 0x170, 0xB1, func_09A63198(D_09A44C5C, D_089B5558[U8(self, 0x576) & 1]));
    if (OPT_FLAGS & 1) {
        x = 0x128;
        w = 0xB0;
        func_088920A8(D_08A631A8, 0x130, 0xB1, func_09A63198(D_09A44C5C, D_089B5558[2]));
    }
    func_0882DCA8(self, x, 0xA8, w, 0x20, 0xFF);
}

/* description of the current entry */
void menu_draw_entry_desc(void *self)
{
    s16 id = S16(self, 0x5FC);

    if (U8(self, 0x5FE) == 3) {
        if (id == 8 && U8(self, 0x5E4) == 1)
            id = 0xC;
        else if (id == 10 && U8(self, 0x60A) == 1)
            id = 0xC;
        else if (id == 8 && U8(self, 0x5E4) == 2)
            id = 0xD;
        else if (id == 10 && U8(self, 0x60A) == 2)
            id = 0xD;
    }
    menu_draw_desc(self, U8(self, 0x5FE), (u16)id, S8(self, 0x5FF));
}

void menu_draw_item_stats(void *self, u32 id, s32 x, s32 y, u32 mode);

/* description box: kind = entry type, id = item/entry */
void menu_draw_desc(void *self, u32 kind_arg, u32 id_arg, int arg)
{
    u8 kind = kind_arg;
    u16 id = id_arg;
    const u8 *layout;
    const char *s;
    u32 f;

    if (kind > 7 || id == 0xFFFF)
        return;
    if (kind == 1 && id > 0x18) {
        s16 x = 0x48, y = 0x28;
        u16 it;
        const u8 *item;

        if (U32(self, 0x528) & 0x200) {
            y = 0x88;
            x = 0x48;
        } else if (U32(self, 0x528) & 0x800) {
            x = 0xD8;
            y = 200;
        }
        func_0882DCA0(self, x, y, 0x100, 0x40, 0xFF);
        it = id - 0x18;
        menu_draw_item_stats(self, it, x + 0x30, y + 9, arg);
        func_0882E2C0(self, it, x + 10, y + 8, 0x20, 0x20, 1, 0xFF);
        item = D_0899D198 + it * 0x18;
        func_08890E38(D_08A631A8, (s8)func_088303A0(self, item[2]));
        func_08890DE8(D_08A631A8, 12, 12);
        func_088920A8(D_08A631A8, x + 8, y + 0x2A, func_0885151C(D_089CC438, *(u16 *)0x089AC864), item[2] + 1);
        func_08890E38(D_08A631A8, 0);
        return;
    }
    f = U32(self, 0x528);
    if (f & 0x200)
        layout = (f & 0x10) ? D_0893B158 : D_0893B144;
    else if (f & 0x100)
        layout = D_0893B158;
    else if (kind == 3 && S16(self, 0x5FC) == 8)
        layout = D_0893B1A8;
    else if (kind == 3 && id < 9)
        layout = D_0893B194;
    else if (kind == 3 && U16(self, 0x5FC) > 8)
        layout = D_0893B180;
    else
        layout = D_0893B16C;
    if (kind == 5 || kind == 4)
        s = func_09A7B790(D_09B22D70, D_0893AF34[kind][id]);
    else
        s = func_0885151C(D_089CC438, D_0893AF34[kind][id]);
    func_0882FC50(self, layout, s, 0xFF);
}

/* item statistics (attack, elements, ...) in the description box */
void menu_draw_item_stats(void *self, u32 id_arg, s32 x_arg, s32 y_arg, u32 mode_arg)
{
    u16 id = id_arg;
    s16 x = x_arg;
    s16 y = y_arg;
    u8 mode = mode_arg;
    const u8 *item = D_0899D198 + id * 0x18;
    u8 kind;

    func_08890DE8(D_08A631A8, 14, 14);
    if (*(s16 *)(item + 0x16) == 0 || (s8)mode < 0) {
        kind = 0;
    } else {
        kind = mode & 3;
        func_08890E38(D_08A631A8, 0);
        func_08890DFC(D_08A631A8, (s16)(x + 0x8C), (s16)(y + 0x1C));
        func_08892004(D_08A631A8, D_08938478,
                      func_0885151C(D_089CC438, (mode & 3) == 0 ? *(u16 *)0x089AC880 : *(u16 *)0x089AC882));
    }
    func_08890E38(D_08A631A8, 0);
    if (kind == 1) {
        u16 n = *(u16 *)(item + 0x16);
        const u8 *e = D_089779DC + n * 0x10;
        int i;

        for (i = 0; i < 4; i++, e += 2) {
            func_08890DFC(D_08A631A8, (s16)(x + D_0893AF54[i][0]), (s16)(y + D_0893AF54[i][1]));
            if (e[0] == 0) {
                if (i == 0)
                    func_0889134C(D_08A631A8, D_08938478, func_0885151C(D_089CC438, *(u16 *)0x089AC9D2));
                break;
            }
            func_08890DE8(D_08A631A8, 12, 14);
            func_0889134C(D_08A631A8, D_08938478, func_0885151C(D_089CC438, D_089ACC70[e[0]]));
            if (e[0] != 1) {
                func_08890DE8(D_08A631A8, 14, 14);
                func_08890DFC(D_08A631A8, (s16)(x + D_0893AF54[i][0] + 0x3C), (s16)(y + D_0893AF54[i][1]));
                func_0889134C(D_08A631A8, func_0885151C(D_089CC438, *(u16 *)0x089AC87E), (s8)e[1]);
            }
        }
        func_088915B0(D_08A631A8, x_arg, (s16)(y + 0x1C), 5, func_0885151C(D_089CC438, *(u16 *)0x089AC87C),
                      D_089779D4[n * 0x10]);
    } else if (kind == 0) {
        func_088920A8(D_08A631A8, x_arg, y_arg, D_08938478, func_08851534(D_089CC438, id_arg)); /* values received, not reduced */
    }
}

/* item list (24 slots): flag 0x20 = compact and sort the list */
void menu_draw_item_list(void *self, int sel, u32 color, u32 flags, int d)
{
    s16 list[48];
    s16 *src;
    int i, n, used;

    if ((flags & 2) == 0)
        src = (s16 *)func_08856884(D_089CC438, 0);
    else
        src = (s16 *)(D_089CC438 + 0x43C8);
    if (flags & 0x20) {
        s16 *dst = list;

        n = 0;
        for (i = 0x18; i != 0; i--, src += 2) {
            if (src[0] != 0) {
                n++;
                dst[0] = src[0];
                dst[1] = src[1];
                dst += 2;
            }
        }
        used = n;
        for (n = 0x18 - n; n != 0; n--, dst += 2) {
            dst[1] = 0;
            dst[0] = 0;
        }
        func_0885C5DC(list, used);   /* sorts the used entries */
        src = list;
        color = 0;
        flags = (flags & 1) ? 0x21 : 0x20;
        sel = sel % 8;
    }
    menu_draw_items_page(self, src, sel, color, flags, d);
}
