/* Result screens at the end of the quest (3): hunter rank points, points of the online
 * friends, Felyne comrade experience, training points. The object is D_08A62CB4.
 * (Guessed names.) */
#include "menu.h"

extern u8 *D_08A62CB4;
extern u8 *D_08A62D8C;
extern u8 *D_09A0B544;             /* players data (overlay) */
extern void *D_09A44C5C;
extern void *D_09CFBBC8;
extern const char D_089A891C[];
extern const u16 D_089B53B8, D_089B53C2, D_089B53C4, D_089B53C6, D_089B53C8, D_089B53CC;
extern const u16 D_089B53CE, D_089B53D0, D_089B53D2, D_089B53D4, D_089B53D6, D_089B53E0;
extern const s16 D_089A88F0, D_089A88F2, D_089A88F4, D_089A88F6, D_089A88F8;
extern const s16 D_089A88FC[7], D_089A890C[7];

u32 func_08883988(void *snd, int a, int b, int c, int d, int e, int f, int g, int h);
void func_088854A0(u8 *snd, u32 h, int a);
u32 game_buttons(u8 *g);
void game_add_money2(u8 *g, s32 n);
void menu_begin_draw(void *menu);
void menu_draw_sprite(void *menu, u32 x, u32 y, u32 id);
s32 quest_state(u8 *self, s32 mode);
u8 *quest_text(u8 *self, s32 i);
u32 quest_comrade_allowed(u8 *self);
u8 *friend_marked(u8 *g);
s32 func_088D6A1C(u32 h, int a);
int memcmp(const void *a, const void *b, size_t n);
s32 func_0887E9AC(u8 *a, s32 b, s32 c);
void record_add_best(u8 *g, u8 *log, s32 kind, s32 i, u32 v);
void func_09C267D0(void *a, u8 *f);
void func_08890E08(void *font, int a);
void func_08890DFC(void *font, int x, int y);
void func_08892004(void *font, const char *fmt, ...);
void func_088912BC(void *font, int x, int y, const char *s);
void func_0887D02C(s32 x, s32 y, const char *s, s32 c);
void result_draw_windows(u8 *self);
void result_text_center(u8 *self, s32 y, const char *s);

#define SE(id) func_08883988(D_08A62D8C, 0, id, 0, 0, 0, 0, 0, 0)
#define RTEXT(id) func_09A63198(D_09A44C5C, id)

/* training and tutorial quests: no points */
static int is_training(u32 q)
{
    switch ((u16)q) {
    case 0x3E9: case 0x3EA: case 0x3EB: case 0x44D: case 0x44E: case 0x44F: case 0x7D1:
    case 0x7D2: case 0x7D3: case 0x835: case 0x836: case 0x837: case 0xBB9: case 0xBBA:
    case 0xBBB: case 0xBBC: case 0xBBD: case 0xC1D: case 0xC1E: case 0x2775: case 0x2776:
    case 0x2777: case 0x27D9: case 0x27DA: case 0x283D:
        return 1;
    }
    return 0;
}

static s32 comrade_level(s16 v)
{
    if (v < 1000)
        return 0;
    if (v < 2500)
        return 1;
    if (v < 4500)
        return 2;
    if (v < 6000)
        return 3;
    if (v < 7000)
        return 4;
    return 5;
}

/* points of the quest: hunter rank points, friendship points of the online partners and
 * experience of the Felyne comrade */
