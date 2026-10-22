/* Libreria C: ricerca della struct _reent del thread/modulo corrente e versioni
 * "rientranti" delle chiamate di sistema, che copiano errno nella struct _reent. */
#include "mprec.h"
#include <pspintrman.h>
#include "sce_extra.h"

#define REENT_RESULT(r)   (*(_Bigint **)((u8 *)(r) + 0x40)) /* ultimo risultato di dtoa */
#define REENT_RESULT_K(r) (*(int *)((u8 *)(r) + 0x44))
#define REENT_CVTBUF(r)   (*(void **)((u8 *)(r) + 0x54))

extern char D_08910548[]; /* "C" */

/* Tabella modulo -> struct _reent, a blocchi concatenati (indice = id modulo % 32). */
struct modreent_table {
    struct modreent_table *next;
    struct _reent *reent[32];
    int modid[32];
};
extern struct modreent_table g_modreent_table;

/* Il registro k0 punta ai dati del thread corrente; al byte 4 c'è la sua struct _reent. */
static inline u8 *thread_data(void)
{
    u8 *k0;
    __asm__ volatile("move %0, $k0" : "=r"(k0));
    return k0;
}

struct _reent *__getreent(void)
{
    u8 *td = thread_data();
    struct _reent *r;

    if (td != NULL && (r = *(struct _reent **)(td + 4)) != NULL)
        return r;
    return _impure_ptr;
}

/* struct _reent del modulo a cui appartiene il thread corrente */
struct _reent *_getmodreent(void)
{
    u8 *td = thread_data();
    u8 *thr;
    struct modreent_table *t;
    int intr, modid, h;

    if (td == NULL || (thr = *(u8 **)(td + 4)) == NULL) {
        sceKernelPrintf("libc:%s: no reent structure\n", "_getmodreent");
        return _impure_ptr;
    }
    intr = sceKernelCpuSuspendIntr();
    modid = *(int *)(thr + 0x37C);
    h = modid % 32;
    for (t = &g_modreent_table; t != NULL; t = t->next) {
        if (t->modid[h] == modid) {
            struct _reent *r = t->reent[h];
            sceKernelCpuResumeIntr(intr);
            return r;
        }
    }
    sceKernelCpuResumeIntr(intr);
    sceKernelPrintf("libc:%s: no reent structure found\n", "_getmodreent");
    return _impure_ptr;
}

/* errno è il primo campo della struct _reent */
int *__errno(void)
{
    return &_REENT->_errno;
}

int _close_r(struct _reent *r, int fd)
{
    int ret;

    g_errno = 0;
    if ((ret = close(fd)) == -1 && g_errno != 0)
        r->_errno = g_errno;
    return ret;
}

int _fstat_r(struct _reent *r, int fd, void *st)
{
    int ret;

    g_errno = 0;
    if ((ret = fstat(fd, st)) == -1 && g_errno != 0)
        r->_errno = g_errno;
    return ret;
}

int _lseek_r(struct _reent *r, int fd, int off, int whence)
{
    int ret;

    g_errno = 0;
    if ((ret = lseek(fd, off, whence)) == -1 && g_errno != 0)
        r->_errno = g_errno;
    return ret;
}

int _read_r(struct _reent *r, int fd, void *buf, int n)
{
    int ret;

    g_errno = 0;
    if ((ret = read(fd, buf, n)) == -1 && g_errno != 0)
        r->_errno = g_errno;
    return ret;
}

void *_sbrk_r(struct _reent *r, int incr)
{
    void *ret;

    g_errno = 0;
    if ((ret = sbrk(incr)) == (void *)-1 && g_errno != 0)
        r->_errno = g_errno;
    return ret;
}

/* --- struct _reent per modulo ------------------------------------------------------- */
/* (nomi ipotetici: le funzioni non hanno nome nel gioco) */
#define REENT_MODID(r) (*(int *)((u8 *)(r) + 0x37C)) /* id del modulo proprietario */

/* Associa la struct _reent r al modulo modid; -1 se il modulo ne ha già una. */
int _setmodreent(int modid, struct _reent *r)
{
    struct modreent_table *t, *last, *nt;
    int intr = sceKernelCpuSuspendIntr();
    int h = modid % 32;

    REENT_MODID(r) = modid;
    t = &g_modreent_table;
    for (;;) {
        if (t->modid[h] == 0) {
            t->reent[h] = r;
            t->modid[h] = modid;
            sceKernelCpuResumeIntr(intr);
            return 0;
        }
        if (t->modid[h] == modid) {
            sceKernelCpuResumeIntr(intr);
            return -1;
        }
        last = t;
        if ((t = t->next) == NULL)
            break;
    }
    sceKernelCpuResumeIntr(intr);

    /* tutte le tabelle hanno quel posto occupato: se ne aggiunge una */
    nt = _malloc_r(_impure_ptr, sizeof(*nt));
    memset(nt, 0, sizeof(*nt));
    nt->reent[h] = r;
    nt->modid[h] = modid;
    intr = sceKernelCpuSuspendIntr();
    last->next = nt;
    sceKernelCpuResumeIntr(intr);
    return 0;
}

