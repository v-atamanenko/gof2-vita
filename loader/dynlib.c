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
#include <android/asset_manager.h>
#include <android/configuration.h>
#include <android/input.h>
#include <wctype.h>
#include <OpenSLES.h>
#include <android/native_window.h>
#include <android/native_activity.h>
#include <ctype.h>

#include "utils/glutil.h"

#include "reimpl/io.h"
#include "reimpl/log.h"
#include "reimpl/mem.h"
#include "reimpl/sys.h"
#include "reimpl/pthr.h"

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

void glUseProgram_wrap(uint32_t program) {
    sceClibPrintf("glUseProgram, program: %i\n", program);
    glUseProgram(program);
}

void glCompileShader_wrap(uint32_t shader) {
    sceClibPrintf("glCompileShader, shader: %i (ignoring)\n", shader);
}

uint32_t glCreateProgram_wrap() {
    uint32_t ret = glCreateProgram();
    sceClibPrintf("glCreateProgram, ret: %i\n", ret);
    return ret;
}

void glAttachShader_wrap(GLuint prog, GLuint shad) {
    sceClibPrintf("glAttachShader, prog: %i, shad: %i\n", prog, shad);
    return glAttachShader(prog, shad);
}

void * memalign_fake(size_t alignment, size_t size) {
    size_t size_aligned = (size + (alignment - 1)) & (-alignment);

    sceClibPrintf("calling memalign(%i, %i): aligned to %i\n", alignment, size, size_aligned);
    //sceClibPrintf("calling memalign(%i, %i)\n", alignment, size);
    //void* ret = memalign(alignment, size);
    void* ret = malloc(size_aligned);
    if (ret == NULL) {
        sceClibPrintf("failed memalign\n");
    }
    sceClibPrintf("memalign done\n");
    return ret;
}

void __aeabi_memclr_impl(void *dest, size_t n) {
    memset(dest, 0, n);
}

void __aeabi_memset_impl(void *dest, size_t n, int c) {
    memset(dest, c, n);
}

void * malloc_fake (size_t size) {
    return memalign(16, size);
}

EGLBoolean eglSwapBuffers_wrap(EGLDisplay display, EGLSurface surface) {
    sceClibPrintf("eglSwapBuffers_wrap\n");
    vglSwapBuffers(GL_FALSE);
    return GL_TRUE;
}

