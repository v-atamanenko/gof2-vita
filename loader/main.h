/*
 * main.c
 *
 * ARMv7 Shared Libraries loader. Baba Is You edition.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2022 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#ifndef SOLOADER_MAIN_H
#define SOLOADER_MAIN_H

#include <android/native_activity.h>
#include "config.h"
#include "utils/utils.h"

#include "so_util.h"

typedef struct android_app {
    void * unk_field1;
    void (* onAppCmd)(struct android_app *, int32_t);
    void (* onInputEvent)(struct android_app *, AInputEvent* ev);
    ANativeActivity * activity;
    void * AConfiguration;
    void * savedState;
    size_t savedStateSize;
    void * ALooper;
    void * inputQueue;
    ANativeWindow * active_native_window;
    void * unk_field2;
    void * unk_field3;
    void * unk_field4;
    void * unk_field5;
    void * unk_field6;
    void * unk_field7;
    pthread_mutex_t * pthrMutex;
    pthread_cond_t * pthrCond;
    int pipe_read_fd;
    int pipe_write_fd;
    void * unk_field8;
    int cmdPollSource_id;
    struct android_app * cmdPollSource_app;
    void (* cmdPollSource_process)(struct android_app *);
    int inputPollSource_id;
    struct android_app * inputPollSource_app;
    void (* inputPollSource_process)(struct android_app *);
    int running;
    void * unk_field9;
    void * unk_field10;
    void * unk_field11;
    void * unk_field12;
    ANativeWindow * pending_native_window;
    void * unk_field13;
    void * unk_field14;
    void * unk_field15;
    void * unk_field16;
} android_app;

extern struct android_app* app;
extern so_module so_mod;

void *game_thread();

#endif // SOLOADER_MAIN_H
