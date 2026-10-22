/* Menu di gioco: movimento del cursore con i tasti e piccole azioni del menu.
 * Tasti (bit): 0x10 su, 0x20 destra, 0x40 giù, 0x80 sinistra (come usati qui).
 * Stessa classe di menu_box.c. (Nomi ipotetici.) */
#include "menu.h"

extern void *D_08A62D8C;          /* gestore dei suoni */

void func_08883988(void *snd, int a, int id, int b, int c, int d, int e, int f, int g);
s32 func_08826734(void *self, u8 idx);
int func_088268E0(void *self, u16 *out);
void func_08826688(void *self);
int menu_info_available(void *self);
s32 func_08826F80(void *self, int a);
void func_08826FCC(void *self, u32 a, int b);

/* suono di interfaccia (0x11 = pagina, 0x17 = cursore, 9 = selezione) */
static inline void play_se(int id)
{
    func_08883988(D_08A62D8C, 0, id, 0, 0, 0, 0, 0, 0);
}

int menu_cursor_vertical(void *self, u8 *pos, u32 keys, u32 count);

/* cursore a due colonne (count voci, metà per colonna) */
void menu_cursor_two_columns(void *self, u8 *pos, u32 keys, u32 count_arg)
{
    u8 count = count_arg;
    u8 half = count >> 1;

    if (*pos < half) {
        menu_cursor_vertical(self, pos, keys, half);
    } else {
        *pos -= half;
        menu_cursor_vertical(self, pos, keys, half);
        *pos += half;
    }
    if ((keys & 0xFFFF) & 0xA0) {
        if ((keys & 0xFFFF) & 0x80) {
            if (*pos < half)
                *pos += count;
            *pos -= half;
        } else {
            *pos += half;
            if (*pos >= count)
                *pos -= count;
        }
        play_se(0x11);
    }
}

/* cursore a pagine da 8 voci (sinistra/destra cambia pagina) */
void menu_cursor_pages(void *self, u8 *pos, u32 keys, u32 count_arg)
{
    u8 count = count_arg;
    u32 base;

    for (base = 0; base + 8 <= *pos; base = (base + 8) & 0xFF)
        ;
    *pos -= base;
    menu_cursor_vertical(self, pos, keys, 8);
    *pos += base;
    if (count > 8 && (keys & 0xA0)) {
        if (keys & 0x80) {
            if (*pos < 8)
                *pos += count;
            *pos -= 8;
        } else {
            *pos += 8;
            if (*pos >= count)
                *pos -= count;
        }
        play_se(0x11);
    }
}

void menu_item_toggle_sub(void *self, u32 id, u32 keys);

/* cursore sulla lista degli oggetti: aggiorna la descrizione mostrata */
u16 menu_cursor_items(void *self, u8 *pos, u32 keys)
{
    u16 id;

    menu_cursor_pages(self, pos, keys, 24);
    id = *func_08856884(D_089CC438, *pos);
    S16(self, 0x5FC) = id + 0x18;
    menu_item_toggle_sub(self, id, (keys & 0xFFFF) & 0x8000);
    return id;
}

/* con il tasto 0x8000 alterna la seconda pagina di descrizione degli oggetti che la hanno */
void menu_item_toggle_sub(void *self, u32 arg, u32 keys)
{
    u16 id = arg;   /* l'originale usa solo i 16 bit bassi */

    if (*(s16 *)(D_0899D198 + id * 0x18 + 0x16) != 0 && id != 0 && (keys & 0x8000)) {
        play_se(9);
        U8(self, 0x5FF) = (U8(self, 0x5FF) + 1) & 1;
    }
}

/* cursore su/giù con ritorno all'inizio; 1 se è cambiato */
int menu_cursor_vertical(void *self, u8 *pos, u32 keys, u32 count_arg)
{
    u8 count = count_arg;
    u8 old = *pos, v;

    if (keys & 0x10) {
        v = old == 0 ? count - 1 : old - 1;
    } else {
        v = old;
        if (keys & 0x40) {
            v = old + 1;
            if (v >= count)
                v = 0;
        }
    }
    if (v == old)
        return 0;
    *pos = v;
    play_se(0x17);
    return 1;
}

