/* Guild card of the hunter: equipment, monster records and crowns. (Guessed names.) */
#include "menu.h"

extern const u8 D_0899CB60[];     /* monster groups (every 2 bytes) */
extern const u8 D_0899CBA0[];     /* index of the crown thresholds of each group */
extern const s16 D_08930BDC[][7]; /* crown thresholds (sizes) */
u32 game_group_count_total(u8 *g, u32 id);
u32 game_best_record_max(u8 *g, u32 id, u8 *which);
u32 game_best_record_min(u8 *g, u32 id, u8 *which);
f32 game_size_percent(void *g, u32 id, u32 v);
int game_test_bit62(u8 *g, u32 i_arg);
s32 progress_count(u8 *g);

/* crown of a size v: 3 (biggest / smallest), 2 (silver), 1, 0 */
static u32 crown_bits(u32 v, const s16 *t)
{
    if (!(t[4] < (s32)v))
        return 3;
    if (!((s32)v < t[3]))
        return 2;
    if (!((s32)v < t[2]))
        return 1;
    return 0;
}

/* updates the guild card of the hunter from the current state */
void guild_card_update(u8 *g)
{
    u8 which;
    int i;
    s32 n;

    U16(g, 0x6776A) = g[0x4C1];
    U16(g, 0x6776C) = U16(g, 0x4C2);
    U16(g, 0x6776E) = U16(g, 0x4CE);
    U16(g, 0x67770) = U16(g, 0x4DA);
    U16(g, 0x67772) = U16(g, 0x4E6);
    U16(g, 0x67774) = U16(g, 0x4F2);
    U16(g, 0x67776) = U16(g, 0x4FE);
    for (i = 0; i < 0x1F; i++) {
        u8 *c = g + i * 8;
        u16 v;

        U16(c, 0x678AA) = (U16(c, 0x678AA) & 0xC000) | (game_group_count_total(g, D_0899CB60[i * 2]) & 0x3FFF);
        v = game_best_record_max(g, D_0899CB60[i * 2], &which);
        U16(c, 0x678AC) = (u32)(10.0f * (0.05f + game_size_percent(g, which, v)));
        c[0x678A8] = (c[0x678A8] & 0xF9) | (crown_bits(v, D_08930BDC[D_0899CBA0[i]]) << 1);
        v = game_best_record_min(g, D_0899CB60[i * 2], &which);
        U16(c, 0x678AE) = (u32)(10.0f * (0.05f + game_size_percent(g, which, v)));
        c[0x678A8] = (c[0x678A8] & 0xE7) | (crown_bits(v, D_08930BDC[D_0899CBA0[i]]) << 3);
        if (game_test_bit62(g_game, D_0899CBA0[i]) != 0)
            c[0x678A8] = (c[0x678A8] & 0xFE) | 1;
    }
    g[0x677EF] = g[0x44BC];
    n = progress_count(g);
    if (n >= 0x164)
        g[0x677F0] = 6;
    else if (n >= 0x10F)
        g[0x677F0] = 5;
    else if (n >= 0xC0)
        g[0x677F0] = 4;
    else if (n >= 0xA7)
        g[0x677F0] = 3;
    else if (n >= 0x7A)
        g[0x677F0] = 2;
    else if (n >= 0x46)
        g[0x677F0] = 1;
    else
        g[0x677F0] = 0;
}
