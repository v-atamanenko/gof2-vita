/*
 * dynlib.c
 *
 * Resolving dynamic imports of the .so.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021 Rinnegatamante
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

// Disable IDE complaints about _identifiers and global interfaces
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
#pragma ide diagnostic ignored "cppcoreguidelines-interfaces-global-init"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#include "dynlib.h"

#include <OpenSLES.h>
#include <math.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/processmgr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/unistd.h>

#include <so_util/so_util.h>
#include <AFakeNative/AFakeNative.h>

#include "utils/utils.h"
#include "utils/glutil.h"
#include "utils/settings.h"
#include "reimpl/io.h"
#include "reimpl/log.h"
#include "reimpl/mem.h"
#include "reimpl/pthr.h"

extern void *__aeabi_atexit;
extern void *__cxa_pure_virtual;
extern void *__stack_chk_fail;
extern void *__stack_chk_guard;

extern void * __aeabi_d2f;
extern void * __aeabi_d2iz;
extern void * __aeabi_dadd;
extern void * __aeabi_dcmpeq;
extern void * __aeabi_dcmpgt;
extern void * __aeabi_dcmplt;
extern void * __aeabi_ddiv;
extern void * __aeabi_dmul;
extern void * __aeabi_dsub;
extern void * __aeabi_f2d;
extern void * __aeabi_f2iz;
extern void * __aeabi_f2lz;
extern void * __aeabi_f2ulz;
extern void * __aeabi_fadd;
extern void * __aeabi_fcmpeq;
extern void * __aeabi_fcmpge;
extern void * __aeabi_fcmpgt;
extern void * __aeabi_fcmple;
extern void * __aeabi_fcmplt;
extern void * __aeabi_fdiv;
extern void * __aeabi_fmul;
extern void * __aeabi_fsub;
extern void * __aeabi_i2d;
extern void * __aeabi_i2f;
extern void * __aeabi_idiv;
extern void * __aeabi_idivmod;
extern void * __aeabi_l2f;
extern void * __aeabi_ldivmod;
extern void * __aeabi_lmul;
extern void * __aeabi_ui2d;
extern void * __aeabi_ui2f;
extern void * __aeabi_uidiv;
extern void * __aeabi_uidivmod;
extern void * __aeabi_ul2f;
extern void * __aeabi_uldivmod;
extern void * __dso_handle;
extern void * __swbuf;
extern void * _ZdaPv;
extern void * _ZdlPv;
extern void * _Znaj;
extern void * _Znwj;

extern const char *BIONIC_ctype_;
extern const short *BIONIC_tolower_tab_;
extern const short *BIONIC_toupper_tab_;

static FILE __sF_fake[3];

bool first_swap = true;

uint32_t last_render_time;
uint32_t delta;

EGLBoolean eglSwapBuffers_wrapper(EGLDisplay display, EGLSurface surface) {
    if (setting_fpsLock == 0) return eglSwapBuffers(display, surface);

    if (first_swap) {
        last_render_time = sceKernelGetProcessTimeLow();
        delta = (1000000 / (setting_fpsLock+1));
        first_swap = false;
    }

    while (sceKernelGetProcessTimeLow() - last_render_time < delta) {
        sched_yield();
    }

    last_render_time = sceKernelGetProcessTimeLow();
    return eglSwapBuffers(display, surface);
}

so_default_dynlib default_dynlib[] = {
        { "AConfiguration_delete", (uintptr_t)&AConfiguration_delete },
        { "AConfiguration_fromAssetManager", (uintptr_t)&AConfiguration_fromAssetManager },
        { "AConfiguration_getOrientation", (uintptr_t)&AConfiguration_getOrientation },
        { "AConfiguration_new", (uintptr_t)&AConfiguration_new },
        { "AInputEvent_getSource", (uintptr_t)&AInputEvent_getSource },
        { "AInputQueue_attachLooper", (uintptr_t)&AInputQueue_attachLooper },
        { "AInputQueue_detachLooper", (uintptr_t)&AInputQueue_detachLooper },
        { "AInputQueue_finishEvent", (uintptr_t)&AInputQueue_finishEvent },
        { "AInputQueue_getEvent", (uintptr_t)&AInputQueue_getEvent },
        { "AInputQueue_preDispatchEvent", (uintptr_t)&AInputQueue_preDispatchEvent },
        { "AKeyEvent_getAction", (uintptr_t)&AKeyEvent_getAction },
        { "AKeyEvent_getKeyCode", (uintptr_t)&AKeyEvent_getKeyCode },
        { "ALooper_pollAll", (uintptr_t)&ALooper_pollAll },
        { "ALooper_prepare", (uintptr_t)&ALooper_prepare },
        { "AMotionEvent_getAction", (uintptr_t)&AMotionEvent_getAction },
        { "AMotionEvent_getPointerCount", (uintptr_t)&AMotionEvent_getPointerCount },
        { "AMotionEvent_getPointerId", (uintptr_t)&AMotionEvent_getPointerId },
        { "AMotionEvent_getX", (uintptr_t)&AMotionEvent_getX },
        { "AMotionEvent_getY", (uintptr_t)&AMotionEvent_getY },
        { "ANativeWindow_getFormat", (uintptr_t)&ANativeWindow_getFormat },
        { "ANativeWindow_getHeight", (uintptr_t)&ANativeWindow_getHeight },
        { "ANativeWindow_getWidth", (uintptr_t)&ANativeWindow_getWidth },
        { "ANativeWindow_setBuffersGeometry", (uintptr_t)&ANativeWindow_setBuffersGeometry },
        { "ASensorEventQueue_disableSensor", (uintptr_t)&ASensorEventQueue_disableSensor },
        { "ASensorEventQueue_enableSensor", (uintptr_t)&ASensorEventQueue_enableSensor },
        { "ASensorEventQueue_getEvents", (uintptr_t)&ASensorEventQueue_getEvents },
        { "ASensorEventQueue_setEventRate", (uintptr_t)&ASensorEventQueue_setEventRate },
        { "ASensorManager_createEventQueue", (uintptr_t)&ASensorManager_createEventQueue },
        { "ASensorManager_getDefaultSensor", (uintptr_t)&ASensorManager_getDefaultSensor },
        { "ASensorManager_getInstance", (uintptr_t)&ASensorManager_getInstance },
        { "SL_IID_BUFFERQUEUE", (uintptr_t)&SL_IID_BUFFERQUEUE },
        { "SL_IID_EFFECTSEND", (uintptr_t)&SL_IID_EFFECTSEND },
        { "SL_IID_ENGINE", (uintptr_t)&SL_IID_ENGINE },
        { "SL_IID_ENVIRONMENTALREVERB", (uintptr_t)&SL_IID_ENVIRONMENTALREVERB },
        { "SL_IID_PLAY", (uintptr_t)&SL_IID_PLAY },
        { "SL_IID_VOLUME", (uintptr_t)&SL_IID_VOLUME },
        { "_ZdaPv", (uintptr_t)&_ZdaPv },
        { "_ZdlPv", (uintptr_t)&_ZdlPv },
        { "_Znaj", (uintptr_t)&_Znaj },
        { "_Znwj", (uintptr_t)&_Znwj },
        { "__aeabi_atexit", (uintptr_t)&__aeabi_atexit },
        { "__aeabi_d2f", (uintptr_t)&__aeabi_d2f },
        { "__aeabi_d2iz", (uintptr_t)&__aeabi_d2iz },
        { "__aeabi_dadd", (uintptr_t)&__aeabi_dadd },
        { "__aeabi_dcmpeq", (uintptr_t)&__aeabi_dcmpeq },
        { "__aeabi_dcmpgt", (uintptr_t)&__aeabi_dcmpgt },
        { "__aeabi_dcmplt", (uintptr_t)&__aeabi_dcmplt },
        { "__aeabi_ddiv", (uintptr_t)&__aeabi_ddiv },
        { "__aeabi_dmul", (uintptr_t)&__aeabi_dmul },
        { "__aeabi_dsub", (uintptr_t)&__aeabi_dsub },
        { "__aeabi_f2d", (uintptr_t)&__aeabi_f2d },
        { "__aeabi_f2iz", (uintptr_t)&__aeabi_f2iz },
        { "__aeabi_f2lz", (uintptr_t)&__aeabi_f2lz },
        { "__aeabi_f2ulz", (uintptr_t)&__aeabi_f2ulz },
        { "__aeabi_fadd", (uintptr_t)&__aeabi_fadd },
        { "__aeabi_fcmpeq", (uintptr_t)&__aeabi_fcmpeq },
        { "__aeabi_fcmpge", (uintptr_t)&__aeabi_fcmpge },
        { "__aeabi_fcmpgt", (uintptr_t)&__aeabi_fcmpgt },
        { "__aeabi_fcmple", (uintptr_t)&__aeabi_fcmple },
        { "__aeabi_fcmplt", (uintptr_t)&__aeabi_fcmplt },
        { "__aeabi_fdiv", (uintptr_t)&__aeabi_fdiv },
        { "__aeabi_fmul", (uintptr_t)&__aeabi_fmul },
        { "__aeabi_fsub", (uintptr_t)&__aeabi_fsub },
        { "__aeabi_i2d", (uintptr_t)&__aeabi_i2d },
        { "__aeabi_i2f", (uintptr_t)&__aeabi_i2f },
        { "__aeabi_idiv", (uintptr_t)&__aeabi_idiv },
        { "__aeabi_idivmod", (uintptr_t)&__aeabi_idivmod },
        { "__aeabi_l2f", (uintptr_t)&__aeabi_l2f },
        { "__aeabi_ldivmod", (uintptr_t)&__aeabi_ldivmod },
        { "__aeabi_lmul", (uintptr_t)&__aeabi_lmul },
        { "__aeabi_ui2d", (uintptr_t)&__aeabi_ui2d },
        { "__aeabi_ui2f", (uintptr_t)&__aeabi_ui2f },
        { "__aeabi_uidiv", (uintptr_t)&__aeabi_uidiv },
        { "__aeabi_uidivmod", (uintptr_t)&__aeabi_uidivmod },
        { "__aeabi_ul2f", (uintptr_t)&__aeabi_ul2f },
        { "__aeabi_uldivmod", (uintptr_t)&__aeabi_uldivmod },
        { "__android_log_print", (uintptr_t)&android_log_print },
        { "__cxa_pure_virtual", (uintptr_t)&__cxa_pure_virtual },
        { "__dso_handle", (uintptr_t)&__dso_handle },
        { "__errno", (uintptr_t)&__errno },
        { "__sF", (uintptr_t)&__sF_fake },
        { "__stack_chk_fail", (uintptr_t)&__stack_chk_fail },
        { "__stack_chk_guard", (uintptr_t)&__stack_chk_guard },
        { "__swbuf", (uintptr_t)&__swbuf },
        { "_toupper_tab_", (uintptr_t)&BIONIC_toupper_tab_ },
        { "abort", (uintptr_t)&abort },
        { "acos", (uintptr_t)&acos },
        { "atan", (uintptr_t)&atan },
        { "atoi", (uintptr_t)&atoi },
        { "calloc", (uintptr_t)&calloc },
        { "chmod", (uintptr_t)&chmod },
        { "clearerr", (uintptr_t)&clearerr },
        { "close", (uintptr_t)&close_soloader },
        { "cos", (uintptr_t)&cos },
        { "cosf", (uintptr_t)&cosf },
        { "eglChooseConfig", (uintptr_t)&eglChooseConfig },
        { "eglCreateContext", (uintptr_t)&eglCreateContext },
        { "eglCreateWindowSurface", (uintptr_t)&eglCreateWindowSurface },
        { "eglDestroyContext", (uintptr_t)&eglDestroyContext },
        { "eglDestroySurface", (uintptr_t)&eglDestroySurface },
        { "eglGetConfigAttrib", (uintptr_t)&eglGetConfigAttrib },
        { "eglGetDisplay", (uintptr_t)&eglGetDisplay },
        { "eglInitialize", (uintptr_t)&eglInitialize },
        { "eglMakeCurrent", (uintptr_t)&eglMakeCurrent },
        { "eglQuerySurface", (uintptr_t)&eglQuerySurface },
        { "eglSwapBuffers", (uintptr_t)&eglSwapBuffers_wrapper },
        { "eglTerminate", (uintptr_t)&eglTerminate },
        { "fclose", (uintptr_t)&fclose_soloader },
        { "fdopen", (uintptr_t)&fdopen },
        { "fopen", (uintptr_t)&fopen_soloader },
        { "fprintf", (uintptr_t)&fprintf },
        { "fread", (uintptr_t)&fread },
        { "free", (uintptr_t)&free },
        { "fseek", (uintptr_t)&fseek },
        { "fseeko", (uintptr_t)&fseeko },
        { "fstat", (uintptr_t)&fstat_soloader },
        { "ftell", (uintptr_t)&ftell },
        { "ftello", (uintptr_t)&ftello_soloader },
        { "fwrite", (uintptr_t)&fwrite },
        { "getpid", (uintptr_t)&getpid },
        { "gettimeofday", (uintptr_t)&gettimeofday },
        { "glActiveTexture", (uintptr_t)&glActiveTexture },
        { "glAttachShader", (uintptr_t)&glAttachShader },
        { "glBindBuffer", (uintptr_t)&glBindBuffer },
        { "glBindTexture", (uintptr_t)&glBindTexture },
        { "glBlendFunc", (uintptr_t)&glBlendFunc },
        { "glBufferData", (uintptr_t)&glBufferData },
        { "glClear", (uintptr_t)&glClear },
        { "glClearColor", (uintptr_t)&glClearColor },
        { "glCompileShader", (uintptr_t)&ret0 },
        { "glCompressedTexImage2D", (uintptr_t)&glCompressedTexImage2D },
        { "glCreateProgram", (uintptr_t)&glCreateProgram },
        { "glCreateShader", (uintptr_t)&glCreateShader },
        { "glCullFace", (uintptr_t)&glCullFace },
        { "glDeleteBuffers", (uintptr_t)&glDeleteBuffers },
        { "glDeleteProgram", (uintptr_t)&glDeleteProgram },
        { "glDeleteShader", (uintptr_t)&glDeleteShader },
        { "glDeleteTextures", (uintptr_t)&glDeleteTextures },
        { "glDepthMask", (uintptr_t)&glDepthMask },
        { "glDisable", (uintptr_t)&glDisable },
        { "glDrawArrays", (uintptr_t)&glDrawArrays },
        { "glDrawElements", (uintptr_t)&glDrawElements },
        { "glEnable", (uintptr_t)&glEnable },
        { "glEnableVertexAttribArray", (uintptr_t)&glEnableVertexAttribArray },
        { "glGenBuffers", (uintptr_t)&glGenBuffers },
        { "glGenTextures", (uintptr_t)&glGenTextures },
        { "glGetAttribLocation", (uintptr_t)&glGetAttribLocation },
        { "glGetError", (uintptr_t)&glGetError },
        { "glGetIntegerv", (uintptr_t)&glGetIntegerv },
        { "glGetProgramInfoLog", (uintptr_t)&glGetProgramInfoLog },
        { "glGetProgramiv", (uintptr_t)&glGetProgramiv },
        { "glGetShaderInfoLog", (uintptr_t)&glGetShaderInfoLog },
        { "glGetShaderiv", (uintptr_t)&glGetShaderiv },
        { "glGetString", (uintptr_t)&glGetString },
        { "glGetUniformLocation", (uintptr_t)&glGetUniformLocation },
        { "glHint", (uintptr_t)&glHint },
        { "glLineWidth", (uintptr_t)&glLineWidth },
        { "glLinkProgram", (uintptr_t)&glLinkProgram },
        { "glPixelStorei", (uintptr_t)&ret0 },
        { "glScissor", (uintptr_t)&glScissor },
        { "glShaderSource", (uintptr_t)&glShaderSourceHook },
        { "glTexImage2D", (uintptr_t)&glTexImage2D },
        { "glTexParameteri", (uintptr_t)&glTexParameteri },
        { "glUniform1f", (uintptr_t)&glUniform1f },
        { "glUniform1i", (uintptr_t)&glUniform1i },
        { "glUniform3f", (uintptr_t)&glUniform3f },
        { "glUniform4fv", (uintptr_t)&glUniform4fv },
        { "glUniformMatrix3fv", (uintptr_t)&glUniformMatrix3fv },
        { "glUniformMatrix4fv", (uintptr_t)&glUniformMatrix4fv },
        { "glUseProgram", (uintptr_t)&glUseProgram },
        { "glVertexAttribPointer", (uintptr_t)&glVertexAttribPointer },
        { "glViewport", (uintptr_t)&glMatrixMode },
        { "glViewport", (uintptr_t)&glViewport },
        { "localtime", (uintptr_t)&localtime },
        { "malloc", (uintptr_t)&malloc },
        { "memchr", (uintptr_t)&memchr },
        { "memcmp", (uintptr_t)&memcmp },
        { "memcpy", (uintptr_t)&memcpy },
        { "memmove", (uintptr_t)&memmove },
        { "memset", (uintptr_t)&memset },
        { "mkstemp", (uintptr_t)&mkstemp },
        { "mktime", (uintptr_t)&mktime },
        { "open", (uintptr_t)&open_soloader },
        { "pow", (uintptr_t)&pow },
        { "pthread_attr_init", (uintptr_t)&pthread_attr_init_soloader },
        { "pthread_attr_setdetachstate", (uintptr_t)&pthread_attr_setdetachstate_soloader },
        { "pthread_cond_broadcast", (uintptr_t)&pthread_cond_broadcast_soloader },
        { "pthread_cond_init", (uintptr_t)&pthread_cond_init_soloader },
        { "pthread_cond_wait", (uintptr_t)&pthread_cond_wait_soloader },
        { "pthread_create", (uintptr_t)&pthread_create_soloader },
        { "pthread_mutex_destroy", (uintptr_t)&pthread_mutex_destroy_soloader },
        { "pthread_mutex_init", (uintptr_t)&pthread_mutex_init_soloader },
        { "pthread_mutex_lock", (uintptr_t)&pthread_mutex_lock_soloader },
        { "pthread_mutex_unlock", (uintptr_t)&pthread_mutex_unlock_soloader },
        { "qsort", (uintptr_t)&qsort },
        { "realloc", (uintptr_t)&realloc },
        { "remove", (uintptr_t)&remove },
        { "rename", (uintptr_t)&rename },
        { "sin", (uintptr_t)&sin },
        { "sinf", (uintptr_t)&sinf },
        { "slCreateEngine", (uintptr_t)&slCreateEngine },
        { "snprintf", (uintptr_t)&snprintf },
        { "sprintf", (uintptr_t)&sprintf },
        { "sqrt", (uintptr_t)&sqrt },
        { "sqrtf", (uintptr_t)&sqrtf },
        { "sscanf", (uintptr_t)&sscanf },
        { "stat", (uintptr_t)&stat_soloader },
        { "strcasecmp", (uintptr_t)&strcasecmp },
        { "strcmp", (uintptr_t)&strcmp },
        { "strcpy", (uintptr_t)&strcpy },
        { "strdup", (uintptr_t)&strdup },
        { "strerror", (uintptr_t)&strerror },
        { "strlen", (uintptr_t)&strlen },
        { "strncmp", (uintptr_t)&strncmp },
        { "strrchr", (uintptr_t)&strrchr },
        { "strstr", (uintptr_t)&strstr },
        { "strtoul", (uintptr_t)&strtoul },
        { "time", (uintptr_t)&time },
        { "umask", (uintptr_t)&ret0 },
        { "usleep", (uintptr_t)&usleep },
};

void resolve_imports(so_module* mod) {
    __sF_fake[0] = *stdin;
    __sF_fake[1] = *stdout;
    __sF_fake[2] = *stderr;
    so_resolve(mod, default_dynlib, sizeof(default_dynlib), 0);
}
