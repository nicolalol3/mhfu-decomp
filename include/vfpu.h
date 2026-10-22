/* Piccole funzioni che usano l'unità vettoriale della PSP (VFPU), per ottenere
 * esattamente gli stessi risultati del gioco originale (seno, coseno, radice inversa). */
#ifndef VFPU_H
#define VFPU_H
#include "common.h"

/* seno di x (in radianti), calcolato come fa il gioco: vsin(x * 2/pi) */
static inline f32 vfpu_sinf(f32 x)
{
    f32 r;
    __asm__ volatile("mtv %1, S000\n"
                     "vcst.s S001, VFPU_2_PI\n"
                     "vmul.s S000, S000, S001\n"
                     "vsin.s S010, S000\n"
                     "mfv %0, S010" : "=r"(r) : "r"(x));
    return r;
}

/* coseno di x (in radianti) */
static inline f32 vfpu_cosf(f32 x)
{
    f32 r;
    __asm__ volatile("mtv %1, S000\n"
                     "vcst.s S001, VFPU_2_PI\n"
                     "vmul.s S000, S000, S001\n"
                     "vcos.s S010, S000\n"
                     "mfv %0, S010" : "=r"(r) : "r"(x));
    return r;
}

/* 1/radice(x), approssimata come dalla VFPU */
static inline f32 vfpu_rsqf(f32 x)
{
    f32 r;
    __asm__ volatile("mtv %1, S000\n"
                     "vrsq.s S010, S000\n"
                     "mfv %0, S010" : "=r"(r) : "r"(x));
    return r;
}

/* radice quadrata con l'istruzione della FPU */
static inline f32 fpu_sqrtf(f32 x)
{
    f32 r;
    __asm__("sqrt.s %0, %1" : "=f"(r) : "f"(x));
    return r;
}

#endif
