#ifndef ANDROID_CONFIGURATION_H
#define ANDROID_CONFIGURATION_H

#include "stdint.h"

#include "AAssetManager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct AConfiguration;
/**
 * {@link AConfiguration} is an opaque type used to get and set
 * various subsystem configurations.
 *
 * A {@link AConfiguration} pointer can be obtained using:
 * - AConfiguration_new()
 * - AConfiguration_fromAssetManager()
 */
typedef struct AConfiguration AConfiguration;

/**
 * Create a new AConfiguration, initialized with no values set.
 */
AConfiguration* AConfiguration_new();

/**
 * Free an AConfiguration that was previously created with
 * AConfiguration_new().
 */
void AConfiguration_delete(AConfiguration* config);

/**
 * Create and return a new AConfiguration based on the current configuration in
 * use in the given {@link AAssetManager}.
 */
void AConfiguration_fromAssetManager(AConfiguration* out, AAssetManager* am);

/**
 * Return the current ACONFIGURATION_ORIENTATION_* set in the configuration.
 */
int32_t AConfiguration_getOrientation(AConfiguration* config);

#ifdef __cplusplus
};
#endif

#endif // ANDROID_CONFIGURATION_H
