#include <psp2/kernel/processmgr.h>

void quitGame(int unused) {
    sceKernelExitProcess(0);
}

void patch__quit_fix() {
    hook_addr(so_symbol(&so_mod, "ndk23_ndkDone"), (uintptr_t)quitGame);
}
