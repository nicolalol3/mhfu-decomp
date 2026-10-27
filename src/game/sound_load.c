/* Sound system (2): loading of the sound data of the channels and of the music. The requests
 * are kept in g_sound (kind 0x8B94, step 0x8B98) and served one step per frame by
 * sound_load_step while the loader is free. (Guessed names.) */
#include "quest.h"

extern void **g_loader;
extern u8 *g_monster_models;
extern u8 *D_08A62D84;            /* the sound bank manager (sound.c) */
extern u8 *D_08A62D94;            /* the music manager */
extern u32 D_089AC1A8[];          /* sizes of the music data: 21 words per music set */
extern const u8 D_089AC3F4[];     /* maps with their own music: 16 bytes per map */
extern u32 D_089AC244[3];
extern const u32 D_089AC3F8[3];

typedef s32 (*LdBusy)(void *self);
typedef s32 (*LdBusy2)(void *self, int a);
typedef s32 (*LdLoad)(void *self, u32 id, u32 dest, s32 size, int a, int b, int c);
typedef s32 (*LdSize)(void *self, u32 id);
#define LOADER(off, T) ((T)((void **)g_loader[0])[(off) / 4])
#define LD_LOAD(id, dest, size, a) LOADER(0x34, LdLoad)(g_loader, id, dest, size, a, 0, 1)
#define LD_SIZE(id) LOADER(0x18, LdSize)(g_loader, id)

u8 sndbank_is_playing(u8 *self, s32 i);
u16 sndbank_get_a(u8 *self, s32 i);
u16 sndbank_get_b(u8 *self, s32 i);
u16 sndbank_get_c(u8 *self, s32 i);
void sndbank_set_a(u8 *self, s32 i, u16 v);
void sndbank_set_b(u8 *self, s32 i, u16 v);
void sndbank_set_c(u8 *self, s32 i, u16 v);
u32 sndbank_get_w0(u8 *self, s32 i);
u32 sndbank_get_w1(u8 *self, s32 i);
u32 sndbank_get_w2(u8 *self, s32 i);
void sndbank_stop_ch(u8 *self, u32 h, s32 now);
void sndbank_clear_abc(u8 *self, s32 i);
u32 sndbank_data_size(u8 *self, u32 h);
s32 sndbank_slot_play(u8 *self, s32 i, u32 a, u32 b);
s32 sound_scale7(u8 *self, s32 a, s32 v);
s32 func_088DE014(u8 *models, u32 id);
s32 func_088DDD9C(u8 *models, u32 dest, s32 idx, s32 size);
void func_0888334C(u8 *self, int a, u32 ch, u32 size);
void func_088832F8(u8 *self, int a, u32 ch0, u32 ch1);
u32 func_0888670C(u8 *mus, s32 slot);
void func_0888664C(u8 *mus, s32 slot);
void func_088865B0(u8 *mus, s32 slot);
void func_08886638(u8 *mus, s32 slot, u32 id, u32 size);
u32 func_08886664(u8 *mus, s32 slot);
void func_08886618(u8 *mus, s32 i, u8 *dest, u32 size);
void func_08886850(u8 *mus, s32 i, s32 v);
void func_08886870(u8 *mus, s32 i, u32 a, s32 v);
u32 func_0888671C(u8 *mus, s32 i);
u32 func_08886734(u8 *mus, s32 i);
void func_08882D9C(u8 *self, s32 i, u32 a, s32 v, int b);

void sound_req_done(u8 *self);
void music_slot_clear(u8 *self, s32 i);
void music_set_size(u8 *self, s32 slot, u32 size);
u32 music_size(u8 *self, s32 slot, u32 id);
void music_place(u8 *self, s32 kind);

/* the table of channel requests (0x8B64): a[4], c[4], b[4], channel[4], extra[4] */
#define RQ_A(i)  U16(self, 0x8B64 + (i) * 2)
#define RQ_C(i)  U16(self, 0x8B6C + (i) * 2)
#define RQ_B(i)  U16(self, 0x8B74 + (i) * 2)
#define RQ_CH(i) S16(self, 0x8B7C + (i) * 2)
#define REQ_KIND U32(self, 0x8B94)
#define REQ_STEP U32(self, 0x8B98)
#define BANK D_08A62D84

