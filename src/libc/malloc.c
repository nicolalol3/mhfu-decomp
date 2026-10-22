/* Dynamic memory of the libc: Doug Lea's malloc (version 2.6, included in newlib),
 * with blocks aligned to 16 bytes. Each block ("chunk") has its size in front of it;
 * free blocks are kept in 128 lists ("bins") sorted by size. */
#include "libc.h"
#include <pspintrman.h>

typedef struct malloc_chunk {
    u32 prev_size;              /* size of the previous block (if free) */
    u32 size;                   /* size; bit 0 = previous block in use */
    struct malloc_chunk *fd;    /* free list: next */
    struct malloc_chunk *bk;    /* free list: previous */
} *mchunkptr;

#define SIZE_SZ          4
#define MALLOC_ALIGNMENT 16
#define MALLOC_ALIGN_MASK (MALLOC_ALIGNMENT - 1)
#define MINSIZE          16
#define PAGESZ           4096
#define PREV_INUSE       1

#define chunk2mem(p)  ((void *)((u8 *)(p) + 2 * SIZE_SZ))
#define mem2chunk(m)  ((mchunkptr)((u8 *)(m) - 2 * SIZE_SZ))
#define chunk_at_offset(p, s) ((mchunkptr)((u8 *)(p) + (s)))
#define chunksize(p)  ((p)->size & ~(PREV_INUSE | 2))
#define next_chunk(p) chunk_at_offset(p, (p)->size & ~PREV_INUSE)
#define prev_chunk(p) chunk_at_offset(p, -(p)->prev_size)
#define inuse_bit_at_offset(p, s) (chunk_at_offset(p, s)->size & PREV_INUSE)
#define set_inuse_bit_at_offset(p, s) (chunk_at_offset(p, s)->size |= PREV_INUSE)
#define set_head(p, s)      ((p)->size = (s))
#define set_head_size(p, s) ((p)->size = ((p)->size & PREV_INUSE) | (s))
#define set_foot(p, s)      (chunk_at_offset(p, s)->prev_size = (s))

/* request2size: requested size -> block size */
#define request2size(req) \
    ((req) + (SIZE_SZ + MALLOC_ALIGN_MASK) < (MINSIZE + MALLOC_ALIGN_MASK) ? MINSIZE \
     : ((req) + (SIZE_SZ + MALLOC_ALIGN_MASK)) & ~MALLOC_ALIGN_MASK)

/* --- global state ---------------------------------------------------------- */
/* av_[0..1]: av_[1] = "binblocks" (one bit every 4 non-empty lists);
 * then for each list the two pointers fd/bk. List 0 contains "top" (the block at
 * the top of the heap), list 1 the last remainder obtained by splitting a block. */
extern mchunkptr __malloc_av_[];
extern u8 *__malloc_sbrk_base;          /* start of the heap, -1 until used */
extern u32 __malloc_trim_threshold;     /* free memory beyond this top is returned */
extern u32 __malloc_top_pad;            /* margine extra chiesto a sbrk */
extern u32 __malloc_current_mallinfo;   /* first field: memory obtained from sbrk */
extern u32 __malloc_max_sbrked_mem;
extern u32 __malloc_max_total_mem;
extern int g_malloc_lock_count;
extern int g_malloc_lock_intr;

#define av_             __malloc_av_
#define bin_at(i)       ((mchunkptr)((u8 *)&av_[2 * (i) + 2] - 2 * SIZE_SZ))
#define next_bin(b)     ((mchunkptr)((u8 *)(b) + 2 * sizeof(mchunkptr)))
#define prev_bin(b)     ((mchunkptr)((u8 *)(b) - 2 * sizeof(mchunkptr)))
#define first(b)        ((b)->fd)
#define last(b)         ((b)->bk)
#define top             (av_[2])
#define last_remainder  (bin_at(1))
#define initial_top     ((mchunkptr)bin_at(0))
#define binblocks       (*(u32 *)&av_[1])
#define sbrked_mem      __malloc_current_mallinfo