void result_points_init(u8 *self)
{
    u8 *q = D_08A62C7C, *g, *s3, *s5, *f, *pl;
    u32 h, pts = 0, old, nw;
    s32 n, k, j;
    u32 a, b, c;
    s32 lv0, lv1;

    U16(self, 0xA4) = 0;
    U16(self, 0xA6) = 0;
    U16(self, 0xAA) = 3600;
    U16(self, 0xA8) = 10;
    if (quest_state(D_08A62C7C, 1) == 1)
        U32(D_08A62CB4, 0x14) = U32((u8 *)U32(D_08A62C7C, 0x4C), 0x3C);
    else
        U32(D_08A62CB4, 0x14) = U32((u8 *)U32(D_08A62C7C, 0x4C), 0x40);
    U32(D_08A62CB4, 0x18) = U32(D_08A62CB4, 0x14);
    h = func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]);
    U32(D_08A62CB4, 0x14) -= ((u8 *)h)[0x113E] * 10;
    if ((s32)U32(D_08A62CB4, 0x14) < 0)
        U32(D_08A62CB4, 0x14) = 0;

    if ((*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) != 0 && quest_state(D_08A62C7C, 1) == 1) {
        n = *(s8 *)(D_09A0B544 + 0xBBD);
        q = D_08A62C7C;
        if ((U32(q, 0x28) & 0x10) != 0)
            pts = ((s32)U32(q, 0x5B0) + (s32)U32(q, 0x5B4)) / 400;
        else if ((U32(q, 0x28) & 0x1000000) != 0)
            pts = U32(self, 0x120) / 200;
        else if (!is_training(U16(D_089CC438, 0x6AF12)))
            pts = U32(D_08A62CB4, 0x14) / 20;
        g = D_089CC438;
        s3 = g + 0x4A0;
        if (n >= 2) {
            s5 = g + 0x4A58;
            for (k = 0; k < 0x60; k++, s5 += 0x1078, s3 += 4) {
                if (U16(s5, 0) == 0)
                    continue;
                for (j = 0; j < n; j++) {
                    pl = D_09A0B544 + j * 200;
                    if (D_089CC438[(u8)j * 0x8C + 0x30] == 0)
                        continue;
                    if (memcmp(s5 + 0x248, pl + 0xBE0 + 0x71, 8) != 0)
                        continue;
                    if (memcmp(s5, pl + 0xBC8, 0x10) != 0)
                        continue;
                    U32(g, 0x67788) += pts;
                    if (U32(g, 0x67788) >= 1000000)
                        U32(g, 0x67788) = 999999;
                    old = U32(s3, 0x69EE4);
                    U32(s3, 0x69EE4) = old + pts;
                    if (old + pts >= 1000000)
                        U32(s3, 0x69EE4) = 999999;
                    nw = U32(s3, 0x69EE4) / 1000;
                    if (old / 1000 != nw) {
                        if (nw != 0 && nw < 6) {
                            D_089CC438[0x6AC3B] += 2;
                            if (D_089CC438[0x6AC3B] >= 100)
                                D_089CC438[0x6AC3B] = 99;
                        } else {
                            D_089CC438[0x6AC3C] += 1;
                            if (D_089CC438[0x6AC3C] >= 100)
                                D_089CC438[0x6AC3C] = 99;
                        }
                    }
                    break;
                }
            }
        }
    }
    if (quest_comrade_allowed(D_08A62C7C) == 0)
        return;
    f = friend_marked(D_089CC438);
    h = func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]);
    if (!is_training(U16(D_089CC438, 0x6AF12))) {
        u32 base = U32(D_08A62CB4, 0x18);

        a = base / 10;
        b = base / 100;
        c = base / 10;
        if (a < 20)
            a = 20;
        if (b < 2)
            b = 2;
        if (c < 20)
            c = 20;
        if (func_088D6A1C(h, 0xBE) != 0) {
            a = a * 15 / 10;
        } else if (func_088D6A1C(h, 0xBF) != 0) {
            a = a * 15 / 10;
            c = c * 15 / 10;
        }
        U16(f, 0x26) += (u16)a;
        if (U16(f, 0x26) > 10000)
            U16(f, 0x26) = 10000;
        U32(f, 0x30) += (u16)b;
        if (U32(f, 0x30) >= 10000)
            U32(f, 0x30) = 9999;
        lv0 = comrade_level(S16(f, 0x2A));
        if (S16(f, 0x2A) < 7000) {
            S16(f, 0x2A) += (s16)(u16)c;
            if (S16(f, 0x2A) >= 7000) {
                S16(f, 0x2A) = 7000;
                if (D_089CC438[0x687CF] < 0xFA)
                    D_089CC438[0x687CF]++;
            }
        }
        lv1 = comrade_level(S16(f, 0x2A));
        if ((u8)lv0 < (u8)lv1) {
            f[0] |= 0x80;
            func_09C267D0(D_09CFBBC8, f);
        }
    }
    if (f[0x4A] < 0xFF)
        f[0x4A]++;
}

