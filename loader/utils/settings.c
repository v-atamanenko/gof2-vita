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

void settings_reset() {
    setting_leftStickDeadZone = 0.11f;
    setting_rightStickDeadZone = 0.11f;
    setting_fpsLock = 0;
    setting_physicalControlsEnabled = true;
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
        fclose(config);
    }
}
