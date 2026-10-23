/* HUD drawn by the menu class during the game (status of the hunter, quest timer,
 * on-line notices). Same class as menu_box.c. (Guessed names.) */
#include "menu.h"
#include "vfpu.h"

extern void *D_08A62D8C;          /* sound */
extern u32 D_08A62C6C;            /* buttons pressed in this frame */
extern u8 *D_08A62C60;
extern const u16 D_09B04DE0, D_09B04DF6, D_09B04DF8;   /* overlay text ids */
extern const char D_0893B220[], D_0893B224[];

void func_08883988(void *snd, int a, int b, int c, int d, int e, int f, int g, int h);
void func_08835470(void *self, int layer);
void func_08835F7C(void *self, int layer);
void func_088378A8(void *self, int layer);
void func_0883D0EC(void *self);
s32 func_08838AB4(void *self, u8 a, u8 b);
void func_08890E08(void *font, int v);
const char *func_09A7B790(void *a, u16 id);
void hud_draw_label_bg(u8 *self, u32 x_arg, s32 y_arg, s32 w_arg, s32 h_arg, u32 c, u32 flags_arg);
#define func_08838B08 hud_draw_label_bg
void func_08838E0C(void *self, int x, int y, int id, s32 t, int n);
void menu_begin_draw(void *self);
void hud_draw_online(u8 *self, s32 layer);

/* hides / shows the HUD with the button 0x100 (on-line only) */
static void hud_toggle(u8 *self)
{
    if ((*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) == 1 && (D_08A62C6C & 0x100) != 0) {
        if (U32(self, 0x528) & 0x2000) {
            U32(self, 0x528) &= ~0x2000;
            func_08883988(D_08A62D8C, 0, 17, 0, 0, 0, 0, 0, 0);
        } else {
            U32(self, 0x528) |= 0x2000;
            func_08883988(D_08A62D8C, 0, 20, 0, 0, 0, 0, 0, 0);
        }
    }
}

/* HUD of the frame, depending on the state of the game */
void hud_draw(u8 *self)
{
    s32 *st = (s32 *)D_09B22D70;

    switch (st[0]) {
    case 20:
        func_08835470(self, 0);
        func_08835F7C(self, 1);
        func_088378A8(self, 2);
        break;
    case 8:
        hud_toggle(self);
        func_08835470(self, 0);
        hud_draw_online(self, 1);
        func_088378A8(self, 2);
        break;
    case 0:
        ((u8 *)st)[8] = 0;
        /* fall through */
    case 22:
        hud_toggle(self);
        func_08835470(self, 0);
        func_08835F7C(self, 1);
        func_088378A8(self, 2);
        break;
    case 12:
        if (D_08A62C60[0x74] != 1)
            func_08835470(self, 0);
        break;
    }
    func_0883D0EC(self);
}

/* notice of the on-line state (connection, or players waiting) */
void hud_draw_online(u8 *self, s32 layer)
{
    u8 *h = (u8 *)func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]);
    s16 w;

    if (h[0x299] != 16 && (u8)func_08838AB4(self, h[0x298], h[0x299]) == 1) {
        self[0x535] = 40;
        return;
    }
    switch (layer) {
    case 0: self[0x534] = 9; break;
    case 1: self[0x534] = 10; break;
    case 2: self[0x534] = 11; break;
    case 3: self[0x534] = 12; break;
    case 4: self[0x534] = 13; break;
    case 5: self[0x534] = 14; break;
    }
    func_08890E08(D_08A631A8, (s8)layer);
    menu_begin_draw(self);
    func_08890E38(D_08A631A8, 0);
    func_08890DE8(D_08A631A8, 14, 14);
    if (U32(h, 0x554) == 0 || S16(*(u8 **)(h + 0x554), 2) != 2) {
        self[0x535] = 40;
        func_08891474(D_08A631A8, 36, 18, D_0893B220, func_09A7B790(D_09B22D70, D_09B04DF6));
        w = func_08890E48(D_08A631A8, func_09A7B790(D_09B22D70, D_09B04DF6));
        func_08838B08(self, 0, 16, (s16)((w + 2) * 7 + 36), 16, 0x801D3905, 0);
    } else {
        s8 t = self[0x535] - 1;

        self[0x535] = t;
        if (t < 1)
            self[0x535] = 40;
        func_08890DFC(D_08A631A8, 36, 18);
        func_08891474(D_08A631A8, 36, 18, D_0893B224, func_09A7B790(D_09B22D70, D_09B04DE0),
                      func_09A7B790(D_09B22D70, D_09B04DF8));
        w = func_08890E48(D_08A631A8, func_09A7B790(D_09B22D70, D_09B04DF8));
        func_08838B08(self, 0, 16, (s16)((w + 4) * 7 + 36), 16, 0x801D3905, 0);
        func_08838E0C(self, 16, 16, 53, (s8)self[0x535], 20);
    }
}

extern const u8 D_0893B234[20];    /* box of the player list title */
extern const Layout D_0893B248;    /* box of the player list */
extern const u16 D_09B05C90, D_09B05C92;
extern u8 *D_09A0B544;            /* players data (overlay) */
void func_0882FC50(void *self, const void *layout, const char *text, int alpha);
void func_0882A9DC(void *self, u16 x, u16 y, int id);
s32 func_0883B498(void *self, u16 id);
void func_0883B53C(void *self, s32 x, s32 y, s32 i, int a);
void *func_0889C518(void *a);
int memcmp(const void *a, const void *b, size_t n);
s32 func_0883D3D4(void *self);
void func_0883D290(void *self);

/* list of the players of the on-line room (hidden with the button 0x100) */
void hud_draw_players(u8 *self, s32 layer)
{
    u32 t[5];
    Layout l;
    const u32 *src = (const u32 *)D_0893B234;
    u8 *h;
    u32 u;
    s16 k;

    if ((*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) == 0)
        return;
    switch (layer) {
    case 0: self[0x534] = 9; break;
    case 1: self[0x534] = 10; break;
    case 2: self[0x534] = 11; break;
    case 3: self[0x534] = 12; break;
    case 4: self[0x534] = 13; break;
    case 5: self[0x534] = 14; break;
    }
    func_08890E08(D_08A631A8, (s8)layer);
    menu_begin_draw(self);
    t[0] = src[0];
    t[1] = src[1];
    t[2] = src[2];
    t[3] = src[3];
    t[4] = src[4];
    if (U32(self, 0x528) & 0x2000) {
        func_0882FC50(self, t, func_09A7B790(D_09B22D70, D_09B05C90), 255);
        func_0882A9DC(self, (u16)(U16(t, 12) - 24), (u16)(U16(t, 14) - 2), 54);
        return;
    }
    func_0882FC50(self, t, func_09A7B790(D_09B22D70, D_09B05C92), 255);
    func_0882A9DC(self, (u16)(U16(t, 12) - 24), (u16)(U16(t, 14) - 2), 54);
    l.x = D_0893B248.x;
    l.y = D_0893B248.y;
    l.w = D_0893B248.w;
    l.h = D_0893B248.h;
    l.cw = D_0893B248.cw;
    l.b9 = D_0893B248.b9;
    l.ba = D_0893B248.ba;
    l.bb = D_0893B248.bb;
    l.tx = D_0893B248.tx;
    l.ty = D_0893B248.ty;
    l.lh = D_0893B248.lh;
    l.e = D_0893B248.e;
    l.ptr = D_0893B248.ptr;
    l.color = D_0893B248.color;
    l.f[0] = D_0893B248.f[0];
    l.f[1] = D_0893B248.f[1];
    l.f[2] = D_0893B248.f[2];
    l.f[3] = D_0893B248.f[3];
    l.f[4] = D_0893B248.f[4];
    l.f[5] = D_0893B248.f[5];
    l.e = (s8)D_09A0B544[0xEE3];
    l.h = (u16)l.h - (4 - (s8)D_09A0B544[0xEE3]) * 16;
    func_0882F580(self, &l, 0, -1, 255, 0);
    h = (u8 *)func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]);
    if (func_0883B498(self, U16(h, 0x1E6)) != -1)
        func_0883B53C(self, l.tx, l.ty, func_0883B498(self, U16(h, 0x1E6)), 0);
    k = 1;
    for (u = 0; (s32)u < (s8)D_09A0B544[0xEE3]; u++) {
        u8 *p = D_09A0B544;

        if (memcmp(func_0889C518(D_09A0B544), p + (u & 0xFF) * 200 + 0x57C, 6) != 0) {
            func_0883B53C(self, l.tx, (s16)(l.ty + l.lh * k), u, 0);
            k++;
        }
    }
}

/* notice shown for 150 frames when the flag 0x80 is set */
void hud_update_notice(u8 *self)
{
    if ((U32(self, 0x5E8) & 0x80) != 0 && (u8)func_0883D3D4(self) == 1) {
        U32(self, 0x600) = 150;
        U32(self, 0x5E8) &= ~0x80;
    }
    if ((u8)func_0883D3D4(self) == 0)
        U32(self, 0x600) = 0;
    if ((s32)U32(self, 0x600) > 0) {
        func_0883D290(self);
        U32(self, 0x600) -= 1;
    }
}

u8 *func_0885CA50(void *a, void *eq);
extern void *D_08A62C54;

/* 1 if the armor piece eq can be worn by the current hunter (gender, class) */
int hud_armor_wearable(void *self, u8 *eq)
{
    u8 *g = D_089CC438;
    u8 *a;
    u8 m;

    if ((u32)(eq[1] - 5) < 2)
        return 1;
    a = func_0885CA50(D_08A62C54, eq);
    m = g[0x4B3] == 0 ? 1 : 2;
    if ((a[4] & m) == 0)
        return 0;
    m = g[U16(g, 0x44C0) * 0xC + 0x549] == 5 ? 4 : 8;
    return (a[4] & m) != 0;
}

extern void *D_09CFB898, *D_09CFB798, *D_09B0FF88, *D_09CFBBF0;
extern const u16 D_09B05238, D_09B0523A, D_09B0523C, D_09B0523E, D_09B05240, D_09B05242, D_09B05244;
extern const u16 D_09B05246, D_09B05248, D_09B0524A, D_09B0524C, D_09B0524E, D_09B05250, D_09B05252;
extern const u16 D_09B05254, D_09B05256, D_09B05258, D_09B0525A, D_09B0525C, D_09B0525E, D_09B05260;
extern const u16 D_09B05262, D_09B05264, D_09B05266, D_09B05268, D_09B0526A, D_09B0526C, D_09B0526E;
extern const u16 D_09B05270, D_09B05272, D_09B05274, D_09B05276, D_09B05278, D_09B05284;
extern const u16 D_09B04E08[];
extern const char D_0893B22C[];
s16 *func_09A7B7D0(void *a, u16 id);
u8 func_09C1B868(void *a);
u8 func_09C1B888(void *a);
u8 func_09C19580(void *a, int b);
u8 func_09A62E58(void *a, u8 b);
u8 func_09A7BFA8(void *a);
u8 func_0885057C(void *game);
s32 func_09C294B8(void *a);
u8 func_09C294F8(void *a, u16 b);
u8 func_09C29548(void *a, u16 b);
u8 func_09C29598(void *a);
u8 func_09C295E0(void *a);
u8 func_09C29688(void *a);
u8 func_09C29630(void *a, u16 b);

#define TXT(id) func_09A7B790(D_09B22D70, (id))
/* objective line: "<mark> <text id>" at (36, 18); name = its text (for the width) */
#define OBJ_LINE(id) do {                                                               \
        func_08891474(D_08A631A8, 36, 18, D_0893B224, TXT(D_09B04DE0), TXT(id));         \
        name = TXT(id);                                                                 \
    } while (0)
#define OBJ_LINE_HERE(id) do {                                                          \
        func_0889134C(D_08A631A8, D_0893B224, TXT(D_09B04DE0), TXT(id));                 \
        name = TXT(id);                                                                 \
    } while (0)

