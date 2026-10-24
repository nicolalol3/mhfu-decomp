/* On-screen keyboard of the system (sceUtilityOsk), used to type names and messages.
 * The object starts with SceUtilityOskParams (0x40 bytes), followed by SceUtilityOskData.
 * (Guessed names.) */
#include "menu.h"

extern u8 *D_08A62C64;             /* pad (controller) */
extern u8 *D_089CC01C;             /* screen fades */
extern const char D_08931628[];    /* empty text */

void fade_start_black(void *f, int a, int b);
int sceUtilityOskGetStatus(void);
int sceUtilityOskShutdownStart(void);
int sceUtilityOskUpdate(int n);
int sceUtilityOskInitStart(void *params);

void osk_set_language(u8 *self);
void osk_setup(u8 *self, u32 mode, void *desc, void *intext, void *outtext, u32 outlen,
               u32 lines);
void osk_finish(u8 *self);

void osk_init(u8 *self)
{
    self[0x74] = 0;
    osk_set_language(self);
}

/* language of the keyboard from the language of the game */
void osk_set_language(u8 *self)
{
    switch (LANG) {
    case 1: self[0x75] = 1; self[0x76] = 1; break;
    case 2: self[0x75] = 4; self[0x76] = 1; break;
    case 3: self[0x75] = 2; self[0x76] = 1; break;
    case 4: self[0x75] = 3; self[0x76] = 1; break;
    case 5: self[0x75] = 5; self[0x76] = 1; break;
    case 6: self[0x75] = 9; self[0x76] = 0; break;
    default: self[0x75] = 0; self[0x76] = 0; break;
    }
    switch (self[0x75]) {
    case 0: self[0x77] = 1; break;
    case 1: self[0x77] = 2; break;
    case 2: self[0x77] = 3; break;
    case 3: self[0x77] = 4; break;
    case 4: self[0x77] = 5; break;
    case 5: self[0x77] = 6; break;
    case 6: self[0x77] = 7; break;
    case 7: self[0x77] = 8; break;
    case 8: self[0x77] = 9; break;
    default: self[0x77] = 0; break;
    }
}

void osk_update(u8 *self)
{
    int st;

    if (self[0x74] == 0)
        return;
    st = sceUtilityOskGetStatus();
    if (st == 0)
        osk_finish(self);
    else if (st == 4)
        ;
    else if (st == 3)
        sceUtilityOskShutdownStart();
    else if (st == 2)
        sceUtilityOskUpdate(2);
}

void osk_open(u8 *self, u32 mode, void *desc, void *intext, void *outtext, u32 outlen,
              u32 lines)
{
    D_08A62C64[0x17] = 1;
    self[0x74] = 1;
    osk_set_language(self);
    osk_setup(self, mode, desc, intext, outtext, outlen, lines);
    if (sceUtilityOskInitStart(self) == 0) {
        fade_start_black(D_089CC01C, 4, 1);
        D_089CC01C[0x29] = 0xC0;
    } else {
        D_08A62C64[0x17] = 0;
        self[0x74] = 0;
    }
}

void osk_setup(u8 *self, u32 mode, void *desc, void *intext, void *outtext, u32 outlen,
               u32 lines)
{
    u32 *p = (u32 *)self;
    u32 *d = p + 0x10;

    memset(self, 0, 0x40);
    p[0] = 0x40;
    p[1] = self[0x75];
    p[2] = self[0x76];
    p[3] = 0x2A;
    p[4] = 0x2C;
    p[5] = 0x2B;
    p[6] = 0x29;
    p[0xC] = 1;
    p[0xD] = (u32)d;
    d[0] = 1;
    d[1] = 0;
    d[2] = self[0x77];
    d[3] = 0;
    d[5] = lines;
    d[6] = 1;
    d[7] = desc == NULL ? (u32)D_08931628 : (u32)desc;
    d[8] = intext == NULL ? (u32)D_08931628 : (u32)intext;
    d[9] = outlen;
    d[10] = (u32)outtext;
    d[11] = 0;
    d[12] = 0;
    switch ((u8)mode) {
    case 4: d[4] = 1; break;
    case 3: d[4] = 0x101; break;
    case 2: d[4] = 0xF; break;
    case 1: d[4] = 0xDF00; break;
    default: d[4] = 0; break;
    }
}

/* copies the typed text (result 1 = OK) */
void osk_finish(u8 *self)
{
    u32 *d = (u32 *)(self + 0x40);
    s32 n;
    u16 *dst, *src;

    fade_start_black(D_089CC01C, 1, 0);
    if (d[0xB] == 1) {
        n = d[9];
        dst = (u16 *)d[10];
        src = (u16 *)d[8];
        while (n != 0) {
            *dst = *src;
            if (*src == 0)
                break;
            n--;
            dst++;
            src++;
        }
        if (n == 0)
            dst[-1] = 0;
    }
    D_08A62C64[0x17] = 0;
    self[0x74] = 0;
}