/* cursore sinistra/destra con ritorno all'inizio (suono di pagina) */
int menu_cursor_horizontal_page(void *self, u8 *pos, u32 keys, u32 count_arg)
{
    u8 count = count_arg;
    u8 old, v;

    if (count < 2) {
        *pos = 0;
        return 0;
    }
    old = *pos;
    if (keys & 0x80) {
        v = old == 0 ? count - 1 : old - 1;
    } else {
        v = old;
        if (keys & 0x20) {
            v = old + 1;
            if (v >= count)
                v = 0;
        }
    }
    if (v == old)
        return 0;
    *pos = v;
    play_se(0x11);
    return 1;
}

/* come sopra, con il suono del cursore */
int menu_cursor_horizontal(void *self, u8 *pos, u32 keys, u32 count_arg)
{
    u8 count = count_arg;
    u8 old, v;

    if (count < 2) {
        *pos = 0;
        return 0;
    }
    old = *pos;
    if (keys & 0x80) {
        v = old == 0 ? count - 1 : old - 1;
    } else {
        v = old;
        if (keys & 0x20) {
            v = old + 1;
            if (v >= count)
                v = 0;
        }
    }
    if (v == old)
        return 0;
    *pos = v;
    play_se(0x17);
    return 1;
}

/* scelta sì/no (0x537) con i tasti premuti (a & b) */
void menu_cursor_yes_no(void *self, u32 a, u32 b)
{
    u32 k = (a & 0xFFFF) & (b & 0xFFFF);

    if (U8(self, 0x537) == 0) {
        if (k & 0x60) {
            U8(self, 0x537) = 1;
            play_se(0x17);
        }
    } else if (k & 0x90) {
        U8(self, 0x537) = 0;
        play_se(0x17);
    }
}

/* come sopra ma sul valore v: restituisce il nuovo valore */
u32 menu_toggle_yes_no(u32 v, u32 keys)
{
    if ((v & 0xFF) == 0) {
        if (((keys & 0xFFFF) & 0x60) == 0)
            return v;
        v = 1;
        play_se(0x17);
    } else if ((keys & 0xFFFF) & 0x90) {
        v = 0;
        play_se(0x17);
    }
    return v;
}

/* cursore su una griglia: g[4] = indice, g[6] = colonne, g[7] = colonna, g[8] = riga,
 * g[9] = righe, g[10] = colonne dell'ultima riga */
void menu_cursor_grid(void *self, u8 *g, u32 keys, s32 quiet_arg)
{
    s8 quiet = quiet_arg;
    s8 col = g[7];
    s16 c2 = col, c = col;
    u16 row = g[8];
    u8 w;

    if ((keys & 0xA0) != 0 && g[9] >= 2) {
        u32 rows = g[9];

        if ((keys & 0x80) == 0) {
            row++;
            if ((u32)(s32)(s16)row >= rows)
                row = 0;
        } else {
            row--;
            if ((s16)row < 0)
                row = g[9] - 1;
        }
        c = col;
        if ((s16)row == (s32)(rows - 1)) {
            if ((s16)g[10] <= (s16)col) {
                c2 = g[10] - 1;
                g[7] = c2;
            }
            c = c2;
        }
        g[8] = row;
        if (quiet == 0) {
            play_se(0x11);
            w = g[6];
            *(u16 *)(g + 4) = c + w * row;
            return;
        }
        play_se(0x17);
    } else if (keys & 0x50) {
        u16 n = (s16)row < (s32)(g[9] - 1) ? g[6] : g[10];

        if (n < 2) {
            g[7] = col;
        } else {
            if ((keys & 0x10) == 0) {
                c = col + 1;
                if ((s16)n <= c)
                    c = 0;
            } else {
                c = col - 1;
                if (c < 0)
                    c = n - 1;
            }
            play_se(0x17);
            g[7] = c;
        }
    }
    w = g[6];
    *(u16 *)(g + 4) = c + w * row;
}

/* apre la lista degli oggetti */
int menu_open_item_list(void *self)
{
    u32 f;

    U8(self, 0x585) = 0;
    U8(self, 0x586) = 0;
    U8(self, 0x589) = 0;
    U8(self, 0x588) = 0;
    U8(self, 0x587) = 0;
    if ((*(u32 *)(D_089CC438 + 0x6AF14) & 1) == 0) {
        if (S8(self, 0x587) == D_089CC438[0x28])
            U8(self, 0x587)++;
    }
    f = U32(self, 0x528);
    U32(self, 0x528) = f & ~0x10;
    U32(self, 0x5E8) |= 4;
    U32(self, 0x528) |= 0x100;
    U8(self, 0x5FE) = 1;
    S16(self, 0x5FC) = (s16)func_08826734(self, U8(self, 0x586)) + 0x18;
    U8(self, 0x5FF) = 0;
    U8(self, 0x1207) = 0;
    return 0;
}

