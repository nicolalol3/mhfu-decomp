/* powf: wrapper around __ieee754_powf that reports errors according to _LIB_VERSION
 * (calling matherr and setting errno), as in the newlib used by the game. */
#include "libm.h"

#define EDOM 33
#define ERANGE 34

/* _LIB_VERSION values */
#define _IEEE_  -1
#define _SVID_  0
#define _XOPEN_ 1
#define _POSIX_ 2

/* error types for struct exception */
#define DOMAIN    1
#define OVERFLOW  3
#define UNDERFLOW 4

struct exception {
    int type;
    char *name;
    f64 arg1;
    f64 arg2;
    f64 retval;
    int err;
};

extern int _LIB_VERSION; /* in the game it is _XOPEN_ */
int *__errno(void);
int matherr(struct exception *exc);
int finitef(f32 x);
int isnanf(f32 x);
f64 rint(f64 x);

#define HUGE_VAL __builtin_huge_val()
#define HUGE     3.40282346638528859812e+38 /* massimo float */

f32 powf(f32 x, f32 y)
{
    f32 z;
    struct exception exc;

    z = __ieee754_powf(x, y);
    if (_LIB_VERSION == _IEEE_ || isnanf(y))
        return z;
    if (isnanf(x)) {
        if (y == (f32)0.0) {
            /* powf(NaN, 0.0) */
            exc.type = DOMAIN;
            exc.name = "powf";
            exc.err = 0;
            exc.arg1 = (f64)x;
            exc.arg2 = (f64)y;
            exc.retval = x;
            if (_LIB_VERSION == _IEEE_ || _LIB_VERSION == _POSIX_)
                exc.retval = 1.0;
            else if (!matherr(&exc))
                *__errno() = EDOM;
            if (exc.err != 0)
                *__errno() = exc.err;
            return (f32)exc.retval;
        }
        return z;
    }
    if (x == (f32)0.0) {
        if (y == (f32)0.0) {
            /* powf(0.0, 0.0) */
            exc.type = DOMAIN;
            exc.name = "powf";
            exc.err = 0;
            exc.arg1 = (f64)x;
            exc.arg2 = (f64)y;
            exc.retval = 0.0;
            if (_LIB_VERSION != _SVID_)
                exc.retval = 1.0;
            else if (!matherr(&exc))
                *__errno() = EDOM;
            if (exc.err != 0)
                *__errno() = exc.err;
            return (f32)exc.retval;
        }
        if (finitef(y) && y < 0.0) {
            /* 0 raised to a negative power */
            exc.type = DOMAIN;
            exc.name = "powf";
            exc.err = 0;
            exc.arg1 = (f64)x;
            exc.arg2 = (f64)y;
            if (_LIB_VERSION == _SVID_)
                exc.retval = 0.0;
            else
                exc.retval = -HUGE_VAL;
            if (_LIB_VERSION == _POSIX_)
                *__errno() = EDOM;
            else if (!matherr(&exc))
                *__errno() = EDOM;
            if (exc.err != 0)
                *__errno() = exc.err;
            return (f32)exc.retval;
        }
        return z;
    }
    if (!finitef(z)) {
        if (finitef(x) && finitef(y)) {
            if (isnanf(z)) {
                /* negative number raised to a non-integer power */
                exc.type = DOMAIN;
                exc.name = "powf";
                exc.err = 0;
                exc.arg1 = (f64)x;
                exc.arg2 = (f64)y;
                if (_LIB_VERSION == _SVID_)
                    exc.retval = 0.0;
                else
                    exc.retval = 0.0 / 0.0; /* X/Open ammette NaN */
                if (_LIB_VERSION == _POSIX_)
                    *__errno() = EDOM;
                else if (!matherr(&exc))
                    *__errno() = EDOM;
                if (exc.err != 0)
                    *__errno() = exc.err;
                return (f32)exc.retval;
            } else {
                /* overflow */
                exc.type = OVERFLOW;
                exc.name = "powf";
                exc.err = 0;
                exc.arg1 = (f64)x;
                exc.arg2 = (f64)y;
                if (_LIB_VERSION == _SVID_) {
                    exc.retval = HUGE;
                    y *= 0.5;
                    if (x < 0.0 && rint(y) != y)
                        exc.retval = -HUGE;
                } else {
                    exc.retval = HUGE_VAL;
                    y *= 0.5;
                    if (x < 0.0 && rint(y) != y)
                        exc.retval = -HUGE_VAL;
                }
                if (_LIB_VERSION == _POSIX_)
                    *__errno() = ERANGE;
                else if (!matherr(&exc))
                    *__errno() = ERANGE;
                if (exc.err != 0)
                    *__errno() = exc.err;
                return (f32)exc.retval;
            }
        }
    }
    if (z == (f32)0.0 && finitef(x) && finitef(y)) {
        /* underflow */
        exc.type = UNDERFLOW;
        exc.name = "powf";
        exc.err = 0;
        exc.arg1 = (f64)x;
        exc.arg2 = (f64)y;
        exc.retval = 0.0;
        if (_LIB_VERSION == _POSIX_)
            *__errno() = ERANGE;
        else if (!matherr(&exc))
            *__errno() = ERANGE;
        if (exc.err != 0)
            *__errno() = exc.err;
        return (f32)exc.retval;
    }
    return z;
}
