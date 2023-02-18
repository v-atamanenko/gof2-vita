/*
 * main.c
 *
 * ARMv6 Shared Libraries loader. Galaxy on Fire 2 edition
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021-2022 Rinnegatamante
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "main.h"

#include <psp2/kernel/threadmgr.h>
#include <FalsoJNI/FalsoJNI.h>
#include <psp2/apputil.h>
#include <psp2/system_param.h>
#include <sys/unistd.h>
#include "utils/glutil.h"
#include "utils/logger.h"

#include "AFakeNative/AFakeNative.h"

__attribute__((unused)) int _newlib_heap_size_user = MEMORY_NEWLIB_MB * 1024 * 1024;

so_module so_mod;

void setCountry() {
    void (*setCountryCodeOfDevice)(JNIEnv *env, void *unused, jint countryCode) = (void *) so_symbol(&so_mod, "Java_net_fishlabs_GalaxyonFire2_GOF2NA_setCountryCodeOfDevice");

    int lang = -1;
    sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &lang);
    switch (lang) {
        case SCE_SYSTEM_PARAM_LANG_ITALIAN:
            setCountryCodeOfDevice(&jni, NULL, 3);
            break;
        case SCE_SYSTEM_PARAM_LANG_GERMAN:
            setCountryCodeOfDevice(&jni, NULL, 1);
            break;
        case SCE_SYSTEM_PARAM_LANG_KOREAN:
            setCountryCodeOfDevice(&jni, NULL, 14);
            break;
        case SCE_SYSTEM_PARAM_LANG_JAPANESE:
            setCountryCodeOfDevice(&jni, NULL, 15);
            break;
        case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_PT:
        case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_BR:
            setCountryCodeOfDevice(&jni, NULL, 7);
            break;
        case SCE_SYSTEM_PARAM_LANG_SPANISH:
            setCountryCodeOfDevice(&jni, NULL, 4);
            break;
        case SCE_SYSTEM_PARAM_LANG_FRENCH:
            setCountryCodeOfDevice(&jni, NULL, 2);
            break;
        case SCE_SYSTEM_PARAM_LANG_POLISH:
            setCountryCodeOfDevice(&jni, NULL, 6);
            break;
        case SCE_SYSTEM_PARAM_LANG_RUSSIAN:
            setCountryCodeOfDevice(&jni, NULL, 5);
            break;
        default:
            setCountryCodeOfDevice(&jni, NULL, 0);
            break;
    }
}

int main(int argc, char*argv[]) {
    soloader_init_all();

    //vglSetParamBufferSize(8 * 1024 * 1024);
    //vglUseCachedMem(GL_TRUE);
    //vglInitWithCustomThreshold(0, 960, 544, 256 * 1024 * 1024, 0, 0, 0, SCE_GXM_MULTISAMPLE_4X);
    //log_info("gl_init() passed");

    int (*JNI_OnLoad)(JavaVM* jvm) = (void*)so_symbol(&so_mod,"JNI_OnLoad");

    void (*setAPKPath)(JNIEnv *env, void *unused, jstring apk_path) = (void*)so_symbol(&so_mod, "Java_net_fishlabs_GalaxyonFire2_GOF2NA_setAPKPath");
    void (*SetDirectory)(JNIEnv *env, void *unused, jstring data_directory) = (void*)so_symbol(&so_mod, "Java_net_fishlabs_GalaxyonFire2_GOF2NA_SetDirectory");
    void (*KeyBoardState)(JNIEnv *env, void *unused, int state) = (void*)so_symbol(&so_mod, "Java_net_fishlabs_GalaxyonFire2_GOF2NA_KeyBoardState");

    int (*ANativeActivity_onCreate)(ANativeActivity* activity) = (void*)so_symbol(&so_mod,"ANativeActivity_onCreate");

    JNI_OnLoad(&jvm);
    log_info("JNI_OnLoad() passed");

    setAPKPath(&jni, NULL, APK_PATH);
    log_info("setAPKPath() passed");

    SetDirectory(&jni, NULL, DATA_PATH);
    log_info("SetDirectory() passed");

    setCountry();
    log_info("setCountry() passed");

    ANativeActivity * activity = ANativeActivity_create();
    ANativeActivity_onCreate(activity);
    log_info("ANativeActivity_onCreate() passed");

    logv_info("onInputQueueCreated is %p, onNativeWindowCreated is %p\n", activity->callbacks->onInputQueueCreated, activity->callbacks->onNativeWindowCreated);

    AInputQueue * aInputQueue = AInputQueue_create();
    activity->callbacks->onInputQueueCreated(activity, aInputQueue);
    log_info("onInputQueueCreated() passed");

    KeyBoardState(&jni, NULL, 1);

    ANativeWindow * aNativeWindow = ANativeWindow_create();
    activity->callbacks->onNativeWindowCreated(activity, aNativeWindow);
    log_info("onNativeWindowCreated() passed");

    sceKernelExitDeleteThread(0);
}
