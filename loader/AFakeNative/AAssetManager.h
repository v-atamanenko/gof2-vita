#ifndef ANDROID_ASSET_MANAGER_H
#define ANDROID_ASSET_MANAGER_H

#include <sys/cdefs.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct AAssetManager;
/**
 * {@link AAssetManager} provides access to an application's raw assets by
 * creating {@link AAsset} objects.
 *
 * AAssetManager is a wrapper to the low-level native implementation
 * of the java {@link AAssetManager}, a pointer can be obtained using
 * AAssetManager_fromJava().
 *
 * The asset hierarchy may be examined like a filesystem, using
 * {@link AAssetDir} objects to peruse a single directory.
 *
 * A native {@link AAssetManager} pointer may be shared across multiple threads.
 */
typedef struct AAssetManager AAssetManager;




#ifdef __cplusplus
};
#endif

#endif      // ANDROID_ASSET_MANAGER_H
