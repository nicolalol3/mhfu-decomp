/* Menu: queue of the pop-up messages, money box, small state helpers.
 * Same class as menu_box.c. (Guessed names.) */
#include "menu.h"
#include "vfpu.h"
#undef menu_special_mode
#undef menu_item_category
#undef menu_draw_sprite
#undef layout_draw

extern void *D_08A62C40;          /* display list manager */
extern void *g_sound;          /* sound */
extern const s16 D_08938614[6];
extern const char D_08938620[];
extern const u16 D_089AD020[];
extern const s16 D_08938690[];

u8 save_in_progress(void *game);
void font_set_layer(void *font, int v);
void popup_update(void *msg);
void popup_show_text(void *msg, u32 a, u8 b, int c);
void popup_show(void *msg, s32 a, s32 b, s32 c, u8 d);
void font_draw_centered(u16 x, u16 y, const char *s, int size);
u32 *dl_cur(void *dl);
void dl_link(void *dl, u32 *p, int n, u8 layer);
void dl_set_cur(void *dl, u32 *end);
void sound_play(void *snd, int a, int b, int c, int d, int e, int f, int g, int h);
s16 menu_slot_first(void *self, u8 v);
void gfx_draw_sprite(void *self, u16 x, u16 y, u8 r, u8 g, u8 b, u32 icon, int c);
void menu_set_state(void *self);
void menu_msg_next(void *self);
void menu_begin_draw(void *self);

/* 1 if the menu is opened in a special mode (game flag or hunter flag) */
int menu_special_mode(void *self)
{
    if ((*(u32 *)(g_game + 0x6AF14) & 1) != 0)
        return 1;
    if ((U32(*(void **)((u8 *)self + 0x530), 0x614) & 4) != 0)
        return 1;
    return 0;
}

/* draws the pop-up messages of the queue */
void menu_draw_messages(void *self)
{
    u8 *p = self;
    int i;

    if (save_in_progress(g_game) == 1)
        return;
    U8(self, 0x534) = 14;
    font_set_layer(g_font, 5);
    menu_begin_draw(self);
    menu_msg_next(self);
    for (i = 0; i < 20; i++, p += 0x24)
        if (U8(p, 0xE28) != 0)
            popup_update(p + 0xE28);
}

/* 1 if a message of kind k is waiting in the queue */
int menu_msg_queued(void *self, u32 k_arg)
{
    u8 k = k_arg;
    s8 n = S8(self, 0x10F8);
    s32 i, h;
    s16 j;

    if (n == 0)
        return 0;
    if (n <= 0)
        return 0;
    h = S8(self, 0x10F9);
    for (j = 0; j < n; j++, h++) {
        if (U8((u8 *)self + (s16)(h % 20) * 0x24, 0xE43) == k)
            return 1;
    }
    return 0;
}

/* shows the next message of the queue, when no message is on screen */
void menu_msg_next(void *self)
{
    u8 *e;
    s8 h;
    int i;

    if ((*(u32 *)(g_game + 0x6AF14) & 1) == 0 && G_TIME_STOP == 1)
        return;
    for (i = 0; i < 20; i++)
        if (U8((u8 *)self + i * 0x24, 0xE28) != 0)
            return;
    if (S8(self, 0x10F8) <= 0)
        return;
    e = (u8 *)self + S8(self, 0x10F9) * 0x24;
    if (S8(e, 0xE42) == -1)
        popup_show_text(e + 0xE28, U32(e, 0xE48), U8(e, 0xE43), 0);
    else if (S8(e, 0xE42) == -2)
        popup_show_text(e + 0xE28, U32(e, 0xE48), U8(e, 0xE43), 1);
    else
        popup_show(e + 0xE28, S8(e, 0xE42), S16(e, 0xE44), S16(e, 0xE46), U8(e, 0xE43));
    h = S8(self, 0x10F9) + 1;
    S8(self, 0x10F9) = h;
    if (h > 19)
        S8(self, 0x10F9) = 0;
    U8(self, 0x10FA) = 60;
    S8(self, 0x10F8) = S8(self, 0x10F8) - 1;
}

/* box with the money of the player */
void menu_draw_money(void *self, u32 grey, int low)
{
    s16 r[6];
    char buf[32];
    int i;

    for (i = 0; i < 6; i += 2) {
        r[i] = D_08938614[i];
        r[i + 1] = D_08938614[i + 1];
    }
    if (low != 0) {
        r[1] += 28;
        r[5] += 28;
    }
    gfx_draw_window(self, r[0], r[1], (u16)r[2], (u16)r[3], 255);
    sprintf(buf, D_08938620, *(u32 *)(g_game + 0x696F0));
    font_set_size(g_font, 14, 14);
    font_set_color(g_font, (s8)((u8)grey != 0 ? 10 : 0));
    font_draw_centered(r[4], r[5], buf, 14);
}

