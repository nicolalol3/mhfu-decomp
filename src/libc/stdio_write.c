/* C library: writing to FILEs (basic read/write/close, buffers) and the
 * printf/sprintf functions that rely on _vfprintf_r (vfprintf.c). */
#include "libc.h"

#define __SOFF 0x1000 /* _offset valido */

int __sread(void *cookie, char *buf, int n)
{
    FILE *fp = cookie;
    int ret = _read_r(_REENT, fp->_file, buf, n);

    if (ret >= 0)
        fp->_offset += ret;
    else
        fp->_flags &= ~__SOFF;
    return ret;
}

int __swrite(void *cookie, const char *buf, int n)
{
    FILE *fp = cookie;

    if (fp->_flags & __SAPP)
        _lseek_r(_REENT, fp->_file, 0, 2 /* SEEK_END */);
    fp->_flags &= ~__SOFF;
    return _write_r(_REENT, fp->_file, buf, n);
}

int __sclose(void *cookie)
{
    FILE *fp = cookie;

    return _close_r(_REENT, fp->_file);
}

int _write_r(struct _reent *r, int fd, const void *buf, int n)
{
    int ret;

    g_errno = 0;
    if ((ret = write(fd, buf, n)) == -1 && g_errno != 0)
        r->_errno = g_errno;
    return ret;
}

/* Prepares fp for writing (buffer included); -1 if the file cannot be written. */
int __swsetup(FILE *fp)
{
    if (!_REENT->__sdidinit)
        __sinit(_REENT);

    if ((fp->_flags & __SWR) == 0) {
        if ((fp->_flags & __SRW) == 0)
            return -1;
        if (fp->_flags & __SRD) {
            /* discards the characters pushed back with ungetc */
            if (fp->_ub._base != NULL) {
                if (fp->_ub._base != fp->_ubuf)
                    _free_r(_REENT, fp->_ub._base);
                fp->_ub._base = NULL;
            }
            fp->_flags &= ~(__SRD | __SEOF);
            fp->_r = 0;
            fp->_p = fp->_bf._base;
        }
        fp->_flags |= __SWR;
    }
    if (fp->_bf._base == NULL)
        __smakebuf(fp);
    if (fp->_flags & __SLBF) {
        fp->_w = 0;
        fp->_lbfsize = -fp->_bf._size;
    } else {
        fp->_w = fp->_flags & __SNBF ? 0 : fp->_bf._size;
    }
    return 0;
}

#define cantwrite(fp) \
    ((((fp)->_flags & __SWR) == 0 || (fp)->_bf._base == NULL) && __swsetup(fp))