// NOLINT(cppcoreguidelines-interfaces-global-init)
so_default_dynlib default_dynlib[] = {
        // for 1.15
        { "__sF", (uintptr_t)&__sF_fake },
        { "__aeabi_atexit", (uintptr_t)&__aeabi_atexit },
        { "drand48", (uintptr_t)&drand48 },
        { "ldexpf", (uintptr_t)&ldexpf },
        { "expf", (uintptr_t)&expf },
        { "__assert2", (uintptr_t)&ret0 },
        { "fprintf", (uintptr_t)&fprintf },
        { "strncpy", (uintptr_t)&strncpy },
        { "sysconf", (uintptr_t)&sysconf },
        { "fcntl", (uintptr_t)&fcntl_soloader },
        { "munmap", (uintptr_t)&munmap },
        { "mmap", (uintptr_t)&mmap },
        { "isalpha", (uintptr_t)&isalpha },
        { "iscntrl", (uintptr_t)&iscntrl },
        { "isprint", (uintptr_t)&isprint },
        { "ispunct", (uintptr_t)&ispunct },
        { "isspace", (uintptr_t)&isspace },
        { "isxdigit", (uintptr_t)&isxdigit },
        { "isupper", (uintptr_t)&isupper },
        { "islower", (uintptr_t)&islower },
        { "toupper", (uintptr_t)&toupper },
        { "tolower", (uintptr_t)&tolower },
        { "iswalpha", (uintptr_t)&iswalpha },
        { "iswcntrl", (uintptr_t)&iswcntrl },
        { "iswdigit", (uintptr_t)&iswdigit },
        { "iswprint", (uintptr_t)&iswprint },
        { "iswpunct", (uintptr_t)&iswpunct },
        { "iswspace", (uintptr_t)&iswspace },
        { "iswxdigit", (uintptr_t)&iswxdigit },
        { "iswupper", (uintptr_t)&iswupper },
        { "iswlower", (uintptr_t)&iswlower },
        { "wcsncpy", (uintptr_t)&wcsncpy },
        { "wcscmp", (uintptr_t)&wcscmp },
        { "snprintf", (uintptr_t)&snprintf },
        { "fsetpos", (uintptr_t)&fsetpos },
        { "fgetpos", (uintptr_t)&fgetpos },


        { "AAssetDir_close", (uintptr_t)&AAssetDir_close },
        { "AAssetDir_getNextFileName", (uintptr_t)&AAssetDir_getNextFileName },
        { "AAssetManager_open", (uintptr_t)&AAssetManager_open },
        { "AAssetManager_openDir", (uintptr_t)&AAssetManager_openDir },
        { "AAsset_close", (uintptr_t)&AAsset_close },
        { "AAsset_getBuffer", (uintptr_t)&AAsset_getBuffer },
        { "AAsset_getLength", (uintptr_t)&AAsset_getLength },
        { "AAsset_openFileDescriptor", (uintptr_t)&AAsset_openFileDescriptor },
        { "AConfiguration_delete", (uintptr_t)&AConfiguration_delete },
        { "AConfiguration_fromAssetManager", (uintptr_t)&AConfiguration_fromAssetManager },
        { "AConfiguration_getCountry", (uintptr_t)&AConfiguration_getCountry },
        { "AConfiguration_getLanguage", (uintptr_t)&AConfiguration_getLanguage },
        { "AConfiguration_new", (uintptr_t)&AConfiguration_new },
        { "AInputEvent_getDeviceId", (uintptr_t)&AInputEvent_getDeviceId },
        { "AInputEvent_getSource", (uintptr_t)&AInputEvent_getSource },
        { "AInputEvent_getType", (uintptr_t)&AInputEvent_getType },
        { "AInputQueue_attachLooper", (uintptr_t)&AInputQueue_attachLooper },
        { "AInputQueue_detachLooper", (uintptr_t)&AInputQueue_detachLooper },
        { "AInputQueue_finishEvent", (uintptr_t)&AInputQueue_finishEvent },
        { "AInputQueue_getEvent", (uintptr_t)&AInputQueue_getEvent },
        { "AInputQueue_preDispatchEvent", (uintptr_t)&AInputQueue_preDispatchEvent },
        { "AKeyEvent_getAction", (uintptr_t)&AKeyEvent_getAction },
        { "AKeyEvent_getFlags", (uintptr_t)&AKeyEvent_getFlags },
        { "AKeyEvent_getKeyCode", (uintptr_t)&AKeyEvent_getKeyCode },
        { "AKeyEvent_getMetaState", (uintptr_t)&AKeyEvent_getMetaState },
        { "ALooper_addFd", (uintptr_t)&ALooper_addFd },
        { "ALooper_pollAll", (uintptr_t)&ALooper_pollAll },
        { "ALooper_prepare", (uintptr_t)&ALooper_prepare },
        { "AMotionEvent_getAction", (uintptr_t)&AMotionEvent_getAction },
        { "AMotionEvent_getFlags", (uintptr_t)&AMotionEvent_getFlags },
        { "AMotionEvent_getHistoricalX", (uintptr_t)&AMotionEvent_getHistoricalX },
        { "AMotionEvent_getHistoricalY", (uintptr_t)&AMotionEvent_getHistoricalY },
        { "AMotionEvent_getHistorySize", (uintptr_t)&AMotionEvent_getHistorySize },
        { "AMotionEvent_getMetaState", (uintptr_t)&AMotionEvent_getMetaState },
        { "AMotionEvent_getPointerCount", (uintptr_t)&AMotionEvent_getPointerCount },
        { "AMotionEvent_getPointerId", (uintptr_t)&AMotionEvent_getPointerId },
        { "AMotionEvent_getX", (uintptr_t)&AMotionEvent_getX },
        { "AMotionEvent_getY", (uintptr_t)&AMotionEvent_getY },
        { "ANativeActivity_finish", (uintptr_t)&ret0 },
        { "ANativeWindow_getHeight", (uintptr_t)&ANativeWindow_getHeight },
        { "ANativeWindow_getWidth", (uintptr_t)&ANativeWindow_getWidth },
        { "ANativeWindow_setBuffersGeometry", (uintptr_t)&ret0 },
        { "__aeabi_memclr", (uintptr_t)&__aeabi_memclr_impl },
        { "__aeabi_memclr4", (uintptr_t)&__aeabi_memclr_impl },
        { "__aeabi_memclr8", (uintptr_t)&__aeabi_memclr_impl },
        { "__aeabi_memcpy", (uintptr_t)&memcpy },
        { "__aeabi_memcpy4", (uintptr_t)&memcpy },
        { "__aeabi_memcpy8", (uintptr_t)&memcpy },
        { "__aeabi_memmove", (uintptr_t)&memmove },
        { "__aeabi_memmove4", (uintptr_t)&memmove },
        { "__aeabi_memmove8", (uintptr_t)&memmove },
        { "__aeabi_memset", (uintptr_t)&__aeabi_memset_impl },
        { "__aeabi_memset4", (uintptr_t)&__aeabi_memset_impl },
        { "__aeabi_memset8", (uintptr_t)&__aeabi_memset_impl },
        { "__android_log_print", (uintptr_t)&android_log_print },
        { "__cxa_atexit", (uintptr_t)&__cxa_atexit },
        { "__cxa_finalize", (uintptr_t)&__cxa_finalize },
        { "__errno", (uintptr_t)&__errno },
        { "_ctype_", (uintptr_t)&BIONIC_ctype_ },
        { "_tolower_tab_", (uintptr_t)&BIONIC_tolower_tab_ },
        { "_toupper_tab_", (uintptr_t)&BIONIC_toupper_tab_ },
        { "__stack_chk_guard", (uintptr_t)&__stack_chk_guard },
        { "__gnu_Unwind_Find_exidx", (uintptr_t)&ret0 },
        { "__google_potentially_blocking_region_begin", (uintptr_t)&ret0 },
        { "__google_potentially_blocking_region_end", (uintptr_t)&ret0 },
        { "__stack_chk_fail", (uintptr_t)&__stack_chk_fail },
        { "abort", (uintptr_t)&abort },
        { "acos", (uintptr_t)&acos },
        { "asinf", (uintptr_t)&asinf },
        { "atan", (uintptr_t)&atan },
        { "atan2", (uintptr_t)&atan2 },
        { "atan2f", (uintptr_t)&atan2f },
        { "atoi", (uintptr_t)&atoi },
        { "btowc", (uintptr_t)&btowc },
        { "calloc", (uintptr_t)&calloc },
        { "ceil", (uintptr_t)&ceil },
        { "clock", (uintptr_t)&clock },
        { "clock_gettime", (uintptr_t)&clock_gettime_soloader },
        { "close", (uintptr_t)&close },
        { "cos", (uintptr_t)&cos },
        { "cosf", (uintptr_t)&cosf },
        { "difftime", (uintptr_t)&difftime },
        { "dlclose", (uintptr_t)&ret0 },
        { "dlerror", (uintptr_t)&ret0 },
        { "dlopen", (uintptr_t)&ret0 },
        { "dlsym", (uintptr_t)&dlsym_fake },
        { "eglBindAPI", (uintptr_t)&eglBindAPI },
        { "eglChooseConfig", (uintptr_t)&ret0 },
        { "eglCreateContext", (uintptr_t)&ret0 },
        { "eglCreateWindowSurface", (uintptr_t)&ret0 },
        { "eglDestroyContext", (uintptr_t)&ret0 },
        { "eglDestroySurface", (uintptr_t)&ret0 },
        { "eglGetConfigAttrib", (uintptr_t)&eglGetConfigAttrib },
        { "eglGetCurrentContext", (uintptr_t)&ret0 },
        { "eglGetCurrentSurface", (uintptr_t)&ret0 },
        { "eglGetDisplay", (uintptr_t)&eglGetDisplay },
        { "eglGetError", (uintptr_t)&eglGetError },
        { "eglGetProcAddress", (uintptr_t)&eglGetProcAddress },
        { "eglInitialize", (uintptr_t)&eglInitialize },
        { "eglMakeCurrent", (uintptr_t)&ret1 },
        { "eglQuerySurface", (uintptr_t)&eglQuerySurface },
        { "eglSwapBuffers", (uintptr_t)&eglSwapBuffers_wrap },
        { "eglTerminate", (uintptr_t)&ret0 },
        { "exit", (uintptr_t)&exit },
        { "exp", (uintptr_t)&exp },
        { "fclose", (uintptr_t)&fclose },
        { "fdopen", (uintptr_t)&fdopen },
        { "fflush", (uintptr_t)&fflush },
        { "fgets", (uintptr_t)&fgets },
        { "floor", (uintptr_t)&floor },
        { "floorf", (uintptr_t)&floorf },
        { "fmod", (uintptr_t)&fmod },
        { "fmodf", (uintptr_t)&fmodf },
        { "fopen", (uintptr_t)&fopen_soloader },
        { "fputc", (uintptr_t)&fputc },
        { "fputs", (uintptr_t)&fputs },
        { "fread", (uintptr_t)&fread },
        { "free", (uintptr_t)&free },
        { "fseek", (uintptr_t)&fseek },
        { "fstat", (uintptr_t)&fstat },
        { "ftell", (uintptr_t)&ftell },
        { "fwrite", (uintptr_t)&fwrite },
        { "getc", (uintptr_t)&getc },
        { "getwc", (uintptr_t)&getwc },
        { "glActiveTexture", (uintptr_t)&glActiveTexture },
        { "glAttachShader", (uintptr_t)&glAttachShader_wrap },
        { "glBindAttribLocation", (uintptr_t)&glBindAttribLocation },
        { "glBindBuffer", (uintptr_t)&glBindBuffer },
        { "glBindFramebuffer", (uintptr_t)&glBindFramebuffer },
        { "glBindTexture", (uintptr_t)&glBindTexture },
        { "glBlendEquationSeparate", (uintptr_t)&glBlendEquationSeparate },
        { "glBlendFunc", (uintptr_t)&glBlendFunc },
        { "glBlendFuncSeparate", (uintptr_t)&glBlendFuncSeparate },
        { "glBufferData", (uintptr_t)&glBufferData },
        { "glBufferSubData", (uintptr_t)&glBufferSubData },
        { "glCheckFramebufferStatus", (uintptr_t)&glCheckFramebufferStatus },
        { "glClear", (uintptr_t)&glClear },
        { "glClearColor", (uintptr_t)&glClearColor },
        { "glClearDepthf", (uintptr_t)&glClearDepthf },
        { "glClearStencil", (uintptr_t)&glClearStencil },
        { "glColorMask", (uintptr_t)&glColorMask },
        { "glCompileShader", (uintptr_t)&glCompileShader_wrap },
        { "glCompressedTexImage2D", (uintptr_t)&glCompressedTexImage2D },
        { "glCreateProgram", (uintptr_t)&glCreateProgram_wrap },
        { "glCreateShader", (uintptr_t)&glCreateShader },
        { "glCullFace", (uintptr_t)&glCullFace },
        { "glDeleteBuffers", (uintptr_t)&glDeleteBuffers },
        { "glDeleteFramebuffers", (uintptr_t)&glDeleteFramebuffers },
        { "glDeleteTextures", (uintptr_t)&glDeleteTextures },
        { "glDepthFunc", (uintptr_t)&glDepthFunc },
        { "glDepthMask", (uintptr_t)&glDepthMask },
        { "glDisable", (uintptr_t)&glDisable },
        { "glDisableVertexAttribArray", (uintptr_t)&glDisableVertexAttribArray },
        { "glDrawArrays", (uintptr_t)&glDrawArrays },
        { "glDrawElements", (uintptr_t)&glDrawElements },
        { "glEnable", (uintptr_t)&glEnable },
        { "glEnableVertexAttribArray", (uintptr_t)&glEnableVertexAttribArray },
        { "glFramebufferTexture2D", (uintptr_t)&glFramebufferTexture2D },
        { "glFrontFace", (uintptr_t)&glFrontFace },
        { "glGenBuffers", (uintptr_t)&glGenBuffers },
        { "glGenFramebuffers", (uintptr_t)&glGenFramebuffers },
        { "glGenTextures", (uintptr_t)&glGenTextures },
        { "glGetActiveUniform", (uintptr_t)&glGetActiveUniform },
        { "glGetError", (uintptr_t)&glGetError },
        { "glGetIntegerv", (uintptr_t)&glGetIntegerv },
        { "glGetProgramInfoLog", (uintptr_t)&glGetProgramInfoLog },
        { "glGetProgramiv", (uintptr_t)&glGetProgramiv },
        { "glGetShaderInfoLog", (uintptr_t)&glGetShaderInfoLog },
        { "glGetShaderiv", (uintptr_t)&glGetShaderiv },
        { "glGetString", (uintptr_t)&glGetString },
        { "glGetUniformLocation", (uintptr_t)&glGetUniformLocation },
        { "glLineWidth", (uintptr_t)&glLineWidth },
        { "glLinkProgram", (uintptr_t)&glLinkProgram },
        { "glPixelStorei", (uintptr_t)&glPixelStorei },
        { "glPolygonOffset", (uintptr_t)&glPolygonOffset },
        { "glScissor", (uintptr_t)&glScissor },
        { "glShaderSource", (uintptr_t)&glShaderSourceHook },
        { "glTexImage2D", (uintptr_t)&glTexImage2D },
        { "glTexParameteri", (uintptr_t)&glTexParameteri },
        { "glTexSubImage2D", (uintptr_t)&glTexSubImage2D },
        { "glUniform1f", (uintptr_t)&glUniform1f },
        { "glUniform1fv", (uintptr_t)&glUniform1fv },
        { "glUniform1i", (uintptr_t)&glUniform1i },
        { "glUniform1iv", (uintptr_t)&glUniform1iv },
        { "glUniform2fv", (uintptr_t)&glUniform2fv },
        { "glUniform2iv", (uintptr_t)&glUniform2iv },
        { "glUniform3fv", (uintptr_t)&glUniform3fv },
        { "glUniform3iv", (uintptr_t)&glUniform3iv },
        { "glUniform4fv", (uintptr_t)&glUniform4fv },
        { "glUniform4iv", (uintptr_t)&glUniform4iv },
        { "glUniformMatrix2fv", (uintptr_t)&glUniformMatrix2fv },
        { "glUniformMatrix3fv", (uintptr_t)&glUniformMatrix3fv },
        { "glUniformMatrix4fv", (uintptr_t)&glUniformMatrix4fv },
        { "glUseProgram", (uintptr_t)&glUseProgram_wrap },
        { "glVertexAttribPointer", (uintptr_t)&glVertexAttribPointer },
        { "glViewport", (uintptr_t)&glViewport },
        { "ioctl", (uintptr_t)&ret0 },
        { "iswctype", (uintptr_t)&iswctype },
        { "ldexp", (uintptr_t)&ldexp },
        { "localtime", (uintptr_t)&localtime },
        { "log", (uintptr_t)&log },
        { "logf", (uintptr_t)&logf },
        { "lrand48", (uintptr_t)&lrand48 },
        { "lseek", (uintptr_t)&lseek },
        { "malloc", (uintptr_t)&malloc },
        { "mbrtowc", (uintptr_t)&mbrtowc },
        { "memalign", (uintptr_t)&memalign },
        { "memchr", (uintptr_t)&memchr },
        { "memcmp", (uintptr_t)&memcmp },
        { "memcpy", (uintptr_t)&memcpy },
        { "memmem", (uintptr_t)&memmem },
        { "memmove", (uintptr_t)&memmove },
        { "memset", (uintptr_t)&memset },
        { "mktime", (uintptr_t)&mktime },
        { "nanosleep", (uintptr_t)&nanosleep },
        { "open", (uintptr_t)&open },
        { "pipe", (uintptr_t)&pipe },
        { "poll", (uintptr_t)&ret0 },
        { "pow", (uintptr_t)&pow },
        { "powf", (uintptr_t)&powf },
        { "pthread_attr_init", (uintptr_t)&pthread_attr_init_soloader },
        { "pthread_attr_setdetachstate", (uintptr_t)&pthread_attr_setdetachstate_soloader },
        { "pthread_cond_broadcast", (uintptr_t)&pthread_cond_broadcast_soloader },
        { "pthread_cond_destroy", (uintptr_t)&pthread_cond_destroy_soloader },
        { "pthread_cond_init", (uintptr_t)&pthread_cond_init_soloader },
        { "pthread_cond_wait", (uintptr_t)&pthread_cond_wait_soloader },
        { "pthread_create", (uintptr_t)&pthread_create_soloader },
        { "pthread_exit", (uintptr_t)&pthread_exit },
        { "pthread_getspecific", (uintptr_t)&pthread_getspecific },
        { "pthread_join", (uintptr_t)&pthread_join_soloader },
        { "pthread_key_create", (uintptr_t)&pthread_key_create },
        { "pthread_key_delete", (uintptr_t)&pthread_key_delete },
        { "pthread_mutex_destroy", (uintptr_t)&pthread_mutex_destroy_soloader },
        { "pthread_mutex_init", (uintptr_t)&pthread_mutex_init_soloader },
        { "pthread_mutex_lock", (uintptr_t)&pthread_mutex_lock_soloader },
        { "pthread_mutex_unlock", (uintptr_t)&pthread_mutex_unlock_soloader },
        { "pthread_once", (uintptr_t)&pthread_once },
        { "pthread_self", (uintptr_t)&pthread_self_soloader },
        { "pthread_setspecific", (uintptr_t)&pthread_setspecific },
        { "putc", (uintptr_t)&putc },
        { "putwc", (uintptr_t)&putwc },
        { "qsort", (uintptr_t)&qsort },
        { "raise", (uintptr_t)&raise },
        { "read", (uintptr_t)&read },
        { "realloc", (uintptr_t)&realloc },
        { "remove", (uintptr_t)&remove },
        { "rename", (uintptr_t)&rename },
        { "rint", (uintptr_t)&rint },
        { "rintf", (uintptr_t)&rintf },
        { "sched_yield", (uintptr_t)&sched_yield },
        { "setlocale", (uintptr_t)&ret0 },
        { "setvbuf", (uintptr_t)&setvbuf },
        { "sin", (uintptr_t)&sin },
        { "sinf", (uintptr_t)&sinf },
        { "slCreateEngine", (uintptr_t)&slCreateEngine },
        { "SL_IID_BUFFERQUEUE", (uintptr_t)&SL_IID_BUFFERQUEUE },
        { "SL_IID_DYNAMICINTERFACEMANAGEMENT", (uintptr_t)&SL_IID_DYNAMICINTERFACEMANAGEMENT },
        { "SL_IID_EFFECTSEND", (uintptr_t)&SL_IID_EFFECTSEND },
        { "SL_IID_ENGINE", (uintptr_t)&SL_IID_ENGINE },
        { "SL_IID_PLAY", (uintptr_t)&SL_IID_PLAY },
        { "SL_IID_PLAYBACKRATE", (uintptr_t)&SL_IID_PLAYBACKRATE },
        { "SL_IID_SEEK", (uintptr_t)&SL_IID_SEEK },
        { "SL_IID_VOLUME", (uintptr_t)&SL_IID_VOLUME },
        { "slCreateEngine", (uintptr_t)&slCreateEngine },
        { "sprintf", (uintptr_t)&sprintf },
        { "sqrt", (uintptr_t)&sqrt },
        { "sqrtf", (uintptr_t)&sqrtf },
        { "srand48", (uintptr_t)&srand48 },
        { "sscanf", (uintptr_t)&sscanf },
        { "strcat", (uintptr_t)&strcat },
        { "strchr", (uintptr_t)&strchr },
        { "strcmp", (uintptr_t)&strcmp },
        { "strcoll", (uintptr_t)&strcoll },
        { "strcpy", (uintptr_t)&strcpy },
        { "strerror", (uintptr_t)&strerror },
        { "strftime", (uintptr_t)&strftime },
        { "strlen", (uintptr_t)&strlen },
        { "strncmp", (uintptr_t)&strncmp },
        { "strstr", (uintptr_t)&strstr },
        { "strtod", (uintptr_t)&strtod },
        { "strtol", (uintptr_t)&strtol },
        { "strxfrm", (uintptr_t)&strxfrm },
        { "syscall", (uintptr_t)&ret0 },
        { "tan", (uintptr_t)&tan },
        { "tanf", (uintptr_t)&tanf },
        { "time", (uintptr_t)&time },
        { "towlower", (uintptr_t)&towlower },
        { "towupper", (uintptr_t)&towupper },
        { "ungetc", (uintptr_t)&ungetc },
        { "ungetwc", (uintptr_t)&ungetwc },
        { "vsnprintf", (uintptr_t)&vsnprintf },
        { "vsprintf", (uintptr_t)&vsprintf },
        { "wcrtomb", (uintptr_t)&wcrtomb },
        { "wcscoll", (uintptr_t)&wcscoll },
        { "wcsftime", (uintptr_t)&wcsftime },
        { "wcslen", (uintptr_t)&wcslen },
        { "wcsxfrm", (uintptr_t)&wcsxfrm },
        { "wctob", (uintptr_t)&wctob },
        { "wctype", (uintptr_t)&wctype },
        { "wmemchr", (uintptr_t)&wmemchr },
        { "wmemcmp", (uintptr_t)&wmemcmp },
        { "wmemcpy", (uintptr_t)&wmemcpy },
        { "wmemmove", (uintptr_t)&wmemmove },
        { "wmemset", (uintptr_t)&wmemset },
        { "write", (uintptr_t)&write },
        { "writev", (uintptr_t)&ret0 },
};

void resolve_imports(so_module* mod) {
    __sF_fake[0] = *stdin;
    __sF_fake[1] = *stdout;
    __sF_fake[2] = *stderr;
    so_resolve(mod, default_dynlib, sizeof(default_dynlib), 0);
}