/* resets the state of the hunter view */
void menu_set_state(void *self)
{
    U8(self, 0x120A) = 0;
    U8(self, 0x120B) = 0;
    U8(self, 0x120E) = 0;
    U8(self, 0x120F) = 0;
    U32(self, 0x1214) = 3600;
    U8(self, 0x1218) = 0;
    U8(self, 0x5FE) = 6;
    U16(self, 0x5FC) = 4;
    U32(self, 0x5E8) |= 4;
}

void menu_set_state_ex(void *self)
{
    menu_set_state(self);
    U8(self, 0x1218) = 1;
}

/* name of the category of an item (NULL if none) */
const char *menu_item_category(void *self, u8 *item)
{
    if (item == NULL)
        return NULL;
    return text_menu(g_game, D_089AD020[item[5]]);
}

/* per-frame update of a counter of the hunter */
void menu_update_counter(u8 *h)
{
    if (h[0x5F0] == 0) {
        if ((h[0x606] & 0x80) == 0 && (U32(h, 0x5E8) & 0x40) == 0) {
            U32(h, 0x5E8) &= ~2;
            if (S16(h, 0x5FA) != 0)
                S16(h, 0x5FA) -= 1;
        }
    } else if ((U32(h, 0x5E8) & 2) == 0) {
        S16(h, 0x5FA) = 0;
    }
}

/* starts the display list of the menu (2D drawing state) */
void menu_begin_draw(void *self)
{
    u32 *p = dl_cur(D_08A62C40);

    p[0] = 0x23000000;
    p[1] = 0xDE000000;
    p[2] = 0x21000001;
    p[3] = 0xDF000032;
    p[4] = 0x22000000;
    p[5] = 0xDBFF0003;
    p[6] = 0xC9000100;
    p[7] = 0xC0000000;
    p[8] = 0xC7000000;
    p[9] = 0xC6000000;
    p[10] = 0x10000000;
    p[11] = 0x08000000;
    dl_link(D_08A62C40, p, 12, U8(self, 0x534));
    dl_set_cur(D_08A62C40, p + 12);
}

/* goes back one page (with sound) */
void menu_page_back(void *self)
{
    U8(self, 0x585) -= 1;
    sound_play(g_sound, 0, 20, 0, 0, 0, 0, 0, 0);
    S16(self, 0x5FC) = menu_slot_first(self, U8(self, 0x586)) + 24;
}

/* two equipment names side by side with a blinking arrow */
void menu_draw_compare(void *self, u32 a, u32 b, s32 x, s32 y, u32 flags, s32 big)
{
    int n = big == 0 ? 1 : 2;
    f32 s;

    equip_draw_name_box(self, a, x, y, (flags | 0x20) & ~8, b, n);
    equip_draw_name_box(self, b, x, (s16)((s16)y + 120 + D_08938690[big]), flags, a, n);
    s = vfpu_sinf((f32)(u16)((*(u32 *)(g_sys + 0xF7A554) & 0x1F) << 11) * (3.14159265f / 32768.0f));
    gfx_draw_sprite(self, (s16)x + 104, (s16)y + 112 + D_08938690[big],
                  (s8)(int)(s * 95.0f) + 159, (s8)(int)(s * 63.0f) + 191, 255, 0x9C, 0);
}

extern void *D_089CBBE8;          /* text converter */
extern const s8 D_089AA1D2;
int utf8_byte_len(void *self, const char *s);
void *heap_alloc(void *heap, int size, int align);
void *memcpy(void *d, const void *s, size_t n);
s16 func_08892708(void *font, s32 i, u8 *ch, const char *s, void *aux);
s32 func_088855D0(void *snd, int v);

/* slot of the next message of the queue (0xFF = full) */
#define MSG_SLOT(self) ((u8 *)(self) + (s8)((S8(self, 0x10F9) + S8(self, 0x10F8)) % 20) * 0x24)

/* queues the message text id at (x, y); returns its serial number */
u8 menu_msg_push(u8 *self, u32 id, u32 x, u32 y)
{
    u8 *e;

    if (S8(self, 0x10F8) >= 19)
        return 0xFF;
    g_game[0x380] = (g_game[0x380] + 1) & 0x7F;
    e = MSG_SLOT(self);
    e[0xE42] = id;
    U16(e, 0xE44) = x;
    e[0xE43] = g_game[0x380];
    U16(e, 0xE46) = y;
    S8(self, 0x10F8) += 1;
    return g_game[0x380];
}

