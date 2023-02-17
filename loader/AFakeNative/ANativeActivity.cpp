#include <cstdlib>
#include "ANativeActivity.h"

#include <FalsoJNI/FalsoJNI.h>

ANativeActivity * ANativeActivity_create() {
    auto * ret = (ANativeActivity *) malloc(sizeof(ANativeActivity));
    ret->callbacks = (ANativeActivityCallbacks *) malloc(sizeof(ANativeActivityCallbacks));
    ret->env = &jni;
    ret->vm = &jvm;
    ret->clazz = (jclass) 0x42424242;
    ret->internalDataPath = DATA_PATH;
    ret->externalDataPath = DATA_PATH;
    ret->sdkVersion = 14;
    ret->instance = nullptr;

    return ret;
}