/* screen of the points: 1 when finished */
s32 result_points_update(u8 *self)
{
    s32 n;
    u32 h;

    func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]);
    if (S16(self, 0xAA) > 0)
        S16(self, 0xAA)--;
    switch (S16(self, 0xA4)) {
    case 0:
    case 1:
    case 2:
    case 3:
        S16(self, 0xA8)--;
        if (S16(self, 0xA8) < 0) {
            static const u8 se[4] = {0x10, 0x11, 0x11, 0x13};

            n = S16(self, 0xA4);
            S16(self, 0xA4)++;
            U16(self, 0xA8) = 10;
            SE(se[n]);
        }
        break;
    case 4:
        S16(self, 0xA8)--;
        if (S16(self, 0xA8) < 0) {
            S16(self, 0xA4)++;
            U16(self, 0xA8) = 10;
            U16(self, 0xAE) = 0;
        }
        break;
    case 5:
        if ((s32)U32(D_08A62CB4, 0x14) <= 0) {
            func_088854A0(D_08A62D8C, U32(self, 0xC8), 0);
            S16(self, 0xA4)++;
            U16(self, 0xA8) = 10;
            SE(0x19);
            break;
        }
        n = 1;
        if (((u16)game_buttons(D_089CC438) & 0x4000) != 0)
            n = U32(D_08A62CB4, 0x14);
        U32(D_08A62CB4, 0x14) -= n;
        game_add_money2(D_089CC438, n);
        if (S16(self, 0xAE) == 0) {
            h = SE(8);
            U32(self, 0xC8) = h;
            S16(self, 0xAE)++;
        }
        break;
    case 6:
        if (S16(self, 0xAA) > 0 && ((u16)game_buttons(D_089CC438) & 0x4000) == 0)
            return 0;
        S16(self, 0xA4)++;
        SE(0x13);
        return 1;
    default:
        return 0;
    }
    return 0;
}

void result_points_draw(u8 *self)
{
    s16 t[7];
    s32 i, lang;
    u8 *h;
    char buf[0x100];
    s16 x;

    U8(D_089CBBEC, 0x534) = 9;
    func_08890E08(D_08A631A8, 0);
    menu_begin_draw(D_089CBBEC);
    h = (u8 *)func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]);
    result_draw_windows(self);
    func_08890E38(D_08A631A8, 0);
    func_08890DE8(D_08A631A8, 0x10, 0x10);
    for (i = 0; i < 7; i++)
        t[i] = D_089A88FC[i];
    lang = LANG;
    switch (S16(self, 0xA4)) {
    default:
        x = t[lang];
        menu_draw_sprite(D_089CBBEC, (u16)(x + 0x1C), 0xF0, 0x33);
        func_08890DFC(D_08A631A8, (s16)(x + 0x30), 0xF2);
        func_08892004(D_08A631A8, RTEXT(D_089B53B8));
        /* fall through */
    case 4:
    case 5:
        func_0887D02C(0xF4, D_089A88F6, RTEXT(D_089B53CC), 0x10);
        func_08891474(D_08A631A8, 0xF4, D_089A88F6, D_089A891C, U32(self, 0x14));
        /* fall through */
    case 3:
        sprintf(buf, RTEXT(D_089B53CE), -(h[0x113E] * 10));
        func_0887D02C(0xF4, D_089A88F4, buf, 0x10);
        if (h[0x113E] != 0) {
            func_08890DFC(D_08A631A8, D_089A88F8, D_089A88F4);
            func_08892004(D_08A631A8, RTEXT(D_089B53D0), h[0x113E]);
        }
        /* fall through */
    case 2:
        sprintf(buf, RTEXT(D_089B53D6), U32(D_08A62CB4, 0x18));
        func_0887D02C(0xF4, D_089A88F2, buf, 0x10);
        func_08890DFC(D_08A631A8, D_089A88F8, D_089A88F2);
        if (quest_state(D_08A62C7C, 1) == 1)
            func_08892004(D_08A631A8, RTEXT(D_089B53D4));
        else
            func_08892004(D_08A631A8, RTEXT(D_089B53D2));
        /* fall through */
    case 1:
        if (quest_state(D_08A62C7C, 1) == 1)
            sprintf(buf, RTEXT(D_089B53C2));
        else
            sprintf(buf, RTEXT(D_089B53C4));
        result_text_center(self, 0x30, buf);
        result_text_center(self, D_089A88F0, (const char *)quest_text(D_08A62C7C, 0));
        /* fall through */
    case 0:
        func_08890DFC(D_08A631A8, 0xFC, 0xF2);
        func_08892004(D_08A631A8, RTEXT(D_089B53C6), S16(self, 0xAA) / 30);
        for (i = 0; i < 7; i++)
            t[i] = D_089A890C[i];
        x = t[lang] + 0xF4;
        func_0887D02C(x, 0xF, RTEXT(D_089B53C8), 0x10);
        func_088912BC(D_08A631A8, x, 0xF, (const char *)(D_089CC438 + 0x4A0));
        func_0887D02C(0xF4, 0xB2, RTEXT(D_089B53E0), 0x10);
        func_08891474(D_08A631A8, 0xF4, 0xB2, D_089A891C, U32(D_089CC438, 0x696EC));
        break;
    }
}