/* the entries of req already in the sounds list `which` are set to -1; 1 if any */
u32 sound_drop_loaded(u8 *self, s16 *req, s8 which)
{
    s16 *l;
    s32 i, j;
    u32 r = 0;

    if (which == 2)
        l = &S16(self, 0x56244C);
    else if (which == 1)
        l = &S16(self, 0x562444);
    else
        l = &S16(self, 0x56243C);
    for (i = 0; i < 4; i++)
        for (j = i + 1; j < 4; j++)
            if (l[i] == l[j] && req[j] != -1) {
                req[j] = -1;
                r = 1;
            }
    return r;
}

static int channel_busy(s32 ch)
{
    int busy = 0;

    if (sndbank_is_playing(BANK, ch) == 1)
        return 1;
    if (sndbank_get_a(BANK, ch) != 0xFFFF)
        busy = 1;
    if (sndbank_get_c(BANK, ch) != 0xFFFF)
        busy = 1;
    if (sndbank_get_b(BANK, ch) != 0xFFFF)
        busy = 1;
    return busy;
}

/* sounds of the monsters of the quest (from the first not loaded to n) */
void sound_request_monsters(u8 *self, u32 *ch, u16 *a, u16 *c, u16 *b, u8 *e, s32 n, u32 unused,
                            u32 *min)
{
    s32 i;
    u32 sz;

    for (i = QUEST->main_models; i < n; i++) {
        if (channel_busy(ch[i])) {
            sndbank_stop_ch(BANK, ch[i], 0);
            sndbank_clear_abc(BANK, ch[i]);
        }
        sz = LD_SIZE(c[i]);
        if (min != NULL && sz < min[i])
            sz = min[i];
        func_0888334C(self, 1, ch[i], sz);
        if (REQ_KIND == 0) {
            RQ_A(i) = a[i];
            RQ_C(i) = c[i];
            RQ_B(i) = b[i];
            RQ_CH(i) = ch[i];
            self[0x8B84 + i] = e[i];
        }
    }
    if (REQ_KIND == 0 && n > 0) {
        REQ_KIND = 4;
        U32(self, 0x8B88) = n;
        U32(self, 0x8B8C) = QUEST->main_models;
        func_088832F8(self, 1, ch[QUEST->main_models], ch[n - 1]);
    }
}

void sound_request_one(u8 *self, s32 ch, u16 a, u16 c, u16 b, u8 e, s32 i)
{
    if (channel_busy(ch)) {
        sndbank_stop_ch(BANK, ch, 0);
        sndbank_clear_abc(BANK, ch);
    }
    if (REQ_KIND == 0) {
        RQ_A(i) = a;
        RQ_C(i) = c;
        RQ_B(i) = b;
        RQ_CH(i) = ch;
        self[0x8B84 + i] = e;
    }
    if (REQ_KIND == 0) {
        REQ_KIND = 5;
        U32(self, 0x8B90) = i;
    }
}

/* music of the slot (flag 1: the 3 slots from slot) */
u32 music_request(u8 *self, s32 slot, u32 id, s32 flag, u8 x)
{
    s32 i;

    if (id == (u16)func_0888670C(D_08A62D94, slot)) {
        id = 0xFFFF;
    } else if (flag == 1) {
        music_set_size(self, 0x13, 0);
        music_set_size(self, 0x14, 0);
        for (i = 0; i < 3; i++) {
            music_slot_clear(self, i);
            func_0888664C(D_08A62D94, slot + i);
        }
    } else {
        func_0888664C(D_08A62D94, slot);
    }
    if (REQ_KIND != 0)
        return 0;
    S16(self, 0x8B2E) = slot;
    S16(self, 0x8B2C) = id;
    self[0x8B30] = x;
    REQ_KIND = 1;
    if (flag == 1)
        music_place(self, 2);
    return 1;
}