/* Toglie la struct _reent r dalla tabella; ritorna l'id del modulo o -1. */
int _delmodreent(struct _reent *r)
{
    int modid = REENT_MODID(r);
    int intr = sceKernelCpuSuspendIntr();
    struct modreent_table *t;

    for (t = &g_modreent_table; t != NULL; t = t->next) {
        if (t->modid[modid % 32] == modid) {
            t->modid[modid % 32] = 0;
            t->reent[modid % 32] = NULL;
            sceKernelCpuResumeIntr(intr);
            return modid;
        }
    }
    sceKernelCpuResumeIntr(intr);
    return -1;
}

/* Prepara una struct _reent nuova (come _REENT_INIT di newlib). */
void _reent_init(struct _reent *r)
{
    u8 *p = (u8 *)r;
    int i;

#define W(off) (*(u32 *)(p + (off)))
#define H(off) (*(u16 *)(p + (off)))
    W(0x000) = 0;                        /* _errno */
    W(0x004) = (u32)&r->__sf[0];         /* _stdin */
    W(0x008) = (u32)&r->__sf[1];         /* _stdout */
    W(0x00C) = (u32)&r->__sf[2];         /* _stderr */
    W(0x010) = 0;                        /* _inc */
    for (i = 0; i < 25; i++)             /* _emergency */
        p[0x14 + i] = 0;
    W(0x030) = 0;                        /* _current_category */
    W(0x034) = (u32)D_08910548;          /* _current_locale = "C" */
    for (i = 0x38; i <= 0x5C; i += 4)    /* __sdidinit, __cleanup, dati di dtoa, ... */
        W(i) = 0;
    p[0x60] = 0;
    for (i = 0; i < 36; i++)
        p[0x7C + i] = 0;
    W(0x0A8) = 1;                        /* seme di rand (64 bit) */
    W(0x0AC) = 0;
    H(0x0B0) = 0x330e;                   /* stato di rand48 */
    H(0x0B2) = 0xabcd;
    H(0x0B4) = 0x1234;
    H(0x0B6) = 0xe66d;
    H(0x0B8) = 0xdeec;
    H(0x0BA) = 5;
    H(0x0BC) = 0xb;
    W(0x0A0) = 0;
    for (i = 0xC0; i <= 0xD4; i += 4)
        W(i) = 0;
    for (i = 0xFC; i <= 0x120; i += 4)
        W(i) = 0;
    p[0xD8] = 0;
    p[0xE0] = 0;
    W(0x0F8) = 0;
    for (i = 0x148; i <= 0x154; i += 4)  /* _atexit, _atexit0 */
        W(i) = 0;
    W(0x254) = 0;
    W(0x1D4) = 0;
    for (i = 0x258; i <= 0x264; i += 4)  /* _sig_func, __sglue */
        W(i) = 0;
    memset(&r->__sf[0], 0, sizeof(r->__sf));
    W(0x37C) = 0;                        /* id del modulo */
#undef W
#undef H
}

/* libera la catena di blocchi di FILE, dall'ultimo */
void cleanup_glue(struct _reent *ptr, struct _glue *glue)
{
    if (glue->_next)
        cleanup_glue(ptr, glue->_next);
    _free_r(ptr, glue);
}

/* Libera la memoria di una struct _reent che non serve più (non quella principale). */
void _reclaim_reent(struct _reent *ptr)
{
    if (ptr == _impure_ptr)
        return;

    /* dati di dtoa */
    if (REENT_RESULT(ptr)) {
        REENT_RESULT(ptr)->_k = REENT_RESULT_K(ptr);
        REENT_RESULT(ptr)->_maxwds = 1 << REENT_RESULT_K(ptr);
        Bfree(ptr, REENT_RESULT(ptr));
    }
    if (REENT_P5S(ptr)) {
        _Bigint *p = REENT_P5S(ptr), *next;

        do {
            next = p->_next;
            Bfree(ptr, p);
            p = next;
        } while (p);
    }
    if (REENT_FREELIST(ptr)) {
        int i;

        for (i = 0; i < 15; i++) {
            _Bigint *thisone, *nextone = REENT_FREELIST(ptr)[i];

            while (nextone) {
                thisone = nextone;
                nextone = nextone->_next;
                _free_r(ptr, thisone);
            }
        }
        _free_r(ptr, REENT_FREELIST(ptr));
    }

    /* blocchi di atexit */
    if (ptr->_atexit && ptr->_atexit != &ptr->_atexit0) {
        struct _atexit *p, *q;

        for (p = ptr->_atexit; p != &ptr->_atexit0;) {
            q = p;
            p = p->_next;
            _free_r(ptr, q);
        }
    }

    if (REENT_CVTBUF(ptr))
        _free_r(ptr, REENT_CVTBUF(ptr));

    if (ptr->__sdidinit) {
        struct _glue *g;

        ptr->__cleanup(ptr);
        /* buffer dei FILE allocati con malloc */
        for (g = &ptr->__sglue; g != NULL; g = g->_next) {
            FILE *fp = g->_iobs;
            int n;

            for (n = 0; n < g->_niobs; n++, fp++) {
                if ((fp->_flags & __SMBF) && fp->_bf._base != NULL) {
                    _free_r(ptr, fp->_bf._base);
                    fp->_bf._base = NULL;
                }
            }
        }
        if (ptr->__sglue._next)
            cleanup_glue(ptr, ptr->__sglue._next);
    }
}
