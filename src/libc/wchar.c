/* C library: conversion of "wide" (16-bit) characters to multibyte characters, according to
 * the encoding chosen with setlocale: "C-UTF-8", "C-SJIS", "C-EUCJP", "C-JIS" or plain. */
#include "libc.h"

typedef u16 wchar_t16;
typedef struct { int __count; int __value; } mbstate_t; /* __count: JIS state */

#define EILSEQ 138

extern char __lc_ctype[]; /* name of the character encoding (LC_CTYPE) */

#define _issjis1(c) (((c) >= 0x81 && (c) <= 0x9f) || ((c) >= 0xe0 && (c) <= 0xef))
#define _issjis2(c) (((c) >= 0x40 && (c) <= 0x7e) || ((c) >= 0x80 && (c) <= 0xfc))
#define _iseucjp(c) ((c) >= 0xa1 && (c) <= 0xfe)
#define _isjis(c)   ((c) >= 0x21 && (c) <= 0x7e)

/* Writes the wide character wchar into s; returns the bytes written, or -1 if it cannot be represented.
 * With s == NULL says whether the encoding has a state (1) or not (0). */
int _wctomb_r(struct _reent *r, char *s, wchar_t16 wchar, mbstate_t *state)
{
    u8 char1 = wchar >> 8, char2 = (u8)wchar;

    if (strlen(__lc_ctype) <= 1) {
        /* codifica semplice */
    } else if (!strcmp(__lc_ctype, "C-UTF-8")) {
        if (s == NULL)
            return 0;
        if (wchar <= 0x7f) {
            *s = wchar;
            return 1;
        }
        if (wchar <= 0x7ff) {
            *s++ = 0xc0 | ((wchar & 0x7c0) >> 6);
            *s = 0x80 | (wchar & 0x3f);
            return 2;
        }
        if (wchar >= 0xd800 && wchar <= 0xdfff) /* half of a UTF-16 surrogate pair */
            return -1;
        *s++ = 0xe0 | ((wchar & 0xf000) >> 12);
        *s++ = 0x80 | ((wchar & 0xfc0) >> 6);
        *s = 0x80 | (wchar & 0x3f);
        return 3;
    } else if (!strcmp(__lc_ctype, "C-SJIS")) {
        if (s == NULL)
            return 0;
        if (char1 != 0) {
            if (_issjis1(char1) && _issjis2(char2)) {
                *s++ = char1;
                *s = char2;
                return 2;
            }
            return -1;
        }
    } else if (!strcmp(__lc_ctype, "C-EUCJP")) {
        if (s == NULL)
            return 0;
        if (char1 != 0) {
            if (_iseucjp(char1) && _iseucjp(char2)) {
                *s++ = char1;
                *s = char2;
                return 2;
            }
            return -1;
        }
    } else if (!strcmp(__lc_ctype, "C-JIS")) {
        int cnt = 0;

        if (s == NULL)
            return 1;
        if (char1 != 0) {
            if (_isjis(char1) && _isjis(char2)) {
                if (state->__count == 0) {
                    /* switches from ASCII to JIS */
                    state->__count = 1;
                    *s++ = 0x1b;
                    *s++ = '$';
                    *s++ = 'B';
                    cnt = 3;
                }
                *s++ = char1;
                *s = char2;
                return cnt + 2;
            }
            return -1;
        }
        if (state->__count != 0) {
            /* switches back from JIS to ASCII */
            state->__count = 0;
            *s++ = 0x1b;
            *s++ = '(';
            *s++ = 'B';
            cnt = 3;
        }
        *s = char2;
        return cnt + 1;
    }
    if (s == NULL)
        return 0;
    *s = (char)wchar;
    return 1;
}

int _wcrtomb_r(struct _reent *ptr, char *s, wchar_t16 wc, mbstate_t *ps)
{
    int retval;
    char buf[16];

    if (s == NULL)
        retval = _wctomb_r(ptr, buf, 0, ps);
    else
        retval = _wctomb_r(ptr, s, wc, ps);
    if (retval == -1) {
        ps->__count = 0;
        ptr->_errno = EILSEQ;
        return -1;
    }
    return retval;
}

/* Converts the wide string *src; dst == NULL: only counts the bytes needed.
 * Stops before a character that does not fit in len bytes. */
size_t _wcsrtombs_r(struct _reent *r, char *dst, const wchar_t16 **src, size_t len, mbstate_t *ps)
{
    char *ptr = dst;
    char buff[16];
    const wchar_t16 *pwcs;
    size_t n;
    int i;

    if (dst == NULL)
        len = (size_t)-1;
    n = 0;
    pwcs = *src;
    while (n < len) {
        int count = ps->__count;
        int wch = ps->__value;
        int bytes = _wcrtomb_r(r, buff, *pwcs, ps);

        if (bytes == -1) {
            r->_errno = EILSEQ;
            ps->__count = 0;
            return (size_t)-1;
        }
        if (len - bytes < n || len <= (size_t)bytes) {
            /* no room: restores the state as it was before */
            ps->__value = wch;
            ps->__count = count;
            return n;
        }
        n += bytes;
        if (dst) {
            for (i = 0; i < bytes; ++i)
                *ptr++ = buff[i];
            ++*src;
        }
        if (*pwcs++ == 0) {
            if (dst)
                *src = NULL;
            ps->__count = 0;
            return n - 1;
        }
    }
    return n;
}