/* box with the objective of the current quest (top left of the screen) */
void hud_draw_objective(u8 *self, s32 layer)
{
    u8 *h = (u8 *)func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]);
    const char *name = NULL;
    s32 w = 120;
    u16 *q;
    s16 *p;
    s16 k;

    if ((h[0x299] != 16 && (u8)func_08838AB4(self, h[0x298], h[0x299]) == 1) || h[0x298] != 0
        || (u8)func_0882A864(self, 0) == 0 || (h[0x298] == 0 && (u32)(h[0x299] - 13) < 2)) {
        self[0x535] = 40;
        return;
    }
    switch (layer) {
    case 0: self[0x534] = 9; break;
    case 1: self[0x534] = 10; break;
    case 2: self[0x534] = 11; break;
    case 3: self[0x534] = 12; break;
    case 4: self[0x534] = 13; break;
    case 5: self[0x534] = 14; break;
    }
    func_08890E08(D_08A631A8, (s8)layer);
    menu_begin_draw(self);
    func_08890E38(D_08A631A8, 0);
    func_08890DE8(D_08A631A8, 14, 14);
    if (U32(h, 0x554) == 0) {
        self[0x535] = 40;
        return;
    }
    if ((s8)(self[0x535] = self[0x535] - 1) < 1)
        self[0x535] = 40;
    func_08890DFC(D_08A631A8, 36, 18);
    q = *(u16 **)(h + 0x554);
    switch (q[1]) {
    case 1:
        p = func_09A7B7D0(D_09B22D70, q[11]);
        k = *p;
        if (k == 0x11) {
            OBJ_LINE_HERE(D_09B0525A);
        } else if (k == 0xD) {
            if (func_09C1B868(D_09CFB898) == 0)
                return;
            OBJ_LINE_HERE(D_09B05256);
        } else if (k == 0x2E) {
            if (func_09C19580(D_09CFB798, 5) == 0)
                return;
            if (*(s16 *)(D_089CC438 + 0x6AF0E) == 0x11)
                OBJ_LINE_HERE(D_09B0525C);
            else
                OBJ_LINE_HERE(D_09B05252);
        } else if (k == 0x55) {
            s16 a, b;
            s32 wb;

            if (func_09C19580(D_09CFB798, 7) == 0)
                return;
            func_0889134C(D_08A631A8, D_0893B224, TXT(D_09B04DE0), TXT(D_09B0524E));
            a = func_08890E48(D_08A631A8, TXT(D_09B0524E));
            func_08890DFC(D_08A631A8, 36, 34);
            func_0889134C(D_08A631A8, D_0893B224, TXT(D_09B04DE0), TXT(D_09B05250));
            b = func_08890E48(D_08A631A8, TXT(D_09B05250));
            w = (s16)((a + 4) * 7 + 36);
            wb = (s16)((b + 4) * 7 + 36);
            if (w < wb)
                w = wb;
        } else if (k == 0x8B) {
            if (*(s16 *)(D_089CC438 + 0x6AF0E) == 0x2E)
                OBJ_LINE_HERE(D_09B05254);
            else
                OBJ_LINE_HERE(D_09B0524C);
        } else if (k == 0x42) {
            if (*(s16 *)(D_089CC438 + 0x6AF0E) == 0xD)
                OBJ_LINE_HERE(D_09B05258);
            else
                OBJ_LINE_HERE(D_09B0524A);
        }
        if (name != NULL)
            w = (s16)(((s16)func_08890E48(D_08A631A8, name) + 4) * 7 + 36);
        if (func_09A62E58(D_09B0FF88, ((u8 *)p)[3]) == 1) {
            s32 w2 = (s16)(((s16)func_08890E48(D_08A631A8, TXT(D_09B05278)) + 4) * 7 + 36);

            if ((s16)w < w2)
                w = w2;
        }
        func_08838B08(self, 0, 16, w, 16, 0x801D3905, 0);
        func_08838E0C(self, 16, 16, 53, (s8)self[0x535], 20);
        k = *p;
        if (k == 0x55) {
            func_08838B08(self, 0, 32, w, 16, 0x801D3905, 0);
            func_08838E0C(self, 16, 32, 50, (s8)self[0x535], 20);
        }
        if (func_09A62E58(D_09B0FF88, ((u8 *)p)[3]) == 1) {
            s8 y = ((k == 0x55) + 1) * 16;

            func_08890DFC(D_08A631A8, 36, (s8)(y + 18));
            func_0889134C(D_08A631A8, D_0893B224, TXT(D_09B04DE0), TXT(D_09B05278));
            y += 16;
            func_08838B08(self, 0, y, w, 16, 0x801D3905, 0);
            func_08838E0C(self, 16, y, 52, (s8)self[0x535], 20);
        }
        return;
    case 2:
        if ((*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) == 0) {
            if (func_09A7BFA8(D_09B22D70) == 0)
                return;
        } else if (func_09A7BFA8(D_09B22D70) == 0) {
            return;
        }
        if ((*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) == 0 || D_09A0B544[0x12D6] != 0)
            OBJ_LINE(D_09B0523C);
        else
            OBJ_LINE(D_09B05246);
        break;
    case 3:
        if (func_0885057C(D_089CC438) == 0)
            return;
        OBJ_LINE(D_09B05262);
        break;
    case 5:
        OBJ_LINE(D_09B05240);
        break;
    case 6:
        OBJ_LINE(D_09B05242);
        break;
    case 7:
        OBJ_LINE(D_09B05244);
        break;
    case 8:
        if ((u8)func_08838AB4(self, h[0x298], h[0x299]) == 1)
            OBJ_LINE(D_09B0523A);
        else
            OBJ_LINE(D_09B05238);
        break;
    case 9:
        if ((*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) == 0)
            return;
        OBJ_LINE(D_09B0523E);
        break;
    case 10:
        OBJ_LINE(D_09B05260);
        break;
    case 11:
        w = 160;
        OBJ_LINE(D_09B0525E);
        break;
    case 12:
        w = 160;
        OBJ_LINE(D_09B05284);
        break;
    case 13:
        if (func_09C1B888(D_09CFB898) != 0)
            return;
        OBJ_LINE(D_09B05248);
        break;
    case 14: {
        u16 n = q[0];

        if (func_09C294B8(D_09CFBBF0) < (s32)n)
            return;
        if ((q[11] & 1) == 0)
            OBJ_LINE(D_09B05274);
        else
            OBJ_LINE(D_09B05276);
        break;
    }
    case 15:
        if (func_09C294F8(D_09CFBBF0, q[11]) == 0)
            return;
        OBJ_LINE(D_09B0526C);
        break;
    case 16:
        if (func_09C29548(D_09CFBBF0, q[11]) == 0)
            return;
        OBJ_LINE(D_09B0526E);
        break;
    case 17:
        if (func_09C29598(D_09CFBBF0) == 0)
            return;
        OBJ_LINE(D_09B05270);
        break;
    case 18:
        if (func_09C295E0(D_09CFBBF0) == 0)
            return;
        OBJ_LINE(D_09B05272);
        break;
    case 19: {
        s32 wb;

        if (func_09C29688(D_09CFBBF0) == 0)
            return;
        func_08891474(D_08A631A8, 36, 18, D_0893B224, TXT(D_09B04DE0), TXT(D_09B05266));
        wb = (s16)(((s16)func_08890E48(D_08A631A8, TXT(D_09B05266)) + 6) * 7 + 36);
        func_08838B08(self, 0, 16, wb, 16, 0x801D3905, 0);
        func_08838E0C(self, 16, 16, 53, (s8)self[0x535], 20);
        func_08891474(D_08A631A8, 8, 34, D_0893B22C, TXT(D_09B05268),
                      TXT(D_09B04E08[D_089CC438[0x6A23F]]), TXT(D_09B0526A));
        func_08838B08(self, 0, 32, wb, 16, 0x801D3905, 0);
        return;
    }
    case 20:
        if (func_09C29630(D_09CFBBF0, q[11]) == 0)
            return;
        OBJ_LINE(D_09B05264);
        break;
    case 21:
        return;
    case 29:
        OBJ_LINE(D_09B05248);
        break;
    }
    if (name != NULL)
        w = (s16)(((s16)func_08890E48(D_08A631A8, name) + 4) * 7 + 36);
    func_08838B08(self, 0, 16, w, 16, 0x801D3905, 0);
    func_08838E0C(self, 16, 16, 53, (s8)self[0x535], 20);
}

extern void *D_089CBBE4;          /* camera */
extern void *D_09B23820;
extern const u16 D_0893AE68[];     /* weapon icons */
extern const u8 D_0893AF74[];      /* colors of the quest marks (r, g, b, -) */
extern const s16 D_08939830, D_0893983C;
void camera_world_to_screen(void *cam, f32 *out, const f32 *p);
int wstr_len(void *self, const s16 *s);
extern void *D_089CBBE8;          /* text converter */
void menu_draw_sprite(u8 *self, u32 x, u32 y, s32 id);
void func_088912BC(void *font, int x, int y, const char *s);
u8 *menu_quest_data(void *self);
s8 func_09A884C8(void *a, void *q);
u8 func_0889E1BC(void *a, s32 i);
void gfx_draw_sprite(u8 *self, u32 x, u32 y, u32 r, u32 g, u32 b, s32 id, s32 flip);

/* screen coordinate as the original converts it (float -> unsigned, low 16 bits) */
#define SCR(f) ((u16)(u32)(f))

/* mark of the quest state above a hunter */
#define QUEST_MARK(c, half, id)                                                       \
    gfx_draw_sprite(D_089CBBEC, (u32)(pos[0] - (f32)((half) >> 1)), (u32)(pos[1] - 16.0f), \
                    D_0893AF74[(c) * 4], D_0893AF74[(c) * 4 + 1], D_0893AF74[(c) * 4 + 2], id, 0)

/* names of the hunters above their heads, with the weapon icon and the quest mark */
void hud_draw_names(u8 *self, s32 layer)
{
    int online, i;

    switch (layer) {
    case 0: self[0x534] = 9; break;
    case 1: self[0x534] = 10; break;
    case 2: self[0x534] = 11; break;
    case 3: self[0x534] = 12; break;
    case 4: self[0x534] = 13; break;
    case 5: self[0x534] = 14; break;
    }
    func_08890E08(D_08A631A8, (s8)layer);
    menu_begin_draw(self);
    func_08890DE8(D_08A631A8, 16, 14);
    for (i = 0; i < 4; i++) {
        u8 *h = (u8 *)func_088DCEBC(D_09A4F0C4, i);
        const char *name;
        u8 lvl;
        f32 in[4], pos[4];
        s16 sh;

        if (h == NULL || (U32(h, 4) & 1) == 0 || (U32(h, 4) & 4) != 0)
            continue;
        online = (*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) != 0;
        if (!online) {
            lvl = D_089CC438[0x44BC];
            name = (const char *)(D_089CC438 + 0x4A0);
        } else {
            u8 *p;

            if (func_0883B498(self, U16(h, 0x1E6)) == -1)
                continue;
            p = D_09A0B544 + (func_0883B498(self, U16(h, 0x1E6)) & 0xFF) * 200;
            lvl = p[0x604];
            name = (const char *)(p + 0x584);
        }
        in[0] = F32(h, 0x200);
        in[1] = F32(h, 0x204) + 190.0f;
        in[2] = F32(h, 0x208);
        in[3] = 1.0f;
        camera_world_to_screen(D_089CBBE4, pos, in);
        if (!(pos[0] < 700.0f) || pos[0] <= -60.0f || !(pos[1] < 500.0f) || pos[1] <= -20.0f
            || pos[3] <= 0.0f)
            continue;
        sh = (s16)((wstr_len(D_089CBBE8, (const s16 *)name) << 4) >> 2);
        menu_draw_sprite(self, (u16)(SCR(pos[0]) - sh - 14), (u16)(SCR(pos[1]) - 2), D_0893AE68[h[0x1E8]]);
        func_08890E38(D_08A631A8, 1);
        func_088912BC(D_08A631A8, (s16)(SCR(pos[0]) - sh + 1), (s16)(SCR(pos[1]) + 1), name);
        switch (lvl) {
        case 0: func_08890E38(D_08A631A8, 5); break;
        case 1: func_08890E38(D_08A631A8, 6); break;
        case 2: case 3: func_08890E38(D_08A631A8, 7); break;
        case 4: case 5: func_08890E38(D_08A631A8, 2); break;
        case 6: case 7: func_08890E38(D_08A631A8, 24); break;
        case 8: func_08890E38(D_08A631A8, 23); break;
        }
        func_088912BC(D_08A631A8, (s16)(SCR(pos[0]) - sh), (s16)SCR(pos[1]), name);
        if (!online) {
            if (func_09A7BFA8(D_09B22D70) == 1) {
                s8 c = func_09A884C8(D_09B23820, menu_quest_data(self));

                QUEST_MARK(c, D_0893983C, 0x8C);
            }
        } else {
            s8 st = D_09A0B544[(func_0883B498(self, U16(h, 0x1E6)) & 0xFF) * 200 + 0x63B];

            if (st == 3 || st == 4) {
                u8 c = func_0889E1BC(D_09A0B544, func_0883B498(self, U16(h, 0x1E6)));

                if (D_09A0B544[(func_0883B498(self, U16(h, 0x1E6)) & 0xFF) * 200 + 0x63A] == 0) {
                    if (st == 3 || (*(u32 *)(D_08A631A0 + 0xF7A554) & 0x10) != 0)
                        QUEST_MARK(c, D_08939830, 0x8B);
                } else {
                    QUEST_MARK(c, D_0893983C, 0x8C);
                }
            }
        }
    }
}