/* training points of the two hunters (training school), with the record of the quest */
void result_training_init(u8 *self)
{
    u8 *q = D_08A62C7C;

    U16(self, 0xA4) = 0;
    U16(self, 0xA6) = 0;
    U16(self, 0xAA) = 3600;
    U16(self, 0xA8) = 10;
    if (D_089CC438[0x28] == 0) {
        U32(self, 0xB0) = U32(q, 0x5B0);
        U32(self, 0xB4) = U32(D_08A62C7C, 0x5B4);
    } else {
        U32(self, 0xB0) = U32(q, 0x5B4);
        U32(self, 0xB4) = U32(D_08A62C7C, 0x5B0);
    }
    U32(self, 0xBC) = 0;
    U32(self, 0xB8) = 0;
    U32(self, 0x120) = U32(self, 0xB0) + U32(self, 0xB4);
    U32(self, 0x14) = func_0887E9AC(self, U32(self, 0xB0) + U32(self, 0xB4), 0);
    if (U32(D_089CC438, 0x6397C + U16(D_089CC438, 0x6AF12) * 4) < U32(self, 0x120)) {
        U16(self, 0xAC) = 1;
        record_add_best(D_089CC438, D_08A62C7C + 0x684, 10, U32(self, 0x120), 0);
    } else {
        U16(self, 0xAC) = 0;
    }
}

extern u32 D_08A62C6C;             /* buttons pressed */
extern const char D_089A8870[];
extern const u16 D_089B53AE, D_089B53EC, D_089B53F0, D_089B53F4;
extern const u16 D_089AD058[0x30];

void game_add_points(u8 *g, s32 n);
void *memcpy(void *d, const void *s, size_t n);
const char *text_menu(void *g, u32 id);
void gfx_dim_screen(void *menu, int a);
void gfx_draw_window(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 alpha);
void result_init_sounds(u8 *self);

/* training school: the two counters grow together, then the record and the points */
s32 result_training_update(u8 *self)
{
    u8 *src;
    u32 q;
    s32 v;

    if (S16(self, 0xAA) > 0)
        S16(self, 0xAA)--;
    switch (S16(self, 0xA4)) {
    case 0:
        if (--S16(self, 0xA8) <= 0) {
            S16(self, 0xA4)++;
            U16(self, 0xA8) = 10;
            SE(0x10);
        }
        break;
    case 1:
        if (--S16(self, 0xA8) <= 0) {
            S16(self, 0xA4)++;
            if ((s32)U32(self, 0xB4) < (s32)U32(self, 0xB0))
                S16(self, 0xA8) = (s32)U32(self, 0xB0) / 60;
            else
                S16(self, 0xA8) = (s32)U32(self, 0xB4) / 60;
            SE(0x11);
            U16(self, 0xAE) = 0;
        }
        break;
    case 2:
        if (!((s32)U32(self, 0xB8) < (s32)U32(self, 0xB0)) &&
            !((s32)U32(self, 0xBC) < (s32)U32(self, 0xB4))) {
            func_088854A0(D_08A62D8C, U32(self, 0xC8), 0);
            S16(self, 0xA4)++;
            U16(self, 0xA8) = 10;
            SE(0x11);
            return 0;
        }
        U32(self, 0xB8) += S16(self, 0xA8);
        if ((s32)U32(self, 0xB0) < (s32)U32(self, 0xB8))
            U32(self, 0xB8) = U32(self, 0xB0);
        U32(self, 0xBC) += S16(self, 0xA8);
        if ((s32)U32(self, 0xB4) < (s32)U32(self, 0xBC))
            U32(self, 0xBC) = U32(self, 0xB4);
        if (S16(self, 0xAE) == 0) {
            U32(self, 0xC8) = SE(8);
            S16(self, 0xAE)++;
        }
        break;
    case 3:
        if (--S16(self, 0xA8) <= 0) {
            S16(self, 0xA4)++;
            U16(self, 0xA8) = 10;
            SE(0x11);
        }
        break;
    case 4:
        if (--S16(self, 0xA8) > 0)
            break;
        if (S16(self, 0xAC) != 1) {
            S16(self, 0xA4) += 2;
            U16(self, 0xA8) = 10;
            SE(0x10);
            U16(self, 0xAE) = 0;
            break;
        }
        /* new record of the training quest: time and names of the two hunters */
        S16(self, 0xA4)++;
        U16(self, 0xA8) = 0x5A;
        result_init_sounds(self);
        self[0xC1] = 1;
        src = NULL;
        if ((*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) != 0)
            src = D_089CC438[0x28] == 0 ? D_09A0B544 + 0xC90 : D_09A0B544 + 0xBC8;
        v = U32(self, 0x120);
        q = U16(D_089CC438, 0x6AF12);
        if (v >= 1000000)
            v = 999999;
        U32(D_089CC438, 0x6397C + q * 4) = v;
        if (src != NULL) {
            memcpy(D_089CC438 + 0x6781C + (q - 0xFA1) * 0x12, src, 0x12);
            U16(self, 0xAE) = 0;
            return 0;
        }
        D_089CC438[0x55ECA + q * 0x12] = 0;
        U16(self, 0xAE) = 0;
        break;
    case 5:
        S16(self, 0xA8)--;
        if (self[0xC1] == 0 && S16(self, 0xA8) <= 0) {
            S16(self, 0xA4)++;
            U16(self, 0xA8) = 10;
            SE(0x10);
        }
        break;
    case 6:
        if (U32(self, 0x14) == 0) {
            func_088854A0(D_08A62D8C, U32(self, 0xC8), 0);
            S16(self, 0xA4)++;
            SE(0x19);
            break;
        }
        if (S16(self, 0xAE) == 0) {
            U32(self, 0xC8) = SE(8);
            S16(self, 0xAE)++;
        }
        if ((D_08A62C6C & 0x4000) != 0) {
            game_add_points(D_089CC438, U32(self, 0x14));
            U32(self, 0x14) = 0;
        } else {
            game_add_points(D_089CC438, 1);
            U32(self, 0x14)--;
        }
        break;
    case 7:
        if (--S16(self, 0xA8) <= 0) {
            S16(self, 0xA4)++;
            U16(self, 0xA8) = 10;
        }
        break;
    case 8:
        if (S16(self, 0xAA) > 0 && ((u16)game_buttons(D_089CC438) & 0x4000) == 0)
            return 0;
        S16(self, 0xA4)++;
        SE(0x13);
        return 1;
    default:
        return 0;
    }
    return 0;
}

