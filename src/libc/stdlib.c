/* C library: program exit (abort, atexit, exit), abs and a minimal locale. */
#include "libc.h"
#include <pspintrman.h>

int raise(int sig);

/* original strings: setlocale returns exactly this address */
extern const char D_089104C0[]; /* "C" */
extern const char D_089104AC[]; /* "" */
extern u8 D_089104C4[];         /* struct lconv */

void abort(void)
{
    raise(6); /* SIGABRT */
    _exit(1);
}

int abs(int i)
{
    return i < 0 ? -i : i;
}

/* Registers a function to call at exit; returns 0, or -1 if out of memory. */
int atexit(void (*fn)(void))
{
    int intr = sceKernelCpuSuspendIntr();
    struct _reent *r = _getmodreent();
    struct _atexit *p = r->_atexit;
    int ind;

    if (p == NULL)
        r->_atexit = p = &r->_atexit0;
    ind = p->_ind;
    if (ind >= 32) { /* block full: allocates another one */
        sceKernelCpuResumeIntr(intr);
        p = _malloc_r(r, sizeof(*p));
        if (p == NULL)
            return -1;
        intr = sceKernelCpuSuspendIntr();
        ind = 0;
        p->_ind = 0;
        p->_next = r->_atexit;
        p->_fntypes = 0;
        r->_atexit = p;
    }
    p->_fns[ind] = fn;
    p->_ind = ind + 1;
    sceKernelCpuResumeIntr(intr);
    return 0;
}

/* Calls the registered functions (from the last to the first) and ends the program. */
void exit(int code)
{
    struct _atexit *p;
    int n, i;

    for (p = _getmodreent()->_atexit; p != NULL; p = p->_next) {
        for (n = p->_ind - 1, i = n >= 0 ? 1 << n : 0; n >= 0; --n, i >>= 1) {
            if (p->_fntypes & i)
                ((void (*)(int, void *))p->_fns[n])(code, p->_fnargs[n]);
            else
                p->_fns[n]();
        }
    }
    _exit(code);
}

/* Only the settings "C" and "" are accepted (equivalent). */
char *_setlocale_r(struct _reent *p, int category, const char *locale)
{
    if (locale != NULL) {
        if (strcmp(locale, D_089104C0) != 0 && strcmp(locale, D_089104AC) != 0)
            return NULL;
        p->_current_category = category;
        p->_current_locale = locale;
    }
    return (char *)D_089104C0;
}

void *_localeconv_r(struct _reent *data)
{
    return D_089104C4;
}

char *setlocale(int category, const char *locale)
{
    return _setlocale_r(_REENT, category, locale);
}

void *localeconv(void)
{
    return _localeconv_r(_REENT);
}

/* --- random numbers ------------------------------------------------------------------ */
#define RAND_NEXT(r) (*(u64 *)((u8 *)(r) + 0xA8)) /* 64-bit state in struct _reent */

void srand(unsigned int seed)
{
    RAND_NEXT(_REENT) = seed;
}

/* linear congruential generator with 64 bits; returns 31 bits */
int rand(void)
{
    RAND_NEXT(_REENT) = RAND_NEXT(_REENT) * 6364136223846793005ULL + 1;
    return (int)((RAND_NEXT(_REENT) >> 32) & 0x7fffffff);
}
