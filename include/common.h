/* Types and macros shared by all the decompiled code of MHFU. */
#ifndef COMMON_H
#define COMMON_H

/* u8..u64 and s8..s64 come from the PSPSDK headers */
#include <psptypes.h>

typedef float  f32;
typedef double f64;

#ifndef NULL
#define NULL ((void *)0)
#endif

/* Functions and data not yet decompiled are called func_XXXXXXXX / D_XXXXXXXX:
 * their original addresses are supplied automatically to the linker (tools/symbols.py). */

#endif
