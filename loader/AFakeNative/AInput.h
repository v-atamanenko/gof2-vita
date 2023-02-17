#ifndef _ANDROID_INPUT_H
#define _ANDROID_INPUT_H

#include <sys/cdefs.h>
#include <stdint.h>

#include "ALooper.h"
#include "keycodes.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Key event actions.
 */
enum {
    /** The key has been pressed down. */
    AKEY_EVENT_ACTION_DOWN = 0,

    /** The key has been released. */
    AKEY_EVENT_ACTION_UP = 1,

    /**
     * Multiple duplicate key events have occurred in a row, or a
     * complex string is being delivered.  The repeat_count property
     * of the key event contains the number of times the given key
     * code should be executed.
     */
    AKEY_EVENT_ACTION_MULTIPLE = 2
};


/**
 * Bit shift for the action bits holding the pointer index as
 * defined by AMOTION_EVENT_ACTION_POINTER_INDEX_MASK.
 */
#define AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT 8

/** Motion event actions */
enum {
    /** Bit mask of the parts of the action code that are the action itself. */
    AMOTION_EVENT_ACTION_MASK = 0xff,

    /**
     * Bits in the action code that represent a pointer index, used with
     * AMOTION_EVENT_ACTION_POINTER_DOWN and AMOTION_EVENT_ACTION_POINTER_UP.  Shifting
     * down by AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT provides the actual pointer
     * index where the data for the pointer going up or down can be found.
     */
    AMOTION_EVENT_ACTION_POINTER_INDEX_MASK = 0xff00,

    /** A pressed gesture has started, the motion contains the initial starting location. */
    AMOTION_EVENT_ACTION_DOWN = 0,

    /**
     * A pressed gesture has finished, the motion contains the final release location
     * as well as any intermediate points since the last down or move event.
     */
    AMOTION_EVENT_ACTION_UP = 1,

    /**
     * A change has happened during a press gesture (between AMOTION_EVENT_ACTION_DOWN and
     * AMOTION_EVENT_ACTION_UP).  The motion contains the most recent point, as well as
     * any intermediate points since the last down or move event.
     */
    AMOTION_EVENT_ACTION_MOVE = 2,

    /**
     * The current gesture has been aborted.
     * You will not receive any more points in it.  You should treat this as
     * an up event, but not perform any action that you normally would.
     */
    AMOTION_EVENT_ACTION_CANCEL = 3,

    /**
     * A movement has happened outside of the normal bounds of the UI element.
     * This does not provide a full gesture, but only the initial location of the movement/touch.
     */
    AMOTION_EVENT_ACTION_OUTSIDE = 4,

    /**
     * A non-primary pointer has gone down.
     * The bits in AMOTION_EVENT_ACTION_POINTER_INDEX_MASK indicate which pointer changed.
     */
    AMOTION_EVENT_ACTION_POINTER_DOWN = 5,

    /**
     * A non-primary pointer has gone up.
     * The bits in AMOTION_EVENT_ACTION_POINTER_INDEX_MASK indicate which pointer changed.
     */
    AMOTION_EVENT_ACTION_POINTER_UP = 6,

    /**
     * A change happened but the pointer is not down (unlike AMOTION_EVENT_ACTION_MOVE).
     * The motion contains the most recent point, as well as any intermediate points since
     * the last hover move event.
     */
    AMOTION_EVENT_ACTION_HOVER_MOVE = 7,

    /**
     * The motion event contains relative vertical and/or horizontal scroll offsets.
     * Use getAxisValue to retrieve the information from AMOTION_EVENT_AXIS_VSCROLL
     * and AMOTION_EVENT_AXIS_HSCROLL.
     * The pointer may or may not be down when this event is dispatched.
     * This action is always delivered to the winder under the pointer, which
     * may not be the window currently touched.
     */
    AMOTION_EVENT_ACTION_SCROLL = 8,

    /** The pointer is not down but has entered the boundaries of a window or view. */
    AMOTION_EVENT_ACTION_HOVER_ENTER = 9,

