/*
 * dynlib.c
 *
 * Resolving dynamic imports of the .so.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

// Disable IDE complaints about _identifiers and global interfaces
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
#pragma ide diagnostic ignored "cppcoreguidelines-interfaces-global-init"

#include "dynlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <netdb.h>
#include <string.h>
#include <wchar.h>

#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <psp2/kernel/clib.h>
#include <wctype.h>
#include <OpenSLES.h>
#include <ctype.h>

#include "utils/glutil.h"

#include "reimpl/io.h"
#include "reimpl/log.h"
#include "reimpl/mem.h"
#include "reimpl/sys.h"
#include "reimpl/pthr.h"
#include "AFakeNative/AFakeNative.h"

unsigned int __page_size = 4096;

extern void *__aeabi_atexit;
extern void *__cxa_atexit;
extern void *__cxa_finalize;
extern void *__cxa_pure_virtual;
extern void *__cxa_guard_acquire;
extern void *__cxa_guard_release;
extern void *__stack_chk_fail;
extern void *__stack_chk_guard;

extern void* __aeabi_memclr;
extern void* __aeabi_memclr4;
extern void* __aeabi_memclr8;
extern void* __aeabi_memcpy;
extern void* __aeabi_memcpy4;
extern void* __aeabi_memcpy8;
extern void* __aeabi_memmove;
extern void* __aeabi_memmove4;
extern void* __aeabi_memmove8;
extern void* __aeabi_memset;
extern void* __aeabi_memset4;
extern void* __aeabi_memset8;
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

int timezone = 0;
char *__tzname[2] = { (char *) "GMT", (char *) "GMT" };
static FILE __sF_fake[3];

#define __ATOMIC_INLINE__ static __inline__ __attribute__((always_inline))

__ATOMIC_INLINE__ int __atomic_swap(int new_value, volatile int *ptr)
{
    int old_value;
    do {
        old_value = *ptr;
    } while (__sync_val_compare_and_swap(ptr, old_value, new_value) != old_value);
    return old_value;
}

__ATOMIC_INLINE__ int __atomic_inc(volatile int *ptr)
{
    return __sync_fetch_and_add (ptr, 1);
}

__ATOMIC_INLINE__ int __atomic_dec(volatile int *ptr)
{
    return __sync_fetch_and_sub (ptr, 1);
}

__ATOMIC_INLINE__ int __atomic_cmpxchg(int old_value, int new_value, volatile int* ptr)
{
    /* We must return 0 on success */
    return __sync_val_compare_and_swap(ptr, old_value, new_value) != old_value;
}

/*
void *dlsym_fake(void *restrict handle, const char *restrict symbol) {
    printf("dlsym: 0x%x, %s\n", handle, symbol);
    if (strcmp(symbol, "AMotionEvent_getHistoricalAxisValue") == 0) {
        return (void*)&AMotionEvent_getHistoricalAxisValue;
    }
    if (strcmp(symbol, "AMotionEvent_getAxisValue") == 0) {
        return (void*)&AMotionEvent_getAxisValue;
    }
    return NULL;
}
*/

EGLBoolean eglChooseConfig (EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config) {
    return EGL_TRUE;
}

EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list) {
    return strdup("ctx");
}

EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, void * win, const EGLint *attrib_list)
{
    return strdup("surface");
}


EGLBoolean eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx) {
    return EGL_TRUE;
}

int eglDestroyContext() {
    sceClibPrintf("RET0D: eglDestroyContext\n");
    return 0;
}
int eglDestroySurface() {
    sceClibPrintf("RET0D: eglDestroySurface\n");
    return 0;
}

int eglTerminate() {
    sceClibPrintf("RET0D: eglTerminate\n");
    return 0;
}

void glHint_fake(uint32_t target, uint32_t mode) {
    sceClibPrintf("glHint(0x%x, 0x%x), return addr: 0x%x\n", target, mode, __builtin_return_address(0));
    glHint(target, mode);
}

void glViewport_fake(int32_t x, int32_t y, int32_t w, int32_t h) {
    sceClibPrintf("glViewport(%i, %i, %i, %i), return addr: 0x%x\n", x,y,w,h, __builtin_return_address(0));
    glViewport(x,y,w,h);
}

void glMatrixMode_fake(uint32_t mode) {
    sceClibPrintf("glMatrixMode(0x%x), return addr: 0x%x\n", mode, __builtin_return_address(0));
    glMatrixMode(mode);
}

