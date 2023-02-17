/*
 * utils/controls.cpp
 *
 * Copyright (C) 2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "controls.h"

#include <FalsoJNI/FalsoJNI.h>
#include <psp2/kernel/threadmgr.h>
#include <pthread.h>
#include <stdio.h>
#include <cstring>
#include "../keycodes.h"
#include "../AInput.h"

#define L_OUTER_DEADZONE 0.992f
#define R_OUTER_DEADZONE 1.0f

AInputQueue * inputQueue;

float lerp(float x1, float y1, float x3, float y3, float x2) {
    return ((x2-x1)*(y3-y1) / (x3-x1)) + y1;
}

float coord_normalize(float val, float deadzone_min, float deadzone_max) {
    float sign = 1.0f;
    if (val < 0) sign = -1.0f;

    if (fabsf(val) < deadzone_min) return 0.f;
    if (fabsf(val) > deadzone_max) return 1.0f*sign;
    return lerp(0.f, deadzone_min * sign, 1.0f*sign, deadzone_max*sign, val);
}

void controls_init(AInputQueue * queue) {
    // Enable analog sticks and touchscreen
    sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG_WIDE);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);

    sceMotionStartSampling();

    inputQueue = queue;

    pthread_t t;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 32*1024);
    pthread_create(&t, &attr, controls_poll, NULL);
    pthread_detach(t);
}

void * controls_poll(void * arg) {
    while (1) {
        pollTouch();
        //pollPad();
        //pollAccel();
        sceKernelDelayThread(15000);
    }
}

SceTouchData touch_old;
SceTouchData touch;
inputEvent ev;
int numPointersDown = 0;

int getIdxById(inputEvent * ev, int id) {
    for (int i = 0; i < ev->motion_ptrcount; ++i) {
        if (ev->motion_ptridx[i] == id) {
            return i;
        }
    }
    return -1;
}

void removeById(inputEvent * ev, int id) {
    int idx = getIdxById(ev, id);

    inputEvent ev_backup = *ev;
    memset(ev->motion_ptridx, 0, sizeof(ev->motion_ptridx));
    memset(ev->motion_x, 0, sizeof(ev->motion_x));
    memset(ev->motion_y, 0, sizeof(ev->motion_y));
    ev->motion_ptrcount--;

    int u = 0;
    for (int i = 0; i < ev_backup.motion_ptrcount; ++i) {
        if (i == idx) continue;
        ev->motion_ptridx[u] = ev_backup.motion_ptridx[i];
        ev->motion_x[u] = ev_backup.motion_x[i];
        ev->motion_y[u] = ev_backup.motion_y[i];
        u++;
    }
}

void pollTouch() {
    int finger_id = 0;

    memcpy(&touch_old, &touch, sizeof(touch_old));

    int numPointersMoved = 0;

    sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
    if (touch.reportNum > 0) {
        for (int i = 0; i < touch.reportNum; i++) {
            int finger_down = 0;

            if (touch_old.reportNum > 0) {
                for (int j = 0; j < touch_old.reportNum; j++) {
                    if (touch.report[i].id == touch_old.report[j].id) {
                        finger_down = 1;
                    }
                }
            }

            float x = ((float)touch.report[i].x * 960.f / 1920.0f);
            float y = ((float)touch.report[i].y * 960.f / 1920.0f);

            finger_id = touch.report[i].id;

            // Send touch down event only if finger wasn't already down before
            if (!finger_down) {
                ev.source = AINPUT_SOURCE_TOUCHSCREEN;
                ev.motion_ptrcount = numPointersDown + 1;
                ev.motion_x[numPointersDown] = x;
                ev.motion_y[numPointersDown] = y;
                ev.motion_ptridx[numPointersDown] = finger_id;

                // Get global event state to have up-to-date indices and coordinates,
                // but send a copy to not send MOVE too early / too often
                inputEvent ev_ptrdown = ev;
                ev_ptrdown.motion_action = (numPointersDown == 0) ? AMOTION_EVENT_ACTION_DOWN : AMOTION_EVENT_ACTION_POINTER_DOWN;

                numPointersDown++;
                AInputEvent* aie = AInputEvent_create(&ev_ptrdown);
                AInputQueue_enqueueEvent(inputQueue, aie);
            }
            // Otherwise, send touch move
            else {
                int idx = getIdxById(&ev, finger_id);
                if (idx != -1) {
                    ev.motion_x[idx] = x;
                    ev.motion_y[idx] = y;
                    numPointersMoved++;
                }
            }
        }
    }

    if (numPointersMoved > 0) {
        ev.motion_action = AMOTION_EVENT_ACTION_MOVE;

        AInputEvent* aie = AInputEvent_create(&ev);
        AInputQueue_enqueueEvent(inputQueue, aie);
    }

    // some fingers might have been let go
    if (touch_old.reportNum > 0) {
        for (int i = 0; i < touch_old.reportNum; i++) {
            int finger_up = 1;
            if (touch.reportNum > 0) {
                for (int j = 0; j < touch.reportNum; j++) {
                    if (touch.report[j].id == touch_old.report[i].id) {
                        finger_up = 0;
                    }
                }
            }

            if (finger_up == 1) {
                float x = ((float)touch_old.report[i].x * 960.f / 1920.0f);
                float y = ((float)touch_old.report[i].y * 960.f / 1920.0f);
                finger_id = touch_old.report[i].id;

                int idx = getIdxById(&ev, finger_id);
                if (idx != -1) {
                    ev.motion_x[idx] = x;
                    ev.motion_y[idx] = y;
                    ev.motion_action = (numPointersDown == 1) ? AMOTION_EVENT_ACTION_UP
                                                                      : AMOTION_EVENT_ACTION_POINTER_UP;
                    numPointersDown--;

                    AInputEvent* aie = AInputEvent_create(&ev);
                    AInputQueue_enqueueEvent(inputQueue, aie);

                    removeById(&ev, finger_id);
                }
            }
        }
    }
}

static ButtonMapping mapping[] = {
        { SCE_CTRL_UP,        AKEYCODE_DPAD_UP },
        { SCE_CTRL_DOWN,      AKEYCODE_DPAD_DOWN },
        { SCE_CTRL_LEFT,      AKEYCODE_DPAD_LEFT },
        { SCE_CTRL_RIGHT,     AKEYCODE_DPAD_RIGHT },
        { SCE_CTRL_CROSS,     AKEYCODE_DPAD_CENTER },
        { SCE_CTRL_CIRCLE,    AKEYCODE_BACK },
        { SCE_CTRL_SQUARE,    AKEYCODE_BUTTON_X },
        { SCE_CTRL_TRIANGLE,  AKEYCODE_BUTTON_Y },
        { SCE_CTRL_L1,        AKEYCODE_BUTTON_L1 },
        { SCE_CTRL_R1,        AKEYCODE_BUTTON_R1 },
        { SCE_CTRL_START,     AKEYCODE_BUTTON_START },
        { SCE_CTRL_SELECT,    AKEYCODE_BUTTON_SELECT },
};

uint32_t old_buttons = 0, current_buttons = 0, pressed_buttons = 0, released_buttons = 0;

float lastLx = 0.0f, lastLy = 0.0f, lastRx = 0.0f, lastRy = 0.0f;
float lastLastLx = 0.0f, lastLastLy = 0.0f, lastLastRx = 0.0f, lastLastRy = 0.0f;
float lx = 0.0f, ly = 0.0f, rx = 0.0f, ry = 0.0f;
/*
void pollPad() {
    SceCtrlData pad;
    sceCtrlPeekBufferPositiveExt2(0, &pad, 1);

    { // Gamepad buttons
        old_buttons = current_buttons;
        current_buttons = pad.buttons;
        pressed_buttons = current_buttons & ~old_buttons;
        released_buttons = ~current_buttons & old_buttons;

        for (int i = 0; i < sizeof(mapping) / sizeof(ButtonMapping); i++) {
            if (pressed_buttons & mapping[i].sce_button) {
                NativeOnKeyDown(&jni, (void *) 0x42424242, 600, mapping[i].android_button, 1);
            }
            if (released_buttons & mapping[i].sce_button) {
                NativeOnKeyUp(&jni, (void *) 0x42424242, 600, mapping[i].android_button, 1);
            }
        }
    }

    // Analog sticks

    lx = coord_normalize(((float)pad.lx - 128.0f) / 128.0f, 0.10, L_OUTER_DEADZONE);
    ly = coord_normalize(((float)pad.ly - 128.0f) / 128.0f, 0.10, L_OUTER_DEADZONE);
    rx = coord_normalize(((float)pad.rx - 128.0f) / 128.0f, 0.10, R_OUTER_DEADZONE);
    ry = coord_normalize(((float)pad.ry - 128.0f) / 128.0f, 0.10, R_OUTER_DEADZONE);

    if ((lx == 0.f && ly == 0.f) && (lastLx == 0.f && lastLy == 0.f) && (lastLastLx != 0.f || lastLastLy != 0.f)) {
        // lstick stop
        NativeDispatchGenericMotionEvent(&jni, (void *) 0x42424242, lx, ly, rx, ry, 2);
    }
    if ((rx == 0.f && ry == 0.f) && (lastRx == 0.f && lastRy == 0.f) && (lastLastRx != 0.f || lastLastRy != 0.f)) {
        // rstick stop
        NativeDispatchGenericMotionEvent(&jni, (void *) 0x42424242, lx, ly, rx, ry, 3);
    }
    if ((lx != 0.f || ly != 0.f) || ((lx == 0.f && ly == 0.f) && (lastLx != 0.f || lastLy != 0.f))) {
        // lstick move
        NativeDispatchGenericMotionEvent(&jni, (void *) 0x42424242, lx, ly, rx, ry, 0);
    }
    if ((rx != 0.f || ry != 0.f) || ((rx == 0.f && ry == 0.f) && (lastRx != 0.f || lastRy != 0.f))) {
        // rstick move
        NativeDispatchGenericMotionEvent(&jni, (void *) 0x42424242, lx, ly, rx, ry, 1);
    }

    lastLastLx = lastLx;
    lastLastLy = lastLy;
    lastLastRx = lastRx;
    lastLastRy = lastRy;

    lastLx = lx;
    lastLy = ly;
    lastRx = rx;
    lastRy = ry;
}

void pollAccel() {
    SceMotionSensorState sensor;
    sceMotionGetSensorState(&sensor, 1);

    float x = sensor.accelerometer.x*GRAVITY_CONSTANT;
    float y = sensor.accelerometer.y*GRAVITY_CONSTANT;
    float z = sensor.accelerometer.z*GRAVITY_CONSTANT;
    NativeOnAcceleration(&jni, (void*)0x42424242, x, y, z);
}
*/