/* queues a message with a text in memory owned by the caller */
u8 menu_msg_push_text(u8 *self, const char *s)
{
    u8 *e;

    if (S8(self, 0x10F8) >= 19)
        return 0xFF;
    g_game[0x380] = (g_game[0x380] + 1) & 0x7F;
    e = MSG_SLOT(self);
    e[0xE43] = g_game[0x380];
    e[0xE42] = 0xFF;
    *(const char **)(e + 0xE48) = s;
    S8(self, 0x10F8) += 1;
    return g_game[0x380];
}

/* queues a message with a copy of the text s */
u8 menu_msg_push_copy(u8 *self, const char *s)
{
    s32 n;
    char *p;
    u8 *e;

    if (S8(self, 0x10F8) >= 19)
        return 0xFF;
    n = utf8_byte_len(D_089CBBE8, s) + 1;
    p = heap_alloc(self + 0x500, n, 16);
    if (p == NULL)
        return 0xFF;
    g_game[0x380] = (g_game[0x380] + 1) & 0x7F;
    e = MSG_SLOT(self);
    e[0xE43] = g_game[0x380];
    e[0xE42] = 0xFE;
    memcpy(p, s, n);
    *(char **)(e + 0xE48) = p;
    S8(self, 0x10F8) += 1;
    return g_game[0x380];
}

/* plays the sound of a new character of the typewriter text */
static void typewriter_sound(int quest_sound, int alt_sound)
{
    if ((*(u32 *)(g_game + 0x6AF14) & 1) == 0)
        sound_play(g_sound, func_088855D0(g_sound, 0x4A), quest_sound, D_089AA1D2, 0, 0, 0, 0, 0);
    else
        sound_play(g_sound, 1, alt_sound, 0, 0, 0, 0, 0, 0);
}

/* text s shown one character at a time: t = time since the start, mode 3/4 = close,
 * 2 = wait at the end, 1 = end. Returns 1 while writing, 0 at the end, -1 if busy */
s32 menu_typewriter(u8 *self, const char *s, u32 t, s32 mode, s32 last)
{
    u8 ch[4];
    u8 aux[4];
    u32 n, cost;
    s16 i, k;

    if (self[0x604] != 0)
        return -1;
    if (mode == 3) {
        self[0x5F0] = 0x10;
        return 0;
    }
    if (mode == 4) {
        self[0x5F0] = 0x20;
        return 0;
    }
    if (t < U32(self, 0x5F4)) {
        S16(self, 0x5F2) = 0;
    } else if (*(const char **)(self + 0x5EC) == s) {
        U32(self, 0x5F4) = t;
        goto start;
    } else {
        S16(self, 0x5F2) = 0;
    }
    self[0x5F8] = 0;
    U32(self, 0x5F4) = t;
start:
    self[0x5F0] = 1;
    *(const char **)(self + 0x5EC) = s;
    n = 0;
    i = 0;
    for (;;) {
        do {
            k = func_08892708(g_font, i, ch, s, aux);
            if (k == 4) {
                if ((u32)(s32)S16(self, 0x5F2) < n)
                    typewriter_sound(0x1D, 0x84);
                S16(self, 0x5F2) = n;
                if (mode == 2) {
                    self[0x5F0] |= 8;
                    if (self[0x5F8] == 0) {
                        self[0x5F8] = 1;
                        typewriter_sound(0x1F, 0x86);
                    }
                } else if (mode == 1) {
                    self[0x5F0] |= 2;
                    if (last == 0)
                        self[0x5F0] |= 4;
                }
                return 0;
            }
            i++;
        } while ((s8)ch[0] == '\n');
        n++;
        cost = 3;
        if (k == 1) {
            u16 c = (((s8)ch[0] << 8) + ch[1]) & 0xFFFF;

            if (c == 0x8142 || c == 0x8148)
                cost = 15;
            else if (c == 0x8141)
                cost = 8;
        } else if (k == 0) {
            if ((s8)ch[0] == '.' || (s8)ch[0] == '?')
                cost = 15;
        }
        if (t < cost)
            break;
        t -= cost;
    }
    if ((u32)(s32)S16(self, 0x5F2) < n)
        typewriter_sound(0x1D, 0x84);
    S16(self, 0x5F2) = n;
    return 1;
}

extern const u8 D_08938770[];      /* layout of the text box */
extern const u16 D_089AC868[2], D_089AC86C;
extern const u16 D_0893B208[];     /* save data file ids per language */
extern const s16 D_08939A34;
extern void **g_loader;         /* save data manager (C++ object) */
extern void *g_heap;
void layout_draw(void *self, const void *layout, const char *text, int alpha);
void font_set_height2(void *font, int v);
void func_088921FC(void *font, s32 n, const char *s);
s32 temp_alloc(void *a, int b, s32 c);
s32 temp_get(void *a, int b);
void menu_draw_sprite(u8 *self, u32 x, u32 y, s32 id);
void gfx_draw_sprite_scaled(u8 *self, u32 x, u32 y, u32 w, u32 h, s32 id);

