/* Item box of the house, hunter card data, progress checks (lists of progress flags).
 * (Guessed names.) */
#include "menu.h"

extern void *D_089CBBE8;          /* text helper */
extern const u16 *D_089B1EBC[], *D_089B1E90[], *D_089B1E64[];
extern const u16 D_0899CBC0[];
extern const u16 D_089ACAE8;
s32 game_box_size(void *g);
int game_has_progress(u8 *g, u32 id);
void game_add_money(u8 *g, s32 n);
void box_item_notify(void *a, u32 item);
void func_08816730(void *a, void *dst, int n, const char *s);
void guild_card_update(u8 *g);
u64 func_08904350(u32 a, u32 b, u32 c, u32 d);
void log_add(u8 *g, int a, u32 b);
void *memcpy(void *d, const void *s, size_t n);
int sceWlanGetEtherAddr(u8 *mac);
int sceRtcGetCurrentClockLocalTime(void *t);

/* 1 for punctuation and symbols (used by the word filter) */
int text_is_symbol(void *g, u32 c_arg)
{
    static const u16 sym[] = {
        0x201E, 0x201C, 0x201A, 0x2018, 0xBF, 0xBB, 0xB5, 0xB4, 0xB0, 0xAB, 0xAA, 0xA6, 0xA1, 0x20AC,
        0xF7, 0xD7, 0xA7, 0xA5, 0xA4, 0xA3, 0xA2, 0xFFE5, 0xFF5E, 0xFF5D, 0xFF5C, 0xFF5B, 0xFF40, 0xFF3F,
        0xFF3E, 0xFF3D, 0xFF3C, 0xFF3B, 0xFF20, 0xFF1F, 0xFF1E, 0xFF1D, 0xFF1C, 0xFF1B, 0xFF1A, 0xFF19,
        0xFF18, 0xFF17, 0xFF16, 0xFF15, 0xFF14, 0xFF13, 0xFF12, 0xFF11, 0xFF10, 0xFF0F, 0xFF0D, 0xFF0E,
        0xFF0C, 0xFF0B, 0xFF0A, 0xFF09, 0xFF08, 0xFF06, 0xFF05, 0xFF04, 0xFF03, 0xFF01, 0x30FC, 0x30FB,
        0x300D, 0x300C, 0x3002, 0x3001, 0x3000, 0x201D, 0x2019,
    };
    u16 c = c_arg;
    int r = 0;
    unsigned i;

    for (i = 0; i < sizeof(sym) / sizeof(sym[0]); i++) {
        if (c == sym[i]) {
            r = 1;
            break;
        }
    }
    if (c >= 0x20 && c < 0x41)
        r = 1;
    if (c >= 0x5B && c < 0x61)
        r = 1;
    if (c >= 0x7B && c < 0x80)
        r = 1;
    return r;
}

/* puts n items of the kind item in the box (first in the slot, then where it fits);
 * returns the number of items that did not fit */
u32 box_add_item(u8 *g, u32 item_arg, u32 n_arg, u32 slot_arg)
{
    u16 item = item_arg, slot = slot_arg;
    u32 n = n_arg;          /* returned unmasked when nothing changes it, as the original */
    s16 count = game_box_size(g);
    u16 *box = (u16 *)(g + 0x3428);
    u32 max = D_0899D19B[item * 0x18];
    s16 i;

    if (max != 0xFF) {
        max = 99;
        if ((u16)n > max)
            n = max;
        if (slot != 0xFFFF && box[slot * 2] == item) {
            s16 have = box[slot * 2 + 1];
            u32 room = max - have;

            if ((u16)n <= room) {
                box[slot * 2 + 1] = have + n;
                box_item_notify(D_08A62C50, item_arg);
                return 0;
            }
            n = (u16)(n - room);
            box[slot * 2 + 1] = max;
        }
        for (i = 0; i < count; i++) {
            u16 *e = box + i * 2;

            if (e[0] == item) {
                s16 have = e[1];

                if ((u32)have < max) {
                    u32 room = max - have;

                    if ((u16)n <= room) {
                        e[1] = have + n;
                        box_item_notify(D_08A62C50, item_arg);
                        return 0;
                    }
                    n = (u16)(n - room);
                    e[1] = max;
                }
            }
        }
    }
    for (i = 0; i < count; i++) {
        if (box[i * 2] == 0) {
            box[i * 2] = item_arg;
            box[i * 2 + 1] = n;
            box_item_notify(D_08A62C50, item_arg);
            return 0;
        }
    }
    return n;
}

