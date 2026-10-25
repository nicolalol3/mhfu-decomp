/* Game texts: tables of strings in the text file loaded at D_089CC440
 * (a list of offsets to the tables, each table a list of offsets to the strings),
 * plus small helpers of the hunter data. (Guessed names.) */
#include "menu.h"
#undef text_menu
#undef text_item_name
#undef text_item_info

extern u8 D_089CC440[];           /* text file */
extern const u16 D_0899A1FC[], D_0899AAF8[], D_0899ADBC[], D_0899B124[], D_0899B46C[], D_0899B7A4[],
    D_0899BAD8[];
extern const u8 D_08973834[], D_08963254[], D_08967674[], D_0896B814[], D_0896F84C[];
u32 game_rand(void *a, int b);
void *memcpy(void *d, const void *s, size_t n);
int memcmp(const void *a, const void *b, size_t n);

/* Felyne / partner: every 10 points of experience raises one of two stats */
void partner_add_exp(void *g, u8 *p)
{
    U16(p, 0x22)++;
    while (U16(p, 0x22) >= 10) {
        u16 thr;

        U16(p, 0x22) -= 10;
        thr = (p[0x4C] < p[0x4D]) ? 0x54 : 0x10;
        if ((s32)(u16)game_rand(D_08A631A0, 1) % 100 < thr) {
            if (p[0x24] < 99)
                p[0x24]++;
        } else if (p[0x25] < 99) {
            p[0x25]++;
        }
    }
}

/* rank flags of the guild card entries of the hunter i, from the friends list */
void guild_update_friend_ranks(u8 *g, s32 i)
{
    u8 *h = g + i * 0x1078;
    u8 *x;
    int a;

    if (U16(h, 0x4A58) == 0)
        return;
    x = h + 0x55E8;
    for (a = 0; a < 13; a++, x += 0x40) {
        u8 *y;
        int b;

        if (S8(x, 0) == 0 || memcmp(g + 0x679A0, x + 0x2A, 8) != 0)
            continue;
        y = g + 0x69BEC;
        for (b = 0; b < 13; b++, y += 0x70) {
            u8 n;
            u16 f;

            if (S8(y, 0) == 0 || memcmp(y + 0x60, x + 0x2A, 0x10) != 0)
                continue;
            n = x[0xB];
            f = U16(y, 0x20);
            if (n >= 50 && (f & 0x40) == 0) {
                U16(y, 0x20) = f | 0x80;
                U16(y, 0x20) &= 0xFFD5;
            } else if (n >= 30 && (f & 0x10) == 0) {
                if ((f & 0x80) == 0) {
                    U16(y, 0x20) = f | 0x20;
                    U16(y, 0x20) &= 0xFFF5;
                }
            } else if (n >= 15 && (f & 4) == 0) {
                if ((f & 0x80) == 0 && (f & 0x20) == 0) {
                    U16(y, 0x20) = f | 8;
                    U16(y, 0x20) &= 0xFFFD;
                }
            } else if (n >= 5 && (f & 1) == 0 && (f & 0x80) == 0 && (f & 0x20) == 0 && (f & 8) == 0) {
                U16(y, 0x20) = f | 2;
            }
            break;
        }
    }
}

/* start of the text table t */
u8 *text_table(void *g, s32 t)
{
    return D_089CC440 + ((s32 *)D_089CC440)[t + 2];
}

/* string id of the table t (NULL if id is 0xFFFF or the string is missing) */
const char *text_get(void *g, s32 t, u32 id)
{
    u8 *tab = text_table(g, t);
    s32 off;

    if ((id & 0xFFFF) == 0xFFFF)
        return NULL;
    memcpy(&off, tab + (id & 0xFFFF) * 4, 4);
    if (off == -1)
        return NULL;
    return (const char *)(tab + off);
}

const char *text_menu(void *g, u32 id) { return text_get(g, 0, id); }
const char *text_item_name(void *g, u32 id) { return text_get(g, 1, id); }
const char *text_item_info(void *g, u32 id) { return text_get(g, 2, id); }
const char *text_table3(void *g, u32 id) { return text_get(g, 3, id); }
const char *text_table4(void *g, u32 id) { return text_get(g, 4, D_0899A1FC[id & 0xFFFF]); }
const char *text_table5(void *g, u32 id) { return text_get(g, 5, id); }
const char *text_table6(void *g, u32 id) { return text_get(g, 6, D_0899AAF8[id & 0xFFFF]); }
const char *text_table7(void *g, u32 id) { return text_get(g, 7, id); }
const char *text_table8(void *g, u32 id) { return text_get(g, 8, D_0899ADBC[id & 0xFFFF]); }
const char *text_table9(void *g, u32 id) { return text_get(g, 9, id); }
const char *text_table10(void *g, u32 id) { return text_get(g, 10, D_0899B124[id & 0xFFFF]); }
const char *text_table11(void *g, u32 id) { return text_get(g, 11, id); }
const char *text_table12(void *g, u32 id) { return text_get(g, 12, D_0899B46C[id & 0xFFFF]); }
const char *text_table13(void *g, u32 id) { return text_get(g, 13, id); }
const char *text_table14(void *g, u32 id) { return text_get(g, 14, D_0899B7A4[id & 0xFFFF]); }
const char *text_table15(void *g, u32 id) { return text_get(g, 15, id); }
const char *text_table16(void *g, u32 id) { return text_get(g, 16, D_0899BAD8[id & 0xFFFF]); }
const char *text_table17(void *g, u32 id) { return text_get(g, 17, id); }
const char *text_table18(void *g, u32 id) { return text_get(g, 18, id); }
const char *text_table21(void *g, u32 id) { return text_get(g, 21, id); }
const char *text_table22(void *g, u32 id) { return text_get(g, 22, id); }

/* defense / slots of the equipment piece kind of the hunter i (-1 = unknown kind) */
u32 hunter_equip_value(u8 *p, s32 i, u32 kind)
{
    u8 *h = p + i * 0x8C;
    u16 v;

    switch (kind) {
    case 0:
        return *(const u16 *)(D_08973834 + (h[0x40] != 0) * 2 + U16(h, 0x6E) * 0x28);
    case 1:
        return h[0x43];
    case 2:
        v = *(const u16 *)(D_08963254 + (h[0x40] != 0) * 2 + U16(h, 0x7A) * 0x28);
        if (v == 0)
            return h[0x44];
        return v;
    case 3:
        return *(const u16 *)(D_08967674 + (h[0x40] != 0) * 2 + U16(h, 0x86) * 0x28);
    case 4:
        return *(const u16 *)(D_0896B814 + (h[0x40] != 0) * 2 + U16(h, 0x92) * 0x28);
    case 5:
        return *(const u16 *)(D_0896F84C + (h[0x40] != 0) * 2 + U16(h, 0x9E) * 0x28);
    }
    return 0xFFFFFFFF;
}