extern const u16 D_09B04E1E;
extern const char D_0893B278[], D_0893B280[], D_0893B28C[];
extern const s16 D_0893B270, D_0893B272, D_0893B274, D_0893B276;
extern const u8 D_0893B298[4], D_0893B29C[4];
extern const s16 D_0893B2A0, D_0893B2A2, D_0893B2A4;
extern const char D_0893B2A8[], D_0893B2B0[], D_0893B2D0[], D_0893B2E8[], D_0893B300[], D_0893B310[];
extern const s16 D_08939854, D_08939856;
u32 hud_pulse_color(void);
s32 func_0885AED0(void *a, u32 id);
s32 func_0885C33C(void *a, u32 id);
s32 func_0885C3DC(void *a, u32 id);
void gfx_draw_sprite_mode(u8 *self, u32 x, u32 y, s32 w_arg, s32 h_arg, u32 r, u32 g, u32 b, u8 a,
                          s32 id, u8 mode);
void gfx_draw_item_icon_alpha(u8 *self, u32 id, u32 x, u32 y, u32 w, s32 h_arg, u8 mode, u8 alpha);

/* "n / total" selector with blinking arrows */
void hud_draw_selector(u8 *self, s32 x, s32 y, s32 cur_arg, s32 total_arg)
{
    s16 cur = cur_arg;
    s16 total = total_arg;
    u32 c = hud_pulse_color();
    char buf[128];
    s16 digits;

    if (total / 100 != 0)
        digits = 2;
    else if (total / 10 != 0)
        digits = 1;
    else
        digits = 0;
    if (digits == 2)
        sprintf(buf, D_0893B28C, cur + 1, func_09A7B790(D_09B22D70, D_09B04E1E), total);
    else if (digits == 1)
        sprintf(buf, D_0893B280, cur + 1, func_09A7B790(D_09B22D70, D_09B04E1E), total);
    else if (digits == 0)
        sprintf(buf, D_0893B278, cur + 1, func_09A7B790(D_09B22D70, D_09B04E1E), total);
    func_08890DE8(D_08A631A8, 14, 14);
    func_08890E38(D_08A631A8, 0);
    func_08890DFC(D_08A631A8, x, y);
    func_0889134C(D_08A631A8, D_0893B220, buf);
    if (total >= 2) {
        s16 ay = (s16)y - (u16)D_0893B272;
        s16 x16 = x;

        gfx_draw_sprite_mode(self, (u16)(x16 - (u16)D_0893B270), (u16)ay, D_08939854, D_08939856,
                             (u8)(c >> 16), (u8)(c >> 8), (u8)c, c >> 24, 142, 1);
        gfx_draw_sprite_mode(self, (u16)(x16 + (u16)D_0893B274 + (u16)D_0893B276 * digits), (u16)ay,
                             D_08939854, D_08939856, (u8)(c >> 16), (u8)(c >> 8), (u8)c, c >> 24, 142, 0);
    }
}

/* yellow pulsing color of the selector arrows */
u32 hud_pulse_color(void)
{
    f32 s = vfpu_sinf((f32)(u16)((*(u32 *)(D_08A631A0 + 0xF7A554) & 0x3F) << 10) * (3.14159265f / 32768.0f));

    return (((s8)(int)(80.0f * s) + 159) << 24) | 0xFF00;
}

/* line of an item needed by a recipe: icon, name, count in the pouch, in the box,
 * needed (colored when not enough) */
void hud_draw_item_need(u8 *self, s32 x_arg, s32 y, s32 id_arg, s32 need_arg)
{
    s16 x = x_arg;
    s16 id = id_arg;
    char sa[4], sb[4];
    s16 have;
    s32 box, need;

    sa[0] = D_0893B298[0];
    sa[1] = D_0893B298[1];
    sa[2] = D_0893B298[2];
    sa[3] = D_0893B298[3];
    sb[0] = D_0893B29C[0];
    sb[1] = D_0893B29C[1];
    sb[2] = D_0893B29C[2];
    sb[3] = D_0893B29C[3];
    if (id == 0)
        return;
    have = (s16)func_0885AED0(D_08A62C50, (u16)id);
    box = func_0885C33C(D_08A62C50, (u16)id);
    if (box > 999)
        box = 999;
    gfx_draw_item_icon_alpha(self, (u16)id, (u16)x, (u16)((s16)y - 1), (u16)D_0893B2A0, (u16)D_0893B2A2, 1, 255);
    func_08890E38(D_08A631A8, 0);
    func_08890DFC(D_08A631A8, (s16)(x + D_0893B2A4), y);
    need = (s16)need_arg;
    if (have + box < need) {
        func_08892004(D_08A631A8, D_0893B310, func_08851528(D_089CC438, (u16)id));
        func_08890DFC(D_08A631A8, (s16)(x + D_0893B2A4 + 112), y);
        func_08892004(D_08A631A8, D_0893B300, have, sa, box, sb, need);
    } else if (have < D_0899D19B[id * 0x18]) {
        func_08892004(D_08A631A8, D_0893B2A8, func_08851528(D_089CC438, (u16)id));
        func_08890DFC(D_08A631A8, (s16)(x + D_0893B2A4 + 112), y);
        if (func_0885C3DC(D_08A62C50, (u16)id) == 0)
            func_08892004(D_08A631A8, D_0893B2E8, have, sa, box, sb, need);
        else
            func_08892004(D_08A631A8, D_0893B300, have, sa, box, sb, need);
    } else {
        func_08892004(D_08A631A8, D_0893B2A8, func_08851528(D_089CC438, (u16)id));
        func_08890DFC(D_08A631A8, (s16)(x + D_0893B2A4 + 112), y);
        if (func_0885C3DC(D_08A62C50, (u16)id) == 0)
            func_08892004(D_08A631A8, D_0893B2B0, have, sa, box, sb, need);
        else
            func_08892004(D_08A631A8, D_0893B2D0, have, sa, box, sb, need);
    }
}

extern const u16 D_09B04E48[];     /* texts of the equipment classes */
extern const u16 D_089AC8CE, D_089AC8C6;
extern const char D_0893B218[];
extern const u8 D_0893B318[];
extern const Layout D_0893B32C;
extern void *D_08A62C54;          /* equipment data */
u32 func_0885DC30(void *a, u8 *eq);
void func_0882F580(void *self, void *layout, int a, int b, int alpha, int c);
void layout_build(Layout *out, const u8 *src, int title, s32 dx_arg, s32 dy_arg);
void layout_draw(void *self, Layout *l, const char *text, u32 alpha);
void layout_draw_pages(void *self, Layout *l, u32 cur, u32 total_arg, s32 color, s32 alt);
void menu_draw_resist_table(void *self, int x, int y, u32 page, s32 bar, int rows, u32 sel);
void menu_draw_skills_ex(void *self, u8 *hunter, int x, int y, const s8 *colors);
void hud_draw_skill_compare(u8 *self, u8 *hunter, u8 *eq, s32 x, s32 y);
void equip_save(void *self, u8 *hunter);
void equip_restore(void *self, u8 *hunter);
void equip_try_weapon(void *self, u8 *hunter, u8 *eq);
void equip_try_armor(void *self, u8 *hunter, u8 *eq);
int menu_armor_wearable(void *self, u8 *hunter, void *eq);
s8 func_088D8DF4(void *hunter, u8 skill);
void func_09AE9780(void *a, void *hunter);
s8 func_09AE99E8(void *a, u8 skill);
s8 func_09AE9818(void *a, u8 skill, void *armor, void *weapon);

/* class of the armor piece (type, id) (blademaster / gunner, gender), centered at x */
void hud_draw_armor_class(void *self, s32 type_arg, u32 id, s32 x_arg, s32 y_arg)
{
    s16 type = type_arg;
    s16 x = x_arg;
    s16 y = y_arg;
    s32 lang = LANG;
    u8 eq[4];
    u32 f;
    s32 k;

    if (type == 6 || type == 5)
        return;
    eq[1] = type;
    U16(eq, 2) = id;
    f = func_0885DC30(D_08A62C54, eq);
    k = 0xFF;
    if ((f & 3) == 3) {
        if ((f & 0xC) != 0xC)
            k = (f & 4) ? 1 : 0;
    } else if ((f & 1) == 0) {
        if ((f & 0xC) == 8)
            k = 4;
        else if ((f & 0xC) == 4)
            k = 6;
        else
            k = 2;
    } else {
        if ((f & 0xC) == 8)
            k = 5;
        else if ((f & 0xC) == 4)
            k = 7;
        else
            k = 3;
    }
    if (k == 0xFF)
        return;
    func_08890E38(D_08A631A8, 5);
    x -= (s16)((func_08890E48(D_08A631A8, func_09A7B790(D_09B22D70, D_09B04E48[k])) * 14) >> 1);
    if (lang == 2 && k == 7)
        x += 14;
    else if (lang == 4 && k == 5)
        x += 7;
    func_08891474(D_08A631A8, x, y, D_0893B220, func_09A7B790(D_09B22D70, D_09B04E48[k]));
}

/* box of the equipment being tried: resistances and skills compared with the current ones */
void hud_draw_equip_trial(u8 *self, u8 *hunter, s32 dx, s32 dy, u32 page)
{
    u8 *ov = (u8 *)D_09B22D70 + 0xA888;
    char title[64];
    Layout l, t;

    sprintf(title, D_0893B218, func_0885151C(D_089CC438, D_089AC8CE));
    layout_build(&l, D_0893B318, (int)title, dx, dy);
    if (ov[0x1E] == 0) {
        t.x = D_0893B32C.x;
        t.y = D_0893B32C.y;
        t.w = D_0893B32C.w;
        t.h = D_0893B32C.h;
        t.cw = D_0893B32C.cw;
        t.b9 = D_0893B32C.b9;
        t.bb = D_0893B32C.bb;
        t.tx = D_0893B32C.tx;
        t.ty = D_0893B32C.ty;
        l.ba = 10;
        *(u32 *)&t.lh = *(const u32 *)&D_0893B32C.lh;
        t.ba = D_0893B32C.ba;
        func_0882F580(self, &l, (int)title, -1, 255, 0);
        t.ba = 10;
        layout_draw(self, &t, func_0885151C(D_089CC438, D_089AC8C6), 255);
    } else {
        func_0882F580(self, &l, (int)title, -1, 255, 0);
        if (ov[0x20] > 1)
            layout_draw_pages(self, &l, page & 0xF, ov[0x20], 0, 0);
        menu_draw_resist_table(self, l.tx, l.ty, page & 0xF, ov[0x1F], 9, ov[0x13]);
        layout_draw(self, (Layout *)&D_0893B32C, func_0885151C(D_089CC438, D_089AC8C6), 255);
        hud_draw_skill_compare(self, hunter, ov + 0x12, D_0893B32C.tx, (s16)(D_0893B32C.ty + (s16)D_0893B32C.lh));
    }
}

/* skills of the hunter wearing eq, colored compared with the current equipment */
void hud_draw_skill_compare(u8 *self, u8 *hunter, u8 *eq, s32 x, s32 y)
{
    s8 col[6];
    u32 i;

    func_08890E38(D_08A631A8, 0);
    col[5] = 0;
    col[4] = 0;
    col[3] = 0;
    col[2] = 0;
    col[1] = 0;
    col[0] = 0;
    if (eq != NULL) {
        equip_save(self, hunter);
        func_09AE9780((u8 *)D_09B22D70 + 0xA888, hunter);
        if (eq[1] == 5 || eq[1] == 6) {
            equip_try_weapon(self, hunter, eq);
        } else {
            if ((u8)menu_armor_wearable(self, hunter, eq) == 0)
                return;
            equip_try_armor(self, hunter, eq);
        }
        for (i = 0; i < 6; i++) {
            u8 *s = hunter + i + 0x634;
            s8 now, before;

            if (*s == 0)
                break;
            now = func_088D8DF4(hunter, *s);
            before = func_09AE99E8((u8 *)D_09B22D70 + 0xA888, *s);
            if ((u8)before == 0) {
                if (now > 0)
                    col[i] = 14;
                else if (now < 0)
                    col[i] = 2;
            } else if ((u8)before != hunter[(i & 0xFF) + 0x676]) {
                s8 old = func_09AE9818((u8 *)D_09B22D70 + 0xA888, *s, self + 0x11CA, self + 0x11BE);

                if (old < now)
                    col[i] = 4;
                else if (now < old)
                    col[i] = 15;
            }
        }
    }
    menu_draw_skills_ex(self, hunter, x, y, col);
    equip_restore(self, hunter);
}

