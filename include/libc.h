/* Structures and functions of the game's C library (newlib for PSP, old version).
 * The field offsets are the ones used by the original code. */
#ifndef LIBC_H
#define LIBC_H
#include "common.h"

#include <stdarg.h>

typedef unsigned int size_t;

/* --- files (stdio FILE) ------------------------------------------------ */
typedef struct __sFILE {
    u8 *_p;                 /* 0x00 current position in the buffer */
    int _r;                 /* 0x04 bytes still to read */
    int _w;                 /* 0x08 space still free for writing */
    short _flags;           /* 0x0C __SRD, __SWR, ... */
    short _file;            /* 0x0E descrittore */
    struct { u8 *_base; int _size; } _bf; /* 0x10 buffer */
    int _lbfsize;           /* 0x18 */
    void *_cookie;          /* 0x1C passed to the four functions below */
    int (*_read)(void *cookie, char *buf, int n);        /* 0x20 */
    int (*_write)(void *cookie, const char *buf, int n); /* 0x24 */
    int (*_seek)(void *cookie, int off, int whence);     /* 0x28 */
    int (*_close)(void *cookie);                         /* 0x2C */
    struct { u8 *_base; int _size; } _ub; /* 0x30 buffer for ungetc */
    u8 *_up;                /* 0x38 */
    int _ur;                /* 0x3C */
    u8 _ubuf[3];            /* 0x40 */
    u8 _nbuf[1];            /* 0x43 one-byte buffer for files without a buffer */
    struct { u8 *_base; int _size; } _lb; /* 0x44 */
    int _blksize;           /* 0x4C */
    int _offset;            /* 0x50 */
    u8 _rest[0x5C - 0x54];
} FILE;                     /* 0x5C byte */

#define __SLBF 0x0001 /* line buffer */
#define __SNBF 0x0002 /* unbuffered */
#define __SRD  0x0004 /* in lettura */
#define __SWR  0x0008 /* in scrittura */
#define __SRW  0x0010
#define __SEOF 0x0020
#define __SERR 0x0040 /* error */
#define __SMBF 0x0080 /* buffer allocated with malloc */
#define __SAPP 0x0100
#define __SSTR 0x0200
#define __SOPT 0x0400 /* fseek ottimizzabile */
#define __SNPT 0x0800 /* fseek cannot be optimized */

/* block of FILEs allocated together */
struct _glue {
    struct _glue *_next;
    int _niobs;
    FILE *_iobs;
};

/* functions registered with atexit (at most 32 per block) */
struct _atexit {
    struct _atexit *_next;  /* 0x000 */
    int _ind;               /* 0x004 quante usate */
    void (*_fns[32])(void); /* 0x008 */
    void *_fnargs[32];      /* 0x088 argomenti (on_exit) */
    u32 _fntypes;           /* 0x108 bit n = function n takes (code, argument) */
};                          /* 0x10C byte */

/* state of the C library per thread/module */
struct _reent {
    int _errno;                     /* 0x000 */
    FILE *_stdin, *_stdout, *_stderr; /* 0x004 */
    u8 _pad010[0x30 - 0x10];
    int _current_category;          /* 0x030 */
    const char *_current_locale;    /* 0x034 */
    int __sdidinit;                 /* 0x038 stdin/out/err already prepared */
    void (*__cleanup)(struct _reent *); /* 0x03C */
    u8 _pad040[0x148 - 0x40];
    struct _atexit *_atexit;        /* 0x148 */
    struct _atexit _atexit0;        /* 0x14C */
    void (**_sig_func)(int);        /* 0x258 signal handlers (32) */
    struct _glue __sglue;           /* 0x25C */
    FILE __sf[3];                   /* 0x268 */
};

extern struct _reent *_impure_ptr;  /* state of the libc of the main module */
extern int g_errno;                 /* global errno written by the system calls */

struct _reent *__getreent(void);
struct _reent *_getmodreent(void);
#define _REENT (__getreent())

/* --- dynamic memory (Doug Lea's malloc, newlib version) --------------- */
void *_malloc_r(struct _reent *r, size_t n);
void _free_r(struct _reent *r, void *p);
void *_realloc_r(struct _reent *r, void *p, size_t n);
void *_calloc_r(struct _reent *r, size_t n, size_t size);
int _malloc_trim_r(struct _reent *r, size_t pad);
void __malloc_lock(struct _reent *r);
void __malloc_unlock(struct _reent *r);

/* --- strings and memory -------------------------------------------------------- */
void *memchr(const void *s, int c, size_t n);
int memcmp(const void *a, const void *b, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);
size_t strlen(const char *s);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t n);
char *strcat(char *s1, const char *s2);
char *strncat(char *s1, const char *s2, size_t n);
char *strchr(const char *s, int c);
char *strstr(const char *s, const char *find);

/* --- system calls with struct _reent -------------------------------- */
int _close_r(struct _reent *r, int fd);
int _fstat_r(struct _reent *r, int fd, void *st);
int _lseek_r(struct _reent *r, int fd, int off, int whence);
int _read_r(struct _reent *r, int fd, void *buf, int n);
void *_sbrk_r(struct _reent *r, int incr);

/* syscalls.c */
void _exit(int status) __attribute__((noreturn));
int lseek(int fd, int offset, int whence);
void *sbrk(int incr);
int read(int fd, void *buf, int n);
int close(int fd);
int fstat(int fd, u32 *st);
int isatty(int fd);
int getpid(void);
int kill(int pid, int sig);

/* stdio */
struct __siov { const void *iov_base; size_t iov_len; };          /* one piece of text */
struct __suio { struct __siov *uio_iov; int uio_iovcnt; int uio_resid; };
int __sread(void *cookie, char *buf, int n);
int __swrite(void *cookie, const char *buf, int n);
int __sclose(void *cookie);
int _write_r(struct _reent *r, int fd, const void *buf, int n);
int write(int fd, const void *buf, int n);
int __swsetup(FILE *fp);
int __sfvwrite(FILE *fp, struct __suio *uio);
int __sprint(FILE *fp, struct __suio *uio);
int _vfprintf_r(struct _reent *data, FILE *fp, const char *fmt, va_list ap);
int vfprintf(FILE *fp, const char *fmt, va_list ap);
int __sbprintf(FILE *fp, const char *fmt, va_list ap);
void *_localeconv_r(struct _reent *data);
void *localeconv(void);
char *_dtoa_r(struct _reent *ptr, f64 d, int mode, int ndigits, int *decpt, int *sign, char **rve);
int isinf(f64 x);
int isnan(f64 x);
void free(void *p);
int fflush(FILE *fp);
int _fwalk(struct _reent *ptr, int (*function)(FILE *));
void __sinit(struct _reent *s);
void __smakebuf(FILE *fp);
struct _glue *__sfmoreglue(struct _reent *d, int n);

#endif