    /** The pointer is not down but has exited the boundaries of a window or view. */
    AMOTION_EVENT_ACTION_HOVER_EXIT = 10,

    /* One or more buttons have been pressed. */
    AMOTION_EVENT_ACTION_BUTTON_PRESS = 11,

    /* One or more buttons have been released. */
    AMOTION_EVENT_ACTION_BUTTON_RELEASE = 12,
};


/**
 * Input source masks.
 *
 * Refer to the documentation on android.view.InputDevice for more details about input sources
 * and their correct interpretation.
 */
enum {
    /** mask */
    AINPUT_SOURCE_CLASS_MASK = 0x000000ff,

    /** none */
    AINPUT_SOURCE_CLASS_NONE = 0x00000000,
    /** button */
    AINPUT_SOURCE_CLASS_BUTTON = 0x00000001,
    /** pointer */
    AINPUT_SOURCE_CLASS_POINTER = 0x00000002,
    /** navigation */
    AINPUT_SOURCE_CLASS_NAVIGATION = 0x00000004,
    /** position */
    AINPUT_SOURCE_CLASS_POSITION = 0x00000008,
    /** joystick */
    AINPUT_SOURCE_CLASS_JOYSTICK = 0x00000010,
};


/**
 * Input sources.
 */
enum {
    /** unknown */
    AINPUT_SOURCE_UNKNOWN = 0x00000000,

    /** keyboard */
    AINPUT_SOURCE_KEYBOARD = 0x00000100 | AINPUT_SOURCE_CLASS_BUTTON,
    /** dpad */
    AINPUT_SOURCE_DPAD = 0x00000200 | AINPUT_SOURCE_CLASS_BUTTON,
    /** gamepad */
    AINPUT_SOURCE_GAMEPAD = 0x00000400 | AINPUT_SOURCE_CLASS_BUTTON,
    /** touchscreen */
    AINPUT_SOURCE_TOUCHSCREEN = 0x00001000 | AINPUT_SOURCE_CLASS_POINTER,
    /** mouse */
    AINPUT_SOURCE_MOUSE = 0x00002000 | AINPUT_SOURCE_CLASS_POINTER,
    /** stylus */
    AINPUT_SOURCE_STYLUS = 0x00004000 | AINPUT_SOURCE_CLASS_POINTER,
    /** bluetooth stylus */
    AINPUT_SOURCE_BLUETOOTH_STYLUS = 0x00008000 | AINPUT_SOURCE_STYLUS,
    /** trackball */
    AINPUT_SOURCE_TRACKBALL = 0x00010000 | AINPUT_SOURCE_CLASS_NAVIGATION,
    /** mouse relative */
    AINPUT_SOURCE_MOUSE_RELATIVE = 0x00020000 | AINPUT_SOURCE_CLASS_NAVIGATION,
    /** touchpad */
    AINPUT_SOURCE_TOUCHPAD = 0x00100000 | AINPUT_SOURCE_CLASS_POSITION,
    /** navigation */
    AINPUT_SOURCE_TOUCH_NAVIGATION = 0x00200000 | AINPUT_SOURCE_CLASS_NONE,
    /** joystick */
    AINPUT_SOURCE_JOYSTICK = 0x01000000 | AINPUT_SOURCE_CLASS_JOYSTICK,
    /** HDMI */
    AINPUT_SOURCE_HDMI = 0x02000000 | AINPUT_SOURCE_CLASS_BUTTON,
    /** sensor */
    AINPUT_SOURCE_SENSOR = 0x04000000 | AINPUT_SOURCE_CLASS_NONE,
    /** rotary encoder */
    AINPUT_SOURCE_ROTARY_ENCODER = 0x00400000 | AINPUT_SOURCE_CLASS_NONE,

    /** any */
    AINPUT_SOURCE_ANY = 0xffffff00,
};


struct AInputEvent;
/**
 * Input events.
 *
 * Input events are opaque structures.  Use the provided accessors functions to
 * read their properties.
 */
typedef struct AInputEvent AInputEvent;

