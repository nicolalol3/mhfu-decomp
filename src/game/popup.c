/* Message popup (item obtained, quest notices, ...): text, size and state.
 * Fields: 0 active, 1 kind, 2 state, 3, 4 f32 y, 8 / 0xA ids, 0xC x, 0xE width, 0x14 height,
 * 0x16 text x, 0x18 timer, 0x20 text (kinds 4 and 6). (Guessed names.) */
#include "menu.h"
#undef layout_draw

extern void *g_sound;          /* sound */
extern const u16 D_089ACA0C[], D_089ACA40[], D_089ACA68[], D_089ACA7C[], D_089AD058[];
extern const u16 D_089ACA84, D_089ACA86, D_089ACA88, D_089ACA8A;
extern const char D_0899D114[], D_0899D118[], D_0899D120[], D_0899D12C[], D_0899D138[];
extern const s16 D_0899D110, D_0899D112;   /* centre of the popup */
extern u8 D_089BAB98[], D_089BAAF4[];
extern const f32 D_0899D174[3], D_0899D180[3];

void sound_play(void *snd, int a, int b, int c, int d, int e, int f, int g, int h);
char *strchr(const char *s, int c);
s32 func_08891074(void *font, const char *s);
s32 font_text_width(void *font, const char *s);
const char *text_table7(void *game, u32 id);
void popup_draw(u8 *m);
void heap_free(u32 *h, u8 *p);
s32 vec3_nonzero(f32 *v);
void stub_nop(void *p);

#define TEXT(id) text_menu(g_game, id)

/* opens the popup: builds the text and computes the size of the window */
void popup_open(u8 *m)
{
    char buf[512];
    char *s, *nl;
    s16 lines = 0, maxw = 0;
    s32 v;
    u16 cx, cy;

    m[2]++;
    m[3] = 0;
    sound_play(g_sound, 0, 0x10, 0, 0, 0, 0, 0, 0);
    switch (m[1]) {
    case 0:
        sprintf(buf, D_0899D114, TEXT(D_089ACA0C[S16(m, 0xA)]));
        break;
    case 1:
        sprintf(buf, TEXT(D_089ACA40[S16(m, 0xA)]), text_item_name(g_game, U16(m, 8)));
        break;
    case 2:
        sprintf(buf, D_0899D118, TEXT(D_089ACA68[S16(m, 0xA)]), text_item_name(g_game, U16(m, 8)));
        break;
    case 3:
        sprintf(buf, D_0899D118, D_0899D120, TEXT(D_089ACA7C[S16(m, 0xA)]));
        break;
    case 4:
    case 6:
        sprintf(buf, D_0899D114, (const char *)U32(m, 0x20));
        break;
    case 5: {
        const char *a = text_item_name(g_game, U16(m, 0xA));
        const char *b = TEXT(D_089ACA84);

        sprintf(buf, D_0899D12C, a, b, S16(m, 8), TEXT(D_089ACA86));
        break;
    }
    case 7: {
        const char *a = TEXT(D_089ACA88);
        const char *b = TEXT(D_089AD058[S16(m, 0xA)]);

        sprintf(buf, D_0899D138, a, b, TEXT(D_089ACA8A));
        break;
    }
    case 8:
        sprintf(buf, TEXT(D_089ACA40[S16(m, 0xA)]), text_table7(g_game, U16(m, 8)));
        break;
    }
    s = buf;
    for (;;) {
        s32 w;

        nl = strchr(s, '\n');
        if (nl == NULL) {
            w = font_text_width(g_font, s);
            if (maxw < w)
                maxw = w;
            break;
        }
        lines++;
        w = func_08891074(g_font, s);
        if (maxw < w)
            maxw = w;
        s = nl + 1;
    }
    S16(m, 0x14) = lines * 16 + 0x20;
    v = (s16)(maxw - 1) * 7 + 0x20;
    S16(m, 0xE) = (v / 16) * 16;
    if (v % 16 != 0)
        S16(m, 0xE) += 16;
    cx = D_0899D110;
    cy = D_0899D112;
    S16(m, 0xC) = cx - (S16(m, 0xE) >> 1);
    U16(m, 0x18) = 60;
    F32(m, 4) = ((f32)cy - (f32)(S16(m, 0x14) >> 1)) / 3.0f;
    U32(m, 0x10) = 0;
    S16(m, 0x16) = cx - ((maxw * 7) >> 1);
    popup_draw(m);
}