/* 1 if the quest state (a, b) is one of the special event states */
int hud_event_state(void *self, s32 a_arg, u32 b_arg)
{
    s8 a = a_arg;
    u8 b = b_arg;

    if (a != 0)
        return 0;
    switch (b) {
    case 0x0F: case 0x10: case 0x12: case 0x13: case 0x14: case 0x15:
    case 0x16: case 0x17: case 0x18: case 0x19: case 0x1A: case 0x1B:
        return 1;
    default:
        return 0;
    }
}

extern u8 *D_08A62C40;            /* display list manager */
extern const u8 D_0893B340[];
u32 *func_088597FC(void *dl);
void func_088596C8(void *dl, u32 *p, int n, u8 layer);
void func_0885980C(void *dl, u32 *end);
void gfx_draw_sprite_scaled_rgba(u8 *self, u32 x, u32 y, s32 w_arg, s32 h_arg, u32 r, u32 g, u32 b,
                                 u8 a, s32 id);
void menu_draw_main_page(u8 *self, int dim);
void func_08892148(void *font, int x, int y, int color, const char *fmt, ...);

/* label background: horizontal gradient of color c (0x00RRGGBB as bytes B, G, R) with
 * a rounded end (flags 0: right end, else left end; 0x80: pulsing, no end) */
void hud_draw_label_bg(u8 *self, u32 x_arg, s32 y_arg, s32 w_arg, s32 h_arg, u32 c, u32 flags_arg)
{
    u16 x = x_arg;
    s16 y = y_arg;
    s16 w = w_arg;
    u8 flags = flags_arg;
    f32 s = vfpu_sinf((f32)(u16)((*(u32 *)(D_08A631A0 + 0xF7A554) & 0x3F) << 10) * (3.14159265f / 32768.0f));
    u32 al, ar, rgb, ca, cb, xe, yb, yt;
    u32 *p;

    if ((flags & 0x80) == 0) {
        ar = 229;
        al = 229;
        if (flags == 0)
            gfx_draw_sprite_scaled_rgba(self, (u16)((s16)x + w), (u16)y, 32, h_arg, 255, 255, 255, 229, 259);
        else
            gfx_draw_sprite_mode(self, (u16)((s16)x - 32), (u16)y, 32, h_arg, 255, 255, 255, 229, 259, 1);
    } else {
        u32 v;

        if ((flags & 0x7F) == 0) {
            al = 224;
            ar = 96;
        } else {
            al = 96;
            ar = 224;
        }
        v = (int)(16.0f * s) & 0xFF;
        al = (al + v) & 0xFF;
        ar = (ar + v) & 0xFF;
    }
    p = func_088597FC(D_08A62C40);
    yb = (u32)(y + (s16)h_arg) << 16;
    xe = (u16)((s16)x + w);
    yt = (u32)y << 16;
    rgb = ((c & 0xFF) << 16) | (((c >> 8) & 0xFF) << 8) | ((c >> 16) & 0xFF);
    ca = (al << 24) | rgb;
    cb = (ar << 24) | rgb;
    p[0] = ca;
    p[1] = yt | (u16)(s16)x;
    p[2] = 0;
    p[3] = ca;
    p[4] = (u16)(s16)x | yb;
    p[5] = 0;
    p[6] = cb;
    p[7] = yt | xe;
    p[8] = 0;
    p[9] = cb;
    p[10] = yb | xe;
    p[11] = 0;
    p[12] = (((u32)p & 0xFF000000) >> 8) | 0x10000000;
    p[13] = ((u32)p & 0xFFFFFF) | 0x01000000;
    p[14] = 0x1E000000;
    p[15] = 0x50000001;
    p[16] = 0x1280011C;
    p[17] = 0x04040004;
    p[18] = 0x10000000;
    p[19] = 0x08000000;
    func_088596C8(D_08A62C40, p + 12, 8, self[0x534]);
    func_0885980C(D_08A62C40, p + 20);
}

/* icon id drawn only while t >= n (blinking) */
void hud_draw_blink_icon(u8 *self, u32 x, u32 y, u32 id, s32 t, s32 n)
{
    if (t >= n)
        menu_draw_sprite(self, x, y, id & 0xFFFF);
}

/* page of the quest list (4 entries, the third one only on-line) */
void hud_draw_quest_page(u8 *self)
{
    Layout l;
    s8 col[4];
    s16 y;
    int i;

    if (self[0x57E] != 0)
        return;
    menu_draw_main_page(self, 1);
    layout_build(&l, D_0893B340, 0, 0, 0);
    func_0882F580(self, &l, 0, self[0x57F], 255, 1);
    col[3] = 0;
    col[2] = 0;
    col[1] = 0;
    col[0] = 0;
    if ((*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) == 0)
        col[2] = 10;
    y = l.ty;
    for (i = 0; i < 4; i++) {
        func_08892148(D_08A631A8, l.tx, y, col[i], D_0893B220,
                      func_09A7B790(D_09B22D70, ((const u16 *)0x09B052C8)[i]));
        y += l.lh;
    }
}

extern const u8 D_0893B354[];
extern const Layout D_0893B368;
extern const s16 D_0893B36A, D_0893B36C, D_0893B36E;
extern const u16 D_09B05560;
extern void *D_09A5242C;
void func_09C240A0(u8 a, u8 b, int k, s32 c, s32 d, int e);
void menu_draw_give(u8 *self, int alt);
void gfx_draw_subwindow(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 alpha);
u32 func_088F0088(void *a, u32 keys, u8 sel);
void func_088F0020(void *a);
int menu_cursor_vertical(void *self, u8 *pos, u32 keys, u32 count);
void func_0883A07C(void *self, u8 a);
void func_0883A758(void *self, int x, int y, s32 sel, int a);

/* page of the guild cards (map of the village, card list, text of the entry) */
void hud_draw_card_page(u8 *self)
{
    Layout l;
    s8 col[4];
    u8 *h;
    u8 st = self[0x57E];

    if (st != 0) {
        func_09C240A0(self[0x117D], self[0x117A], st >= 3 ? 4 : 3, (s8)self[0x117B], (s8)self[0x117C], 0);
        func_08833290(self, 16, 2, U32(self, 0x118C), 0);
        if (U16(self, 0x5FC) != 0) {
            st = self[0x57E];
            if (st == 1 || st == 2 || st == 4 || st == 10)
                layout_draw(D_089CBBEC, (Layout *)&D_0893B368,
                            func_09A7B790(D_09B22D70, ((const u16 *)0x09B05D10)[U16(self, 0x5FC)]), 255);
            if (self[0x57E] == 3) {
                layout_draw(D_089CBBEC, (Layout *)&D_0893B368,
                            func_09A7B790(D_09B22D70, ((const u16 *)0x09B05D10)[U16(self, 0x5FC)]), 255);
                func_088920A8(D_08A631A8, (s16)(D_0893B368.x + D_0893B36C - 148),
                              (s16)(D_0893B36A + D_0893B36E - 22), D_0893B220,
                              func_0885151C(D_089CC438, D_089AC588[self[0x537]]));
            }
        }
        st = self[0x57E];
        if (st == 10) {
            self[0x589] = 1;
            menu_draw_give(self, 1);
            return;
        }
        if (st < 10) {
            gfx_draw_subwindow(D_089CBBEC, 0x138, 0x88, 0xA0, 0x20, 255);
            func_08890DE8(D_08A631A8, 14, 14);
            func_08892148(D_08A631A8, 0x142, 0x92, 0, D_0893B220, func_09A7B790(D_09B22D70, D_09B05560));
        }
        return;
    }
    menu_draw_main_page(self, 1);
    h = *(u8 **)(self + 0x530);
    layout_build(&l, D_0893B354, 0, 0, 0);
    func_0882F580(self, &l, 0, self[0x57F], 255, 1);
    col[2] = 9;
    col[1] = 9;
    col[0] = 9;
    if ((U32(h, 0x614) & 4) != 0) {
        u8 *g = D_089CC438;
        s8 n = 0;
        u32 k;

        for (k = 0; k < 13; k++, g += 0x70)
            if ((s8)g[0x69BEC] != 0)
                n++;
        if (n != 0) {
            col[2] = 0;
            col[1] = 0;
            col[0] = 0;
        }
    }
    {
        s16 y = l.ty;
        int i;

        for (i = 0; i < 3; i++) {
            func_08892148(D_08A631A8, l.tx, y, col[i], D_0893B220,
                          func_09A7B790(D_09B22D70, ((const u16 *)0x09B051C0)[i]));
            y += l.lh;
        }
    }
}

/* input of the guild card page */
u32 hud_card_input(u8 *self, u32 keys)
{
    if (self[0x57E] == 1) {
        u32 r = func_088F0088(D_09A5242C, keys, self[0x57F]);

        keys = r & 0xFFFF;
        if (r & 0x2000) {
            keys = 0;
            self[0x57E] -= 1;
            U32(self, 0x5E8) |= 4;
        }
        return keys;
    }
    if (self[0x57E] != 0)
        return keys;
    if (keys & 0x4000) {
        s8 c = self[0x57F];

        if (c == 3 || c == 1 || c == 0
            || (c == 2 && (*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) != 0)) {
            func_08883988(D_08A62D8C, 0, 19, 0, 0, 0, 0, 0, 0);
            self[0x57E] += 1;
            U32(self, 0x5E8) &= ~4;
            func_088F0020(D_09A5242C);
            return 0;
        }
        if (c == 2)
            func_08883988(D_08A62D8C, 0, 21, 0, 0, 0, 0, 0, 0);
    }
    menu_cursor_vertical(self, self + 0x57F, keys, 4);
    U16(self, 0x5FC) = self[0x57F] + 14;
    return keys;
}

/* page of the players of the room: opens the card of the selected player */
void hud_draw_player_page(u8 *self)
{
    if (self[0x57E] == 1) {
        u8 *p = NULL;
        s32 i, k;

        for (i = 0; i < (s8)D_09A0B544[0xEE3]; i++) {
            const u8 *a = D_09A0B544 + i * 200 + 0x57C;
            int r = 0;

            for (k = 0; k < 6; k++) {
                if (self[0x1172 + k] < a[k]) {
                    r = -1;
                    break;
                }
                if (a[k] < self[0x1172 + k]) {
                    r = 1;
                    break;
                }
            }
            if (r == 0) {
                p = D_09A0B544 + i * 200 + 0x57C;
                break;
            }
        }
        if (p != NULL)
            func_0883A07C(self, self[0x582]);
    } else if (self[0x57E] == 0) {
        menu_draw_main_page(self, 1);
        func_0883A758(self, 240, 40, (s8)self[0x57F], 0);
    }
}

extern u8 *D_09A0FDC4;            /* ids of the hunters (8 bytes each, overlay) */
extern const u8 D_0893B3D4[];
extern const char *const D_0893AF70;
s32 func_0889D7E8(void *a, u32 b);
s32 func_0889D848(void *a, u32 b);
int menu_cursor_horizontal(void *self, u8 *pos, u32 keys, u32 count_arg);

/* k-th player of the room other than the local one (NULL if none) */
u8 *hud_other_player(void *self, s32 k)
{
    s32 n = 0;
    u32 i;

    for (i = 0; (s32)i < (s8)D_09A0B544[0xEE3]; i++) {
        u8 *p = D_09A0B544 + (i & 0xFF) * 200 + 0x57C;

        if (memcmp(func_0889C518(D_09A0B544), p, 6) != 0) {
            if (n == k)
                return p;
            n++;
        }
    }
    return NULL;
}

/* index in the room of the player of hunter id (-1 if absent) */
s32 hud_player_index(void *self, s32 id)
{
    u8 *a = D_09A0FDC4 + id * 8 + 0x50;
    s32 i;

    for (i = 0; i < (s8)D_09A0B544[0xEE3]; i++)
        if (memcmp(a, D_09A0B544 + (i & 0xFF) * 200 + 0x57C, 6) == 0)
            return i;
    return -1;
}

int hud_nonzero(void *self, s32 v)
{
    return v != 0;
}

/* page of the players (variant with the second column) */
void hud_draw_player_page2(u8 *self)
{
    menu_draw_main_page(self, 1);
    func_0883A758(self, 240, 40, (s8)self[0x57F], 1);
}

/* 1 if the room is busy or player p is busy */
int hud_room_busy(void *self, u32 p)
{
    s32 a = func_0889D7E8(D_09A0B544, p);
    s32 b = func_0889D848(D_09A0B544, p);

    return a != 0 || b != 0;
}

