/* Libreria C: funzioni di base su blocchi di memoria (memchr, memcmp, memcpy, memmove,
 * memset). Lavorano a parole di 4 byte quando gli indirizzi lo permettono. */
#include "libc.h"

/* parola di 4 byte letta/scritta a un indirizzo qualsiasi (lwl/lwr, swl/swr) */
typedef struct { u32 v; } __attribute__((packed)) unaligned_u32;

/* vero se la parola x contiene un byte a zero */
#define DETECTNULL(x) (((x) - 0x01010101) & ~(x) & 0x80808080)

void *memchr(const void *src, int c, size_t n)
{
    const u8 *s = src;
    u8 d = c;

    if (n >= 4 && ((u32)s & 3) == 0) {
        u32 mask = 0;
        int i;

        for (i = 0; i < 4; i++)
            mask = (mask << 8) + d;
        while (n >= 4) {
            u32 w = *(const u32 *)s ^ mask;
            if (DETECTNULL(w))
                break; /* il byte cercato è in questa parola */
            n -= 4;
            s += 4;
        }
    }
    while (n--) {
        if (*s == d)
            return (void *)s;
        s++;
    }
    return NULL;
}

int memcmp(const void *m1, const void *m2, size_t n)
{
    const u8 *s1 = m1, *s2 = m2;

    if (n >= 4 && (((u32)s1 | (u32)s2) & 3) == 0) {
        while (n >= 4 && *(const u32 *)s1 == *(const u32 *)s2) {
            s1 += 4;
            s2 += 4;
            n -= 4;
        }
    }
    while (n--) {
        if (*s1 != *s2)
            return *s1 - *s2;
        s1++;
        s2++;
    }
    return 0;
}

/* Copia in avanti: a blocchi di 16 byte se entrambi gli indirizzi sono allineati,
 * altrimenti a parole non allineate. Con zone sovrapposte il risultato dipende da
 * quest'ordine di copia, che è quello dell'originale. */
void *memcpy(void *dst0, const void *src0, size_t len)
{
    u8 *dst = dst0;
    const u8 *src = src0;

    if ((((u32)src | (u32)dst) & 3) == 0) {
        u32 *d = (u32 *)dst;
        const u32 *s = (const u32 *)src;
        u32 i, rest = len & 0xF;

        for (i = len >> 4; i != 0; i--) {
            u32 a = s[0], b = s[1], c = s[2], e = s[3];
            d[0] = a;
            d[1] = b;
            d[2] = c;
            d[3] = e;
            s += 4;
            d += 4;
        }
        for (i = rest >> 2; i != 0; i--)
            *d++ = *s++;
        dst = (u8 *)d;
        src = (const u8 *)s;
        len &= 3;
    } else {
        int n = (int)len;
        int words = n / 4;

        for (; words != 0; words--) {
            u32 w = ((const unaligned_u32 *)src)->v;
            ((unaligned_u32 *)dst)->v = w;
            src += 4;
            dst += 4;
        }
        len = n % 4;
    }
    while ((int)len > 0) {
        *dst++ = *src++;
        len--;
    }
    return dst0;
}

void *memmove(void *dst0, const void *src0, size_t len)
{
    u8 *dst = dst0;
    const u8 *src = src0;

    if (src < dst && dst < src + len) {
        /* zone sovrapposte con la destinazione dopo: copia all'indietro.
         * Stranezza dell'originale: in questo caso ritorna dst0 + len. */
        u8 *ret = dst + len;
        src += len;
        dst += len;
        while (len--)
            *--dst = *--src;
        return ret;
    } else {
        while (len--)
            *dst++ = *src++;
    }
    return dst0;
}

void *memset(void *m, int c, size_t n)
{
    u8 *s = m;
    u8 d = c;

    if (n >= 16) {
        u32 w;
        u32 i;

        while ((u32)s & 3) { /* allinea a 4 byte */
            *s++ = d;
            n--;
        }
        w = d;
        if (w != 0) {
            w |= w << 8;
            w |= w << 16;
        }
        for (i = n >> 3; i != 0; i--) {
            ((u32 *)s)[0] = w;
            ((u32 *)s)[1] = w;
            s += 8;
        }
        n &= 7;
        if (n >= 4) {
            *(u32 *)s = w;
            s += 4;
            n -= 4;
        }
    }
    while (n--)
        *s++ = d;
    return m;
}

/* --- stringhe C (terminate da 0) ------------------------------------------------- */

size_t strlen(const char *str)
{
    const char *s = str;

    while (*s)
        s++;
    return s - str;
}

char *strcpy(char *dst0, const char *src)
{
    char *dst = dst0;

    while ((*dst++ = *src++) != '\0')
        ;
    return dst0;
}

char *strcat(char *s1, const char *s2)
{
    strcpy(s1 + strlen(s1), s2);
    return s1;
}

/* aggiunge al più n caratteri di s2 in fondo a s1, poi lo 0 finale */
char *strncat(char *s1, const char *s2, size_t n)
{
    char *s = s1 + strlen(s1);

    while (n-- != 0 && (*s++ = *s2++) != '\0') {
        if (n == 0)
            *s = '\0';
    }
    return s1;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *(const u8 *)s1 - *(const u8 *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    if (n == 0)
        return 0;
    while (n-- != 0 && *s1 == *s2) {
        if (n == 0 || *s1 == '\0')
            return 0;
        s1++;
        s2++;
    }
    return *(const u8 *)s1 - *(const u8 *)s2;
}

/* copia al più n caratteri e riempie di zeri fino a n */
char *strncpy(char *dst0, const char *src, size_t count)
{
    char *dst = dst0;

    while (count > 0) {
        --count;
        if ((*dst++ = *src++) == '\0')
            break;
    }
    while (count-- > 0)
        *dst++ = '\0';
    return dst0;
}

/* primo c in s (anche lo 0 finale se c == 0), NULL se non c'è */
char *strchr(const char *s, int i)
{
    u8 c = i;

    while (*s && *(const u8 *)s != c)
        s++;
    if (*(const u8 *)s == c)
        return (char *)s;
    return NULL;
}

char *strstr(const char *searchee, const char *lookfor)
{
    if (*searchee == 0)
        return *lookfor ? NULL : (char *)searchee;
    while (*searchee) {
        size_t i = 0;

        for (;;) {
            if (lookfor[i] == 0)
                return (char *)searchee;
            if (lookfor[i] != searchee[i])
                break;
            i++;
        }
        searchee++;
    }
    return NULL;
}