void popup_next(u8 *m);
void popup_close(u8 *m);

/* update of the popup, by state */
void popup_update(u8 *m)
{
    switch (m[2]) {
    case 0:
        popup_open(m);
        break;
    case 1:
        popup_draw(m);
        break;
    case 2:
        popup_next(m);
        break;
    case 3:
        popup_close(m);
        break;
    }
}

void popup_next(u8 *m)
{
    m[2]++;
}

void popup_close(u8 *m)
{
    m[0] = 0;
    m[3] = 0;
    m[2] = 0;
    if (m[1] == 6)
        heap_free((u32 *)((u8 *)g_menu + 0x500), (u8 *)U32(m, 0x20));
}

/* shows a message of the kind with the ids b, a */
void popup_show(u8 *m, u32 kind, u32 b, u32 a, u32 x)
{
    m[1] = kind;
    U16(m, 0xA) = b;
    U16(m, 8) = a;
    m[0] = 1;
    m[2] = 0;
    g_game[0x381] = x;
}

/* shows the text s (allocated: freed when the popup closes) */
void popup_show_text(u8 *m, const char *s, u32 x, s32 allocated)
{
    m[1] = allocated ? 6 : 4;
    m[0] = 1;
    m[2] = 0;
    U32(m, 0x20) = (u32)s;
    g_game[0x381] = x;
}

u32 *popup_obj_ctor(u32 *o)
{
    f32 v[3];

    o[0] = (u32)D_089BAB98;
    v[0] = D_0899D174[0];
    v[1] = D_0899D174[1];
    v[2] = D_0899D174[2];
    if (vec3_nonzero(v) != 0) {
        ((f32 *)o)[1] = v[0];
        ((f32 *)o)[2] = v[1];
        ((f32 *)o)[3] = v[2];
    }
    o[0] = (u32)D_089BAAF4;
    o[7] = 0;
    return o;
}

u32 *popup_obj_dtor(u32 *o, s32 flag)
{
    if (o != NULL) {
        o[0] = (u32)D_089BAAF4;
        o[0] = (u32)D_089BAB98;
        if ((s16)flag > 0)
            stub_nop(o);
    }
    return o;
}

void popup_obj_reset(u8 *o)
{
    f32 v[3];

    U32(o, 0x1C) = 0;
    v[0] = D_0899D180[0];
    v[1] = D_0899D180[1];
    v[2] = D_0899D180[2];
    if (vec3_nonzero(v) != 0) {
        F32(o, 4) = v[0];
        F32(o, 8) = v[1];
        F32(o, 0xC) = v[2];
    }
}

extern const Layout D_0899D144;   /* window of the popup */
extern const char D_0899D158[];
extern u8 *g_net_players;
void layout_draw(void *self, Layout *l, const char *text, u32 alpha);
void font_puts_at(void *font, int x, int y, const char *s);
int font_char_count(void *font, const char *s);

