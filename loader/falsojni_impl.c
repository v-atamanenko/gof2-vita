/*
 * falojni_impl.c
 *
 * Implementations for JNI functions and fields to be used by FalsoJNI.
 *
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include <FalsoJNI/FalsoJNI_Impl.h>
#include <FalsoJNI/FalsoJNI_Logger.h>

#include <string.h>

#pragma ide diagnostic ignored "UnusedParameter"

jobject getClassLoader (jmethodID id, va_list args) {
    fjni_log_dbg("called");
    return (jobject) 0x42424242;
}

jobject loadClass (jmethodID id, va_list args) {
    jstring name = va_arg(args, jstring);
    fjni_logv_dbg("loadClass %s", (const char*)name);
    return (jobject) 0x42424242;
}

char device_info[] = "OS_Version:" "4.3.3" "(" "100" ")" "\n"
                     "OS_API_Level:" "14" "\n"
                     "Manufacturer:" "Sony" "\n"
                     "Device:" "PlayStation" "\n"
                     "Model:" "Vita" "\n"
                     "Product:" "PSVita" "\n"
                     "Hardware:" "ARM Cortex-A9" "\n"
                     "CPU_ABI_1:" "armeabi-v7a" "\n"
                     "CPU_ABI_2:" "armeabi-v7a" "\n";

jobject getDeviceInfo(jmethodID id, va_list args) {
    fjni_log_dbg("called");
    return &device_info;
}

float getDisplayDensityInfo(jmethodID id, va_list args) {
    fjni_log_dbg("called");
    return 1.0f;
}

jboolean getNetworkAvailability(jmethodID id, va_list args) {
    fjni_log_dbg("called");
    return JNI_TRUE;
}

jobject getInputDeviceNameByDeviceId(jmethodID id, va_list args) {
    int arg = va_arg(args, int);
    fjni_logv_dbg("device id %i", arg);
    return strdup("Sony PlayStation Vita Built-In Gamepad");
}

void launchBrowser (jmethodID id, va_list args) {
    jstring arg = va_arg(args, jstring);
    fjni_logv_dbg("launch browser with %s", (const char*)arg);
}

void displayToast (jmethodID id, va_list args) {
    jstring arg = va_arg(args, jstring);
    fjni_logv_dbg("displayToast with %s", (const char*)arg);
}

void displayDialog (jmethodID id, va_list args) {
    jstring arg = va_arg(args, jstring);
    fjni_logv_dbg("displayDialog with %s", (const char*)arg);
}

void init (jmethodID id, va_list args) {
    jclass arg = va_arg(args, jclass);
    fjni_logv_dbg("init with 0x%x", (int)arg);
}

void deinit(jmethodID id, va_list args) {
    fjni_log_dbg("called");
}

int initBannerAd(jmethodID id, va_list args) {
    jstring arg1 = va_arg(args, jstring);
    int arg2 = va_arg(args, int);
    int arg3 = va_arg(args, int);
    int arg4 = va_arg(args, int);
    fjni_logv_dbg("initBannerAd with %s, %i, %i, %i", arg1, arg2, arg3, arg4);
    return 1;
}

void displayBannerAd(jmethodID id, va_list args) {
    int arg1 = va_arg(args, int);
    int arg2 = va_arg(args, int);
    int arg3 = va_arg(args, int);
    int arg4 = va_arg(args, int);
    int arg5 = va_arg(args, int);
    fjni_logv_dbg("displayBannerAd with %i, %i, %i, %i, %i", arg1, arg2, arg3, arg4, arg5);
}

void dismissBannerAd(jmethodID id, va_list args) {
    int arg = va_arg(args, int);
    fjni_logv_dbg("dismissBannerAd with %i", arg);
}

jboolean isBannerAdLoaded (jmethodID id, va_list args) {
    int arg = va_arg(args, int);
    fjni_logv_dbg("isBannerAdLoaded with %i", arg);
    return JNI_TRUE;
}

jboolean isBannerAdShowing (jmethodID id, va_list args) {
    int arg = va_arg(args, int);
    fjni_logv_dbg("isBannerAdShowing with %i", arg);
    return JNI_TRUE;
}

int initInterstitialAd(jmethodID id, va_list args) {
    jstring s = va_arg(args, jstring);
    fjni_logv_dbg("initInterstitialAd with %s", s);
    return 1;
}

void displayInterstitialAd(jmethodID id, va_list args) {
    int arg = va_arg(args, int);
    fjni_logv_dbg("displayInterstitialAd with %i", arg);
}

int pollAdClicked(jmethodID id, va_list args) {
    fjni_log_dbg("called");
    return 1;
}

NameToMethodID nameToMethodId[] = {
        { 1,  "getClassLoader", METHOD_TYPE_OBJECT },
        { 2,  "loadClass", METHOD_TYPE_OBJECT },
        { 3,  "getDeviceInfo", METHOD_TYPE_OBJECT },
        { 4,  "getInputDeviceNameByDeviceId", METHOD_TYPE_OBJECT },
        { 5,  "getNetworkAvailability", METHOD_TYPE_BOOLEAN },
        { 6,  "isBannerAdLoaded", METHOD_TYPE_BOOLEAN },
        { 7,  "isBannerAdShowing", METHOD_TYPE_BOOLEAN },
        { 8,  "launchBrowser", METHOD_TYPE_VOID },
        { 9,  "displayToast", METHOD_TYPE_VOID },
        { 10, "displayDialog", METHOD_TYPE_VOID },
        { 11, "init", METHOD_TYPE_VOID },
        { 12, "deinit", METHOD_TYPE_VOID },
        { 13, "displayBannerAd", METHOD_TYPE_VOID },
        { 14, "dismissBannerAd", METHOD_TYPE_VOID },
        { 15, "displayInterstitialAd", METHOD_TYPE_VOID },
        { 16, "initBannerAd", METHOD_TYPE_INT },
        { 17, "initInterstitialAd", METHOD_TYPE_INT },
        { 18, "pollAdClicked", METHOD_TYPE_INT },
        { 19, "getDisplayDensityInfo", METHOD_TYPE_FLOAT }
};

MethodsBoolean methodsBoolean[] = {
        { 5,  getNetworkAvailability },
        { 6,  isBannerAdLoaded },
        { 7,  isBannerAdShowing },
};
MethodsByte methodsByte[] = {};
MethodsChar methodsChar[] = {};
MethodsDouble methodsDouble[] = {};
MethodsFloat methodsFloat[] = {
        { 19, getDisplayDensityInfo }
};
MethodsInt methodsInt[] = {
        { 16, initBannerAd },
        { 17, initInterstitialAd },
        { 18, pollAdClicked },
};
MethodsLong methodsLong[] = {};
MethodsObject methodsObject[] = {
        { 1,  getClassLoader },
        { 2,  loadClass },
        { 3,  getDeviceInfo },
        { 4,  getInputDeviceNameByDeviceId },
};
MethodsShort methodsShort[] = {};
MethodsVoid methodsVoid[] = {
        { 8,  launchBrowser },
        { 9,  displayToast },
        { 10, displayDialog },
        { 11, init },
        { 12, deinit },
        { 13, displayBannerAd },
        { 14, dismissBannerAd },
        { 15, displayInterstitialAd },
};

NameToFieldID nameToFieldId[] = {};

FieldsBoolean fieldsBoolean[] = {};
FieldsByte fieldsByte[] = {};
FieldsChar fieldsChar[] = {};
FieldsDouble fieldsDouble[] = {};
FieldsFloat fieldsFloat[] = {};
FieldsInt fieldsInt[] = {};
FieldsObject fieldsObject[] = {};
FieldsLong fieldsLong[] = {};
FieldsShort fieldsShort[] = {};

__FALSOJNI_IMPL_CONTAINER_SIZES