/* new hunter card: identifier from the MAC address and the time, default comment */
void hunter_card_init(u8 *g)
{
    u8 mac[8];
    u16 t[8];
    u32 frame;

    memset(g + 0x4A58, 0, 0x62D00);
    memcpy(g + 0x67758, g + 0x4A0, 0x12);
    sceWlanGetEtherAddr(mac);
    sceRtcGetCurrentClockLocalTime(t);
    g[0x679A0] = mac[3];
    g[0x679A1] = mac[4];
    g[0x679A2] = mac[5];
    g[0x679A3] = t[0];
    g[0x679A4] = ((t[1] & 0xF) << 4) + ((t[2] & 0x1E) >> 1);
    g[0x679A5] = ((t[2] & 1) << 7) + (t[4] & 0x1F);
    frame = *(u32 *)(D_08A631A0 + 0xF7A554);
    g[0x679A6] = (t[5] & 0x3F) + ((frame & 0x300) >> 2);
    g[0x679A7] = *(u32 *)(D_08A631A0 + 0xF7A554);
    U16(g, 0x677F2) = 0;
    U16(g, 0x677F4) = 0;
    U16(g, 0x677F6) = 0;
    func_08816730(D_089CBBE8, g + 0x6778C, 0x62, func_0885151C(g, D_089ACAE8));
    guild_card_update(g);
}

/* current date: day, month, year */
void get_date(void *g, u8 *out)
{
    u16 t[8];

    sceRtcGetCurrentClockLocalTime(t);
    U16(out, 2) = t[0];
    out[1] = t[1];
    out[0] = t[2];
}

/* 1 if all the progress flags of the list are set */
static int all_progress(u8 *g, const u16 *l)
{
    for (; *l != 0; l++) {
        if ((u8)game_has_progress(g, *l) == 0)
            return 0;
    }
    return 1;
}

int progress_list_a(u8 *g, s32 i) { return all_progress(g, D_089B1EBC[i]); }
int progress_list_b(u8 *g, s32 i) { return all_progress(g, D_089B1E90[i]); }

int progress_a01(u8 *g) { return (u8)progress_list_a(g, 0) && (u8)progress_list_a(g, 1); }
int progress_a23(u8 *g) { return (u8)progress_list_a(g, 2) && (u8)progress_list_a(g, 3); }
int progress_a4(u8 *g) { return (u8)progress_list_a(g, 4) != 0; }

int progress_a0_5(u8 *g)
{
    int i;

    for (i = 0; i < 6; i++) {
        if ((u8)progress_list_a(g, i) == 0)
            return 0;
    }
    return 1;
}

int progress_a6(u8 *g) { return (u8)progress_list_a(g, 6) != 0; }
int progress_a7(u8 *g) { return (u8)progress_list_a(g, 7) != 0; }
int progress_a8(u8 *g) { return (u8)progress_list_a(g, 8) != 0; }
int progress_b01(u8 *g) { return (u8)progress_list_b(g, 0) && (u8)progress_list_b(g, 1); }

int progress_b234(u8 *g)
{
    return (u8)progress_list_b(g, 2) && (u8)progress_list_b(g, 3) && (u8)progress_list_b(g, 4);
}

int progress_b567(u8 *g)
{
    return (u8)progress_list_b(g, 5) && (u8)progress_list_b(g, 6) && (u8)progress_list_b(g, 7);
}

int progress_b8(u8 *g) { return (u8)progress_list_b(g, 8) != 0; }
int progress_b9(u8 *g) { return (u8)progress_list_b(g, 9) != 0; }
int progress_b10(u8 *g) { return (u8)progress_list_b(g, 10) != 0; }

/* all the lists completed */
int progress_all(u8 *g)
{
    return (u8)progress_a0_5(g) && (u8)progress_list_a(g, 6) && (u8)progress_list_a(g, 7)
        && (u8)progress_list_a(g, 8) && (u8)progress_b01(g) && (u8)progress_b234(g) && (u8)progress_b567(g)
        && (u8)progress_list_b(g, 8) && (u8)progress_list_b(g, 9) && (u8)progress_list_b(g, 10);
}

int progress_c(u8 *g)
{
    return all_progress(g, D_0899CBC0);
}

/* 64 bit flags at g+0x678A0 */
void game_set_flag64(u8 *g, u32 i)
{
    u64 m = func_08904350(1, 0, i, 0);

    U32(g, 0x678A0) |= (u32)m;
    U32(g, 0x678A4) |= (u32)(m >> 32);
    log_add(g, 8, i);
}

int game_test_flag64(u8 *g, u32 i)
{
    u64 m = func_08904350(1, 0, i, 0);

    return ((U32(g, 0x678A0) & (u32)m) | (U32(g, 0x678A4) & (u32)(m >> 32))) != 0;
}

/* number of the progress flags set in the lists D_089B1E64 (without 0xFA1..0xFA7) */
s32 progress_count(u8 *g)
{
    s32 n = 0;
    u8 i;

    for (i = 0; i < 11; i++) {
        const u16 *l = D_089B1E64[i];

        for (; *l != 0; l++) {
            if (*l >= 0xFA1 && *l <= 0xFA7)
                continue;
            if ((u8)game_has_progress(g, *l) == 1)
                n++;
        }
    }
    return n;
}

/* pays the pending reward money */
void game_pay_pending(u8 *g)
{
    if (U32(g, 0x47C) != 0) {
        game_add_money(g, U32(g, 0x47C));
        U32(g, 0x47C) = 0;
    }
}

int quest_flag_10(void)
{
    return (U32(D_08A62C7C, 0x28) & 0x10) != 0;
}