#define NAV             128
#define MAX_SMALLBIN    63
#define MAX_SMALLBIN_SIZE 512
#define SMALLBIN_WIDTH  8
#define BINBLOCKWIDTH   4
#define is_small_request(nb) ((nb) < MAX_SMALLBIN_SIZE - SMALLBIN_WIDTH)
#define smallbin_index(sz)   ((int)((sz) >> 3))
#define idx2binblock(ix)     ((u32)1 << ((ix) / BINBLOCKWIDTH))
#define mark_binblock(ii)    (binblocks |= idx2binblock(ii))

static inline int bin_index(u32 sz)
{
    u32 s = sz >> 9;
    if (s == 0)     return sz >> 3;
    if (s <= 4)     return 56 + (sz >> 6);
    if (s <= 20)    return 91 + (sz >> 9);
    if (s <= 84)    return 110 + (sz >> 12);
    if (s <= 340)   return 119 + (sz >> 15);
    if (s <= 1364)  return 124 + (sz >> 18);
    return 126;
}

/* removes P from its list */
#define unlink(P, BK, FD) { BK = (P)->bk; FD = (P)->fd; FD->bk = BK; BK->fd = FD; }

/* inserts P (size S) into its list; the large lists are sorted */
#define frontlink(P, S, IDX, BK, FD)                                   \
    {                                                                  \
        if ((S) < MAX_SMALLBIN_SIZE) {                                 \
            IDX = smallbin_index(S);                                   \
            mark_binblock(IDX);                                        \
            BK = bin_at(IDX);                                          \
            FD = BK->fd;                                               \
            (P)->bk = BK;                                              \
            (P)->fd = FD;                                              \
            FD->bk = BK->fd = (P);                                     \
        } else {                                                       \
            IDX = bin_index(S);                                        \
            BK = bin_at(IDX);                                          \
            FD = BK->fd;                                               \
            if (FD == BK) {                                            \
                mark_binblock(IDX);                                    \
            } else {                                                   \
                while (FD != BK && (S) < chunksize(FD))                \
                    FD = FD->fd;                                       \
                BK = FD->bk;                                           \
            }                                                          \
            (P)->bk = BK;                                              \
            (P)->fd = FD;                                              \
            FD->bk = BK->fd = (P);                                     \
        }                                                              \
    }

#define link_last_remainder(P) \
    { last_remainder->fd = last_remainder->bk = (P); (P)->fd = (P)->bk = last_remainder; }
#define clear_last_remainder \
    (last_remainder->fd = last_remainder->bk = last_remainder)

/* copy/zeroing of small blocks done by hand, the others with memcpy/memset */
#define MALLOC_ZERO(charp, nbytes)                                     \
    do {                                                               \
        u32 mzsz = (nbytes);                                           \
        if (mzsz <= 9 * sizeof(mzsz)) {                                \
            u32 *mz = (u32 *)(charp);                                  \
            if (mzsz >= 5 * sizeof(mzsz)) {                            \
                *mz++ = 0;                                             \
                *mz++ = 0;                                             \
                if (mzsz >= 7 * sizeof(mzsz)) {                        \
                    *mz++ = 0;                                         \
                    *mz++ = 0;                                         \
                    if (mzsz >= 9 * sizeof(mzsz)) {                    \
                        *mz++ = 0;                                     \
                        *mz++ = 0;                                     \
                    }                                                  \
                }                                                      \
            }                                                          \
            *mz++ = 0;                                                 \
            *mz++ = 0;                                                 \
            *mz = 0;                                                   \
        } else                                                         \
            memset((charp), 0, mzsz);                                  \
    } while (0)

#define MALLOC_COPY(dest, src, nbytes)                                 \
    do {                                                               \
        u32 mcsz = (nbytes);                                           \
        if (mcsz <= 9 * sizeof(mcsz)) {                                \
            u32 *mcsrc = (u32 *)(src);                                 \
            u32 *mcdst = (u32 *)(dest);                                \
            if (mcsz >= 5 * sizeof(mcsz)) {                            \
                *mcdst++ = *mcsrc++;                                   \
                *mcdst++ = *mcsrc++;                                   \
                if (mcsz >= 7 * sizeof(mcsz)) {                        \
                    *mcdst++ = *mcsrc++;                               \
                    *mcdst++ = *mcsrc++;                               \
                    if (mcsz >= 9 * sizeof(mcsz)) {                    \
                        *mcdst++ = *mcsrc++;                           \
                        *mcdst++ = *mcsrc++;                           \
                    }                                                  \
                }                                                      \
            }                                                          \
            *mcdst++ = *mcsrc++;                                       \
            *mcdst++ = *mcsrc++;                                       \
            *mcdst = *mcsrc;                                           \
        } else                                                         \
            memcpy(dest, src, mcsz);                                   \
    } while (0)

