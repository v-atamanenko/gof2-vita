#include "AFakeNative/keycodes.h"

void ** Globals__appManager;
void ** gEngine;
int * DAT_00141064;
int * DAT_0014106c;
int * DAT_00141068;
int * DAT_00141060;
float * FLOAT_0013de38;
float * FLOAT_0013de40;
int* Globals__turret_view_y;
int* Globals__turret_view_x;
int* Globals__fire_y;
int* Globals__fire_x;
int* Globals__sec_fire_y;
int* Globals__sec_fire_x;
int* Globals__boost_y;
int* Globals__boost_x;
int* Globals__autopilot_y;
int* Globals__autopilot_x;
int* Globals__fast_forward_y;
int* Globals__fast_forward_x;
int* Globals__pause_y;
int* Globals__pause_x;
int* Globals__action_menu_y;
int* Globals__action_menu_x;
int* Globals__smallButton_dim;
int* Globals__touch_stick_x;
int* Globals__touch_stick_y;

int (*GetCurrentApplicationModule)(void *appManager);

void (*OnTouchBegin)(void * this, int x, int y, int keycode);
void (*OnTouchEnd)(void * this, int x, int y, int keycode);

void (*AddTouch)(int pointerId, int action, int x, int y);

void keyPressed(int keycode) {

    if (GetCurrentApplicationModule(*Globals__appManager) == 2) {
        void * appManager = *(void **)(*(void **)(gEngine) + 0x28);
        switch(keycode) {
            case AKEYCODE_DPAD_UP:
                *DAT_00141064 = 1;
                *FLOAT_0013de38 = -0.1f;
                break;
            case AKEYCODE_DPAD_DOWN:
                *DAT_00141068 = 1;
                *FLOAT_0013de38 = 0.1f;
                break;
            case AKEYCODE_DPAD_LEFT:
                *DAT_00141060 = 1;
                *FLOAT_0013de40 = 0.1f;
                break;
            case AKEYCODE_DPAD_RIGHT:
                *DAT_0014106c = 1;
                *FLOAT_0013de40 = -0.1f;
                break;
            case AKEYCODE_DPAD_CENTER:
                OnTouchBegin(appManager, *Globals__fire_x, *Globals__fire_y, keycode);
                break;
            case AKEYCODE_BACK:
                OnTouchBegin(appManager, *Globals__turret_view_x, *Globals__turret_view_y, keycode);
                break;
            case AKEYCODE_MENU:
            case AKEYCODE_BUTTON_START:
                OnTouchBegin(appManager, *Globals__pause_x, *Globals__pause_y, keycode);
                break;
            case AKEYCODE_BUTTON_X:
                OnTouchBegin(appManager, *Globals__sec_fire_x, *Globals__sec_fire_y, keycode);
                break;
            case AKEYCODE_BUTTON_Y:
                OnTouchBegin(appManager, *Globals__boost_x, *Globals__boost_y, keycode);
                break;
            case AKEYCODE_BUTTON_L1:
                OnTouchBegin(appManager, *Globals__autopilot_x, *Globals__autopilot_y, keycode);
                break;
            case AKEYCODE_BUTTON_R1:
                OnTouchBegin(appManager, 250, 480, keycode);
                break;
            case AKEYCODE_BUTTON_SELECT:
                OnTouchBegin(appManager, *Globals__action_menu_x, *Globals__action_menu_y, keycode);
                break;
        }
    }
}

void keyReleased(int keycode) {
    if (GetCurrentApplicationModule(*Globals__appManager) == 2) {
        void * appManager = *(void **)(*(void **)(gEngine) + 0x28);

        switch(keycode) {
            case AKEYCODE_BACK:
                OnTouchEnd(appManager, *Globals__turret_view_x, *Globals__turret_view_y, keycode);
                break;
            case AKEYCODE_DPAD_UP:
                *DAT_00141064 = 0;
                *FLOAT_0013de38 = 0.5f;
                break;
            case AKEYCODE_DPAD_DOWN:
                *DAT_00141068 = 0;
                *FLOAT_0013de38 = 0.5f;
                break;
            case AKEYCODE_DPAD_LEFT:
                *DAT_00141060 = 0;
                *FLOAT_0013de40 = 0.5f;
                break;
            case AKEYCODE_DPAD_RIGHT:
                *DAT_0014106c = 0;
                *FLOAT_0013de40 = 0.5f;
                break;
            case AKEYCODE_DPAD_CENTER:
                OnTouchEnd(appManager, *Globals__fire_x, *Globals__fire_y, keycode);
                break;
            case AKEYCODE_MENU:
            case AKEYCODE_BUTTON_START:
                OnTouchEnd(appManager, *Globals__pause_x, *Globals__pause_y, keycode);
                break;
            case AKEYCODE_BUTTON_X:
                OnTouchEnd(appManager, *Globals__sec_fire_x, *Globals__sec_fire_y, keycode);
                break;
            case AKEYCODE_BUTTON_Y:
                OnTouchEnd(appManager, *Globals__boost_x, *Globals__boost_y, keycode);
                break;
            case AKEYCODE_BUTTON_L1:
                OnTouchEnd(appManager, *Globals__autopilot_x, *Globals__autopilot_y, keycode);
                break;
            case AKEYCODE_BUTTON_R1:
                OnTouchEnd(appManager, 250, 480, keycode);
                break;
            case AKEYCODE_BUTTON_SELECT:
                OnTouchEnd(appManager, *Globals__action_menu_x, *Globals__action_menu_y, keycode);
                break;
        }
    }
}

