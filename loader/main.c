/*
 * main.c
 *
 * ARMv6 Shared Libraries loader. Galaxy on Fire 2 edition
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2022 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "main.h"

#include <pthread.h>
#include <psp2/sysmodule.h>
#include <psp2/appmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <string.h>
#include <android/native_activity.h>
#include <android/asset_manager_jni.h>
#include <malloc.h>
#include <sys/unistd.h>

#include "FalsoJNI/FalsoJNI.h"
#include "reimpl/controls.h"
#include "utils/glutil.h"
#include "utils/logger.h"
#include "reimpl/pthr.h"

__attribute__((unused)) int _newlib_heap_size_user = MEMORY_NEWLIB_MB * 1024 * 1024;

so_module so_mod;
struct android_app* app;


int crasher(unsigned int argc, void *argv) {
	uint32_t *nullptr = NULL;
	for (;;) {
		SceCtrlData pad;
		sceCtrlPeekBufferPositive(0, &pad, 1);
		if (pad.buttons & SCE_CTRL_SELECT) *nullptr = 0;
		sceKernelDelayThread(100);
	}
}

extern void (*android_app_write_cmd)(struct android_app* app, int8_t cmd);

void android_app_set_window(android_app *a_app, ANativeWindow *win) {
    ANativeWindow *temp_win_ptr;

    pthread_mutex_lock_soloader(&a_app->pthrMutex);

    if (a_app->pending_native_window != (ANativeWindow *)0x0) {
        android_app_write_cmd(a_app, 2);
    }
    a_app->pending_native_window = win;
    if (win == NULL) {
        temp_win_ptr = NULL;
    }
    else {
        android_app_write_cmd(a_app, 1);
        temp_win_ptr = a_app->pending_native_window;
    }
    if (a_app->active_native_window != temp_win_ptr) {
        do {
            pthread_cond_wait_soloader(&a_app->pthrCond, &a_app->pthrMutex);
        } while (a_app->active_native_window != a_app->pending_native_window);
    }

    pthread_mutex_unlock_soloader(&a_app->pthrMutex);
}

void onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window) {
    android_app_set_window(activity->instance, window);
}

int main(int argc, char*argv[]) {
    soloader_init_all();

    SceUID crasher_thread = sceKernelCreateThread("crasher", crasher, 0x40, 0x1000, 0, 0, NULL);
    sceKernelStartThread(crasher_thread, 0, NULL);

    vglSetParamBufferSize(8 * 1024 * 1024);
    vglUseCachedMem(GL_TRUE);
    vglInitWithCustomThreshold(0, 960, 544, 256 * 1024 * 1024, 0, 0, 0, SCE_GXM_MULTISAMPLE_4X);
    log_info("gl_init() passed");

    ANativeActivityCallbacks* _ANativeActivityCallbacks = (ANativeActivityCallbacks *)malloc(sizeof(struct ANativeActivityCallbacks));
    ANativeActivity* _ANativeActivity = (ANativeActivity *)malloc(sizeof(struct ANativeActivity));
    AAssetManager * _AAssetManager = AAssetManager_fromJava(&jni, NULL);

    _ANativeActivity->callbacks = _ANativeActivityCallbacks; // ANativeActivityCallbacks* callbacks
    _ANativeActivity->vm = &jvm; // JavaVM*
    _ANativeActivity->env = &jni; // JNIEnv*
    _ANativeActivity->clazz = (void*)0x42424242; // jobject
    _ANativeActivity->internalDataPath = DATA_PATH_INT; // const char*
    _ANativeActivity->externalDataPath = DATA_PATH_INT; // const char*
    _ANativeActivity->sdkVersion = 14; // int32_t
    _ANativeActivity->instance = NULL; // void*
    _ANativeActivity->assetManager = _AAssetManager; // AAssetManager*
    _ANativeActivity->obbPath = DATA_PATH_INT; // const char*

    void (*ana_onc)(ANativeActivity* activity, void* savedState, size_t savedStateSize) = (void*)so_symbol(&so_mod,"ANativeActivity_onCreate");
    ana_onc(_ANativeActivity, NULL, 0);
    log_info("Passed ANativeActivity_onCreate");

    sleep(3);

    //log_info("Starting onInputQueueCreated");
    //AInputQueue * aiq = acquire_AInputQueue();
    //_ANativeActivity->callbacks->onInputQueueCreated(_ANativeActivity, aiq);
    //log_info("Passed onInputQueueCreated");

    sleep(3);



    log_info("Starting onNativeWindowCreated");
    _ANativeActivity->callbacks->onNativeWindowCreated(_ANativeActivity, (ANativeWindow*)strdup("fake_window"));
    log_info("Passed onNativeWindowCreated");

    sleep(3);

    app = _ANativeActivity->instance;

/*


    log_info("Starting processInputEvents thread");

    pthread_t thd;
    pthread_create(&thd, NULL, processInputEvents, NULL);
    pthread_detach(thd);
    log_info("Passed starting processInputEvents thread");*/