/* cursor of a 4 x 3 grid; with the button 0x4000 stores the choice in the hunter */
u32 hud_grid_input(u8 *self, u32 keys)
{
    u8 row = self[0x57F] >> 2;
    u8 col = self[0x57F] & 3;

    if (menu_cursor_horizontal(self, &col, keys, 4) != 0)
        self[0x57F] = col + row * 4;
    else if (menu_cursor_vertical(self, &row, keys, 3) != 0)
        self[0x57F] = col + row * 4;
    if (keys & 0x4000)
        U8(*(u8 **)(self + 0x530), 0x565) = self[0x57F] + 1;
    return keys;
}

/* list of the other players of the room (empty lines up to 4) */
void hud_draw_player_list(u8 *self, s32 x, s32 y_arg, s32 sel_arg, s32 a)
{
    s8 sel = sel_arg;
    Layout l;
    s32 me, i;
    s32 y;

    layout_build(&l, D_0893B3D4, 0, x, y_arg);
    func_0882F580(self, &l, 0, sel, 255, 0);
    me = hud_player_index(self, D_089CC438[0x28]);
    y = l.ty;
    for (i = 0; i < (s8)D_09A0B544[0xEE3]; i++) {
        if (me != i) {
            func_0883B53C(self, l.tx, y, i, a);
            y = (s16)(y + l.lh);
        }
    }
    func_08890E38(D_08A631A8, 0);
    for (; i < 4; i++) {
        func_08891474(D_08A631A8, l.tx, y, D_0893AF70);
        y = (s16)(y + l.lh);
    }
}

extern const s16 D_0893B3E8, D_0893B3EA, D_0893B3EC, D_0893B3EE, D_0893B3F0, D_0893B3F2;
extern const s16 D_0893B3F4, D_0893B3F6, D_0893B3F8, D_0893B3FA, D_0893B3FC, D_0893B3FE;
extern const u16 D_089AD030[];
extern const u16 D_09B04DFE;
u8 func_0889DFAC(void *a, u32 i);
void gfx_fill_rect_pulse(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 color);
u8 menu_msg_push_text(u8 *self, const char *s);
void menu_close_state(u8 *self);
void func_0883AE3C(void *self, u8 *p);
void func_0883ABE0(void *self, u8 *p);

/* quest mark of player i next to its name */
#define PLAYER_MARK(xx, id) do {                                                   \
        u8 c_ = func_0889E1BC(D_09A0B544, i);                                      \
        gfx_draw_sprite(self, xx, ya, D_0893AF74[c_ * 4], D_0893AF74[c_ * 4 + 1],    \
                        D_0893AF74[c_ * 4 + 2], id, 0);                            \
    } while (0)

/* line of player i of the room: name (colored by state) and quest mark or status icon */
void hud_draw_player_line(u8 *self, s32 x, s32 y, u32 i, s32 mode)
{
    s16 x16 = x;
    s16 xa = x16 + 72;
    s16 ya = (s16)y - 1;
    u8 col = 0;
    u32 idx = (i & 0xFF) * 200;

    if (mode == 0) {
        s8 st;

        if (D_09A0B544[idx + 0x63B] == 5)
            col = 10;
        else if (func_0889DFAC(D_09A0B544, i) == 1)
            col = 6;
        st = D_09A0B544[idx + 0x63B];
        if (st == 4) {
            u8 c = func_0889E1BC(D_09A0B544, i);

            if (D_09A0B544[idx + 0x63A] == 0) {
                if (*(u32 *)(D_08A631A0 + 0xF7A554) & 0x10)
                    gfx_draw_sprite(self, (u16)(x16 + 79), (u16)ya, D_0893AF74[c * 4], D_0893AF74[c * 4 + 1],
                                    D_0893AF74[c * 4 + 2], 0x8B, 0);
            } else {
                gfx_draw_sprite(self, (u16)xa, (u16)ya, D_0893AF74[c * 4], D_0893AF74[c * 4 + 1],
                                D_0893AF74[c * 4 + 2], 0x8C, 0);
            }
        } else if (st == 3) {
            u8 c = func_0889E1BC(D_09A0B544, i);

            if (D_09A0B544[idx + 0x63A] == 0)
                gfx_draw_sprite(self, (u16)(x16 + 79), (u16)ya, D_0893AF74[c * 4], D_0893AF74[c * 4 + 1],
                                D_0893AF74[c * 4 + 2], 0x8B, 0);
            else
                gfx_draw_sprite(self, (u16)xa, (u16)ya, D_0893AF74[c * 4], D_0893AF74[c * 4 + 1],
                                D_0893AF74[c * 4 + 2], 0x8C, 0);
        }
    } else {
        if (func_0889D7E8(D_09A0B544, (u32)(D_09A0B544 + idx + 0x57C)) != 0)
            menu_draw_sprite(self, (u16)xa, (u16)ya, 0x1B);
        else if (func_0889D848(D_09A0B544, (u32)(D_09A0B544 + idx + 0x57C)) != 0)
            menu_draw_sprite(self, (u16)xa, (u16)ya, 0x1EE);
        else
            col = 10;
    }
    func_08890DE8(D_08A631A8, 16, 14);
    func_08890E38(D_08A631A8, col);
    func_088912BC(D_08A631A8, x, y, (const char *)(D_09A0B544 + idx + 0x584));
    func_08890DE8(D_08A631A8, 14, 14);
}

/* grid of 4 x 3 choices with the pulsing cursor */
void hud_draw_choice_grid(u8 *self)
{
    int c, r;

    gfx_draw_subwindow(self, D_0893B3E8, D_0893B3EA, D_0893B3EC, D_0893B3EE, 255);
    func_08890E38(D_08A631A8, 0);
    func_08890DE8(D_08A631A8, 14, 14);
    gfx_fill_rect_pulse(self, D_0893B3F8 + D_0893B3F4 * (self[0x57F] & 3),
                        D_0893B3FA + D_0893B3F6 * (u16)(self[0x57F] >> 2), D_0893B3FC, D_0893B3FE,
                        self[0x580] != 0 ? 0x80808080 : 0x8000C040);
    for (c = 0; c < 4; c++)
        for (r = 0; r < 3; r++)
            func_08891474(D_08A631A8, (s16)(D_0893B3F0 + D_0893B3F4 * (s16)c),
                          (s16)(D_0893B3F2 + D_0893B3F6 * (s16)r),
                          func_0885151C(D_089CC438, D_089AD030[c + r * 4]));
}

/* input of the "invite a player" page */
u32 hud_invite_input(u8 *self, u32 keys)
{
    s8 st = self[0x57E];

    if (st == 3) {
        func_08883988(D_08A62D8C, 0, 19, 0, 0, 0, 0, 0, 0);
        menu_close_state(self);
    } else if (st == 2) {
        menu_msg_push_text(D_089CBBEC, func_09A7B790(D_09B22D70, D_09B04DFE));
        self[0x57E] = 3;
        func_08883988(D_08A62D8C, 0, 19, 0, 0, 0, 0, 0, 0);
        menu_close_state(self);
    } else if (st == 1) {
        u8 *h = (u8 *)func_088DCEBC(D_09A4F0C4, 0);

        if (h[0x5C8] == 1) {
            self[0x57E] = 3;
        } else if (h[0x5C8] == 2) {
            self[0x57E] = 3;
        } else {
            self[0x580] -= 1;
            if (self[0x580] == 0)
                self[0x57E] = 2;
        }
    } else if (st == 0 && menu_cursor_vertical(self, self + 0x57F, keys, 3) == 0) {
        if (keys & 0x4000) {
            u8 *h = (u8 *)func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]);

            if (h[0x62C] == 0) {
                if (func_0889D7E8(D_09A0B544, (u32)hud_other_player(self, self[0x57F])) == 0)
                    func_0883AE3C(self, hud_other_player(self, self[0x57F]));
                else
                    func_0883ABE0(self, hud_other_player(self, self[0x57F]));
            } else {
                func_08883988(D_08A62D8C, 0, 21, 0, 0, 0, 0, 0, 0);
            }
        } else if (keys & 0x2000) {
            return keys;
        }
    }
    return (keys & 0xFFFF) & ~0x2000;
}

extern const u16 D_089ACA8C;
extern const u16 D_09B05D06, D_09B05D08, D_09B05D0C;
s16 func_0885AF28(void *a, s16 id);
void func_0889D8A8(void *a, u8 *p, int b, int c);
void func_09AAEE70(void *h, int a, int b, int c);
s32 func_0884FB38(void *game, int v);
u8 menu_msg_push(u8 *self, u32 id, u32 x, u32 y);
void *memcpy(void *d, const void *s, size_t n);

/* asks player p to trade the requested item */
void hud_request_trade(u8 *self, u8 *p)
{
    u8 *h = (u8 *)func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]);
    s16 *req;

    if (h[0x62C] != 0)
        return;
    req = (s16 *)func_0889D7E8(D_09A0B544, (u32)p);
    if (req == NULL || p[0xBF] == 5) {
        func_08883988(D_08A62D8C, 0, 21, 0, 0, 0, 0, 0, 0);
        return;
    }
    if (func_0885AF28(D_08A62C50, req[0]) < 1) {
        menu_msg_push_text(D_089CBBEC, func_0885151C(D_089CC438, D_089ACA8C));
        func_08883988(D_08A62D8C, 0, 21, 0, 0, 0, 0, 0, 0);
        U8((u8 *)func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]), 0x62C) = 90;
    } else if (func_0885AF28(D_08A62C50, req[0]) < req[1]) {
        menu_msg_push(D_089CBBEC, 1, 15, req[0]);
        func_08883988(D_08A62D8C, 0, 21, 0, 0, 0, 0, 0, 0);
        U8((u8 *)func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]), 0x62C) = 90;
    } else {
        func_08883988(D_08A62D8C, 0, 19, 0, 0, 0, 0, 0, 0);
        self[0x580] = 150;
        h = (u8 *)func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]);
        S16(h, 0x5CA) = req[0];
        S16(h, 0x5CC) = req[1];
        func_0889D8A8(D_09A0B544, p, 7, 0);
        func_09AAEE70(h, 0, 6, 0);
        self[0x57E] = 1;
    }
}

/* asks player p for its guild card (if not already present and there is room) */
void hud_request_card(u8 *self, u8 *p)
{
    u8 *h = (u8 *)func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]);
    u8 *card;
    int dup, room;
    s32 i;
    u8 *c;

    if (h[0x62C] != 0)
        return;
    card = (u8 *)func_0889D848(D_09A0B544, (u32)p);
    if (card == NULL || p[0xBF] == 5) {
        func_08883988(D_08A62D8C, 0, 21, 0, 0, 0, 0, 0, 0);
        return;
    }
    if (func_0884FB38(D_089CC438, 0x25) == 0) {
        menu_msg_push_text(D_089CBBEC, func_09A7B790(D_09B22D70, D_09B05D0C));
        func_08883988(D_08A62D8C, 0, 21, 0, 0, 0, 0, 0, 0);
        h[0x62C] = 90;
        return;
    }
    U32(h, 0x5D8) = 0xFFFFFFFF;
    room = 0;
    dup = 0;
    c = D_089CC438 + 0x696FC;
    for (i = 0; i < 5; i++, c += 0x70)
        if (*c != 0 && memcmp(card, c + 0x60, 16) == 0)
            dup = 1;
    c = D_089CC438 + 0x69BEC;
    for (i = 0; i < 13; i++, c += 0x70) {
        if (*c == 0) {
            if (i > 2) {
                room = 1;
                if ((s32)U32(h, 0x5D8) < 0)
                    U32(h, 0x5D8) = i;
            }
        } else if (memcmp(card, c + 0x60, 16) == 0) {
            dup = 1;
        }
    }
    if (dup) {
        menu_msg_push_text(D_089CBBEC, func_09A7B790(D_09B22D70, D_09B05D08));
        func_08883988(D_08A62D8C, 0, 21, 0, 0, 0, 0, 0, 0);
        h[0x62C] = 90;
    } else if (room) {
        func_08883988(D_08A62D8C, 0, 19, 0, 0, 0, 0, 0, 0);
        self[0x580] = 150;
        memcpy(h + 0x5DC, card, 16);
        func_0889D8A8(D_09A0B544, p, 17, 0);
        func_09AAEE70(h, 0, 6, 0);
        self[0x57E] = 1;
    } else {
        menu_msg_push_text(D_089CBBEC, func_09A7B790(D_09B22D70, D_09B05D06));
        func_08883988(D_08A62D8C, 0, 21, 0, 0, 0, 0, 0, 0);
        h[0x62C] = 90;
    }
}

