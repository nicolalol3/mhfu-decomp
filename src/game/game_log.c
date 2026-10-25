/* Hunter log: the last 10 events (0x88 bytes each at g+0x67D98). (Guessed names.) */
#include "menu.h"

extern void *g_equip_data;
extern const s16 D_0899CCFC[20];
void *memcpy(void *d, const void *s, size_t n);
s32 equip_rarity(void *a, u32 b, u32 c);
void log_entry_init(void *g, u8 *e, u32 a, u32 b);

#define LOG(g, i) ((g) + 0x67D98 + (i) * 0x88)

/* copies an entry (the padding bytes 0xB and 0x85..0x87 are not copied) */
static void log_copy(u8 *d, const u8 *s)
{
    memcpy(d, s, 0xB);
    memcpy(d + 0xC, s + 0xC, 0x79);
}

int log_replace(u8 *g, u8 *e);

/* adds the entry e at the top of the log (unless it updates the last one) */
void log_push(u8 *g, u8 *e)
{
    u8 i;

    if (U32(g, 0x67D98) == U32(e, 0) && (u8)log_replace(g, e) == 0)
        return;
    for (i = 9; i != 0; i--)
        log_copy(LOG(g, i), LOG(g, i - 1));
    log_copy(LOG(g, 0), e);
}

/* same day: removes an older entry of the same kind (0 = e is less important, keep the log) */
int log_replace(u8 *g, u8 *e)
{
    u32 i;
    u8 *l;

    if (S16(e, 4) >= 5)
        return 1;
    for (i = 0; i < 10; i++) {
        l = LOG(g, i);
        if (U32(l, 0) != U32(e, 0))
            return 1;
        if (S16(l, 4) < 5 && S16(l, 4) != 3 && S16(l, 6) == S16(e, 6)) {
            if (S16(e, 4) < S16(l, 4))
                return 0;
            break;
        }
    }
    if (i < 10 && S16(LOG(g, i), 4) != 4) {
        for (; i < 9; i++)
            log_copy(LOG(g, i), LOG(g, i + 1));
    }
    return 1;
}

/* counts the monsters hunted; every 10 a log entry */
void log_count_hunt(void *unused, u8 *m)
{
    u8 e[0x88];
    u8 k = equip_rarity(g_equip_data, m[1], U16(m, 2));
    u32 *p = (u32 *)(g_game + 0x6AD7C + k * 4);

    if (*p == 0xFFFFFFFF)
        return;
    *p = *p + 1;
    if (U32(g_game, 0x6AD7C + k * 4) % 10 != 0)
        return;
    log_entry_init(g_game, e, 5, 0);
    U32(e, 0xC) = k;
    U32(e, 0x18) = U32(g_game, 0x6AD7C + k * 4);
    log_push(g_game, e);
}

/* keeps in the quest data the best of the ids of the list D_0899CCFC */
void quest_update_best(u8 *g, s32 id_arg)
{
    s16 id = id_arg;
    u32 cur = 0, n = 0;
    const s16 *l;
    int better;

    if (U32(g, 0x6AF14) & 1)
        return;
    if (D_0899CCFC[0] != 0) {
        for (l = D_0899CCFC; *l != 0; l++, cur++) {
            if (S16(g_quest, 0x682) == *l)
                break;
        }
    }
    if (D_0899CCFC[0] == 0) {
        better = cur != 0;
    } else {
        for (l = D_0899CCFC; *l != 0; l++, n++) {
            if (id == *l)
                break;
        }
        better = n < cur;
    }
    if (better)
        S16(g_quest, 0x682) = id;
}
