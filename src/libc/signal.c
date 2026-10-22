/* C library: signals (raise). The PSP has no real signal system: if no handler
 * is set for a signal, it "sends" it to itself with kill, which does
 * nothing. */
#include "libc.h"

#define NSIG    32
#define SIG_DFL ((void (*)(int))0)
#define SIG_IGN ((void (*)(int))1)
#define SIG_ERR ((void (*)(int))-1)
#define EINVAL  22

int _getpid_r(struct _reent *r);
int _kill_r(struct _reent *r, int pid, int sig);

/* creates the handler table (all "default") */
int _init_signal_r(struct _reent *ptr)
{
    int i;

    if (ptr->_sig_func == NULL) {
        ptr->_sig_func = _malloc_r(ptr, sizeof(*ptr->_sig_func) * NSIG);
        if (ptr->_sig_func == NULL)
            return -1;
        for (i = 0; i < NSIG; i++)
            ptr->_sig_func[i] = SIG_DFL;
    }
    return 0;
}

int _raise_r(struct _reent *ptr, int sig)
{
    void (*func)(int);

    if ((u32)sig >= NSIG) {
        ptr->_errno = EINVAL;
        return -1;
    }
    if (ptr->_sig_func == NULL && _init_signal_r(ptr) != 0)
        return -1;

    func = ptr->_sig_func[sig];
    if (func == SIG_DFL)
        return _kill_r(ptr, _getpid_r(ptr), sig);
    if (func == SIG_IGN)
        return 0;
    if (func == SIG_ERR) {
        ptr->_errno = EINVAL;
        return 1;
    }
    ptr->_sig_func[sig] = SIG_DFL; /* the handler is only used once */
    func(sig);
    return 0;
}

int raise(int sig)
{
    return _raise_r(_REENT, sig);
}

int _kill_r(struct _reent *r, int pid, int sig)
{
    int ret;

    g_errno = 0;
    if ((ret = kill(pid, sig)) == -1 && g_errno != 0)
        r->_errno = g_errno;
    return ret;
}

int _getpid_r(struct _reent *r)
{
    return getpid();
}