/* input of the page of the players: choose a player, then an action (2 columns) */
u32 hud_player_page_input(u8 *self, u32 keys)
{
    if (self[0x57E] == 1) {
        u8 *p = NULL;
        s32 i, k;

        for (i = 0; i < (s8)D_09A0B544[0xEE3]; i++) {
            const u8 *a = D_09A0B544 + i * 200 + 0x57C;
            int r = 0;

            for (k = 0; k < 6; k++) {
                if (self[0x1172 + k] < a[k]) {
                    r = -1;
                    break;
                }
                if (a[k] < self[0x1172 + k]) {
                    r = 1;
                    break;
                }
            }
            if (r == 0) {
                p = D_09A0B544 + i * 200 + 0x57C;
                break;
            }
        }
        if ((u8)hud_nonzero(self, (s32)p) == 0 || (keys & 0x2000) != 0) {
            keys &= 0xDFFF;
            self[0x57E] -= 1;
            U32(self, 0x5E8) |= 4;
            func_08883988(D_08A62D8C, 0, 20, 0, 0, 0, 0, 0, 0);
        } else {
            menu_cursor_horizontal(self, self + 0x582, keys, 2);
        }
        return keys;
    }
    if (self[0x57E] != 0)
        return keys;
    if ((keys & 0x4000) == 0) {
        menu_cursor_vertical(self, self + 0x57F, keys, 3);
    } else {
        u8 *p = hud_other_player(self, self[0x57F]);

        if ((u8)hud_nonzero(self, (s32)p) == 0) {
            func_08883988(D_08A62D8C, 0, 21, 0, 0, 0, 0, 0, 0);
        } else {
            self[0x57E] += 1;
            memcpy(self + 0x1172, p, 8);
            self[0x582] = 0;
            func_08883988(D_08A62D8C, 0, 19, 0, 0, 0, 0, 0, 0);
            U32(self, 0x5E8) &= ~4;
        }
    }
    return keys;
}

extern u8 D_0893B386;
extern const u8 D_0893B37C[];
extern const char D_0893B390[], D_0893B398[];
extern const u16 D_09B051B4;
extern u8 *D_09A44C60;            /* network manager */
extern u8 *D_08A62C64;
extern char D_089CBBF0[];
void gfx_draw_arrows_blink(u8 *self, u32 x, u32 y, u32 dx, u8 a);
s32 func_088BFCF0(void *net);
void func_088BA860(void *net);
void func_088BA760(void *net, int a, int b);
void func_088BFBEC(void *net);
void func_088BFEF8(void *net, const char *s);
const char *func_08851680(void *game, u16 id);
void func_08880760(void *snd, s32 v);

/* one centered value of the options page */
#define OPTION_TEXT(id, yy) do {                                                          \
        s16 w_ = func_08890E48(D_08A631A8, func_09A7B790(D_09B22D70, (id)));                \
        func_08891474(D_08A631A8, (s16)((s16)(l.x + 196) - (s16)(w_ * 7 >> 1)), yy, D_0893B220, \
                      func_09A7B790(D_09B22D70, (id)));                                   \
    } while (0)

/* page of the options (sound, vibration, network) */
void hud_draw_options(u8 *self)
{
    u8 *m = (u8 *)D_089CBBEC;
    Layout l;
    char title[32];

    D_0893B386 = (*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) == 0 ? 3 : 2;
    layout_build(&l, D_0893B37C, 1, 0, 0);
    sprintf(title, D_0893B390, func_09A7B790(D_09B22D70, D_09B051B4));
    func_0882F580(self, &l, (int)title, -1, 255, 1);
    func_08890E38(D_08A631A8, 5);
    OPTION_TEXT(((const u16 *)0x09B05C98)[(s16)((s8)m[0x608] + 3)], l.ty);
    OPTION_TEXT(((const u16 *)0x09B05C98)[(s16)((s8)m[0x60A] + 5)], (s16)(l.ty + l.lh));
    if ((*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) == 0)
        OPTION_TEXT((&D_09B05C90)[(s16)((s8)m[0x609] ^ 1)], (s16)(l.ty + l.lh * 2));
    gfx_draw_arrows_blink(self, (u16)(l.tx + 0x92), (u16)(l.ty + l.lh * self[0x57F]), 70, 240);
}

/* input of the options page (with the network connection steps 1 and 2) */
u32 hud_options_input(u8 *self, u32 keys, u32 a)
{
    s32 st = U32(self, 0x121C);
    u32 n = 3;

    if (st == 2) {
        if (D_09A44C60[0x971] == 0) {
            D_08A62C64[0x17] = 0;
            U32(self, 0x121C) = 0;
            func_088BA860(D_09A44C60);
        }
        return keys;
    }
    if (st == 1) {
        s32 e, code;

        if (D_09A44C60[0x970] != 0)
            return keys;
        e = *(s32 *)(D_09A44C60 + 0x34);
        if (e == 0) {
            if (func_088BFCF0(D_09A44C60) != 0) {
                U32(self, 0x121C) = 0;
                func_088BA860(D_09A44C60);
                return keys;
            }
            code = 67;
        } else if (e == (s32)0x80110321 || e == (s32)0x80110322) {
            code = 69;
        } else if (e == (s32)0x80110327) {
            code = 70;
        } else if (e == (s32)0x80110326) {
            code = 67;
        } else {
            code = 0;
        }
        if (code == 0) {
            sprintf(D_089CBBF0, func_08851680(D_089CC438, 64), D_0893B398);
        } else {
            const char *f = func_08851680(D_089CC438, 64);

            sprintf(D_089CBBF0, f, func_08851680(D_089CC438, code));
        }
        func_088BFEF8(D_09A44C60, D_089CBBF0);
        D_08A62C64[0x17] = 1;
        self[0x609] = 0;
        D_089CC438[0x6ADDE] = 0;
        U32(self, 0x121C) += 1;
        return keys;
    }
    if (st != 0)
        return keys;
    if ((*(u32 *)(D_08A631A0 + 0xF7A57C) & 1) == 1)
        n = 2;
    menu_cursor_vertical(self, self + 0x57F, keys, n);
    U16(self, 0x5FC) = self[0x57F] + 9;
    if ((keys & 0x20) == 0 && (keys & 0x80) == 0)
        return keys;
    if (self[0x57F] == 1) {
        u8 v = 0;
        s8 m;

        if (keys & 0x80) {
            if ((s8)self[0x60A] == 0)
                self[0x60A] = 2;
            else
                self[0x60A] = (s8)self[0x60A] - 1;
        } else {
            self[0x60A] = (s8)self[0x60A] + 1;
            if ((s8)self[0x60A] >= 3)
                self[0x60A] = 0;
        }
        m = self[0x60A];
        if (m == 2)
            v = 3;
        else if (m == 1)
            v = 2;
        else if (m == 0)
            v = 1;
        D_089CC438[0x6ADD9] = v;
        func_08880760(D_08A62D8C, (s8)D_089CC438[0x6ADD9]);
        func_08883988(D_08A62D8C, 0, 23, 0, 0, 0, 0, 0, 0);
    } else if (self[0x57F] == 2) {
        s8 v = (s8)self[0x609] ^ 1;

        self[0x609] = v;
        D_089CC438[0x6ADDE] = v;
        if ((s8)D_089CC438[0x6ADDE] != 0) {
            U32(self, 0x121C) += 1;
            func_088BA760(D_09A44C60, 128, 0);
            func_088BFBEC(D_09A44C60);
        }
        func_08883988(D_08A62D8C, 0, 23, 0, 0, 0, 0, 0, 0);
    } else if (self[0x57F] == 0) {
        self[0x608] = (s8)self[0x608] ^ 1;
        D_089CC438[0x6ADE3] = ((u8 *)D_089CBBEC)[0x608];
        func_08883988(D_08A62D8C, 0, 23, 0, 0, 0, 0, 0, 0);
    }
    return keys;
}

extern const u16 D_09B051AE;
extern const u8 D_0893B39C[], D_0893B3B0[];
extern const u16 D_089AC8C4[2], D_089AC8E0;
extern const s16 D_0893B3BC, D_0893B3BE, D_0893B3C0, D_0893B3C4, D_0893B3C6, D_0893B3C8;
extern const s16 D_0893B3CA, D_0893B3CC, D_0893B3CE;
extern const char D_0893B3D0[];
extern const s32 D_0893AEB8[5];
const char *func_0885C968(void *a, u32 type, u16 id);
void func_088D897C(void *hunter);
void equip_draw_icon_rarity(void *self, u32 type, u32 id, u32 x, u32 y, u32 w_arg, s32 h_arg,
                            u32 alpha, u8 alt);
void menu_draw_weapon_type(void *self, int x, int y, const u8 *eq);
void menu_draw_unique_list(void *self, const u8 *list, void *p, int x, int y, s32 step);
void func_088911A0(void *font, void *p);

/* copies an equipment piece (as the original: two bytes, then halfwords) */
#define EQ_CP(d, s) do {                                                  \
        U8(d, 0) = U8(s, 0);   U8(d, 1) = U8(s, 1);                       \
        U16(d, 2) = U16(s, 2); U16(d, 4) = U16(s, 4);                     \
        { u16 a_ = U16(s, 8), b_ = U16(s, 10);                             \
          U16(d, 6) = U16(s, 6); U16(d, 8) = a_; U16(d, 10) = b_; }       \
    } while (0)

/* guild card of a player: page 0 name, weapon, rank, title; page 1 equipment and skills */
void hud_draw_player_card(u8 *self, u32 page, u8 *card)
{
    u8 *base = card + 0x20;
    char title[64];
    Layout l;

    sprintf(title, D_0893B218, func_09A7B790(D_09B22D70, D_09B051AE));
    layout_build(&l, D_0893B39C, (int)title, 0, 0);
    if (page == 1) {
        l.ptr = NULL;
        l.h = 144;
    }
    func_0882F580(self, &l, (int)title, -1, 255, 1);
    layout_draw_pages(self, &l, page & 0xFFFF, 2, 0, 0);
    layout_draw(self, (Layout *)D_0893B3B0, func_0885151C(D_089CC438, D_089AC8C4[page]), 255);
    if (page == 1) {
        u8 *g, *h, *ga;
        u8 sv[5][12];
        u8 w0, w1;
        u16 w2, w3, w4, w5, w6;
        int i;

        equip_draw_icon_rarity(self, card[0x41], S16(card, 0x42), D_0893B3C4, D_0893B3C6, D_0893B3C8,
                               D_0893B3C8, 255, 0);
        func_088920A8(D_08A631A8, (s16)(D_0893B3C4 + D_0893B3CC), (s16)(D_0893B3C6 + D_0893B3CE),
                      func_0885151C(D_089CC438, D_089AC8E0),
                      func_0885C968(D_08A62C54, card[0x41], U16(card, 0x42)));
        for (i = 0; i < 5; i++) {
            u32 t = D_0893AEB8[i];

            equip_draw_icon_rarity(self, (s16)t, S16(base + t * 0xC, 0x2E), D_0893B3C4,
                                   (D_0893B3C6 + D_0893B3CA * (i + 1)) & 0xFFFF, D_0893B3C8, D_0893B3C8,
                                   255, 0);
            func_088920A8(D_08A631A8, (s16)(D_0893B3C4 + D_0893B3CC),
                          (s16)(D_0893B3C6 + D_0893B3CE + D_0893B3CA * ((s16)i + 1)),
                          func_0885151C(D_089CC438, D_089AC8E0),
                          func_0885C968(D_08A62C54, t & 0xFF, U16(base + t * 0xC, 0x2E)));
        }
        g = D_089CC438;
        ga = g + 0x4A0;
        h = (u8 *)func_088DCEBC(D_09A4F0C4, g[0x28]);
        w4 = U16(g, 0x4C6);
        w5 = U16(g, 0x4C8);
        w6 = U16(g, 0x4CA);
        w0 = g[0x4C0];
        w1 = g[0x4C1];
        w2 = U16(g, 0x4C2);
        w3 = U16(g, 0x4C4);
        for (i = 0; i < 5; i++)
            EQ_CP(sv[i], ga + 0x2C + i * 0xC);
        EQ_CP(h + 0x4E8, card + 0x40);
        EQ_CP(g + 0x4C0, h + 0x4E8);
        for (i = 0; i < 5; i++) {
            EQ_CP(h + 0x4F4 + i * 0xC, base + 0x2C + i * 0xC);
            EQ_CP(ga + 0x2C + i * 0xC, h + 0x4F4 + i * 0xC);
        }
        func_088D897C(h);
        hud_draw_skill_compare(D_089CBBEC, h, NULL, D_0893B3BC, (s16)(D_0893B3BE + (s16)D_0893B3C0));
        h[0x4E8] = w0;
        h[0x4E9] = w1;
        U16(h, 0x4EA) = w2;
        U16(h, 0x4EC) = w3;
        U16(h, 0x4EE) = w4;
        U16(h, 0x4F0) = w5;
        U16(h, 0x4F2) = w6;
        EQ_CP(g + 0x4C0, h + 0x4E8);
        for (i = 0; i < 5; i++) {
            EQ_CP(h + 0x4F4 + i * 0xC, sv[i]);
            EQ_CP(ga + 0x2C + i * 0xC, h + 0x4F4 + i * 0xC);
        }
        func_088D897C(h);
    } else if (page == 0) {
        s16 x;

        func_08890E38(D_08A631A8, 0);
        x = (s16)(l.tx + l.cw * 4);
        func_08890DFC(D_08A631A8, x, l.ty);
        func_08890DE8(D_08A631A8, 16, 14);
        func_088911A0(D_08A631A8, card + 8);
        func_08890DE8(D_08A631A8, 14, 14);
        menu_draw_weapon_type(self, x, (s16)(l.ty + l.lh), card + 0x40);
        func_08891474(D_08A631A8, x, (s16)(l.ty + l.lh * 2), D_0893B3D0, card[0x88] + 1);
        menu_draw_unique_list(self, card + 0x89, card + 0x40, D_0893B3BC,
                              (s16)(D_0893B3BE + (s16)D_0893B3C0), (s16)D_0893B3C0);
    }
}
#ifndef S32
#define S32(p, off) (*(s32 *)((u8 *)(p) + (off)))
#endif