typedef s32 (*VFunc0)(void *);
typedef s32 (*VFunc1)(void *, u32);
typedef s32 (*VFunc6)(void *, u32, s32, s32, s32, s32, s32);
/* method at byte offset off of the vtable of obj */
#define VCALL(type, obj, off) ((type)((void **)*(obj))[(off) / 4])

/* draws the typewriter text box (22 characters per line) */
void menu_draw_typewriter(u8 *self)
{
    s32 left, budget;
    const char *s;
    const char *next, *next2;
    u8 ch[4];
    s16 y, i, n;

    if ((S8(self, 0x5F0) & 0x10) != 0)
        return;
    layout_draw(self, D_08938770, NULL, 255);
    if ((S8(self, 0x5F0) & 0x20) != 0)
        return;
    font_set_height2(g_font, 16);
    font_set_color(g_font, 0);
    left = S16(self, 0x5F2);
    s = *(const char **)(self + 0x5EC);
    y = 96;
    while (left > 0) {
        i = 0;
        budget = 22;
        n = 0;
        while (left > 0 && budget > 0) {
            s16 k = func_08892708(g_font, i, ch, s, &next);

            i++;
            if ((s8)ch[0] == '\n')
                break;
            left--;
            n++;
            if ((u32)(k - 1) < 2)
                budget--;
            budget--;
        }
        font_set_pos(g_font, 16, y);
        func_088921FC(g_font, n, s);
        y += 16;
        if (budget != 0) {
            s = next;
        } else {
            func_08892708(g_font, i, ch, s, &next2);
            s = (s8)ch[0] != '\n' ? next : next2;
        }
    }
    if (S8(self, 0x5F0) & 2)
        font_draw_at(g_font, 16, y, fmt_s,
                      text_menu(g_game, D_089AC868[(S8(self, 0x5F0) & 4) == 0]));
    if (S8(self, 0x5F0) & 8)
        font_printf_at_color(g_font, 114, y, 2, fmt_s, text_menu(g_game, D_089AC86C));
}

/* 1 if the menu cannot take input (game busy, text running or locked) */
int menu_busy(u8 *self)
{
    if (g_game[0x10] != 0)
        return 1;
    if (self[0x604] != 0)
        return 1;
    return self[0x52F] != 0;
}

/* bar of width w made of a stretched sprite and its end */
void menu_draw_bar(u8 *self, u32 x, u32 y, u32 w, u32 h)
{
    menu_draw_sprite(self, ((x & 0xFFFF) + (w & 0xFFFF) - D_08939A34) & 0xFFFF, y, 182);
    gfx_draw_sprite_scaled(self, x, y, ((w & 0xFFFF) - D_08939A34) & 0xFFFF, h, 181);
}

/* loads the menu save data (3 steps); 1 when done */
int menu_load_data(u8 *self)
{
    s32 st;

    if (U32(self, 0x528) & 0x1000)
        return 1;
    st = U32(self, 0x1198);
    if (st == 2) {
        U32(self, 0x1198) = 0;
        U32(self, 0x528) |= 0x1000;
        return 1;
    }
    if (st == 1) {
        if ((u8)VCALL(VFunc0, g_loader, 0x38)(g_loader) == 0)
            U32(self, 0x1198) += 1;
    } else if (st == 0) {
        u16 id = D_0893B208[LANG];
        s32 r = VCALL(VFunc1, g_loader, 0x18)(g_loader, id);

        r = temp_alloc(g_heap, 11, r);
        VCALL(VFunc6, g_loader, 0x34)(g_loader, id, r, -1, 0, 0, 1);
        U32(self, 0x1198) += 1;
    } else {
        return 0;
    }
    return 0;
}

/* saves the menu data (2 steps); 1 when done */
u8 menu_save_data(u8 *self)
{
    if (U32(self, 0x1198) == 1) {
        if (VCALL(VFunc0, g_loader, 0x38)(g_loader) != 0)
            return 0;
        U32(self, 0x1198) = 0;
        return 1;
    }
    if (U32(self, 0x1198) == 0) {
        u16 id = D_0893B208[LANG];

        VCALL(VFunc6, g_loader, 0x34)(g_loader, id, temp_get(g_heap, 11), -1, 0, 0, 1);
        U32(self, 0x1198) += 1;
        return 0;
    }
    return 1;
}
