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
#include "reimpl/io.h"
#include "utils/glutil.h"

void so_patch(void) {
    //android_app_write_cmd = (void *)(so_mod.text_base + 0x001c3494 + 1);
    //Rb_tree__find = (void *)so_symbol(&so_mod, "_ZNSt8_Rb_treeISsSt4pairIKSsjESt10_Select1stIS2_ESt4lessISsESaIS2_EE4findERS1_");

    //uint16_t nop = 0xbf00;
    //kuKernelCpuUnrestrictedMemcpy((void*)(so_mod.text_base + 0x00142968), &nop, sizeof(nop));
}