/* --- lock ----------------------------------------------------------- */
/* The "lock" disables interrupts; it can be taken several times in a row. */
void __malloc_lock(struct _reent *r)
{
    int intr = sceKernelCpuSuspendIntr();

    if (g_malloc_lock_count == 0)
        g_malloc_lock_intr = intr;
    g_malloc_lock_count++;
}

void __malloc_unlock(struct _reent *r)
{
    if (--g_malloc_lock_count == 0)
        sceKernelCpuResumeIntr(g_malloc_lock_intr);
}

/* --- malloc -------------------------------------------------------------------- */
/* Grows the top of the heap by asking sbrk for memory (included in malloc in the original). */
static inline void malloc_extend_top(struct _reent *r, u32 nb)
{
    u8 *brk, *new_brk;
    u32 front_misalign, correction, top_size;
    mchunkptr old_top = top;
    u32 old_top_size = chunksize(old_top);
    u8 *old_end = (u8 *)chunk_at_offset(old_top, old_top_size);
    u32 sbrk_size = nb + __malloc_top_pad + MINSIZE;

    if (__malloc_sbrk_base != (u8 *)-1)
        sbrk_size = (sbrk_size + (PAGESZ - 1)) & ~(PAGESZ - 1);

    brk = _sbrk_r(r, sbrk_size);
    if (brk == (u8 *)-1 || (brk < old_end && old_top != initial_top))
        return;

    sbrked_mem += sbrk_size;

    if (brk == old_end && ((u32)old_end & (PAGESZ - 1)) == 0) {
        top_size = sbrk_size + old_top_size;
        set_head(top, top_size | PREV_INUSE);
    } else {
        if (__malloc_sbrk_base == (u8 *)-1)
            __malloc_sbrk_base = brk;
        else
            sbrked_mem += brk - old_end; /* someone else called sbrk */

        front_misalign = (u32)chunk2mem(brk) & MALLOC_ALIGN_MASK;
        if (front_misalign > 0) {
            correction = MALLOC_ALIGNMENT - front_misalign;
            brk += correction;
        } else {
            correction = 0;
        }
        /* the next sbrk must start from a page boundary */
        correction += PAGESZ - ((u32)(brk + sbrk_size) & (PAGESZ - 1));

        new_brk = _sbrk_r(r, correction);
        if (new_brk == (u8 *)-1) {
            correction = 0;
            new_brk = brk;
        }
        sbrked_mem += correction;

        top = (mchunkptr)brk;
        top_size = new_brk - brk + correction;
        set_head(top, top_size | PREV_INUSE);

        if (old_top != initial_top) {
            /* the old top is not contiguous: we close it with two fake blocks */
            if (old_top_size < MINSIZE) {
                set_head(top, PREV_INUSE); /* this makes malloc fail */
                return;
            }
            old_top_size = (old_top_size - 3 * SIZE_SZ) & ~MALLOC_ALIGN_MASK;
            set_head_size(old_top, old_top_size);
            chunk_at_offset(old_top, old_top_size)->size = SIZE_SZ | PREV_INUSE;
            chunk_at_offset(old_top, old_top_size + SIZE_SZ)->size = SIZE_SZ | PREV_INUSE;
            if (old_top_size >= MINSIZE)
                _free_r(r, chunk2mem(old_top));
        }
    }

    if (sbrked_mem > __malloc_max_sbrked_mem)
        __malloc_max_sbrked_mem = sbrked_mem;
    if (sbrked_mem > __malloc_max_total_mem)
        __malloc_max_total_mem = sbrked_mem;
}

