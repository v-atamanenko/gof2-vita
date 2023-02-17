#include "AInput.h"
#include "PseudoEpoll.h"
#include "AFakeNative_Utils.h"
#include "AFakeNative/utils/controls.h"

#include <vector>
#include <pthread.h>
#include <cstring>

static AInputQueue * g_AInputQueue = nullptr;

typedef struct inputQueue {
    int mDispatchFd;
    std::vector<ALooper*> mAppLoopers;
    //ALooper * mDispatchLooper;
    //sp<WeakMessageHandler> mHandler;
    //PooledInputEventFactory mPooledInputEventFactory;
    // Guards the pending and finished event vectors
    pthread_mutex_t mLock;
    std::vector<AInputEvent*> mPendingEvents;
    //std::vector<key_value_pair_t<AInputEvent*, bool> > mFinishedEvents;

} inputQueue;

AInputQueue * AInputQueue_create() {
    if (g_AInputQueue) return g_AInputQueue;

    inputQueue iq;
    iq.mDispatchFd = pseudo_eventfd(0, PSEUDO_EFD_NONBLOCK | PSEUDO_EFD_SEMAPHORE);

    if (iq.mDispatchFd < 0) {
        ALOGE("eventfd creation for AInputQueue failed: %s\n", strerror(errno));
    }

    pthread_mutex_init(&iq.mLock, nullptr);

    g_AInputQueue = (AInputQueue *) malloc(sizeof(inputQueue));
    memcpy(g_AInputQueue, &iq, sizeof(inputQueue));

    controls_init(g_AInputQueue);

    return g_AInputQueue;
}

void AInputQueue_attachLooper(AInputQueue* queue, ALooper* looper,
                              int ident, ALooper_callbackFunc callback, void* data) {
    if (!queue || !looper) return;
    auto * q = (inputQueue *) queue;

    pthread_mutex_lock(&q->mLock);

    for (size_t i = 0; i < q->mAppLoopers.size(); i++) {
        if (looper == q->mAppLoopers[i]) {
            pthread_mutex_unlock(&q->mLock);
            return;
        }
    }

    q->mAppLoopers.push_back(looper);
    ALooper_addFd(looper, q->mDispatchFd, ident, ALOOPER_EVENT_INPUT, callback, data);
    pthread_mutex_unlock(&q->mLock);
}

void AInputQueue_detachLooper(AInputQueue* queue) {
    if (!queue) return;
    auto * q = (inputQueue *) queue;

    pthread_mutex_lock(&q->mLock);
    for (size_t i = 0; i < q->mAppLoopers.size(); i++) {
        ALooper_removeFd(q->mAppLoopers[i], q->mDispatchFd);
    }
    q->mAppLoopers.clear();
    pthread_mutex_unlock(&q->mLock);
}

int32_t AInputQueue_getEvent(AInputQueue* queue, AInputEvent** outEvent) {
    if (!queue || !outEvent) return -1;
    auto * q = (inputQueue *) queue;

    pthread_mutex_lock(&q->mLock);
    *outEvent = NULL;
    if (!q->mPendingEvents.empty()) {
        *outEvent = q->mPendingEvents[0];
        q->mPendingEvents.erase(q->mPendingEvents.begin());
    }

    if (q->mPendingEvents.empty()) {
        uint64_t byteread;
        ssize_t nRead;
        do {
            nRead = pseudo_read(q->mDispatchFd, &byteread, sizeof(byteread));
            if (nRead < 0 && errno != EAGAIN) {
                ALOGW("Failed to read from native dispatch pipe: %s", strerror(errno));
            }
        } while (nRead == 8); // reduce eventfd semaphore to 0
    }

    int ret = *outEvent != NULL ? 0 : -EAGAIN;
    pthread_mutex_unlock(&q->mLock);
    return ret;
}

int32_t AInputQueue_preDispatchEvent(AInputQueue* queue, AInputEvent* event) {
    // Never pre-dispatch
    return false;
}

void AInputQueue_finishEvent(AInputQueue* queue, AInputEvent* event, int handled) {
    if (event) free(event);
}

void AInputQueue_enqueueEvent(AInputQueue* queue, AInputEvent* event) {
    if (!queue || !event) return;
    auto * q = (inputQueue *) queue;

    pthread_mutex_lock(&q->mLock);
    q->mPendingEvents.push_back(event);
    if (q->mPendingEvents.size() == 1) {
        uint64_t payload = 1;
        int res = TEMP_FAILURE_RETRY(pseudo_write(q->mDispatchFd, &payload, sizeof(payload)));
        if (res < 0 && errno != EAGAIN) {
            ALOGW("Failed writing to dispatch fd: %s", strerror(errno));
        }
    }
    pthread_mutex_unlock(&q->mLock);
}

/**
 * ========================
 */

AInputEvent *AInputEvent_create(inputEvent *e) {
    auto * ret = (AInputEvent *) malloc(sizeof(inputEvent));
    memcpy(ret, e, sizeof(inputEvent));
    return ret;
}

int32_t AInputEvent_getSource(const AInputEvent* event) {
    if (!event) return AINPUT_SOURCE_UNKNOWN;
    auto * e = (inputEvent *) event;
    return e->source;
}

int32_t AKeyEvent_getAction(const AInputEvent* key_event) {
    if (!key_event) return 0;
    auto * e = (inputEvent *) key_event;
    return e->action;
}

int32_t AKeyEvent_getKeyCode(const AInputEvent* key_event) {
    if (!key_event) return 0;
    auto * e = (inputEvent *) key_event;
    return e->keycode;
}

int32_t AMotionEvent_getAction(const AInputEvent* motion_event) {
    if (!motion_event) return 0;
    auto * e = (inputEvent *) motion_event;
    return e->motion_action;
}

size_t AMotionEvent_getPointerCount(const AInputEvent* motion_event) {
    if (!motion_event) return 0;
    auto * e = (inputEvent *) motion_event;
    return e->motion_ptrcount;
}

int32_t AMotionEvent_getPointerId(const AInputEvent* motion_event, size_t pointer_index) {
    if (!motion_event) return 0;
    auto * e = (inputEvent *) motion_event;
    if (pointer_index >= 10) pointer_index = 9;
    return e->motion_ptridx[pointer_index];
}

float AMotionEvent_getX(const AInputEvent* motion_event, size_t pointer_index) {
    if (!motion_event) return 0;
    auto * e = (inputEvent *) motion_event;
    if (pointer_index >= 10) pointer_index = 9;
    return e->motion_x[pointer_index];
}

float AMotionEvent_getY(const AInputEvent* motion_event, size_t pointer_index) {
    if (!motion_event) return 0;
    auto * e = (inputEvent *) motion_event;
    if (pointer_index >= 10) pointer_index = 9;
    return e->motion_y[pointer_index];
}