extern const u32 D_0893B400[5];   /* box of the notices */

/* adds a notice (player name, kind) to the list at self+0x60C (5 entries of 0x18) */
void hud_notice_add(u8 *self, const char *name, s32 kind)
{
    s32 i, off;

    for (i = 0; i < 5; i++)
        if (S32(self + i * 0x18, 0x620) == -1)
            break;
    if (i >= 5) {
        s32 k;

        for (k = 0; k < 4; k++)
            memcpy(self + k * 0x18 + 0x60C, self + k * 0x18 + 0x624, 0x18);
        i = 4;
    }
    off = i * 0x18;
    memcpy(self + off + 0x60C, name, 0x12);
    S32(self + off, 0x620) = kind;
    U32(self, 0x5E8) |= 0x80;
}

/* clears the notices */
void hud_notice_clear(u8 *self)
{
    S32(self, 0x620) = -1;
    S32(self, 0x638) = -1;
    S32(self, 0x650) = -1;
    S32(self, 0x668) = -1;
    S32(self, 0x680) = -1;
}

/* draws the notices (name of the player and text of the event) */
void hud_notice_draw(u8 *self)
{
    u32 t[5];
    u8 *e = self;
    int i;

    t[0] = D_0893B400[0];
    t[1] = D_0893B400[1];
    t[2] = D_0893B400[2];
    t[3] = D_0893B400[3];
    t[4] = D_0893B400[4];
    layout_draw(self, (Layout *)t, NULL, 255);
    for (i = 0; i < 5; i++, e += 0x18) {
        if (S32(e, 0x620) != -1) {
            func_08890DE8(D_08A631A8, 16, 14);
            func_088912BC(D_08A631A8, S16(t, 12), (s16)(S16(t, 14) + (s16)i * S16(t, 16) * 2),
                          (const char *)(e + 0x60C));
            func_08890DE8(D_08A631A8, 14, 14);
            func_08891474(D_08A631A8, S16(t, 12), (s16)(S16(t, 14) + (s16)i * S16(t, 16) * 2),
                          func_09A7B790(D_09B22D70, ((const u16 *)0x09B052C0)[S32(e, 0x620)]));
        }
    }
}

/* 1 when the notices can be shown (game state 0, 8 or 22) */
int hud_notice_allowed(void *self)
{
    s32 st = *(s32 *)D_09B22D70;

    if (st == 0)
        return 1;
    if (st == 22 || st == 8)
        return 1;
    return 0;
}

/* selects the player targeted by the hunter (1 if found) */
int hud_select_target(u8 *self)
{
    u8 *h = (u8 *)func_088DCEBC(D_09A4F0C4, D_089CC438[0x28]);
    u8 *t = *(u8 **)(h + 0x2F4);

    if (t == NULL)
        return 0;
    if (func_0883B498(self, U16(t, 0x1E6)) == -1)
        return 0;
    self[0x57F] = func_0883B498(self, U16(t, 0x1E6));
    memcpy(self + 0x1172, D_09A0B544 + self[0x57F] * 200 + 0x57C, 8);
    self[0x582] = 0;
    func_08883988(D_08A62D8C, 0, 17, 0, 0, 0, 0, 0, 0);
    return 1;
}

extern const s16 D_0893B474[6];
extern const char D_0893B480[];
extern const u32 D_0893B488[5];
extern const u8 D_0893B0F4[20];
extern const u16 D_089AC84C, D_089AC84E, D_089AC9FE;
extern const u16 D_09B04E06;
extern const u32 D_0893B49C[];
extern void **D_089CC434;         /* save data manager (C++ object) */
extern void *D_09A0B540;
extern void *D_089CBBE0;
void gfx_draw_window(u8 *self, u32 x, u32 y, u32 w, u32 h, u32 alpha);
void func_08852EE0(void *game, u8 v);
void func_088197A0(void *self, u8 a, u32 b, int c, int d);
void func_08819128(void *self, int a, u16 id, int b);
void layout_draw_from(void *self, const u8 *src, int title, int a, int alpha, int c);
void func_08898748(void *a, int b);
s32 func_08898898(void *a, u32 b);
s32 func_08898694(void *a, int b, s32 c);
void func_0881263C(void *bank, int a, s32 b, int c, int d, int e);
void hud_draw_player_card(u8 *self, u32 page, u8 *card);

/* input of the card of the selected player (page change, back) */
int hud_player_card_input(u8 *self, u32 keys)
{
    u8 *p = NULL;
    s32 i, k;

    for (i = 0; i < (s8)D_09A0B544[0xEE3]; i++) {
        const u8 *a = D_09A0B544 + i * 200 + 0x57C;
        int r = 0;

        for (k = 0; k < 6; k++) {
            if (self[0x1172 + k] < a[k]) {
                r = -1;
                break;
            }
            if (a[k] < self[0x1172 + k]) {
                r = 1;
                break;
            }
        }
        if (r == 0) {
            p = D_09A0B544 + i * 200 + 0x57C;
            break;
        }
    }
    if (p == NULL)
        return 1;
    if (p[0xBF] == 5)
        return 1;
    if (keys & 0x2000) {
        func_08883988(D_08A62D8C, 0, 20, 0, 0, 0, 0, 0, 0);
        return 1;
    }
    menu_cursor_horizontal(self, self + 0x582, keys, 2);
    self[0x534] = 10;
    func_08890E08(D_08A631A8, 1);
    menu_begin_draw(self);
    hud_draw_player_card(self, self[0x582], p);
    return 0;
}

/* unlocks the emblems that use the guild card design id */
void hud_unlock_emblem(void *self, s32 id_arg)
{
    s16 id = id_arg;
    const u8 *e = (const u8 *)0x08930BDC;
    int n;

    for (n = 62; n > 0; n--, e += 14)
        if (S16(e, 10) == id)
            func_08852EE0(D_089CC438, e[0]);
}

/* box with the money of the player (menu instance) */
void hud_draw_money(void *self)
{
    s16 r[6];
    int i;

    for (i = 0; i < 6; i += 2) {
        r[i] = D_0893B474[i];
        r[i + 1] = D_0893B474[i + 1];
    }
    func_08890DE8(D_08A631A8, 14, 14);
    func_08890E38(D_08A631A8, 0);
    gfx_draw_window(D_089CBBEC, r[0], r[1], r[2], r[3], 255);
    func_08891474(D_08A631A8, r[4], r[5], D_0893B480, *(u32 *)(D_089CC438 + 0x696F0));
}

/* box of the item just obtained (with the quantity and the "pouch full" choice) */
void hud_draw_item_get(u8 *self)
{
    u32 t[5];
    u8 st = self[0x52F];
    u16 id;

    t[0] = D_0893B488[0];
    t[1] = D_0893B488[1];
    t[2] = D_0893B488[2];
    t[3] = D_0893B488[3];
    t[4] = D_0893B488[4];
    if (st < 3 || st == 7)
        return;
    func_088197A0(self, self[0x586], D_0893AA94[st], 16, 0);
    if (S8(self, 0x592) == 0) {
        Layout c;
        s32 w, h2, u;

        c.x = U16(D_0893B0F4, 0);
        c.y = U16(D_0893B0F4, 2);
        c.w = U16(D_0893B0F4, 4);
        c.h = U16(D_0893B0F4, 6);
        c.cw = D_0893B0F4[8];
        c.b9 = D_0893B0F4[9];
        c.ba = D_0893B0F4[10];
        c.bb = D_0893B0F4[11];
        c.tx = U16(D_0893B0F4, 12);
        c.ty = U16(D_0893B0F4, 14);
        *(u32 *)&c.lh = U32(D_0893B0F4, 16);
        w = func_08890E48(D_08A631A8, func_0885151C(D_089CC438, D_089AC84C));
        c.tx = 240 - (s16)((s32)(w * c.cw) >> 2);
        h2 = (s32)(w * c.cw) >> 1;
        u = ((h2 + 32) / 16) * 16;
        c.w = u;
        c.x = 240 - (s16)((s32)(u & 0xFFFF) >> 1);
        layout_draw(self, &c, func_0885151C(D_089CC438, D_089AC84C), 255);
        U32(*(u8 **)(self + 0x530), 0x614) &= ~1;
        id = U16(self, 0x58E);
    } else {
        s8 n = self[0x593];

        if (n < 2) {
            layout_draw(self, (Layout *)D_0893B108, func_08851528(D_089CC438, U16(self, 0x590)), 255);
            id = U16(self, 0x58E);
        } else {
            char buf[64];
            const char *f = func_0885151C(D_089CC438, D_089AC84E);

            sprintf(buf, f, func_08851528(D_089CC438, U16(self, 0x590)), n);
            layout_draw(self, (Layout *)D_0893B11C, buf, 255);
            id = U16(self, 0x58E);
        }
    }
    func_08819128(self, 1, id, -1);
    st = self[0x52F];
    if ((s8)st == 5) {
        layout_draw_from(self, D_0893B130, 0, self[0x537], 255, 0);
    } else if ((s8)st == 3 || (s8)st == 4) {
        Layout l;
        u8 col;

        if ((s8)st == 4 && self[0x5DA] == 0) {
            menu_draw_sprite(self, 0x10E, 0x50, 0x35);
            func_088915B0(D_08A631A8, 0x120, 0x52, 0, func_0885151C(D_089CC438, D_089AC9FE));
        }
        layout_build(&l, (const u8 *)t, 0, 0, 0);
        l.color = D_0893AAB0[self[0x52F]];
        func_0882F580(self, &l, 0, self[0x58C], 255, 0);
        col = func_0885C3DC(D_08A62C50, U16(self, 0x590)) < (s8)self[0x593] ? 10 : 0;
        func_088915B0(D_08A631A8, l.tx, l.ty, col, func_09A7B790(D_09B22D70, D_09B04E06));
        func_088915B0(D_08A631A8, l.tx, (s16)(l.ty + l.lh), 0,
                      func_0885151C(D_089CC438, *(const u16 *)D_089AC600));
        func_088915B0(D_08A631A8, l.tx, (s16)(l.ty + l.lh * 2), 0,
                      func_0885151C(D_089CC438, *(const u16 *)(D_089AC600 + 2)));
    }
}

typedef s32 (*HVFunc0)(void *);
typedef s32 (*HVFunc1)(void *, u32);
typedef s32 (*HVFunc6)(void *, u32, s32, s32, s32, s32, s32);
#define HVCALL(type, obj, off) ((type)((void **)*(obj))[(off) / 4])

/* loads the data of the current area for the HUD (3 steps); 1 when done */
int hud_load_area_data(u8 *self)
{
    s32 st = U32(self, 0x1198);

    if (st == 2) {
        func_08898748(D_09A0B540, 0x19);
        U32(self, 0x1198) = 0;
        return 1;
    }
    if (st == 1) {
        if ((u8)HVCALL(HVFunc0, D_089CC434, 0x38)(D_089CC434) == 0) {
            func_0881263C(D_089CBBE0, 0x2B, func_08898898(D_09A0B540, 0x19), 0, 0, 0);
            U32(self, 0x1198) += 1;
        }
    } else if (st == 0) {
        u32 id;
        s32 r;

        switch (U16(D_089CC438, 0x6AF12)) {
        case 0x4E21:
        case 0x4E22:
            id = 0x14FB;
            break;
        case 0x4E23:
        case 0x4E24:
        case 0x4E25:
        case 0x4E26:
            id = 0x14FC;
            break;
        default:
            id = D_0893B49C[D_089CC438[0x6AF10]];
        }
        r = HVCALL(HVFunc1, D_089CC434, 0x18)(D_089CC434, id & 0xFFFF);
        r = func_08898694(D_09A0B540, 0x19, r);
        HVCALL(HVFunc6, D_089CC434, 0x34)(D_089CC434, id & 0xFFFF, r, -1, 0, 0, 1);
        U32(self, 0x1198) += 1;
    }
    return 0;
}

