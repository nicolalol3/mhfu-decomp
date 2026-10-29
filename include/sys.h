/* Named offsets of the system object g_sys (used with U16/U32, so the compiled code is the
 * same as with the raw offsets). (Guessed names.) */
#ifndef SYS_H
#define SYS_H

/* fields of the system object g_sys (src/game/sys.c) */
enum {
    SYS_FRAME = 0xF7A554,       /* u32 frames of the main loop */
    SYS_EXIT_THREAD = 0xF7A564, /* u32 */
    SYS_EXIT_FLAG = 0xF7A568,   /* u32 */
    SYS_RAND = 0xF7A56C,        /* u16[3] random generators (seeds) */
    SYS_LANGUAGE = 0xF7A574,    /* u32 system language */
    SYS_TEXT_KIND = 0xF7A578,   /* u32 */
    SYS_OPTIONS = 0xF7A57C,     /* u32 bit 0: online (OPT_FLAGS) */
    SYS_OVERLAY = 0xF7A580,     /* u16[] overlay loaded in each slot */
    SYS_PRIO_THREAD = 0xF7F020, /* u32 */
    SYS_MAIN_THREAD = 0xF7F024, /* u32 */
    SYS_VTIMER = 0xF7F028,      /* u32 */
    SYS_VTIMER_ON = 0xF7F02C,   /* u32 */
    SYS_PRIO_SAVED = 0xF7F030,  /* u32 thread whose priority is restored */
};

#endif