void *_malloc_r(struct _reent *r, size_t bytes)
{
    mchunkptr victim, remainder, bin, q, bck, fwd;
    u32 victim_size, block;
    int idx, startidx, remainder_size;
    u32 nb = request2size(bytes);

    if ((int)nb < 0 || nb < bytes)
        return NULL;

    __malloc_lock(r);

    if (is_small_request(nb)) {
        /* small lists: all blocks in the list have the same size */
        idx = smallbin_index(nb);
        q = bin_at(idx);
        victim = last(q);
        if (victim != q) {
            victim_size = chunksize(victim);
            unlink(victim, bck, fwd);
            set_inuse_bit_at_offset(victim, victim_size);
            __malloc_unlock(r);
            return chunk2mem(victim);
        }
        idx += 2; /* for the search below */
    } else {
        idx = bin_index(nb);
        bin = bin_at(idx);
        for (victim = last(bin); victim != bin; victim = victim->bk) {
            victim_size = chunksize(victim);
            remainder_size = victim_size - nb;
            if (remainder_size >= MINSIZE) { /* too big */
                --idx;
                break;
            } else if (remainder_size >= 0) { /* esatto */
                unlink(victim, bck, fwd);
                set_inuse_bit_at_offset(victim, victim_size);
                __malloc_unlock(r);
                return chunk2mem(victim);
            }
        }
        ++idx;
    }

    /* try with the last remainder */
    if ((victim = last_remainder->fd) != last_remainder) {
        victim_size = chunksize(victim);
        remainder_size = victim_size - nb;
        if (remainder_size >= MINSIZE) { /* split it again */
            remainder = chunk_at_offset(victim, nb);
            set_head(victim, nb | PREV_INUSE);
            link_last_remainder(remainder);
            set_head(remainder, remainder_size | PREV_INUSE);
            set_foot(remainder, remainder_size);
            __malloc_unlock(r);
            return chunk2mem(victim);
        }
        clear_last_remainder;
        if (remainder_size >= 0) { /* use all of it */
            set_inuse_bit_at_offset(victim, victim_size);
            __malloc_unlock(r);
            return chunk2mem(victim);
        }
        frontlink(victim, victim_size, remainder_size, bck, fwd);
    }

    /* search for the best-fitting block in the larger lists, 4 lists at a time */
    if ((block = idx2binblock(idx)) <= binblocks) {
        if ((block & binblocks) == 0) {
            idx = (idx & ~(BINBLOCKWIDTH - 1)) + BINBLOCKWIDTH;
            block <<= 1;
            while ((block & binblocks) == 0) {
                idx += BINBLOCKWIDTH;
                block <<= 1;
            }
        }
        for (;;) {
            startidx = idx;
            q = bin = bin_at(idx);
            do {
                for (victim = last(bin); victim != bin; victim = victim->bk) {
                    victim_size = chunksize(victim);
                    remainder_size = victim_size - nb;
                    if (remainder_size >= MINSIZE) { /* spezza */
                        remainder = chunk_at_offset(victim, nb);
                        set_head(victim, nb | PREV_INUSE);
                        unlink(victim, bck, fwd);
                        link_last_remainder(remainder);
                        set_head(remainder, remainder_size | PREV_INUSE);
                        set_foot(remainder, remainder_size);
                        __malloc_unlock(r);
                        return chunk2mem(victim);
                    } else if (remainder_size >= 0) { /* prende */
                        set_inuse_bit_at_offset(victim, victim_size);
                        unlink(victim, bck, fwd);
                        __malloc_unlock(r);
                        return chunk2mem(victim);
                    }
                }
                bin = next_bin(bin);
                /* with 16-byte alignment the odd small lists are always empty */
                if (idx < MAX_SMALLBIN) {
                    bin = next_bin(bin);
                    ++idx;
                }
            } while ((++idx & (BINBLOCKWIDTH - 1)) != 0);

            /* clears the group bit if all its lists are empty */
            do {
                if ((startidx & (BINBLOCKWIDTH - 1)) == 0) {
                    binblocks &= ~block;
                    break;
                }
                --startidx;
                q = prev_bin(q);
            } while (first(q) == q);

            if ((block <<= 1) <= binblocks && block != 0) {
                while ((block & binblocks) == 0) {
                    idx += BINBLOCKWIDTH;
                    block <<= 1;
                }
            } else {
                break;
            }
        }
    }

    /* use the top of the heap, growing it if needed */
    victim = top;
    remainder_size = chunksize(victim) - nb;
    if (chunksize(victim) < nb || remainder_size < MINSIZE) {
        malloc_extend_top(r, nb);
        victim = top;
        remainder_size = chunksize(victim) - nb;
        if (chunksize(victim) < nb || remainder_size < MINSIZE) {
            __malloc_unlock(r);
            return NULL;
        }
    }
    set_head(victim, nb | PREV_INUSE);
    top = chunk_at_offset(victim, nb);
    set_head(top, remainder_size | PREV_INUSE);
    __malloc_unlock(r);
    return chunk2mem(victim);
}

