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
#include <psp2/kernel/clib.h>
#include "../keycodes.h"
#include "../AInput.h"

extern "C" {
    float L_INNER_DEADZONE __attribute__((weak)) = 0.11f;
    float R_INNER_DEADZONE __attribute__((weak)) = 0.11f;

    int AInput_enableLeftStick __attribute__((weak)) = 0;
    int AInput_enableRightStick __attribute__((weak)) = 0;

    // GoF2-specific hack: send a quick singular touch after we release the
    // stick to avoid the bug with fast forward
    extern void sendFakeTouch();
}

#define L_OUTER_DEADZONE 0.99f
#define R_OUTER_DEADZONE 0.99f

#define TOUCHPAD_X_RADIUS 180
#define TOUCHPAD_Y_RADIUS 180

#define TOUCHPAD_LX_BASE TOUCHPAD_X_RADIUS + 20
#define TOUCHPAD_LY_BASE TOUCHPAD_Y_RADIUS
#define TOUCHPAD_RX_BASE ( 960 - TOUCHPAD_LX_BASE )
#define TOUCHPAD_RY_BASE TOUCHPAD_LY_BASE

#define LSTICK_PTR_ID 0
#define RSTICK_PTR_ID 1


AInputQueue * inputQueue;

float lerp(float x1, float y1, float x3, float y3, float x2) {
    return ((x2-x1)*(y3-y1) / (x3-x1)) + y1;
}

float coord_normalize(float val, float deadzone_min, float deadzone_max) {
    float sign = (val < 0) ? -1.0f : 1.0f;

    if (fabsf(val) < deadzone_min) return 0.f;
    if (fabsf(val) > deadzone_max) return 1.0f*sign;
    return lerp(0.f, deadzone_min * sign, 1.0f*sign, deadzone_max*sign, val);
}

void controls_init(AInputQueue * queue) {
    // Enable analog sticks and touchscreen
    sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG_WIDE);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);

    inputQueue = queue;

    pthread_t t;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 32*1024);
    pthread_create(&t, &attr, controls_poll, nullptr);
    pthread_detach(t);
}

void * controls_poll(void * arg) {
    while (1) {
        pollTouch();
        pollPad();
        sceKernelDelayThread(15000);
    }
}

SceTouchData touch_old;
SceTouchData touch;
inputEvent ev;
int numPointersDown = 0;

int getIdxById(inputEvent * e, int id) {
    for (int i = 0; i < e->motion_ptrcount; ++i) {
        if (e->motion_ptridx[i] == id) {
            return i;
        }
    }
    return -1;
}

