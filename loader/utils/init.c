/*
 * utils/init.c
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021-2022 Rinnegatamante
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "utils/init.h"

#include "utils/dialog.h"
#include "utils/glutil.h"
#include "utils/logger.h"
#include "utils/utils.h"

#include "dynlib.h"
#include "patch.h"

#include <malloc.h>
#include <string.h>

#include <psp2/appmgr.h>
#include <psp2/apputil.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/clib.h>
#include <psp2/power.h>
#include <psp2/touch.h>

#include <FalsoJNI/FalsoJNI.h>
#include <so_util/so_util.h>
#include <unzip/unzip.h>

// Base address for the Android .so to be loaded at
#define LOAD_ADDRESS 0xA0000000

extern so_module so_mod;

void soloader_init_all() {
    // Check if we want to start the configurator app
    sceAppUtilInit(&(SceAppUtilInitParam){}, &(SceAppUtilBootParam){});
    SceAppUtilAppEventParam eventParam;
    sceClibMemset(&eventParam, 0, sizeof(SceAppUtilAppEventParam));
    sceAppUtilReceiveAppEvent(&eventParam);
    if (eventParam.type == 0x05) {
        char buffer[2048];
        sceAppUtilAppEventParseLiveArea(&eventParam, buffer);
        if (strstr(buffer, "-config"))
            sceAppMgrLoadExec("app0:/configurator.bin", NULL, NULL);
    }

    // Set default overclock values
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    // Enable analog stick and touchscreen
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, 1);

    if (!module_loaded("kubridge"))
        fatal_error("Error: kubridge.skprx is not installed.");
    log_info("kubridge check passed.");

    if (!file_exists(APK_PATH)) {
        fatal_error("Looks like you haven't installed the data files for this "
                    "port, or they are in an incorrect location. Please make "
                    "sure that you have %s file exactly at that path.", APK_PATH);
    }

    unzFile apk_file = unzOpen(APK_PATH);
    if (!apk_file)
        fatal_error("Error: could not load %s as an archive. Corrupted file?",
                    APK_PATH);

    unz_file_info file_info;
    int res = unzLocateFile(apk_file, SO_PATH, NULL);
    if (res != UNZ_OK) {
        fatal_error("Error: could not locate the %s inside the APK."
                    "You may be using a wrong game version or corrupted APK.",
                    SO_PATH);
    }
    unzGetCurrentFileInfo(apk_file, &file_info, NULL, 0, NULL, 0, NULL, 0);
    unzOpenCurrentFile(apk_file);
    uint64_t so_size = file_info.uncompressed_size;
    uint8_t *so_buffer = (uint8_t *)malloc(so_size);
    unzReadCurrentFile(apk_file, so_buffer, so_size);
    unzCloseCurrentFile(apk_file);

    char * so_hash = get_string_sha1(so_buffer, so_size);
    if (strcmp(so_hash, "66F317C81795FDF4C8D40D9E6E5C3BF85D602904") != 0) {
        fatal_error("Looks like you installed a wrong version of the game "
                    "that doesn't work with this port. Please make sure "
                    "that you're using the Xperia play release v1.0.4.2. "
                    "Expected SHA1: 66F317C81795FDF4C8D40D9E6E5C3BF85D602904, "
                    "actual SHA1: %s (for file %s).", so_hash, SO_PATH);
    }

    res = so_mem_load(&so_mod, so_buffer, so_size, LOAD_ADDRESS);
    if (res < 0)
        fatal_error("Error: could not load %s.", SO_PATH);
    logv_info("so_mem_load(%s) passed.", SO_PATH);

    free(so_hash);
    free(so_buffer);

    so_relocate(&so_mod);
    log_info("so_relocate() passed.");

    resolve_imports(&so_mod);
    log_info("so_resolve() passed.");

    so_patch();
    log_info("so_patch() passed.");

    so_flush_caches(&so_mod);
    log_info("so_flush_caches() passed.");

    so_initialize(&so_mod);
    log_info("so_initialize() passed.");

    gl_preload();
    log_info("gl_preload() passed.");

    jni_init();
    log_info("jni_init() passed.");
}