/* --- free ---------------------------------------------------------------------- */
void _free_r(struct _reent *r, void *mem)
{
    mchunkptr p, next, bck, fwd;
    u32 hd, sz, nextsz, prevsz;
    int idx, islr;

    if (mem == NULL)
        return;

    __malloc_lock(r);

    p = mem2chunk(mem);
    hd = p->size;
    sz = hd & ~PREV_INUSE;
    next = chunk_at_offset(p, sz);
    nextsz = chunksize(next);

    if (next == top) { /* merges with the top */
        sz += nextsz;
        if (!(hd & PREV_INUSE)) {
            prevsz = p->prev_size;
            p = chunk_at_offset(p, -prevsz);
            sz += prevsz;
            unlink(p, bck, fwd);
        }
        set_head(p, sz | PREV_INUSE);
        top = p;
        if (sz >= __malloc_trim_threshold)
            _malloc_trim_r(r, __malloc_top_pad);
        __malloc_unlock(r);
        return;
    }

    set_head(next, nextsz); /* the block after no longer has the previous one in use */

    islr = 0;
    if (!(hd & PREV_INUSE)) { /* merges with the block before */
        prevsz = p->prev_size;
        p = chunk_at_offset(p, -prevsz);
        sz += prevsz;
        if (p->fd == last_remainder)
            islr = 1;
        else
            unlink(p, bck, fwd);
    }

    if (!inuse_bit_at_offset(next, nextsz)) { /* merges with the block after */
        sz += nextsz;
        if (!islr && next->fd == last_remainder) {
            islr = 1;
            link_last_remainder(p);
        } else {
            unlink(next, bck, fwd);
        }
    }

    set_head(p, sz | PREV_INUSE);
    set_foot(p, sz);
    if (!islr)
        frontlink(p, sz, idx, bck, fwd);

    __malloc_unlock(r);
}

/* Returns to the system the free memory at the top of the heap beyond "pad" bytes. */
int _malloc_trim_r(struct _reent *r, size_t pad)
{
    int top_size, extra;
    u8 *current_brk, *new_brk;

    __malloc_lock(r);

    top_size = chunksize(top);
    extra = ((top_size - pad - MINSIZE + (PAGESZ - 1)) / PAGESZ - 1) * PAGESZ;
    if (extra < PAGESZ) {
        __malloc_unlock(r);
        return 0;
    }

    current_brk = _sbrk_r(r, 0);
    if (current_brk != (u8 *)top + top_size) { /* someone else used sbrk */
        __malloc_unlock(r);
        return 0;
    }

    new_brk = _sbrk_r(r, -extra);
    if (new_brk == (u8 *)-1) {
        current_brk = _sbrk_r(r, 0);
        top_size = current_brk - (u8 *)top;
        if (top_size >= MINSIZE) {
            sbrked_mem = current_brk - __malloc_sbrk_base;
            set_head(top, top_size | PREV_INUSE);
        }
        __malloc_unlock(r);
        return 0;
    }

    set_head(top, (top_size - extra) | PREV_INUSE);
    sbrked_mem -= extra;
    __malloc_unlock(r);
    return 1;
}

