/*
 * patch.c
 *
 * Patching some of the .so internal functions or bridging them to native for
 * better compatibility.
 *
 * Copyright (C) 2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "patch.h"

#include <kubridge.h>
#include <so_util/so_util.h>

extern so_module so_mod;

#include "patch/controls_fix.c"
#include "patch/quit_fix.c"
#include "patch/screen_fix.c"
#include "patch/apk_hook.c"

void so_patch(void) {
    patch__screen_fix();
    patch__quit_fix();
    patch__controls_fix();
    patch__apk_hook();
}