void music_stop_all(u8 *self)
{
    s32 i;

    for (i = 0; i < 3; i++) {
        music_slot_clear(self, i);
        func_088865B0(D_08A62D94, i + 0x12);
    }
}

void music_slot_clear(u8 *self, s32 i)
{
    u16 *p = &U16(self, 0x8B54 + i * 2);

    p[0] = 0xFFFF;
    p[3] = 0;
}

/* up to 3 musics in the slots from slot (0xFFFF: none) */
u32 music_request3(u8 *self, s32 slot, u32 a, u32 b, u32 c, u8 x)
{
    u32 l[3];
    u16 *p = &U16(self, 0x8B54);
    s32 i, ch;

    if (a - 0x19BE < 2) {
        music_set_size(self, 0x14, 0);
        c = 0xFFFF;
    }
    l[0] = a;
    l[1] = b;
    l[2] = c;
    U32(self, 0x562434) = 0;
    for (i = 0; i < 3; i++)
        if (l[U32(self, 0x562434)] != 0xFFFF)
            U32(self, 0x562434)++;
    if (U32(self, 0x562434) == 0)
        U32(self, 0x562434) = 3;
    for (i = 0; i < (s32)U32(self, 0x562434); i++, p++) {
        music_slot_clear(self, i);
        ch = slot + i;
        if (l[i] != (u16)func_0888670C(D_08A62D94, ch))
            func_0888664C(D_08A62D94, ch);
        if (REQ_KIND == 0) {
            p[3] = ch;
            p[0] = l[i];
            if (l[i] != 0xFFFF)
                music_set_size(self, ch, music_size(self, ch, l[i]));
        }
    }
    self[0x8B60] = x;
    if (REQ_KIND != 0)
        return 0;
    REQ_KIND = 6;
    if (slot == 0x12)
        music_place(self, 2);
    else if (slot == 0x13)
        music_place(self, 3);
    else if (slot == 0x14)
        music_place(self, 4);
    return 1;
}

/* size of the music id (the maps of D_089AC3F4 use a fixed size) */
u32 music_size(u8 *self, s32 slot, u32 id)
{
    s32 i, found = 0;
    u32 v = 0;

    if (id == 0x19C4)
        return LD_SIZE(0x19C4);
    if (id == 0x19C3)
        return LD_SIZE(0x19C3);
    if (id == 0x19BF)
        return LD_SIZE(0x19BF);
    if (id == 0x19BE)
        return LD_SIZE(0x19BE);
    for (i = 0; i < 6; i++) {
        if (U32(D_089AC3F4, i * 16) == G_MAP) {
            v = U32(D_089AC3F4, i * 16 + slot * 4 - 0x44);
            found = 1;
        }
    }
    if (found && G_ARENA == 0)
        return v;
    return LD_SIZE(id & 0xFFFF);
}

void music_set_size(u8 *self, s32 slot, u32 size)
{
    D_089AC1A8[slot + (s32)U32(self, 0x308BC0) * 21] = size;
}

void sound_req_done(u8 *self)
{
    REQ_KIND = 0;
    REQ_STEP = 0;
}

void sound_req_step_change(u8 *self);
void sound_req_step_load(u8 *self);
void sound_req_step_monsters(u8 *self);
void sound_req_step_one(u8 *self);
void music_req_step(u8 *self);
void music_req_step3(u8 *self);

/* serves the pending request, one step per frame while the loader is free */
void sound_load_step(u8 *self)
{
    if ((u8)LOADER(0x40, LdBusy2)(g_loader, 2) == 0) {
        if (REQ_KIND == 6)
            music_req_step3(self);
        else if (REQ_KIND == 1)
            music_req_step(self);
    }
    if ((u8)LOADER(0x38, LdBusy)(g_loader) == 0 && REQ_KIND != 0) {
        if (REQ_KIND == 5)
            sound_req_step_one(self);
        else if (REQ_KIND == 4)
            sound_req_step_monsters(self);
        else if (REQ_KIND == 3)
            sound_req_step_load(self);
        else if (REQ_KIND == 2)
            sound_req_step_change(self);
    }
}