GLint glGetUniformLocation_fake(GLuint prog, const GLchar *name) {
    sceClibPrintf("glGetUniformLocation(0x%x, 0x%x), return addr: 0x%x\n", prog, name, __builtin_return_address(0));
    return glGetUniformLocation(prog, name);

}

GLint glGetAttribLocation_fake(GLuint prog, const GLchar *name) {
    sceClibPrintf("glGetAttribLocation(0x%x, 0x%x), return addr: 0x%x\n", prog, name, __builtin_return_address(0));
    return glGetAttribLocation(prog, name);
}
uint32_t glCreateProgram_fake() {
    sceClibPrintf("glCreateProgram(), return addr: 0x%x\n", __builtin_return_address(0));
    return glCreateProgram();
}
uint32_t glCreateShader_fake(uint32_t t) {
    sceClibPrintf("glCreateShader(0x%x), return addr: 0x%x\n",t, __builtin_return_address(0));
    return glCreateShader(t);
}


// NOLINT(cppcoreguidelines-interfaces-global-init)
so_default_dynlib default_dynlib[] = {
        { "__sF", (uintptr_t)&__sF_fake },
        { "abort", (uintptr_t)&abort },
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
        { "__errno", (uintptr_t)&__errno },
        { "__stack_chk_fail", (uintptr_t)&__stack_chk_fail },
        { "__stack_chk_guard", (uintptr_t)&__stack_chk_guard },
        { "_toupper_tab_", (uintptr_t)&BIONIC_toupper_tab_ },
        { "__dso_handle", (uintptr_t)&__dso_handle },
        { "__swbuf", (uintptr_t)&__swbuf },
        { "_ZdaPv", (uintptr_t)&_ZdaPv },
        { "_ZdlPv", (uintptr_t)&_ZdlPv },
        { "_Znaj", (uintptr_t)&_Znaj },
        { "_Znwj", (uintptr_t)&_Znwj },
        { "AConfiguration_delete", (uintptr_t)&AConfiguration_delete },
        { "AConfiguration_fromAssetManager", (uintptr_t)&AConfiguration_fromAssetManager },
        { "AConfiguration_getOrientation", (uintptr_t)&AConfiguration_getOrientation },
        { "AConfiguration_new", (uintptr_t)&AConfiguration_new },
        { "acos", (uintptr_t)&acos },
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
        { "eglMakeCurrent", (uintptr_t)&eglMakeCurrent },
        { "eglTerminate", (uintptr_t)&eglTerminate },
        { "eglGetConfigAttrib", (uintptr_t)&eglGetConfigAttrib },
        { "eglGetDisplay", (uintptr_t)&eglGetDisplay },
        { "eglInitialize", (uintptr_t)&eglInitialize },
        { "eglQuerySurface", (uintptr_t)&eglQuerySurface },
        { "eglSwapBuffers", (uintptr_t)&eglSwapBuffers },
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
        { "glCreateProgram", (uintptr_t)&glCreateProgram_fake },
        { "glCreateShader", (uintptr_t)&glCreateShader_fake },
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
        { "glGetAttribLocation", (uintptr_t)&glGetAttribLocation_fake },
        { "glGetError", (uintptr_t)&glGetError },
        { "glGetIntegerv", (uintptr_t)&glGetIntegerv },
        { "glGetProgramInfoLog", (uintptr_t)&glGetProgramInfoLog },
        { "glGetProgramiv", (uintptr_t)&glGetProgramiv },
        { "glGetShaderInfoLog", (uintptr_t)&glGetShaderInfoLog },
        { "glGetShaderiv", (uintptr_t)&glGetShaderiv },
        { "glGetString", (uintptr_t)&glGetString },
        { "glGetUniformLocation", (uintptr_t)&glGetUniformLocation_fake },
        { "glHint", (uintptr_t)&glHint_fake },
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
        { "glViewport", (uintptr_t)&glViewport_fake },
        { "glViewport", (uintptr_t)&glMatrixMode_fake },
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
        { "SL_IID_BUFFERQUEUE", (uintptr_t)&SL_IID_BUFFERQUEUE },
        { "SL_IID_EFFECTSEND", (uintptr_t)&SL_IID_EFFECTSEND },
        { "SL_IID_ENGINE", (uintptr_t)&SL_IID_ENGINE },
        { "SL_IID_ENVIRONMENTALREVERB", (uintptr_t)&SL_IID_ENVIRONMENTALREVERB },
        { "SL_IID_PLAY", (uintptr_t)&SL_IID_PLAY },
        { "SL_IID_VOLUME", (uintptr_t)&SL_IID_VOLUME },
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