/* draws the popup and moves it (slide in, wait, slide out) */
void popup_draw(u8 *m)
{
    Layout l;
    s16 x, y;

    l.x = D_0899D144.x;
    l.y = D_0899D144.y;
    l.w = D_0899D144.w;
    l.h = D_0899D144.h;
    l.cw = D_0899D144.cw;
    l.b9 = D_0899D144.b9;
    l.ba = D_0899D144.ba;
    l.bb = D_0899D144.bb;
    l.tx = D_0899D144.tx;
    l.ty = D_0899D144.ty;
    *(u32 *)&l.lh = *(const u32 *)&D_0899D144.lh;
    l.x = S16(m, 0xC);
    l.w = S16(m, 0xE);
    l.h = S16(m, 0x14);
    l.tx = S16(m, 0x16);
    l.y = (u32)F32(m, 0x10);
    l.ty = l.y + 9;
    layout_draw(g_menu, &l, NULL, 0xFF);
    x = l.tx;
    y = l.ty;
    switch (m[1]) {
    case 0:
        font_set_color(g_font, 0);
        font_printf_at(g_font, x, y, D_0899D114, TEXT(D_089ACA0C[S16(m, 0xA)]));
        break;
    case 1:
        font_set_color(g_font, 0);
        font_draw_at(g_font, x, y, TEXT(D_089ACA40[S16(m, 0xA)]), text_item_name(g_game, U16(m, 8)));
        break;
    case 2: {
        s16 w;

        font_set_color(g_font, 0);
        font_printf_at(g_font, x, y, D_0899D114, TEXT(D_089ACA68[S16(m, 0xA)]));
        w = font_char_count(g_font, TEXT(D_089ACA68[S16(m, 0xA)]));
        font_set_color(g_font, 2);
        font_printf_at(g_font, (s16)(x + w * 7), y, D_0899D114, text_item_name(g_game, U16(m, 8)));
        break;
    }
    case 3:
        font_set_color(g_font, 2);
        font_set_size(g_font, 0x10, 0xE);
        font_puts_at(g_font, x, y, (const char *)(g_net_players + m[8] * 200 + 0xBC8));
        font_set_size(g_font, 0xE, 0xE);
        font_set_color(g_font, 0);
        font_printf_at(g_font, x, y, D_0899D118, D_0899D120, TEXT(D_089ACA7C[S16(m, 0xA)]));
        break;
    case 4:
    case 6:
        font_set_color(g_font, 0);
        font_draw_at(g_font, x, y, D_0899D114, (const char *)U32(m, 0x20));
        break;
    case 5: {
        const char *a = text_item_name(g_game, U16(m, 0xA));
        const char *b = TEXT(D_089ACA84);

        font_draw_at(g_font, x, y, D_0899D158, a, b, S16(m, 8), TEXT(D_089ACA86));
        break;
    }
    case 7: {
        const char *a, *b;

        font_set_color(g_font, 0);
        a = TEXT(D_089ACA88);
        b = TEXT(D_089AD058[S16(m, 0xA)]);
        font_printf_at(g_font, x, y, D_0899D138, a, b, TEXT(D_089ACA8A));
        break;
    }
    case 8:
        font_set_color(g_font, 0);
        font_draw_at(g_font, x, y, TEXT(D_089ACA40[S16(m, 0xA)]), text_table7(g_game, U16(m, 8)));
        break;
    }
    switch (m[3]) {
    case 2:
        F32(m, 0x10) = F32(m, 0x10) + F32(m, 4);
        if (F32(m, 0x10) <= 0.0f)
            m[2]++;
        break;
    case 1: {
        s16 t = --S16(m, 0x18);

        if (t > 0) {
            if (t >= 0x1F)
                return;
            if (S8(g_menu, 0x10F8) <= 0)
                return;
        }
        F32(m, 4) = F32(m, 4) * -1.0f;
        g_game[0x381] = 0xFF;
        m[3]++;
        break;
    }
    case 0: {
        f32 lim;

        F32(m, 0x10) = F32(m, 0x10) + F32(m, 4);
        lim = (f32)(0x88 - (S16(m, 0x14) >> 1));
        if (F32(m, 0x10) >= lim) {
            m[3]++;
            F32(m, 0x10) = (f32)(0x88 - (S16(m, 0x14) >> 1));
        }
        break;
    }
    }
}