void handleTouchPadEvent(int unused, int pointerId, int action, float x, float y) {
    if (GetCurrentApplicationModule(*Globals__appManager) == 2) {
        float uVar4 = (float)x * (float)*Globals__smallButton_dim * 2;
        float uVar3 = (float)y * (float)*Globals__smallButton_dim * 2;

        float fVar8 = uVar4 + ((float)*Globals__touch_stick_x - (float)*Globals__smallButton_dim);
        float fVar7 = uVar3 + ((float)*Globals__touch_stick_y - (float)*Globals__smallButton_dim);

        AddTouch(pointerId, action, (int)fVar8, (int)fVar7);
    }
}

void patch__controls_fix() {
    OnTouchBegin = (void *) so_symbol(&so_mod, "_ZN11AbyssEngine18ApplicationManager12OnTouchBeginEiiPv");
    OnTouchEnd = (void *) so_symbol(&so_mod, "_ZN11AbyssEngine18ApplicationManager10OnTouchEndEiiPv");
    GetCurrentApplicationModule = (void *) so_symbol(&so_mod, "_ZNK11AbyssEngine18ApplicationManager27GetCurrentApplicationModuleEv");
    AddTouch = (void *) so_symbol(&so_mod, "_Z8AddTouchiiii");

    Globals__turret_view_y = (int *) so_symbol(&so_mod, "_ZN7Globals13turret_view_yE");
    Globals__turret_view_x = (int *) so_symbol(&so_mod, "_ZN7Globals13turret_view_xE");
    Globals__fire_y = (int *) so_symbol(&so_mod, "_ZN7Globals6fire_yE");
    Globals__fire_x = (int *) so_symbol(&so_mod, "_ZN7Globals6fire_xE");
    Globals__sec_fire_y = (int *) so_symbol(&so_mod, "_ZN7Globals10sec_fire_yE");
    Globals__sec_fire_x = (int *) so_symbol(&so_mod, "_ZN7Globals10sec_fire_xE");
    Globals__boost_y = (int *) so_symbol(&so_mod, "_ZN7Globals7boost_yE");
    Globals__boost_x = (int *) so_symbol(&so_mod, "_ZN7Globals7boost_xE");
    Globals__autopilot_y = (int *) so_symbol(&so_mod, "_ZN7Globals11autopilot_yE");
    Globals__autopilot_x = (int *) so_symbol(&so_mod, "_ZN7Globals11autopilot_xE");
    Globals__fast_forward_y = (int *) so_symbol(&so_mod, "_ZN7Globals14fast_forward_xE");
    Globals__fast_forward_x = (int *) so_symbol(&so_mod, "_ZN7Globals14fast_forward_yE");
    Globals__pause_y = (int *) so_symbol(&so_mod, "_ZN7Globals7pause_yE");
    Globals__pause_x = (int *) so_symbol(&so_mod, "_ZN7Globals7pause_xE");
    Globals__action_menu_y = (int *) so_symbol(&so_mod, "_ZN7Globals13action_menu_yE");
    Globals__action_menu_x = (int *) so_symbol(&so_mod, "_ZN7Globals13action_menu_xE");
    Globals__smallButton_dim = (int *) so_symbol(&so_mod, "_ZN7Globals15smallButton_dimE");
    Globals__touch_stick_x = (int *) so_symbol(&so_mod, "_ZN7Globals13touch_stick_xE");
    Globals__touch_stick_y = (int *) so_symbol(&so_mod, "_ZN7Globals13touch_stick_yE");

    Globals__appManager = (void *) so_symbol(&so_mod, "_ZN7Globals10appManagerE");
    gEngine = (void *) so_symbol(&so_mod, "gEngine");

    DAT_00141064 = (int *)(so_mod.text_base + 0x00141064);
    DAT_0014106c = (int *)(so_mod.text_base + 0x0014106c);
    DAT_00141068 = (int *)(so_mod.text_base + 0x00141068);
    DAT_00141060 = (int *)(so_mod.text_base + 0x00141060);
    FLOAT_0013de38 = (float *)(so_mod.text_base + 0x0013de38);
    FLOAT_0013de40 = (float *)(so_mod.text_base + 0x0013de40);

    hook_addr(so_symbol(&so_mod, "ndk23_keyPressed"), (uintptr_t)keyPressed);
    hook_addr(so_symbol(&so_mod, "ndk23_keyReleased"), (uintptr_t)keyReleased);
    hook_addr(so_symbol(&so_mod, "ndk23_handleTouchPadEvent"), (uintptr_t)handleTouchPadEvent);
}