extern void *D_08A62C7C;
s32 func_088DACF8(void *hunter);
u32 func_088568AC(void *game, s16 id);
u8 func_088567B4(void *game);
void func_0883FDF4(void *self);
void func_08841520(void *self);
void func_088405E4(void *self);
void func_08841100(void *self);
void func_08841908(void *self);
void func_08842628(void *self);
void func_088428EC(void *self);
void func_08846FAC(void *self);
void func_08847CB0(void *self);
void func_088487B8(void *self);
void func_08896484(void *self);
void func_0889645C(void *self);
void func_08842EE8(void *self);
void func_08818950(void *self);
void func_08845A34(void *self);
void func_08849848(void *self);
void func_08849CAC(void *self);
void func_08818C94(void *self, int a);
void func_08818F74(void *self);
void func_08849478(void *self);
s16 func_09A6DEC0(void *h);
u8 func_09A6FA20(void *h);
void gfx_draw_sprite_rotated_pivot(void *self, u32 x, u32 y, s32 px, s32 py, u32 w, u32 h, s32 ang,
                                   u8 a, s32 id);
void hud_draw_clock_hand(u8 *self, s32 t, s32 big);
void hud_draw_clock(u8 *self);
void menu_draw_choice(void *self);
void menu_draw_trade(void *self);
void menu_draw_info(void *self);
void menu_draw_item_box(void *self);
void menu_draw_equip_box(void *self);
void menu_draw_main(void *self);
void menu_draw_entry_desc(void *self);
void menu_draw_typewriter(u8 *self);

/* countdown sounds of the 4 timers at self+0x56C (one beep every 20 frames) */
void hud_timer_sounds(u8 *self)
{
    int i;

    for (i = 0; i < 4; i++, self += 2) {
        s32 t = S16(self, 0x56C);

        if (t < 0)
            continue;
        if (t < 100 && t % 20 == 0)
            func_08883988(D_08A62D8C, 1, t / 20 + 123, 0, 0, 0, 0, 0, 0);
        S16(self, 0x56C) = S16(self, 0x56C) + 1;
        if (S16(self, 0x56C) >= 106)
            S16(self, 0x56C) = -1;
    }
}

/* 1 if the hunter can see the map (flag, or the item of the map) */
int hud_has_map(void *self, u8 *hunter)
{
    if (U32(hunter, 0x614) & 0x10)
        return 1;
    if ((u8)func_088DACF8(hunter) == 1)
        return func_088568AC(D_089CC438, 0x369) != 0;
    return 0;
}

/* layer 9 of the HUD: map, status bars and timer */
void hud_draw_layer0(u8 *self)
{
    s8 k;

    if (func_088567B4(D_089CC438) == 1)
        return;
    self[0x534] = 9;
    func_08890E08(D_08A631A8, 0);
    menu_begin_draw(self);
    func_0883FDF4(self);
    func_08841520(self);
    if (self[0x5DB] == 0) {
        hud_draw_clock(self);
        func_088405E4(self);
        func_08841100(self);
        func_08841908(self);
        func_08842628(self);
    } else if (self[0x57B] == 0) {
        k = U8(*(u8 **)(self + 0x530), 0x1E8);
        if (k == 1 || k == 5)
            func_088428EC(self);
    }
}

/* layer 10 of the HUD: pages of the in-game menu */
void hud_draw_layer1(u8 *self)
{
    self[0x534] = 10;
    func_08890E08(D_08A631A8, 1);
    menu_begin_draw(self);
    if (func_088567B4(D_089CC438) == 1) {
        if (*(u32 *)(D_089CC438 + 0x6AF14) & 0x100) {
            if (self[0x52D] != 0 && self[0x57C] == 1 && self[0x57D] == 4)
                func_08896484(self);
            func_0889645C(self);
        }
    } else if (self[0x604] != 0) {
        if ((*(u32 *)(D_089CC438 + 0x6AF14) & 0x100) == 0)
            func_08842EE8(self);
    } else if (self[0x52F] != 0) {
        func_08818950(self);
    } else if (self[0x52D] == 0) {
        if ((U32(*(u8 **)(self + 0x530), 0x614) & 2) == 0 && self[0x554] == 0) {
            func_08842EE8(self);
            func_08845A34(self);
        }
    } else {
        if (self[0x57C] == 1) {
            switch (self[0x57D]) {
            case 0: menu_draw_choice(self); break;
            case 1: menu_draw_trade(self); break;
            case 2: menu_draw_info(self); break;
            case 3: hud_draw_choice_grid(self); break;
            case 4: func_08896484(self); break;
            case 6: menu_draw_item_box(self); break;
            case 7: menu_draw_equip_box(self); break;
            case 8: menu_draw_main(self); break;
            case 9: func_08849848(self); break;
            case 10: func_08849CAC(self); break;
            }
        } else if (self[0x57C] == 0) {
            func_08818C94(self, 0);
            func_08818F74(self);
        }
        if (U32(self, 0x5E8) & 4)
            menu_draw_entry_desc(self);
    }
    func_08849478(self);
}

/* layer 11 of the HUD: messages and typewriter text */
void hud_draw_layer2(u8 *self)
{
    if (func_088567B4(D_089CC438) == 1)
        return;
    self[0x534] = 11;
    func_08890E08(D_08A631A8, 2);
    menu_begin_draw(self);
    func_08846FAC(self);
    if (self[0x604] == 0 && self[0x52F] == 0 && self[0x52D] == 0) {
        func_08847CB0(self);
        func_088487B8(self);
    }
    if (self[0x5F0] != 0) {
        menu_draw_typewriter(self);
        self[0x5F0] = 0;
    }
}

/* clock of the quest: blinking icon and the two hands (elapsed / total time) */
void hud_draw_clock(u8 *self)
{
    s32 total, left, d;
    u8 c;

    if (U32(self, 0x1194) & 1)
        return;
    S32(self, 0x558) = func_0886A35C(D_08A62C7C, 0);
    total = (s32)func_0886A35C(D_08A62C7C, 1);
    S32(self, 0x55C) = total;
    left = S32(self, 0x558);
    if (left >= 9001) {
        c = 255;
    } else if (left >= 1801) {
        f32 s = vfpu_sinf((f32)(u16)((*(u32 *)(D_08A631A0 + 0xF7A554) & 0x1F) << 11) * (3.14159265f / 32768.0f));

        c = (s8)(int)(60.0f * s) + 192;
    } else {
        f32 s = vfpu_sinf((f32)(u16)((*(u32 *)(D_08A631A0 + 0xF7A554) & 0x1F) << 12) * (3.14159265f / 32768.0f));

        c = (s8)(int)(120.0f * s) + 128;
    }
    gfx_draw_sprite(self, 8, 8, 255, c, c, 0, 0);
    hud_draw_clock_hand(self, total, 1);
    d = total - left;
    if (d < 0)
        d = 0;
    hud_draw_clock_hand(self, d, 0);
}

/* hand of the clock for the time t (one turn every 54000 frames) */
void hud_draw_clock_hand(u8 *self, s32 t, s32 big)
{
    gfx_draw_sprite_rotated_pivot(self, 35, 33, 5, 16, 9, 18, (s16)((t / 9000) * 0x1555), 255, big != 0 ? 2 : 1);
}

/* 1 if the hunter is in danger (low health, poisoned, ...) */
int hud_hunter_danger(u8 *self)
{
    u8 *h = *(u8 **)(self + 0x530);

    if (func_09A6DEC0(h) >= 1)
        return 1;
    if (U16(h, 0x424) & 1)
        return 1;
    return func_09A6FA20(h) == 1;
}

/* color of the health bar (pulsing red when in danger) */
u32 hud_health_color(u8 *self)
{
    u32 c = 0xFF10C010;

    if ((u8)hud_hunter_danger(self) == 1) {
        s16 a = S16(self, 0x11A0) + 256;
        f32 s;

        S16(self, 0x11A0) = a;
        s = vfpu_cosf((3.14159265f / 32768.0f) * (f32)a);
        c = (((s8)(int)(-28.0f * s) + 45) << 16) | 0xFF000000;
        c |= ((s8)(int)(15.0f * s) + 177) << 8;
        c |= (s8)(int)(-72.0f * s) + 88;
    } else {
        S16(self, 0x11A0) = 0;
    }
    return c;
}

extern const s16 D_0893B1BC[];    /* blinking speeds of the bars */
u8 func_088D6AC8(void *hunter, int skill);
u8 func_088DAE98(void *hunter, int flag);

/* color of the health bar: blinking with the skill 0x30 level, or when in danger */
u32 hud_health_bar_color(u8 *self, s32 kind_arg)
{
    s16 kind = kind_arg;
    u8 *h = *(u8 **)(self + 0x530);
    u32 c = 0xFF10C010;
    s16 sp = 0;

    if (S16(h, 0x624) == 0) {
        s8 k = func_088D6AC8(h, 0x30);

        if (k != 0x7B && func_088DAE98(h, 0x100) == 0) {
            s32 i, r = 0;

            if (k == 0x7D)
                i = 3;
            else if (k == 0x7C)
                i = 2;
            else if (k == 0x7A)
                i = 0;
            else
                i = 1;
            if (kind == 2) {
                r = 1;
            } else if (kind == 1) {
                r = 0;
            } else if (kind == 0) {
                r = 0;
                i = 0;
            }
            sp = D_0893B1BC[r * 4 + i];
        }
    }
    if (sp == 0 && (u8)hud_hunter_danger(self) == 1)
        sp = 256;
    if (sp == 0) {
        S16(self, 0x11A0) = 0;
    } else {
        f32 s;

        sp = S16(self, 0x11A0) + sp;
        S16(self, 0x11A0) = sp;
        s = vfpu_cosf((3.14159265f / 32768.0f) * (f32)sp);
        c = (((s8)(int)(-28.0f * s) + 45) << 16) | 0xFF000000;
        c |= ((s8)(int)(15.0f * s) + 177) << 8;
        c |= (s8)(int)(-72.0f * s) + 88;
    }
    return c;
}

/* color of the stamina bar: blinking with the skill 0x31 level */
u32 hud_stamina_bar_color(u8 *self, s32 kind_arg)
{
    s16 kind = kind_arg;
    u8 *h = *(u8 **)(self + 0x530);
    u32 c = 0xFFF0F000;
    s16 sp = 0;

    if (S16(h, 0x626) == 0) {
        s8 k = func_088D6AC8(h, 0x31);

        if (k != 0x7F && func_088DAE98(h, 0x80) == 0) {
            s32 i, r = 0;

            if (k == -0x7F)
                i = 3;
            else if (k == -0x80)
                i = 2;
            else if (k == 0x7E)
                i = 0;
            else
                i = 1;
            if (kind == 4)
                r = 1;
            else if (kind == 3)
                r = 0;
            sp = D_0893B1BC[r * 4 + i];
        }
    }
    if (sp == 0) {
        S16(self, 0x11A2) = 0;
    } else {
        f32 s;

        sp = S16(self, 0x11A2) + sp;
        S16(self, 0x11A2) = sp;
        s = vfpu_cosf((3.14159265f / 32768.0f) * (f32)sp);
        c = (((s8)(int)(58.0f * s) + 182) << 16) | 0xFF000000;
        c |= ((s8)(int)(102.0f * s) + 138) << 8;
        c |= (s8)(int)(-21.0f * s) + 21;
    }
    return c;
}

/* 1 if the hunter can see the monsters on the map (skill 0x33, or the item 0x35F) */
int hud_sees_monsters(void *self, void *hunter)
{
    u8 k = func_088D6AC8(hunter, 0x33);

    if (k == 0)
        return func_088568AC(D_089CC438, 0x35F) != 0;
    return k == 0x85;
}

int hud_area_kind(void *self, s32 area_arg);

/* kind of the current area of the hunter (stored in self+0x578) */
void hud_update_area_kind(u8 *self)
{
    self[0x578] = hud_area_kind(self, S16(*(u8 **)(self + 0x530), 0x29A));
}

/* kind of area: 0 normal, in the towers (game 14/15) 1 for the areas 0x79..0x7B, else 2 */
int hud_area_kind(void *self, s32 area_arg)
{
    s16 area = area_arg;

    if (D_089CC438[0x6AF10] != 15 && D_089CC438[0x6AF10] != 14)
        return 0;
    if (area == 0x7B)
        return 1;
    if (area == 0x7A || area == 0x79)
        return 1;
    return 2;
}
