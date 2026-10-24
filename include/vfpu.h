/* Small functions that use the PSP's vector unit (VFPU), to get
 * exactly the same results as the original game (sine, cosine, reciprocal square root). */
#ifndef VFPU_H
#define VFPU_H
#include "common.h"

/* sine of x (in radians), computed as the game does: vsin(x * 2/pi) */
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

/* cosine of x (in radians) */
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

/* 1/sqrt(x), approximated as the VFPU does */
static inline f32 vfpu_rsqf(f32 x)
{
    f32 r;
    __asm__ volatile("mtv %1, S000\n"
                     "vrsq.s S010, S000\n"
                     "mfv %0, S010" : "=r"(r) : "r"(x));
    return r;
}

/* square root with the VFPU instruction */
static inline f32 vfpu_sqrtf(f32 x)
{
    f32 r;
    __asm__ volatile("mtv %1, S000\n"
                     "vsqrt.s S010, S000\n"
                     "mfv %0, S010" : "=r"(r) : "r"(x));
    return r;
}

/* distance between the points a and b (x, y, z), as vsub.t + vdot.t + vsqrt.s */
static inline f32 vfpu_dist3(const f32 *a, const f32 *b)
{
    f32 r;
    __asm__ volatile("lv.s S000, 0(%1)\n"
                     "lv.s S001, 4(%1)\n"
                     "lv.s S002, 8(%1)\n"
                     "lv.s S010, 0(%2)\n"
                     "lv.s S011, 4(%2)\n"
                     "lv.s S012, 8(%2)\n"
                     "vsub.t C000, C000, C010\n"
                     "vdot.t S010, C000, C000\n"
                     "vsqrt.s S010, S010\n"
                     "mfv %0, S010" : "=r"(r) : "r"(a), "r"(b) : "memory");
    return r;
}

/* squared distance between a and b (vsub.t + vdot.t) */
static inline f32 vfpu_dist3_sq(const f32 *a, const f32 *b)
{
    f32 r;
    __asm__ volatile("lv.s S000, 0(%1)\n"
                     "lv.s S001, 4(%1)\n"
                     "lv.s S002, 8(%1)\n"
                     "lv.s S010, 0(%2)\n"
                     "lv.s S011, 4(%2)\n"
                     "lv.s S012, 8(%2)\n"
                     "vsub.t C000, C000, C010\n"
                     "vdot.t S010, C000, C000\n"
                     "mfv %0, S010" : "=r"(r) : "r"(a), "r"(b) : "memory");
    return r;
}

/* square root with the FPU instruction */
static inline f32 fpu_sqrtf(f32 x)
{
    f32 r;
    __asm__("sqrt.s %0, %1" : "=f"(r) : "f"(x));
    return r;
}

#endif