u32 sound_loading(u8 *self)
{
    if ((u8)LOADER(0x38, LdBusy)(g_loader) == 1)
        return 1;
    return REQ_KIND != 0;
}

u32 music_loading(u8 *self)
{
    if ((u8)LOADER(0x40, LdBusy2)(g_loader, 2) == 1)
        return 1;
    return REQ_KIND != 0;
}

/* loads the data id to dest: from the monster models if there, otherwise from the disc */
static void load_to(u32 id, u32 dest, s32 size)
{
    s32 k = func_088DE014(g_monster_models, id);

    if (k == -1)
        LD_LOAD(id, dest, size, 0);
    else
        func_088DDD9C(g_monster_models, dest, k, size);
}

/* request 2 (sound_request_change): a 0x8B2C, c 0x8B2E, b 0x8B30, channel 0x8B32 */
void sound_req_step_change(u8 *self)
{
    s16 ch = S16(self, 0x8B32);
    u32 sz;

    if (REQ_STEP == 2) {
        if (S16(self, 0x8B2E) != -1)
            sndbank_set_c(BANK, ch, U16(self, 0x8B2E));
        sound_req_done(self);
    } else if (REQ_STEP == 1) {
        if (S16(self, 0x8B2C) != -1)
            sndbank_set_a(BANK, ch, U16(self, 0x8B2C));
        if (S16(self, 0x8B30) != -1)
            sndbank_set_b(BANK, ch, U16(self, 0x8B30));
        if (S16(self, 0x8B2E) != -1) {
            sz = sndbank_data_size(BANK, sndbank_get_w0(BANK, ch));
            load_to(U16(self, 0x8B2E), sndbank_get_w2(BANK, ch), sz);
        }
        REQ_STEP++;
    } else if (REQ_STEP == 0) {
        if (S16(self, 0x8B2C) != -1)
            load_to(U16(self, 0x8B2C), sndbank_get_w0(BANK, ch), -1);
        if (S16(self, 0x8B30) != -1)
            LD_LOAD(U16(self, 0x8B30), sndbank_get_w1(BANK, ch), -1, 0);
        REQ_STEP++;
    }
}

/* request 3 (sound_request_load): a[4] 0x8B2C, b[4] 0x8B34, k 0x8B3C, channel 0x8B3E,
 * n 0x8B40, addresses 0x8B44 */
void sound_req_step_load(u8 *self)
{
    s16 ch = S16(self, 0x8B3E);
    u32 *dst = &U32(self, 0x8B44);
    u32 d, sz;
    s32 i, k;

    if (REQ_STEP == 3) {
        sndbank_set_b(BANK, ch, U16(self, 0x8B3C));
        sound_req_done(self);
    } else if (REQ_STEP == 2) {
        sndbank_set_c(BANK, ch, U16(self, 0x8B34));
        load_to(U16(self, 0x8B3C), sndbank_get_w1(BANK, ch), -1);
        REQ_STEP++;
    } else if (REQ_STEP == 1) {
        sndbank_slot_play(BANK, ch, (u32)dst, U32(self, 0x8B40));
        sndbank_set_a(BANK, ch, U16(self, 0x8B2C));
        d = dst[0];
        for (i = 0; i < (s32)U32(self, 0x8B40); i++) {
            sz = sndbank_data_size(BANK, dst[i]);
            k = func_088DE014(g_monster_models, U16(self, 0x8B34 + i * 2));
            if (k == -1) {
                LD_LOAD(U16(self, 0x8B34 + i * 2), d, sz, 0);
                d += sz;
            } else {
                d += func_088DDD9C(g_monster_models, d, k, -1);
            }
        }
        REQ_STEP++;
    } else if (REQ_STEP == 0) {
        d = sndbank_get_w2(BANK, ch);
        for (i = 0; i < (s32)U32(self, 0x8B40); i++) {
            k = func_088DE014(g_monster_models, U16(self, 0x8B2C + i * 2));
            if (k == -1) {
                LD_LOAD(U16(self, 0x8B2C + i * 2), d, -1, 0);
                dst[i] = d;
                d += LD_SIZE(U16(self, 0x8B2C + i * 2));
            } else {
                dst[i] = d;
                d += func_088DDD9C(g_monster_models, d, k, -1);
            }
        }
        REQ_STEP++;
    }
}

