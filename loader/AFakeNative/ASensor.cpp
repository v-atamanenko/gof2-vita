#include <cstdio>

#include "ASensor.h"

int ASensorEventQueue_enableSensor(ASensorEventQueue* queue, ASensor const* sensor) {
    printf("ASensorEventQueue_enableSensor\n");
    //TODO
    return 0;
}

int ASensorEventQueue_disableSensor(ASensorEventQueue* queue, ASensor const* sensor) {
    printf("ASensorEventQueue_disableSensor\n");
    //TODO
    return 0;
}

ssize_t ASensorEventQueue_getEvents(ASensorEventQueue* queue, ASensorEvent* events, size_t count) {
    printf("ASensorEventQueue_getEvents\n");
    //TODO
    return 0;
}

int ASensorEventQueue_setEventRate(ASensorEventQueue* queue, ASensor const* sensor, int32_t usec) {
    printf("ASensorEventQueue_getEvents\n");
    //TODO
    return 0;
}

ASensorEventQueue* ASensorManager_createEventQueue(ASensorManager* manager,
                                                   ALooper* looper, int ident, ALooper_callbackFunc callback, void* data) {
    printf("ASensorEventQueue_getEvents\n");
    //TODO
    return nullptr;
}

ASensor const* ASensorManager_getDefaultSensor(ASensorManager* manager, int type) {
    printf("ASensorEventQueue_getEvents\n");
    //TODO
    return nullptr;
}

ASensorManager* ASensorManager_getInstance() {
    printf("ASensorEventQueue_getEvents\n");
    //TODO
    return nullptr;
}