/* System functions used by the game but missing from the PSPSDK headers. */
#ifndef SCE_EXTRA_H
#define SCE_EXTRA_H
#include <psptypes.h>

SceUID sceKernelGetModuleId(void);
int sceKernelStopUnloadSelfModuleWithStatus(int exitcode, SceSize argsize, void *argp, int *status, void *option);
int sceKernelSetCompiledSdkVersion500_505(int version);
int sceKernelSetCompilerVersion(int version);
void sceKernelPrintf(const char *format, ...);

#endif
