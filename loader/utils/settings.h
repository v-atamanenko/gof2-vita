/*
 * utils/settings.h
 *
 * Loader settings that can be set via the companion app.
 *
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#ifndef SOLOADER_SETTINGS_H
#define SOLOADER_SETTINGS_H
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

extern float setting_leftStickDeadZone;
extern float setting_rightStickDeadZone;
extern int setting_fpsLock;
extern bool setting_physicalControlsEnabled;
extern bool setting_useHdMod;
extern bool setting_useRebalanceMod;
extern bool setting_useRebalanceModOldPrices;

void settings_load();
void settings_save();
void settings_reset();

#ifdef __cplusplus
};
#endif

#endif // SOLOADER_SETTINGS_H