void result_flags_init(u8 *self)
{
    U16(self, 0xA4) = 0;
    U16(self, 0xA6) = 0;
    U16(self, 0xAA) = 900;
    U16(self, 0xA8) = 10;
    result_init_sounds(self);
}

/* screen of the new flags (unlocked items, ...): 1 when closed */
s32 result_flags_update(u8 *self)
{
    if (S16(self, 0xAA) > 0)
        S16(self, 0xAA)--;
    if (S16(self, 0xA4) == 1) {
        if (self[0xC1] != 0)
            return 0;
        if (S16(self, 0xAA) > 0) {
            if (((u16)game_buttons(D_089CC438) & 0x4000) == 0)
                return 0;
            if (S16(self, 0xAA) > 0x32A)
                return 0;
        }
        S16(self, 0xA4)++;
        SE(0x13);
        return 1;
    }
    if (S16(self, 0xA4) != 0)
        return 0;
    if (--S16(self, 0xA8) <= 0)
        S16(self, 0xA4)++;
    return 0;
}

void result_flags_draw(u8 *self)
{
    char buf[0x100];
    s16 n = 0;
    s32 i;
    u32 lo = 1, hi = 0;

    U8(D_089CBBEC, 0x534) = 9;
    func_08890E08(D_08A631A8, 0);
    menu_begin_draw(D_089CBBEC);
    gfx_dim_screen(D_089CBBEC, 0);
    gfx_draw_window(D_089CBBEC, 0x18, 0x30, 0x1B0, 0xB0, 0xFF);
    func_08890E38(D_08A631A8, 0);
    func_08890DE8(D_08A631A8, 0x10, 0x10);
    result_text_center(self, 0x40, RTEXT(D_089B53F0));
    result_text_center(self, 0x60, RTEXT(D_089B53F4));
    for (i = 0; i < 0x30; i++) {
        if ((U32(self, 0xFC) & hi) != 0 || (U32(self, 0xF8) & lo) != 0) {
            result_text_center(self, (s16)((n + 3) * 0x10 + 0x40), text_menu(D_089CC438, D_089AD058[i]));
            n++;
        }
        hi = hi << 1 | lo >> 31;
        lo <<= 1;
    }
    if (self[0xC1] == 0 && S16(self, 0xAA) < 0x32B) {
        sprintf(buf, RTEXT(D_089B53AE));
        func_08890DFC(D_08A631A8, 400, 0xC0);
        func_08892004(D_08A631A8, D_089A8870, buf);
        menu_draw_sprite(D_089CBBEC, 0x17E, 0xBF, 0x33);
    }
    func_08890DFC(D_08A631A8, 0x40, 0xC0);
    func_08892004(D_08A631A8, RTEXT(D_089B53EC), S16(self, 0xAA) / 30);
}
