/* Tipi e macro comuni a tutto il codice decompilato di MHFU. */
#ifndef COMMON_H
#define COMMON_H

/* u8..u64 e s8..s64 vengono dagli header del PSPSDK */
#include <psptypes.h>

typedef float  f32;
typedef double f64;

#ifndef NULL
#define NULL ((void *)0)
#endif

/* Funzioni e dati non ancora decompilati si chiamano func_XXXXXXXX / D_XXXXXXXX:
 * gli indirizzi originali vengono forniti automaticamente al linker (tools/symbols.py). */

#endif