/*
    do {
        sceKernelDelayThread(1000000);
    } while (ALooper_runs < 2);

    void ** _ZN2xt6Global8gameLoopE = (void*) so_symbol(&so_mod, "_ZN2xt6Global8gameLoopE");
    void ** _ZN2xt6Global11applicationE = (void*) so_symbol(&so_mod, "_ZN2xt6Global11applicationE");

    double (*Time_get_seconds)(void);
    Time_get_seconds = (void*) so_symbol(&so_mod, "_ZN2xt4Time10getSecondsEv");
*/

    /*
    do {
        //double dVar4 = (double)Time_get_seconds();
        //sceClibPrintf("dvar7 must be <= dvar8. getSeconds is dvar4: %i\n", dVar4);
        //sceKernelDelayThread(500000);

        int (*func)(void** param_1);
    func = (void*)(*(int *)_ZN2xt6Global11applicationE + 8);


        sceClibPrintf("_ZN2xt6Global11applicationE is 0x%x\n", (int)_ZN2xt6Global11applicationE);
        sceClibPrintf("*_ZN2xt6Global11applicationE is 0x%x\n", (int)*_ZN2xt6Global11applicationE);
        sceClibPrintf("*_ZN2xt6Global11applicationE + 8 is 0x%x\n", (int)*_ZN2xt6Global11applicationE + 8);
        sceClibPrintf("*_ZN2xt6Global11applicationE + 8 is 0x%x\n", (int)*_ZN2xt6Global11applicationE + 8);

        sceKernelDelayThread(1000000);
        int iVar1 = func(_ZN2xt6Global11applicationE);
        float fVar6 = 1.0f / (float)(long long)iVar1;

        double dVar5;
        if (*(char *)((int)_ZN2xt6Global8gameLoopE + 4) == '\0') {
            dVar5 = *(double *)((int)_ZN2xt6Global8gameLoopE + 8);
        }
        else {
            *(char *)((int)_ZN2xt6Global8gameLoopE + 4) = 0;
            *(double *)((int)_ZN2xt6Global8gameLoopE + 8) = dVar4;
            dVar5 = dVar4;
        }
        double dVar8 = dVar4 - dVar5;
        double dVar7 = (double)fVar6 * 0.8;

        sceClibPrintf("dvar7 must be <= dvar8. dvar7 is %i, dvar8 is %i\n", dVar7, dVar8);
        sceClibPrintf("dvar7 must be <= dvar8. getSeconds is dvar4: %i\n", dVar4);
    } while (1);*/

/*

    log_info("Starting onWindowFocusChanged");
    _ANativeActivity->callbacks->onWindowFocusChanged(_ANativeActivity, 1);
    log_info("Passed onWindowFocusChanged");

    while (1) { sceKernelDelayThread(10000000); }*/
    sceKernelExitDeleteThread(0);
}



void* game_thread() {

}
