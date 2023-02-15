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

// NOLINT(cppcoreguidelines-interfaces-global-init)
so_default_dynlib default_dynlib[] = {
        { "__sF", (uintptr_t)&__sF_fake },
        { "abort", (uintptr_t)&abort },
};

void resolve_imports(so_module* mod) {
    __sF_fake[0] = *stdin;
    __sF_fake[1] = *stdout;
    __sF_fake[2] = *stderr;
    so_resolve(mod, default_dynlib, sizeof(default_dynlib), 0);
}
