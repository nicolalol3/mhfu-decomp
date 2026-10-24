/* Lights of the 3D scene: 3 directional lights (D_08A62C58), from the default table or
 * from the stage data. (Guessed names.) */
#include "menu.h"

extern u8 *D_08A62C58;
extern u8 **D_09A4F060;
extern u8 D_089BAB08[];
extern const f32 D_089A5B90[][3], D_089A5BB4[][4], D_089A5BE4[][4], D_089A5C14[][4];
void light_obj_nop(void);
void *memcpy(void *d, const void *s, size_t n);

u32 *light_obj_dtor(u32 *o, s32 flag)
{
    if (o != NULL) {
        o[0] = (u32)D_089BAB08;
        if ((s16)flag > 0)
            light_obj_nop();
    }
    return o;
}

void *light_obj_ret(void *a, void *b)
{
    return b;
}

void light_obj_nop(void)
{
}

u8 *lights_ctor(u8 *l)
{
    D_08A62C58 = l;
    memset(l, 0, 0x150);
    U16(l, 0x15C) = 0xFFFF;
    return l;
}

u8 *lights_dtor(u8 *l, s32 flag)
{
    if (l != NULL) {
        D_08A62C58 = NULL;
        if ((s16)flag > 0)
            func_0890BFA4(l);
    }
    return l;
}

/* light i: color (mode 1), or color, ambient and direction (mode 0) */
void light_get(u8 *l, f32 *out, s32 i_arg, u32 mode_arg)
{
    s16 i = i_arg;
    u16 mode = mode_arg;

    if (U16(l, 0x15C) == 0) {
        if (mode != 1) {
            if (mode != 0)
                return;
            out[8] = -1.0f * D_089A5B90[i][0];
            out[9] = -1.0f * D_089A5B90[i][1];
            out[10] = -1.0f * D_089A5B90[i][2];
            out[3] = D_089A5BE4[i][0];
            out[4] = D_089A5BE4[i][1];
            out[5] = D_089A5BE4[i][2];
        }
        out[0] = D_089A5BB4[i][0];
        out[1] = D_089A5BB4[i][1];
        out[2] = D_089A5BB4[i][2];
    } else {
        u8 *w = *D_09A4F060 + i * 0x24;

        if (mode != 1) {
            if (mode != 0)
                return;
            out[8] = -1.0f * F32(w, 0x334);
            out[9] = -1.0f * F32(w, 0x338);
            out[10] = -1.0f * F32(w, 0x33C);
            out[3] = F32(w, 0x34C);
            out[4] = F32(w, 0x350);
            out[5] = F32(w, 0x354);
        }
        out[0] = F32(w, 0x340);
        out[1] = F32(w, 0x344);
        out[2] = F32(w, 0x348);
    }
}

/* lights of the stage */
void lights_init(u8 *l, u32 stage)
{
    int i;

    memset(l, 0, 0x150);
    U16(l, 0x15C) = stage;
    for (i = 0; i < 3; i++)
        light_get(l, (f32 *)(l + 0xC0 + i * 0x30), (s16)i, 0);
}

static void light_color_from(u8 *l, u8 *h, int i)
{
    f32 *d = (f32 *)(l + 0xC0 + i * 0x30);

    if (U32(h, 0x1DC) == 0 || i == 2) {
        light_get(l, d, (s16)i, 1);
    } else {
        const f32 *s = (const f32 *)(U32(h, 0x1DC) + i * 0x10);

        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
    }
}

/* light colors of the hunter h (its own colors, if any) */
void lights_set_hunter(u8 *l, u8 *h)
{
    int i;

    for (i = 0; i < 3; i++)
        light_color_from(l, h, i);
}

/* same, with special colors in some areas (0xC, 0xE, 0x1C, 0x1E) */
void lights_set_hunter_area(u8 *l, u8 *h)
{
    int i;

    for (i = 0; i < 3; i++) {
        u16 st = U16(l, 0x15C);

        if ((st == 0x1E || st == 0x1C || st == 0xE || st == 0xC) && (U32(h, 0x614) & 0x4000)) {
            f32 *d = (f32 *)(l + 0xC0 + i * 0x30);

            d[0] = D_089A5C14[i][0];
            d[1] = D_089A5C14[i][1];
            d[2] = D_089A5C14[i][2];
        } else {
            light_color_from(l, h, i);
        }
    }
}

void lights_reset_colors(u8 *l)
{
    int i;

    for (i = 0; i < 3; i++)
        light_get(l, (f32 *)(l + 0xC0 + i * 0x30), (s16)i, 1);
}

/* color of the light i for the hunter h: moves 1/5 of the way to the light color every frame */
void light_fade_color(u32 *out, u8 *l, u8 *h, s32 i)
{
    const f32 *src = (const f32 *)(l + i * 0x30 + 0xC0);
    u8 *c = h + i * 4 + 0x2AC;
    f32 r, g, b;
    u8 rgba[4];

    if (*(u32 *)c == 0) {
        r = src[0];
        g = src[1];
        b = src[2];
    } else {
        r = (f32)c[0] / 255.0f;
        g = (f32)c[1] / 255.0f;
        b = (f32)c[2] / 255.0f;
        r = r + (src[0] - r) / 5.0f;
        g = g + (src[1] - g) / 5.0f;
        b = b + (src[2] - b) / 5.0f;
    }
    rgba[0] = (u32)(255.0f * r);
    rgba[1] = (u32)(255.0f * g);
    rgba[2] = (u32)(255.0f * b);
    rgba[3] = 0xFF;
    memcpy(c, rgba, 4);
    memcpy(out, rgba, 4);
}