void removeById(inputEvent * e, int id) {
    int idx = getIdxById(e, id);

    inputEvent ev_backup = *e;
    memset(e->motion_ptridx, 0, sizeof(e->motion_ptridx));
    memset(e->motion_x, 0, sizeof(e->motion_x));
    memset(e->motion_y, 0, sizeof(e->motion_y));
    e->motion_ptrcount--;

    int u = 0;
    for (int i = 0; i < ev_backup.motion_ptrcount; ++i) {
        if (i == idx) continue;
        e->motion_ptridx[u] = ev_backup.motion_ptridx[i];
        e->motion_x[u] = ev_backup.motion_x[i];
        e->motion_y[u] = ev_backup.motion_y[i];
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
                if (numPointersDown == 0) {
                    ev_ptrdown.motion_action = AMOTION_EVENT_ACTION_DOWN;
                } else {
                    // For Pointer* actions, we have to set pointer index in respective bits
                    ev_ptrdown.motion_action = AMOTION_EVENT_ACTION_POINTER_DOWN | (numPointersDown << AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
                }

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

                    if (numPointersDown == 1) {
                        ev.motion_action = AMOTION_EVENT_ACTION_UP;
                    } else {
                        // For Pointer* actions, we have to set pointer index in respective bits
                        ev.motion_action = AMOTION_EVENT_ACTION_POINTER_UP | (idx << AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
                    }

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
float lx = 0, ly = 0, rx = 0, ry = 0, lastLx = 0, lastLy = 0, lastRx = 0, lastRy = 0;

inputEvent stickInputEvent;
int sticksDown = 0;

void sendTouchpadDown(float x, float y, int id) {
    stickInputEvent.source = AINPUT_SOURCE_TOUCHPAD;
    stickInputEvent.motion_ptrcount = sticksDown + 1;
    stickInputEvent.motion_x[sticksDown] = x;
    stickInputEvent.motion_y[sticksDown] = y;
    stickInputEvent.motion_ptridx[sticksDown] = id;

    // Get global event state to have up-to-date indices and coordinates,
    // but send a copy to not send MOVE too early / too often
    inputEvent ev_ptrdown = stickInputEvent;
    if (sticksDown <= 0) {
        ev_ptrdown.motion_action = AMOTION_EVENT_ACTION_DOWN;
    } else {
        // For Pointer* actions, we have to set pointer index in respective bits
        ev_ptrdown.motion_action = AMOTION_EVENT_ACTION_POINTER_DOWN | (sticksDown << AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
    }

    sticksDown++;
    AInputEvent* aie = AInputEvent_create(&ev_ptrdown);
    AInputQueue_enqueueEvent(inputQueue, aie);
}

void sendTouchpadUp(float x, float y, int id) {
    int idx = getIdxById(&stickInputEvent, id);
    if (idx != -1) {
        stickInputEvent.motion_x[idx] = x;
        stickInputEvent.motion_y[idx] = y;

        if (sticksDown <= 1) {
            ev.motion_action = AMOTION_EVENT_ACTION_UP;
        } else {
            // For Pointer* actions, we have to set pointer index in respective bits
            ev.motion_action = AMOTION_EVENT_ACTION_POINTER_UP | (idx << AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
        }

        sticksDown--;

        AInputEvent *aie = AInputEvent_create(&stickInputEvent);
        AInputQueue_enqueueEvent(inputQueue, aie);

        removeById(&stickInputEvent, id);

        // GoF2-specific hack
        if (ev.motion_action == AMOTION_EVENT_ACTION_UP) {
            sendFakeTouch();
        }
    }
}

void pollPad() {
    SceCtrlData pad;
    sceCtrlPeekBufferPositiveExt2(0, &pad, 1);

    old_buttons = current_buttons;
    current_buttons = pad.buttons;
    pressed_buttons = current_buttons & ~old_buttons;
    released_buttons = ~current_buttons & old_buttons;

    for (auto & i : mapping) {
        if (pressed_buttons & i.sce_button) {
            inputEvent e;
            e.source = AINPUT_SOURCE_KEYBOARD; // Warning: some games may want distinction between AINPUT_SOURCE_KEYBOARD and AINPUT_SOURCE_DPAD
            e.keycode = i.android_button;
            e.action = AKEY_EVENT_ACTION_DOWN;

            AInputEvent* aie = AInputEvent_create(&e);
            AInputQueue_enqueueEvent(inputQueue, aie);
        } else if (released_buttons & i.sce_button) {
            inputEvent e;
            e.source = AINPUT_SOURCE_KEYBOARD; // Warning: some games may want distinction between AINPUT_SOURCE_KEYBOARD and AINPUT_SOURCE_DPAD
            e.keycode = i.android_button;
            e.action = AKEY_EVENT_ACTION_UP;

            AInputEvent* aie = AInputEvent_create(&e);
            AInputQueue_enqueueEvent(inputQueue, aie);
        }
    }

    lastLx = lx;
    lastLy = ly;
    lastRx = rx;
    lastRy = ry;

    lx = coord_normalize(((float)pad.lx - 128.0f) / 128.0f, L_INNER_DEADZONE, L_OUTER_DEADZONE);
    ly = coord_normalize(((float)pad.ly - 128.0f) / 128.0f, L_INNER_DEADZONE, L_OUTER_DEADZONE) * (-1);
    rx = coord_normalize(((float)pad.rx - 128.0f) / 128.0f, R_INNER_DEADZONE, R_OUTER_DEADZONE);
    ry = coord_normalize(((float)pad.ry - 128.0f) / 128.0f, R_INNER_DEADZONE, R_OUTER_DEADZONE) * (-1);

    // Here we simulate how Xperia touchpads work. They are technically one wide
    // touchpad. Let's say that left stick is always pointer id 0, and right stick is pointer id 1.
    // Remember that ids and indexes are different things, so indexes still can be any.

    if (lastLx == 0 && lastLy == 0 && (lx != 0 || ly != 0)) {
        if (AInput_enableLeftStick)
            sendTouchpadDown(TOUCHPAD_LX_BASE, TOUCHPAD_LY_BASE, LSTICK_PTR_ID);
    }

    if (lastRx == 0 && lastRy == 0 && (rx != 0 || ry != 0)) {
        if (AInput_enableRightStick)
            sendTouchpadDown(TOUCHPAD_RX_BASE, TOUCHPAD_RY_BASE, RSTICK_PTR_ID);
    }

    int numPointersMoved = 0;

    if ((lastLx != 0 || lastLy != 0) && (lx != 0 || ly != 0)) {
        int idx = getIdxById(&stickInputEvent, LSTICK_PTR_ID);
        if (idx != -1 && AInput_enableLeftStick) {
            stickInputEvent.motion_x[idx] = TOUCHPAD_LX_BASE + (TOUCHPAD_X_RADIUS * lx);
            stickInputEvent.motion_y[idx] = TOUCHPAD_LY_BASE + (TOUCHPAD_Y_RADIUS * ly);
            numPointersMoved++;
        }
    }

    if ((lastRx != 0 || lastRy != 0) && (rx != 0 || ry != 0)) {
        int idx = getIdxById(&stickInputEvent, RSTICK_PTR_ID);
        if (idx != -1 && AInput_enableRightStick) {
            stickInputEvent.motion_x[idx] = TOUCHPAD_RX_BASE + (TOUCHPAD_X_RADIUS * rx);
            stickInputEvent.motion_y[idx] = TOUCHPAD_RY_BASE + (TOUCHPAD_Y_RADIUS * ry);
            numPointersMoved++;
        }
    }

    if (numPointersMoved > 0) {
        stickInputEvent.motion_action = AMOTION_EVENT_ACTION_MOVE;

        AInputEvent* aie = AInputEvent_create(&stickInputEvent);
        AInputQueue_enqueueEvent(inputQueue, aie);
    }

    if ((lastLx != 0 || lastLy != 0) && (lx == 0 && ly == 0)) {
        if (AInput_enableLeftStick)
            sendTouchpadUp(TOUCHPAD_LX_BASE, TOUCHPAD_LY_BASE, LSTICK_PTR_ID);
    }

    if ((lastRx != 0 || lastRy != 0) && (rx == 0 && ry == 0)) {
        if (AInput_enableRightStick)
            sendTouchpadUp(TOUCHPAD_RX_BASE, TOUCHPAD_RY_BASE, RSTICK_PTR_ID);
    }
}