/* apre la lista per l'uso di un oggetto (se possibile); -1 se non si può */
int menu_open_use_list(void *self)
{
    u16 tmp;

    if ((s8)func_088239FC(self) != 1)
        return -1;
    tmp = 0xFFFF;
    if ((s8)func_088268E0(self, &tmp) == 0)
        return -1;
    if ((*(u32 *)(D_089CC438 + 0x6AF14) & 1) == 0 && (s8)func_09A6E5A8((void *)U32(self, 0x530)) == 1)
        return -1;
    func_08826688(self);
    U32(self, 0x528) &= ~0x10;
    U32(self, 0x5E8) &= ~4;
    U32(self, 0x528) |= 0x200;
    U8(self, 0x5FE) = 1;
    U16(self, 0x5FC) = 0;
    U8(self, 0x5FF) = 0xFF;
    return 0;
}

int menu_open_info(void *self)
{
    if ((*(u32 *)(D_089CC438 + 0x6AF14) & 1) == 0)
        func_08829674(self);
    menu_info_available(self);
    U8(self, 0x57F) = 0;
    U8(self, 0x581) = (*(u32 *)(D_089CC438 + 0x6AF14) & 1) ? 3 : 4;
    U32(self, 0x5E8) &= ~4;
    return 0;
}

int menu_open_status(void *self)
{
    func_08826FCC(self, U32(self, 0x530), 0);
    U8(self, 0x57F) = 0;
    U32(self, 0x5E8) &= ~4;
    return 0;
}

int menu_open_equipment(void *self)
{
    U8(self, 0x57F) = 0;
    U8(self, 0x582) = 0;
    U32(self, 0x5E8) &= ~4;
    return 0;
}

s32 func_088291B4(void *self, int a, int b);
void func_088262B0(void *self, int a, u32 keys);
void func_088260D4(void *self, u32 keys);
void func_0882619C(void *self, u32 keys);
s32 func_09A7BFA8(void *a);
u32 func_088303E8(void *self, void *buf, int a, int b);
u8 *menu_get_equip_slot(void *self, u8 *out, u32 slot, u8 *hunter);
int menu_info_available(void *self);

int menu_open_list_tabs(void *self)
{
    U8(self, 0x57E) = 0;
    U8(self, 0x57F) = 0;
    U8(self, 0x5BB) = 0;
    U32(self, 0x5A8) = 0;
    U8(self, 0x5D9) = 0;
    U32(self, 0x5E8) |= 4;
    U8(self, 0x5FE) = 2;
    U16(self, 0x5FC) = 0;
    return 0;
}

/* gestione dei tasti nella schermata a schede (0x57E: 0 = scelta scheda, 1 = dentro) */
u32 menu_list_tabs_input(void *self, u32 keys)
{
    u32 k;

    if (keys & 8)
        return 8;
    if (U8(self, 0x57E) == 1) {
        k = keys & 0xFFFF;
    } else {
        if (U8(self, 0x57E) != 0)
            return keys;
        U32(self, 0x5E8) |= 4;
        if (keys & 0x2000)
            return keys;
        if ((keys & 0x4000) == 0) {
            menu_cursor_vertical(self, (u8 *)self + 0x57F, keys, 3);
            U16(self, 0x5FC) = U8(self, 0x57F);
            return keys;
        }
        if (U8(self, 0x57F) == 1) {
            U8(self, 0x5D9) = (s8)func_088291B4(self, S8(self, 0x5D9), 0);
        } else if (U8(self, 0x57F) == 0) {
            U16(self, 0x5C0) = 0;
            U8(self, 0x5C2) = 8;
            func_088262B0(self, 1, keys & 0xFFFF);
        }
        U16(self, 0x1208) = 0;
        U8(self, 0x57E)++;
        keys = 0;
        play_se(0x13);
        k = 0;
    }
    if ((k & 0x2000) == 0) {
        U32(self, 0x5E8) &= ~4;
        if (U8(self, 0x57F) == 1)
            func_088260D4(self, keys);
        else if (U8(self, 0x57F) == 0)
            func_0882619C(self, keys);
    } else {
        U32(self, 0x5E8) |= 4;
        U8(self, 0x5FE) = 2;
        U16(self, 0x5FC) = U8(self, 0x57F);
        U8(self, 0x57E) = 0;
        keys = 0;
        play_se(0x14);
        U32(self, 0x528) &= ~0x800;
    }
    return keys;
}