/* Writes into fp a list of text pieces (uio), respecting the buffer type. */
int __sfvwrite(FILE *fp, struct __suio *uio)
{
    size_t len;
    const char *p = NULL;
    struct __siov *iov;
    int w, s;
    char *nl;
    int nlknown, nldist;

    if ((len = uio->uio_resid) == 0)
        return 0;
    if (cantwrite(fp))
        return -1;

    iov = uio->uio_iov;
    len = 0;
#define GETIOV(extra_work)  \
    while (len == 0) {      \
        extra_work;         \
        p = iov->iov_base;  \
        len = iov->iov_len; \
        iov++;              \
    }
#define COPY(n) memmove(fp->_p, p, n)

    if (fp->_flags & __SNBF) {
        /* unbuffered: at most 1024 bytes at a time */
        do {
            GETIOV(;);
            w = fp->_write(fp->_cookie, p, len < 1024 ? len : 1024);
            if (w <= 0)
                goto err;
            p += w;
            len -= w;
        } while ((uio->uio_resid -= w) != 0);
    } else if ((fp->_flags & __SLBF) == 0) {
        /* buffer pieno */
        do {
            GETIOV(;);
            w = fp->_w;
            if (fp->_flags & __SSTR) {
                /* string in memory (sprintf); if allocated with malloc it grows */
                if (len > (size_t)w && (fp->_flags & __SMBF)) {
                    int curpos = fp->_p - fp->_bf._base;
                    u8 *str = _realloc_r(_REENT, fp->_bf._base, curpos + len);

                    if (str == NULL)
                        goto err;
                    fp->_p = str + curpos;
                    fp->_bf._size = curpos + len;
                    fp->_bf._base = str;
                    w = len;
                    fp->_w = len;
                }
                if (len < (size_t)w)
                    w = len;
                COPY(w);
                fp->_w -= w;
                fp->_p += w;
                w = len; /* pretends everything was copied */
            } else if (fp->_p > fp->_bf._base && len > (size_t)w) {
                /* fills and flushes */
                COPY(w);
                fp->_p += w;
                if (fflush(fp))
                    goto err;
            } else if (len >= (size_t)(w = fp->_bf._size)) {
                /* write directly */
                w = fp->_write(fp->_cookie, p, w);
                if (w <= 0)
                    goto err;
            } else {
                /* fill and done */
                w = len;
                COPY(w);
                fp->_w -= w;
                fp->_p += w;
            }
            p += w;
            len -= w;
        } while ((uio->uio_resid -= w) != 0);
    } else {
        /* line buffer: as above, but flushed after each '\n' */
        nlknown = 0;
        nldist = 0;
        do {
            GETIOV(nlknown = 0);
            if (!nlknown) {
                nl = memchr(p, '\n', len);
                nldist = nl ? nl + 1 - p : len + 1;
                nlknown = 1;
            }
            s = (size_t)nldist <= len ? (size_t)nldist : len;
            w = fp->_w + fp->_bf._size;
            if (fp->_p > fp->_bf._base && s > w) {
                COPY(w);
                fp->_p += w;
                if (fflush(fp))
                    goto err;
            } else if (s >= (w = fp->_bf._size)) {
                w = fp->_write(fp->_cookie, p, w);
                if (w <= 0)
                    goto err;
            } else {
                w = s;
                COPY(w);
                fp->_w -= w;
                fp->_p += w;
            }
            if ((nldist -= w) == 0) {
                /* wrote the '\n': flush */
                if (fflush(fp))
                    goto err;
                nlknown = 0;
            }
            p += w;
            len -= w;
        } while ((uio->uio_resid -= w) != 0);
    }
    return 0;

err:
    fp->_flags |= __SERR;
    return -1;
}

/* empties the list of pieces accumulated by vfprintf */
int __sprint(FILE *fp, struct __suio *uio)
{
    int err;

    if (uio->uio_resid == 0) {
        uio->uio_iovcnt = 0;
        return 0;
    }
    err = __sfvwrite(fp, uio);
    uio->uio_resid = 0;
    uio->uio_iovcnt = 0;
    return err;
}

/* --- printf and similar --------------------------------------------------------------- */

/* For unbuffered files: prints into a temporary 1 KB buffer and then writes it. */
int __sbprintf(FILE *fp, const char *fmt, va_list ap)
{
    int ret;
    FILE fake;
    u8 buf[1024];

    fake._flags = fp->_flags & ~__SNBF;
    fake._file = fp->_file;
    fake._cookie = fp->_cookie;
    fake._write = fp->_write;
    fake._bf._base = fake._p = buf;
    fake._bf._size = fake._w = sizeof(buf);
    fake._lbfsize = 0;

    ret = vfprintf(&fake, fmt, ap);
    if (ret >= 0 && fflush(&fake))
        ret = -1;
    if (fake._flags & __SERR)
        fp->_flags |= __SERR;
    return ret;
}

int vfprintf(FILE *fp, const char *fmt, va_list ap)
{
    if (!_REENT->__sdidinit)
        __sinit(_REENT);
    return _vfprintf_r(_REENT, fp, fmt, ap);
}

int sprintf(char *str, const char *fmt, ...)
{
    int ret;
    va_list ap;
    FILE f;

    f._flags = __SWR | __SSTR;
    f._bf._base = f._p = (u8 *)str;
    f._bf._size = f._w = 0x7fffffff;
    f._file = -1;
    va_start(ap, fmt);
    ret = _vfprintf_r(_REENT, &f, fmt, ap);
    va_end(ap);
    *f._p = 0;
    return ret;
}

int printf(const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = vfprintf(_REENT->_stdout, fmt, ap);
    va_end(ap);
    return ret;
}

int _printf_r(struct _reent *ptr, const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = _vfprintf_r(ptr, ptr->_stdout, fmt, ap);
    va_end(ap);
    return ret;
}