/**
 * [Non-Standard]: Real undrelyinh structure for AInputEvent, can be used for creating AInputEvent
 */
typedef struct inputEvent {
    int source; // one of AINPUT_SOURCE_* enum

    //key event only
    int action; // one of AKEY_EVENT_ACTION_* enum
    int keycode; // one of AKEYCODE_* enum

    //motion event only. up to 10 pointers [per source]
    int motion_action;
    int motion_ptrcount;
    int motion_ptridx[10];
    float motion_x[10];
    float motion_y[10];
} inputEvent;

/**
 * [Non-Standard]: Create new AInputEvent object
 */
AInputEvent *AInputEvent_create(inputEvent *e);

/** Get the input event source. */
int32_t AInputEvent_getSource(const AInputEvent *event);

/** Get the key event action. */
int32_t AKeyEvent_getAction(const AInputEvent *key_event);

/**
 * Get the key code of the key event.
 * This is the physical key that was pressed, not the Unicode character.
 */
int32_t AKeyEvent_getKeyCode(const AInputEvent *key_event);

/** Get the combined motion event action code and pointer index. */
int32_t AMotionEvent_getAction(const AInputEvent *motion_event);

/**
 * Get the number of pointers of data contained in this event.
 * Always >= 1.
 */
size_t AMotionEvent_getPointerCount(const AInputEvent *motion_event);

/**
 * Get the pointer identifier associated with a particular pointer
 * data index in this event.  The identifier tells you the actual pointer
 * number associated with the data, accounting for individual pointers
 * going up and down since the start of the current gesture.
 */
int32_t AMotionEvent_getPointerId(const AInputEvent *motion_event, size_t pointer_index);

/**
 * Get the current X coordinate of this event for the given pointer index.
 * Whole numbers are pixels; the value may have a fraction for input devices
 * that are sub-pixel precise.
 */
float AMotionEvent_getX(const AInputEvent *motion_event, size_t pointer_index);

/**
 * Get the current Y coordinate of this event for the given pointer index.
 * Whole numbers are pixels; the value may have a fraction for input devices
 * that are sub-pixel precise.
 */
float AMotionEvent_getY(const AInputEvent *motion_event, size_t pointer_index);

struct AInputQueue;
/**
 * Input queue
 *
 * An input queue is the facility through which you retrieve input
 * events.
 */
typedef struct AInputQueue AInputQueue;

/**
 * [Non-Standard]: Create new AInputQueue object
 */
AInputQueue *AInputQueue_create();

/**
 * [Non-Standard]: Enqueue new input event
 */
void AInputQueue_enqueueEvent(AInputQueue *queue, AInputEvent *event);

/**
 * Add this input queue to a looper for processing.  See
 * ALooper_addFd() for information on the ident, callback, and data params.
 */
void AInputQueue_attachLooper(AInputQueue *queue, ALooper *looper,
                              int ident, ALooper_callbackFunc callback, void *data);

/**
 * Remove the input queue from the looper it is currently attached to.
 */
void AInputQueue_detachLooper(AInputQueue *queue);

/**
 * Returns the next available event from the queue.  Returns a negative
 * value if no events are available or an error has occurred.
 */
int32_t AInputQueue_getEvent(AInputQueue *queue, AInputEvent **outEvent);

/**
 * Sends the key for standard pre-dispatching -- that is, possibly deliver
 * it to the current IME to be consumed before the app.  Returns 0 if it
 * was not pre-dispatched, meaning you can process it right now.  If non-zero
 * is returned, you must abandon the current event processing and allow the
 * event to appear again in the event queue (if it does not get consumed during
 * pre-dispatching).
 */
int32_t AInputQueue_preDispatchEvent(AInputQueue *queue, AInputEvent *event);

/**
 * Report that dispatching has finished with the given event.
 * This must be called after receiving an event with AInputQueue_get_event().
 */
void AInputQueue_finishEvent(AInputQueue *queue, AInputEvent *event, int handled);

#ifdef __cplusplus
}
#endif

#endif // _ANDROID_INPUT_H
