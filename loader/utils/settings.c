/*
 * utils/settings.c
 *
 * Loader settings that can be set via the companion app.
 *
 * Copyright (C) 2021 TheFloW
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include <stdio.h>
#include <string.h>
#include "settings.h"

#define CONFIG_FILE_PATH DATA_PATH"config.txt"

float setting_leftStickDeadZone;
float setting_rightStickDeadZone;
int setting_fpsLock;
bool setting_physicalControlsEnabled;
bool setting_useHdMod;
bool setting_useRebalanceMod;
bool setting_useRebalanceModOldPrices;

void settings_reset() {
    setting_leftStickDeadZone = 0.11f;
    setting_rightStickDeadZone = 0.11f;
    setting_fpsLock = 0;
    setting_physicalControlsEnabled = true;
    setting_useHdMod = true;
    setting_useRebalanceMod = false;
    setting_useRebalanceModOldPrices = false;
}

void settings_load() {
    settings_reset();

    char buffer[30];
    int value;

    FILE *config = fopen(CONFIG_FILE_PATH, "r");

    if (config) {
        while (EOF != fscanf(config, "%[^ ] %d\n", buffer, &value)) {
            if (strcmp("leftStickDeadZone", buffer) == 0) setting_leftStickDeadZone = ((float)value / 100.f);
            else if (strcmp("rightStickDeadZone", buffer) == 0) setting_rightStickDeadZone = ((float)value / 100.f);
            else if (strcmp("fpsLock", buffer) == 0) setting_fpsLock = value;
            else if (strcmp("physicalControlsEnabled", buffer) == 0) setting_physicalControlsEnabled = (bool)value;
            else if (strcmp("useHdMod", buffer) == 0) setting_useHdMod = (bool)value;
            else if (strcmp("useRebalanceMod", buffer) == 0) setting_useRebalanceMod = (bool)value;
            else if (strcmp("useRebalanceModOldPrices", buffer) == 0) setting_useRebalanceModOldPrices = (bool)value;
        }
        fclose(config);
    }
}

void settings_save() {
    FILE *config = fopen(CONFIG_FILE_PATH, "w+");

    if (config) {
        fprintf(config, "%s %d\n", "leftStickDeadZone", (int)(setting_leftStickDeadZone * 100.f));
        fprintf(config, "%s %d\n", "rightStickDeadZone", (int)(setting_rightStickDeadZone * 100.f));
        fprintf(config, "%s %d\n", "fpsLock", (int)setting_fpsLock);
        fprintf(config, "%s %d\n", "physicalControlsEnabled", (int)setting_physicalControlsEnabled);
        fprintf(config, "%s %d\n", "useHdMod", (int)setting_useHdMod);
        fprintf(config, "%s %d\n", "useRebalanceMod", (int)setting_useRebalanceMod);
        fprintf(config, "%s %d\n", "useRebalanceModOldPrices", (int)setting_useRebalanceModOldPrices);
        fclose(config);
    }
}