/* --- realloc ------------------------------------------------------------------- */
void *_realloc_r(struct _reent *r, void *oldmem, size_t bytes)
{
    u32 nb, oldsize, newsize, nextsize, prevsize;
    int remainder_size;
    mchunkptr oldp, newp, next, prev, remainder, bck, fwd;
    void *newmem;

    if (oldmem == NULL)
        return _malloc_r(r, bytes);

    __malloc_lock(r);

    newp = oldp = mem2chunk(oldmem);
    newsize = oldsize = chunksize(oldp);

    nb = request2size(bytes);
    if ((int)nb < 0 || nb < bytes)
        return NULL; /* (the original does not release the lock) */

    if ((int)oldsize < (int)nb) {
        /* prova ad allargarsi in avanti */
        next = chunk_at_offset(oldp, oldsize);
        if (next == top || !inuse_bit_at_offset(next, next->size & ~PREV_INUSE)) {
            nextsize = chunksize(next);
            if (next == top) {
                if ((int)(nextsize + newsize) >= (int)(nb + MINSIZE)) {
                    newsize += nextsize;
                    top = chunk_at_offset(oldp, nb);
                    set_head(top, (newsize - nb) | PREV_INUSE);
                    set_head_size(oldp, nb);
                    __malloc_unlock(r);
                    return chunk2mem(oldp);
                }
            } else if ((int)(nextsize + newsize) >= (int)nb) {
                unlink(next, bck, fwd);
                newsize += nextsize;
                goto split;
            }
        } else {
            next = NULL;
            nextsize = 0;
        }

        /* try moving backwards */
        if (!(oldp->size & PREV_INUSE)) {
            prev = prev_chunk(oldp);
            prevsize = chunksize(prev);

            if (next != NULL) {
                if (next == top) {
                    if ((int)(nextsize + prevsize + newsize) >= (int)(nb + MINSIZE)) {
                        unlink(prev, bck, fwd);
                        newp = prev;
                        newsize += prevsize + nextsize;
                        newmem = chunk2mem(newp);
                        MALLOC_COPY(newmem, oldmem, oldsize - SIZE_SZ);
                        top = chunk_at_offset(newp, nb);
                        set_head(top, (newsize - nb) | PREV_INUSE);
                        set_head_size(newp, nb);
                        __malloc_unlock(r);
                        return newmem;
                    }
                } else if ((int)(nextsize + prevsize + newsize) >= (int)nb) {
                    unlink(next, bck, fwd);
                    unlink(prev, bck, fwd);
                    newp = prev;
                    newsize += nextsize + prevsize;
                    newmem = chunk2mem(newp);
                    MALLOC_COPY(newmem, oldmem, oldsize - SIZE_SZ);
                    goto split;
                }
            }

            if ((int)(prevsize + newsize) >= (int)nb) {
                unlink(prev, bck, fwd);
                newp = prev;
                newsize += prevsize;
                newmem = chunk2mem(newp);
                MALLOC_COPY(newmem, oldmem, oldsize - SIZE_SZ);
                goto split;
            }
        }

        /* must allocate a new block */
        newmem = _malloc_r(r, bytes);
        if (newmem == NULL) {
            __malloc_unlock(r);
            return NULL;
        }
        /* the new block is exactly the one right after: no copy */
        if ((newp = mem2chunk(newmem)) == next_chunk(oldp)) {
            newsize += chunksize(newp);
            newp = oldp;
            goto split;
        }
        MALLOC_COPY(newmem, oldmem, oldsize - SIZE_SZ);
        _free_r(r, oldmem);
        __malloc_unlock(r);
        return newmem;
    }

split:
    /* detaches and frees the extra space */
    remainder_size = newsize - nb;
    if ((u32)remainder_size >= MINSIZE) {
        remainder = chunk_at_offset(newp, nb);
        set_head_size(newp, nb);
        set_head(remainder, remainder_size | PREV_INUSE);
        set_inuse_bit_at_offset(remainder, remainder_size);
        _free_r(r, chunk2mem(remainder));
    } else {
        set_head_size(newp, newsize);
        set_inuse_bit_at_offset(newp, newsize);
    }
    __malloc_unlock(r);
    return chunk2mem(newp);
}

/* --- calloc and free ------------------------------------------------------------- */
void *_calloc_r(struct _reent *r, size_t n, size_t elem_size)
{
    void *mem = _malloc_r(r, n * elem_size);

    if (mem == NULL)
        return NULL;
    MALLOC_ZERO(mem, chunksize(mem2chunk(mem)) - SIZE_SZ);
    return mem;
}

void free(void *p)
{
    _free_r(_REENT, p);
}
