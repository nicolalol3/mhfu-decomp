/* Libreria C: parti di base di stdio (preparazione di stdin/stdout/stderr, buffer,
 * svuotamento dei buffer in scrittura). */
#include "libc.h"

/* Funzioni dei FILE all'indirizzo originale: nei FILE si salvano questi indirizzi, come
 * nel gioco (l'originale poi salta alla versione in C, dove c'è). */
extern int func_0880E978(void *cookie, char *buf, int n);        /* __sread */
extern int func_0880EA08(void *cookie, const char *buf, int n);  /* __swrite */
extern int func_0880EA8C(void *cookie, int off, int whence);     /* __sseek (non ancora in C) */
extern int func_0880EB20(void *cookie);                          /* __sclose */
extern void func_0880A8C8(struct _reent *r);                     /* _cleanup_r */

/* Scrive su disco il contenuto del buffer di fp; fp == NULL: tutti i file aperti. */
int fflush(FILE *fp)
{
    u8 *p;
    int n, t;

    if (fp == NULL)
        return _fwalk(_impure_ptr, fflush);

    if (!_REENT->__sdidinit)
        __sinit(_REENT);

    t = fp->_flags;
    if ((t & __SWR) == 0 || (p = fp->_bf._base) == NULL)
        return 0;
    n = fp->_p - p;
    fp->_p = p;
    fp->_w = (t & (__SLBF | __SNBF)) ? 0 : fp->_bf._size;
    while (n > 0) {
        t = fp->_write(fp->_cookie, (char *)p, n);
        n -= t;
        p += t;
        if (t <= 0) {
            fp->_flags |= __SERR;
            return -1;
        }
    }
    return 0;
}

/* prepara uno dei tre FILE standard */
void std(FILE *ptr, int flags, int file, struct _reent *data)
{
    ptr->_p = NULL;
    ptr->_r = 0;
    ptr->_w = 0;
    ptr->_flags = flags;
    ptr->_file = file;
    ptr->_bf._base = NULL;
    ptr->_bf._size = 0;
    ptr->_lbfsize = 0;
    ptr->_cookie = ptr;
    ptr->_read = func_0880E978;
    ptr->_write = func_0880EA08;
    ptr->_seek = func_0880EA8C;
    ptr->_close = func_0880EB20;
}

/* alloca un nuovo blocco di n FILE (azzerati) */
struct _glue *__sfmoreglue(struct _reent *d, int n)
{
    struct _glue *g = _malloc_r(d, sizeof(*g) + n * sizeof(FILE));
    FILE *p;

    if (g == NULL)
        return NULL;
    p = (FILE *)(g + 1);
    g->_next = NULL;
    g->_niobs = n;
    g->_iobs = p;
    memset(p, 0, n * sizeof(FILE));
    return g;
}

/* prima inizializzazione di stdio per una struct _reent */
void __sinit(struct _reent *s)
{
    s->__cleanup = func_0880A8C8;
    s->__sdidinit = 1;
    s->__sglue._next = NULL;
    s->__sglue._niobs = 3;
    s->__sglue._iobs = &s->__sf[0];
    std(s->_stdin, __SRD, 0, s);
    std(s->_stdout, __SWR | __SLBF, 1, s);
    std(s->_stderr, __SWR | __SNBF, 2, s);
}

/* Chiama function su ogni FILE in uso di ptr e poi su quelli del modulo principale;
 * ritorna l'OR dei risultati. */
static inline int fwalk_glue(struct _reent *ptr, int (*function)(FILE *))
{
    struct _glue *g;
    FILE *fp;
    int n, ret = 0;

    for (g = &ptr->__sglue; g != NULL; g = g->_next)
        for (fp = g->_iobs, n = g->_niobs; --n >= 0; fp++)
            if (fp->_flags != 0)
                ret |= function(fp);
    return ret;
}

int _fwalk(struct _reent *ptr, int (*function)(FILE *))
{
    int ret = 0;

    if (ptr != _impure_ptr)
        ret = fwalk_glue(ptr, function);
    return ret | fwalk_glue(_impure_ptr, function);
}

/* Assegna un buffer a fp: 1 KB allocato, oppure il buffer interno da un byte. */
void __smakebuf(FILE *fp)
{
    u32 st[16];
    int couldbetty;
    void *p;

    if (fp->_flags & __SNBF) {
        fp->_bf._base = fp->_p = fp->_nbuf;
        fp->_bf._size = 1;
        return;
    }
    if (fp->_file < 0 || _fstat_r(_REENT, fp->_file, st) < 0) {
        couldbetty = 0;
        fp->_flags |= __SNPT;
    } else {
        u32 mode = st[1] & 0xF000;
        couldbetty = mode == 0x2000; /* dispositivo a caratteri */
        if (mode == 0x8000 && fp->_seek == func_0880EA8C) { /* file normale */
            fp->_flags |= __SOPT;
            fp->_blksize = 1024;
        } else {
            fp->_flags |= __SNPT;
        }
    }
    if ((p = _malloc_r(_REENT, 1024)) == NULL) {
        fp->_flags |= __SNBF;
        fp->_bf._base = fp->_p = fp->_nbuf;
        fp->_bf._size = 1;
    } else {
        _REENT->__cleanup = func_0880A8C8;
        fp->_flags |= __SMBF;
        fp->_bf._base = fp->_p = p;
        fp->_bf._size = 1024;
        if (couldbetty && isatty(fp->_file))
            fp->_flags |= __SLBF;
    }
}