/* request 4 (sound_request_monsters) */
void sound_req_step_monsters(u8 *self)
{
    s32 i = 0;
    u32 sz;

    if (U32(self, 0x8B8C) != 0)
        i = QUEST->main_models;
    if (REQ_STEP == 2) {
        for (; i < (s32)U32(self, 0x8B88); i++)
            if ((s16)RQ_C(i) != -1)
                sndbank_set_c(BANK, RQ_CH(i), RQ_C(i));
        sound_req_done(self);
    } else if (REQ_STEP == 1) {
        for (; i < (s32)U32(self, 0x8B88); i++) {
            if ((s16)RQ_A(i) != -1)
                sndbank_set_a(BANK, RQ_CH(i), RQ_A(i));
            if ((s16)RQ_B(i) != -1)
                sndbank_set_b(BANK, RQ_CH(i), RQ_B(i));
            sz = sndbank_data_size(BANK, sndbank_get_w0(BANK, RQ_CH(i)));
            if ((s16)RQ_C(i) != -1)
                load_to(RQ_C(i), sndbank_get_w2(BANK, RQ_CH(i)), sz);
        }
        REQ_STEP++;
    } else if (REQ_STEP == 0) {
        for (; i < (s32)U32(self, 0x8B88); i++) {
            if ((s16)RQ_A(i) != -1)
                load_to(RQ_A(i), sndbank_get_w0(BANK, RQ_CH(i)), -1);
            if ((s16)RQ_B(i) != -1)
                load_to(RQ_B(i), sndbank_get_w1(BANK, RQ_CH(i)), -1);
        }
        REQ_STEP++;
    }
}

/* request 5 (sound_request_one): entry 0x8B90 of the table */
void sound_req_step_one(u8 *self)
{
    s32 i = U32(self, 0x8B90);
    u32 sz;

    if (REQ_STEP == 2) {
        if ((s16)RQ_C(i) != -1)
            sndbank_set_c(BANK, RQ_CH(i), RQ_C(i));
        sound_req_done(self);
    } else if (REQ_STEP == 1) {
        if ((s16)RQ_A(i) != -1)
            sndbank_set_a(BANK, RQ_CH(i), RQ_A(i));
        if ((s16)RQ_B(i) != -1)
            sndbank_set_b(BANK, RQ_CH(i), RQ_B(i));
        sz = sndbank_data_size(BANK, sndbank_get_w0(BANK, RQ_CH(i)));
        if ((s16)RQ_C(i) != -1)
            LD_LOAD(RQ_C(i), sndbank_get_w2(BANK, RQ_CH(i)), sz, 0);
        REQ_STEP++;
    } else if (REQ_STEP == 0) {
        if ((s16)RQ_A(i) != -1)
            LD_LOAD(RQ_A(i), sndbank_get_w0(BANK, RQ_CH(i)), -1, 0);
        if ((s16)RQ_B(i) != -1)
            LD_LOAD(RQ_B(i), sndbank_get_w1(BANK, RQ_CH(i)), -1, 0);
        REQ_STEP++;
    }
}

/* request 1 (music_request): id 0x8B2C, slot 0x8B2E */
void music_req_step(u8 *self)
{
    s16 id = S16(self, 0x8B2C);
    s32 k;

    if (REQ_STEP == 1) {
        func_08886638(D_08A62D94, S16(self, 0x8B2E), (u16)id, LD_SIZE((u16)id));
        sound_req_done(self);
    } else if (REQ_STEP == 0) {
        if (id == -1) {
            sound_req_done(self);
            return;
        }
        k = func_088DE014(g_monster_models, (u16)id);
        if (k == -1)
            LD_LOAD((u16)id, func_08886664(D_08A62D94, S16(self, 0x8B2E)), -1, self[0x8B30]);
        else
            func_088DDD9C(g_monster_models, func_08886664(D_08A62D94, S16(self, 0x8B2E)), k, -1);
        REQ_STEP++;
    }
}

