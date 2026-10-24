/* Equipment data of the hunters (own state g+0x4A0.., other hunters in blocks of 0x8C bytes),
 * small random helpers. (Guessed names.) */
#include "menu.h"

extern void *D_08A62C54;
extern const u8 D_08973834[], D_08963254[], D_08967674[], D_0896B814[], D_0896F84C[];
extern const u8 D_08959FCC[], D_08960B9C[];   /* weapon tables: bowguns (0x18 bytes), others (0x1C) */
extern const u8 *D_089B65DC[];
u32 game_rand(void *a, int b);
s32 equip_slot_to_kind(void *g, u32 kind);
s32 equip_kind_to_slot(void *g, u32 kind);
u8 *armor_data(void *a, u32 k, s32 id);
u32 hunter_equip_value(u8 *p, s32 i, u32 kind);

/* same as hunter_equip_value, for the own hunter */
u32 own_equip_value(u8 *g, u32 kind)
{
    u16 v;

    switch (kind) {
    case 0:
        return *(const u16 *)(D_08973834 + (g[0x4B3] != 0) * 2 + U16(g, 0x4CE) * 0x28);
    case 1:
        return g[0x4B6];
    case 2:
        v = *(const u16 *)(D_08963254 + (g[0x4B3] != 0) * 2 + U16(g, 0x4DA) * 0x28);
        if (v == 0)
            return g[0x4B7];
        return v;
    case 3:
        return *(const u16 *)(D_08967674 + (g[0x4B3] != 0) * 2 + U16(g, 0x4E6) * 0x28);
    case 4:
        return *(const u16 *)(D_0896B814 + (g[0x4B3] != 0) * 2 + U16(g, 0x4F2) * 0x28);
    case 5:
        return *(const u16 *)(D_0896F84C + (g[0x4B3] != 0) * 2 + U16(g, 0x4FE) * 0x28);
    }
    return 0xFFFFFFFF;
}

int equip_piece_exists(void *g, u32 kind, s32 gender, u32 id_arg);

/* 1 if the hunter i wears a valid piece of the kind */
int hunter_has_piece(u8 *p, s32 i, u32 kind)
{
    u8 *h = p + i * 0x8C;
    u8 k = equip_slot_to_kind(p, kind & 0xFF);

    if (k >= 5)
        return 0;
    return equip_piece_exists(p, kind, h[0x40], U16(h + k * 0xC, 0x6E));
}

int own_has_piece(u8 *g, u32 kind)
{
    u8 k = equip_slot_to_kind(g, kind & 0xFF);

    if (k >= 5)
        return 0;
    return equip_piece_exists(g, kind, g[0x4B3], U16(g + k * 0xC, 0x4CE));
}

/* 1 if the armor piece id exists for the gender */
int equip_piece_exists(void *g, u32 kind, s32 gender, u32 id_arg)
{
    s16 id = id_arg;
    u8 k = equip_slot_to_kind(g, kind & 0xFF);
    u8 *e;

    if (k >= 5)
        return 0;
    e = armor_data(D_08A62C54, k, id);
    if (gender == 0)
        return U16(e, 0) != 0;
    return U16(e, 2) != 0;
}

/* byte 0x13 of the armor piece of the hunter i (0 if none) */
u8 hunter_piece_byte13(u8 *p, s32 i, u32 kind)
{
    u8 *h = p + i * 0x8C;
    u8 k;

    if ((u8)hunter_has_piece(p, i, kind) != 1)
        return 0;
    k = equip_slot_to_kind(p, kind & 0xFF);
    return armor_data(D_08A62C54, k, S16(h + k * 0xC, 0x6E))[0x13];
}

u8 own_piece_byte13(u8 *g, u32 kind, s32 id)
{
    if ((u8)own_has_piece(g, equip_kind_to_slot(g, kind & 0xFF) & 0xFF) != 1)
        return 0;
    return armor_data(D_08A62C54, kind & 0xFF, (s16)id)[0x13];
}

/* first word of the weapon of the hunter i */
u16 hunter_weapon_word(u8 *p, s32 i)
{
    u8 *h = p + i * 0x8C;

    if (h[0x61] == 5)
        return *(const u16 *)(D_08959FCC + U16(h, 0x62) * 0x18);
    return *(const u16 *)(D_08960B9C + U16(h, 0x62) * 0x1C);
}

u8 hunter_rank_byte(u8 *p, s32 i)
{
    u32 v = hunter_equip_value(p, i, 1);

    return D_089B65DC[p[i * 0x8C + 0x40]][v & 0xFF];
}

u8 own_rank_byte(u8 *g)
{
    u32 v = own_equip_value(g, 1);

    return D_089B65DC[g[0x4B3]][v & 0xFF];
}

u8 hunter_byte42(u8 *p, s32 i)
{
    return p[i * 0x8C + 0x42];
}

/* linear interpolation in a table of (x, y) pairs ending with x = -1 */
void table_interp(f32 x, void *unused, const f32 *t, f32 *out)
{
    for (;;) {
        f32 a = t[0];

        if (a == -1.0f) {
            *out = t[-1];
            return;
        }
        if (x == a) {
            *out = t[1];
            return;
        }
        if (x > a && x < t[2]) {
            *out = t[1] + ((x - a) / (t[2] - a)) * (t[3] - t[1]);
            return;
        }
        t += 2;
    }
}

/* random numbers in [0, 1.023] and [-1.024, 1.022] */
f32 rand_unit(void)
{
    return 0.001f * (f32)((u16)game_rand(D_08A631A0, 1) & 0x3FF);
}

f32 rand_signed(void)
{
    return 0.002f * (f32)(s32)(((u16)game_rand(D_08A631A0, 1) & 0x3FF) - 0x200);
}
