#include <cstdlib>
#include <cstring>
#include "AConfiguration.h"

typedef struct _aconfiguration_internal {
    int dummy; // TODO: mb we will need to store something here in future
} _aconfiguration_internal;

AConfiguration* AConfiguration_new() {
    _aconfiguration_internal conf;

    auto ret = (AConfiguration *) malloc(sizeof(_aconfiguration_internal));
    memcpy(ret, &conf, sizeof(_aconfiguration_internal));

    return ret;
};

void AConfiguration_delete(AConfiguration* config) {
    if (config) free(config);
}

void AConfiguration_fromAssetManager(AConfiguration* out, AAssetManager* am) {
    // do nothing (we wrote all values we wanted in _new())?
}

int32_t AConfiguration_getOrientation(AConfiguration* config) {
    /* Possible values :
     * ACONFIGURATION_ORIENTATION_ANY = 0x0000,
     * ACONFIGURATION_ORIENTATION_PORT = 0x0001,
     * ACONFIGURATION_ORIENTATION_LAND = 0x0002 */
    return 0x0002;
}
