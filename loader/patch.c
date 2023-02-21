/*
 * patch.c
 *
 * Patching some of the .so internal functions or bridging them to native for
 * better compatibility.
 *
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include <so_util.h>
#include <psp2/kernel/clib.h>
#include "patch.h"
#include "main.h"
#include "utils/logger.h"
#include <kubridge.h>
#include <stdlib.h>
#include <malloc.h>
#include <psp2/kernel/threadmgr.h>
#include <string.h>
#include <psp2/kernel/processmgr.h>
#include <math.h>
#include "reimpl/io.h"
#include "utils/glutil.h"
#include "AFakeNative/keycodes.h"

int _ZN11AbyssEngine6Engine10ShaderInitEv(int x) {
    sceClibPrintf("called _ZN11AbyssEngine6Engine10ShaderInitEv. retaddr: 0x%x\n", __builtin_return_address(0));
    return 0;
}
int _ZN11AbyssEngine14ES2LoadProgramEPKcS1_(char * a, char * b) {
    sceClibPrintf("called _ZN11AbyssEngine14ES2LoadProgramEPKcS1_. retaddr: 0x%x\n", __builtin_return_address(0));
    return 0;
}

so_hook setGameOrientation_hook;

void _ZN11AbyssEngine11PaintCanvas18SetGameOrientationENS_13LandscapeModeE(void * this, int mode) {
    //sceClibPrintf("called SetGameOrientation with mode %i. forcing 2 instead. retaddr: 0x%x\n", mode, __builtin_return_address(0));
    SO_CONTINUE(void *, setGameOrientation_hook, this, 2);
}

int _ZN11AbyssEngine6Engine16GetDisplayHeightEv(void * this) {
    return 544;
}

int _ZN11AbyssEngine6Engine15GetDisplayWidthEv(void * this) {
    return 960;
}

#include "patch/controls_fix.c"



int * g_inputAllowedFlag;
int * g_displayWidth;

void quitGame(int unused) {
    sceKernelExitProcess(0);
}

so_hook NewsTicker_Constructor_hook;
void NewsTicker_Constructor(void* this, int posX, int posY, int width, int param4, int param5) {
    // In some parts of the game, it still thinks of portrait orientation as primary,
    // thus we need to invert to multiply horizontal coordinates by the aspect ratio here
    int posX_real = floorf((float)posX * (960.f / 544.f));
    int width_real = floorf((float)width * (960.f / 544.f));
    sceClibPrintf("passing %i %i instead of %i %i\n", posX_real, width_real, posX, width);
    SO_CONTINUE(void *, NewsTicker_Constructor_hook, this, posX_real, posY, width_real, param4, param5 );
}

void so_patch(void) {
    g_inputAllowedFlag = (int *)(so_mod.text_base + 0x0013e29c);
    g_displayWidth = (int *)(so_mod.text_base + 0x0014105c);
    //android_app_write_cmd = (void *)(so_mod.text_base + 0x001c3494 + 1);
    //Rb_tree__find = (void *)so_symbol(&so_mod, "_ZNSt8_Rb_treeISsSt4pairIKSsjESt10_Select1stIS2_ESt4lessISsESaIS2_EE4findERS1_");

    //hook_addr(so_symbol(&so_mod, "_ZN11AbyssEngine6Engine10ShaderInitEv"), (uintptr_t)_ZN11AbyssEngine6Engine10ShaderInitEv);
    //hook_addr(so_symbol(&so_mod, "_ZN11AbyssEngine14ES2LoadProgramEPKcS1_"), (uintptr_t)_ZN11AbyssEngine14ES2LoadProgramEPKcS1_);
    hook_addr(so_symbol(&so_mod, "_ZN11AbyssEngine6Engine16GetDisplayHeightEv"), (uintptr_t)_ZN11AbyssEngine6Engine16GetDisplayHeightEv);
    hook_addr(so_symbol(&so_mod, "_ZN11AbyssEngine6Engine15GetDisplayWidthEv"), (uintptr_t)_ZN11AbyssEngine6Engine15GetDisplayWidthEv);

    hook_addr(so_symbol(&so_mod, "ndk23_ndkDone"), (uintptr_t)quitGame);

    setGameOrientation_hook = hook_addr(so_symbol(&so_mod, "_ZN11AbyssEngine11PaintCanvas18SetGameOrientationENS_13LandscapeModeE"), (uintptr_t)_ZN11AbyssEngine11PaintCanvas18SetGameOrientationENS_13LandscapeModeE);
    NewsTicker_Constructor_hook = hook_addr(so_symbol(&so_mod, "_ZN10NewsTickerC1Eiiiii"), (uintptr_t) NewsTicker_Constructor);

    patch__controls_fix();
    //uint16_t nop = 0xbf00;
    //kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x00142968), &nop, sizeof(nop));
}