u32 menu_info_input(void *self, u32 keys)
{
    U32(self, 0x5E8) &= ~4;
    if ((s8)menu_info_available(self) == 1)
        menu_cursor_horizontal(self, (u8 *)self + 0x57F, keys, U8(self, 0x581));
    return keys;
}

/* scheda del cacciatore: sinistra/destra cambia pagina */
u32 menu_status_input(void *self, u32 keys)
{
    s8 n = (s8)func_08826F80(self, 9);

    if (keys & 0xA0) {
        if ((keys & 0x20) == 0) {
            if (U8(self, 0x57F) == 0)
                U8(self, 0x57F) = n + 1;
            else
                U8(self, 0x57F)--;
        } else {
            U8(self, 0x57F)++;
            if ((u8)(n + 1) < U8(self, 0x57F))
                U8(self, 0x57F) = 0;
        }
        play_se(0x11);
    }
    return keys & 0x2008;
}

u32 menu_equipment_input(void *self, u32 keys)
{
    u8 buf[12];
    u8 *hunter = (u8 *)U32(self, 0x530);
    u8 n;

    menu_cursor_vertical(self, (u8 *)self + 0x57F, keys, 6);
    if (menu_get_equip_slot(self, buf, U8(self, 0x57F), hunter) != NULL) {
        U8(self, 0x583) = (u8)func_088303E8(self, buf, 0x100, 0);
        if (U8(self, 0x582) < U8(self, 0x583)) {
            n = U8(self, 0x583);
        } else {
            U8(self, 0x582) = U8(self, 0x583) - 1;
            n = U8(self, 0x583);
        }
        menu_cursor_horizontal(self, (u8 *)self + 0x582, keys, n);
    }
    return keys;
}

/* la scheda informazioni è disponibile? (imposta il flag 0x40) */
int menu_info_available(void *self)
{
    if ((*(u32 *)(D_089CC438 + 0x6AF14) & 1) && (s8)func_09A7BFA8(D_09B22D70) == 0) {
        U32(self, 0x528) &= ~0x40;
        return 0;
    }
    U32(self, 0x528) |= 0x40;
    return 1;
}

/* copia in out i dati del pezzo di equipaggiamento slot (0 = arma, 1..5 = armatura) */
u8 *menu_get_equip_slot(void *self, u8 *out, u32 slot_arg, u8 *hunter)
{
    u8 slot = slot_arg;
    u8 *e;

    *(u16 *)(out + 4) = 0;
    out[0] = 0;
    switch (slot) {
    case 0:
        out[0] = hunter[0x4E8];
        out[1] = hunter[0x4E9];
        *(u16 *)(out + 2) = *(u16 *)(hunter + 0x4EA);
        *(u16 *)(out + 4) = *(u16 *)(hunter + 0x4EC);
        *(u16 *)(out + 6) = *(u16 *)(hunter + 0x4EE);
        *(u16 *)(out + 8) = *(u16 *)(hunter + 0x4F0);
        *(u16 *)(out + 10) = *(u16 *)(hunter + 0x4F2);
        out[0] = 1;
        return out;
    case 1:
    case 2:
    case 3:
    case 4:
        out[1] = slot;
        break;
    case 5:
        out[1] = 0;
        break;
    default:
        return NULL;
    }
    e = hunter + out[1] * 0xC;
    if (*(s16 *)(e + 0x4F6) == 0)
        return NULL;
    out[0] = 1;
    *(u16 *)(out + 2) = *(u16 *)(e + 0x4F6);
    *(u16 *)(out + 4) = *(u16 *)(e + 0x4F8);
    *(u16 *)(out + 6) = *(u16 *)(e + 0x4FA);
    *(u16 *)(out + 8) = *(u16 *)(e + 0x4FC);
    *(u16 *)(out + 10) = *(u16 *)(e + 0x4FE);
    return out;
}
