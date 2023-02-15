#ifndef _ANDROID_INPUT_H
#define _ANDROID_INPUT_H

#include <sys/cdefs.h>
#include <stdint.h>

#include "ALooper.h"

struct AInputEvent;
/**
 * Input events.
 *
 * Input events are opaque structures.  Use the provided accessors functions to
 * read their properties.
 */
typedef struct AInputEvent AInputEvent;

/** Get the input event source. */
int32_t AInputEvent_getSource(const AInputEvent* event);

/** Get the key event action. */
int32_t AKeyEvent_getAction(const AInputEvent* key_event);

/**
 * Get the key code of the key event.
 * This is the physical key that was pressed, not the Unicode character.
 */
int32_t AKeyEvent_getKeyCode(const AInputEvent* key_event);

/** Get the combined motion event action code and pointer index. */
int32_t AMotionEvent_getAction(const AInputEvent* motion_event);

/**
 * Get the number of pointers of data contained in this event.
 * Always >= 1.
 */
size_t AMotionEvent_getPointerCount(const AInputEvent* motion_event);

/**
 * Get the pointer identifier associated with a particular pointer
 * data index in this event.  The identifier tells you the actual pointer
 * number associated with the data, accounting for individual pointers
 * going up and down since the start of the current gesture.
 */
int32_t AMotionEvent_getPointerId(const AInputEvent* motion_event, size_t pointer_index);

/**
 * Get the current X coordinate of this event for the given pointer index.
 * Whole numbers are pixels; the value may have a fraction for input devices
 * that are sub-pixel precise.
 */
float AMotionEvent_getX(const AInputEvent* motion_event, size_t pointer_index);

/**
 * Get the current Y coordinate of this event for the given pointer index.
 * Whole numbers are pixels; the value may have a fraction for input devices
 * that are sub-pixel precise.
 */
float AMotionEvent_getY(const AInputEvent* motion_event, size_t pointer_index);

struct AInputQueue;
/**
 * Input queue
 *
 * An input queue is the facility through which you retrieve input
 * events.
 */
typedef struct AInputQueue AInputQueue;

/**
 * Add this input queue to a looper for processing.  See
 * ALooper_addFd() for information on the ident, callback, and data params.
 */
void AInputQueue_attachLooper(AInputQueue* queue, ALooper* looper,
                              int ident, ALooper_callbackFunc callback, void* data);

/**
 * Remove the input queue from the looper it is currently attached to.
 */
void AInputQueue_detachLooper(AInputQueue* queue);

/**
 * Returns the next available event from the queue.  Returns a negative
 * value if no events are available or an error has occurred.
 */
int32_t AInputQueue_getEvent(AInputQueue* queue, AInputEvent** outEvent);

/**
 * Sends the key for standard pre-dispatching -- that is, possibly deliver
 * it to the current IME to be consumed before the app.  Returns 0 if it
 * was not pre-dispatched, meaning you can process it right now.  If non-zero
 * is returned, you must abandon the current event processing and allow the
 * event to appear again in the event queue (if it does not get consumed during
 * pre-dispatching).
 */
int32_t AInputQueue_preDispatchEvent(AInputQueue* queue, AInputEvent* event);

/**
 * Report that dispatching has finished with the given event.
 * This must be called after receiving an event with AInputQueue_get_event().
 */
void AInputQueue_finishEvent(AInputQueue* queue, AInputEvent* event, int handled);



#endif // _ANDROID_INPUT_H