/* request 6 (music_request3): ids 0x8B54, slots 0x8B5A */
void music_req_step3(u8 *self)
{
    s16 *p = &S16(self, 0x8B54);
    s32 i, k;

    if (REQ_STEP == 1) {
        for (i = 0; i < (s32)U32(self, 0x562434); i++, p++)
            if (p[0] != -1)
                func_08886638(D_08A62D94, p[3], (u16)p[0], music_size(self, p[3], (u16)p[0]));
        sound_req_done(self);
    } else if (REQ_STEP == 0) {
        for (i = 0; i < (s32)U32(self, 0x562434); i++, p++) {
            if (p[0] == -1) {
                sound_req_done(self);
                continue;
            }
            k = func_088DE014(g_monster_models, (u16)p[0]);
            if (k == -1)
                LD_LOAD((u16)p[0], func_08886664(D_08A62D94, p[3]), -1, self[0x8B60]);
            else
                func_088DDD9C(g_monster_models, func_08886664(D_08A62D94, p[3]), k, -1);
        }
        REQ_STEP++;
    }
}

/* gives to the music player the memory of each music of the set (from the slot of kind) */
void music_place(u8 *self, s32 kind)
{
    s32 first, i;
    u8 *dest = self + 0x8BC0;
    u32 sz;

    if (kind == 0)
        first = 0;
    else if (kind == 1)
        first = 6;
    else if (kind == 2)
        first = 0x12;
    else if (kind == 3)
        first = 0x13;
    else if (kind == 4)
        first = 0x14;
    else
        first = 0;
    if ((s32)U32(self, 0x308BC0) == -1)
        return;
    for (i = 0; i < 0x15; i++) {
        sz = D_089AC1A8[(s32)U32(self, 0x308BC0) * 21 + i];
        if (sz == 0)
            return;
        if (i >= first) {
            func_08886618(D_08A62D94, i, dest, sz);
            if (U32(self, 0x562434) != 3 && U32(self, 0x562434) == (u32)(i - first + 1))
                return;
        }
        dest += sz;
    }
}

void music_select_set(u8 *self, s32 set, s32 kind)
{
    U32(self, 0x562434) = 0;
    D_089AC244[0] = D_089AC3F8[0];
    D_089AC244[1] = D_089AC3F8[1];
    D_089AC244[2] = D_089AC3F8[2];
    if ((s32)U32(self, 0x308BC0) != set) {
        U32(self, 0x308BC0) = set;
        music_place(self, kind);
    }
}

#define VOL(i) ((i) == 0 ? U32(self, 0x5623D4) : U32(self, 0x5623D0))

void music_set_volume(u8 *self, s32 i, s32 v)
{
    func_08886850(D_08A62D94, i, sound_scale7(self, v, VOL(i)));
}

u32 music_state(u8 *self, s32 i)
{
    return func_0888671C(D_08A62D94, i);
}

void music_fade(u8 *self, s32 i, u32 t)
{
    func_08886870(D_08A62D94, i, t, 0);
}

void music_fade_both(u8 *self, u32 t)
{
    func_08886870(D_08A62D94, 0, t, 0);
    func_08886870(D_08A62D94, 1, t, 0);
}

void music_fade_full(u8 *self, s32 i, u32 t)
{
    func_08886870(D_08A62D94, i, t, sound_scale7(self, 0x7F, VOL(i)));
}

void music_fade_to(u8 *self, s32 i, u32 t, s32 v)
{
    func_08886870(D_08A62D94, i, t, sound_scale7(self, v, VOL(i)));
}

u32 music_fading(u8 *self, s32 i)
{
    return func_08886734(D_08A62D94, i);
}

u16 music_current(u8 *self, s32 slot)
{
    return func_0888670C(D_08A62D94, slot);
}

void music_play_full(u8 *self, s32 i, u32 a)
{
    func_08882D9C(self, i, a, sound_scale7(self, 0x7F, VOL(i)), 0);
}
