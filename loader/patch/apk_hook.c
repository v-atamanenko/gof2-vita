#include "utils/utils.h"
#include "utils/logger.h"

#define ARRLEN(a) (sizeof(a)/sizeof(a[0]))

typedef void * zip_file_t;
typedef void * zip_t;
typedef void * zip_flags_t;

zip_t * (*zip_open)(const char *path, int flags, int *errorp);

zip_t * zip_patch_hd = NULL;
zip_t * zip_patch_rebalance_no_prices = NULL;
zip_t * zip_patch_rebalance = NULL;
zip_t * zip_patch_user = NULL;

// These arrays must be sorted alphabetically
char * zip_patch_hd_files [] = {
    "assets/data/textures/additional/planet_big_0.aei",
    "assets/data/textures/additional/planet_big_1.aei",
    "assets/data/textures/additional/planet_big_10.aei",
    "assets/data/textures/additional/planet_big_11.aei",
    "assets/data/textures/additional/planet_big_12.aei",
    "assets/data/textures/additional/planet_big_13.aei",
    "assets/data/textures/additional/planet_big_14.aei",
    "assets/data/textures/additional/planet_big_15.aei",
    "assets/data/textures/additional/planet_big_16.aei",
    "assets/data/textures/additional/planet_big_17.aei",
    "assets/data/textures/additional/planet_big_18.aei",
    "assets/data/textures/additional/planet_big_19.aei",
    "assets/data/textures/additional/planet_big_2.aei",
    "assets/data/textures/additional/planet_big_3.aei",
    "assets/data/textures/additional/planet_big_4.aei",
    "assets/data/textures/additional/planet_big_5.aei",
    "assets/data/textures/additional/planet_big_6.aei",
    "assets/data/textures/additional/planet_big_7.aei",
    "assets/data/textures/additional/planet_big_8.aei",
    "assets/data/textures/additional/planet_big_9.aei",
    "assets/data/textures/additional/planet_small_0.aei",
    "assets/data/textures/additional/planet_small_1.aei",
    "assets/data/textures/additional/planet_small_10.aei",
    "assets/data/textures/additional/planet_small_11.aei",
    "assets/data/textures/additional/planet_small_12.aei",
    "assets/data/textures/additional/planet_small_13.aei",
    "assets/data/textures/additional/planet_small_14.aei",
    "assets/data/textures/additional/planet_small_15.aei",
    "assets/data/textures/additional/planet_small_16.aei",
    "assets/data/textures/additional/planet_small_17.aei",
    "assets/data/textures/additional/planet_small_18.aei",
    "assets/data/textures/additional/planet_small_19.aei",
    "assets/data/textures/additional/planet_small_2.aei",
    "assets/data/textures/additional/planet_small_3.aei",
    "assets/data/textures/additional/planet_small_4.aei",
    "assets/data/textures/additional/planet_small_5.aei",
    "assets/data/textures/additional/planet_small_6.aei",
    "assets/data/textures/additional/planet_small_7.aei",
    "assets/data/textures/additional/planet_small_8.aei",
    "assets/data/textures/additional/planet_small_9.aei",
    "assets/data/textures/gof2_interface.aei",
    "assets/data/textures/gof2_logos.aei",
    "assets/data/textures/planet_big_0.aei",
    "assets/data/textures/planet_big_1.aei",
    "assets/data/textures/planet_big_10.aei",
    "assets/data/textures/planet_big_11.aei",
    "assets/data/textures/planet_big_12.aei",
    "assets/data/textures/planet_big_13.aei",
    "assets/data/textures/planet_big_14.aei",
    "assets/data/textures/planet_big_15.aei",
    "assets/data/textures/planet_big_16.aei",
    "assets/data/textures/planet_big_17.aei",
    "assets/data/textures/planet_big_18.aei",
    "assets/data/textures/planet_big_19.aei",
    "assets/data/textures/planet_big_2.aei",
    "assets/data/textures/planet_big_3.aei",
    "assets/data/textures/planet_big_4.aei",
    "assets/data/textures/planet_big_5.aei",
    "assets/data/textures/planet_big_6.aei",
    "assets/data/textures/planet_big_7.aei",
    "assets/data/textures/planet_big_8.aei",
    "assets/data/textures/planet_big_9.aei",
    "assets/data/textures/planet_small_0.aei",
    "assets/data/textures/planet_small_1.aei",
    "assets/data/textures/planet_small_10.aei",
    "assets/data/textures/planet_small_11.aei",
    "assets/data/textures/planet_small_12.aei",
    "assets/data/textures/planet_small_13.aei",
    "assets/data/textures/planet_small_14.aei",
    "assets/data/textures/planet_small_15.aei",
    "assets/data/textures/planet_small_16.aei",
    "assets/data/textures/planet_small_17.aei",
    "assets/data/textures/planet_small_18.aei",
    "assets/data/textures/planet_small_19.aei",
    "assets/data/textures/planet_small_2.aei",
    "assets/data/textures/planet_small_3.aei",
    "assets/data/textures/planet_small_4.aei",
    "assets/data/textures/planet_small_5.aei",
    "assets/data/textures/planet_small_6.aei",
    "assets/data/textures/planet_small_7.aei",
    "assets/data/textures/planet_small_8.aei",
    "assets/data/textures/planet_small_9.aei",
    "assets/de.lang",
    "assets/es.lang",
    "assets/fr.lang",
    "assets/gb.lang",
    "assets/it.lang",
    "assets/pl.lang",
    "assets/ru.lang"
};

char * zip_patch_rebalance_no_prices_files [] = {
    "assets/data/bin/items.bin",
    "assets/data/bin/ships.bin"
};

char * zip_patch_rebalance_files [] = {
    "assets/data/bin/items.bin",
    "assets/data/bin/ships.bin"
};

static int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

bool has_patched_file(char *fname, char ** file_list, int file_list_len) {
    char **preload_file = bsearch(&fname, file_list, file_list_len, sizeof(file_list[0]), compare_strings);
    if (preload_file) {
        return true;
    }
    return false;
}

so_hook zip_fopen_hook;
zip_t * zip_fopen(zip_t *archive, const char *fname, zip_flags_t flags) {
    if (zip_patch_user != NULL) {
        // User patch takes precedence over everything
        zip_t * ret = SO_CONTINUE(zip_t *, zip_fopen_hook, zip_patch_user, fname, flags);
        if (ret) return ret;
    }

    if (setting_useHdMod && has_patched_file(fname, zip_patch_hd_files, ARRLEN(zip_patch_hd_files)) && zip_patch_hd != NULL) {
        zip_t * ret = SO_CONTINUE(zip_t *, zip_fopen_hook, zip_patch_hd, fname, flags);
        return ret;
    }

    if (setting_useRebalanceModOldPrices && has_patched_file(fname, zip_patch_rebalance_no_prices_files, ARRLEN(zip_patch_rebalance_no_prices_files)) && zip_patch_rebalance_no_prices != NULL) {
        return SO_CONTINUE(zip_t *, zip_fopen_hook, zip_patch_rebalance_no_prices, fname, flags);
    } else if (setting_useRebalanceMod && has_patched_file(fname, zip_patch_rebalance_files, ARRLEN(zip_patch_rebalance_files)) && zip_patch_rebalance != NULL) {
        return SO_CONTINUE(zip_t *, zip_fopen_hook, zip_patch_rebalance, fname, flags);
    }

    return SO_CONTINUE(zip_t *, zip_fopen_hook, archive, fname, flags);
}

void patch__apk_hook() {
    zip_open = (void *) so_symbol(&so_mod, "zip_open");

    if (file_exists("app0:patch/patch_hd.zip") && setting_useHdMod) {
        zip_patch_hd = zip_open("app0:patch/patch_hd.zip", 0, NULL);
        if (!zip_patch_hd) {
            setting_useHdMod = false;
            logv_error("Failed to open patch zip %s", "app0:patch/patch_hd.zip");
        }
    } else {
        logv_info("HD Patch not enabled. File exists: %i; Setting enabled: %i", file_exists("app0:patch/patch_hd.zip"), setting_useHdMod);
    }

    if (file_exists("app0:patch/patch_rebalance_no_prices.zip") && setting_useRebalanceModOldPrices) {
        zip_patch_rebalance_no_prices = zip_open("app0:patch/patch_rebalance_no_prices.zip", 0, NULL);
        if (!zip_patch_rebalance_no_prices) {
            setting_useRebalanceModOldPrices = false;
            logv_error("Failed to open patch zip %s", "app0:patch/patch_rebalance_no_prices.zip");
        }
    } else if (file_exists("app0:patch/patch_rebalance.zip") && setting_useRebalanceMod) {
        zip_patch_rebalance = zip_open("app0:patch/patch_rebalance.zip", 0, NULL);
        if (!zip_patch_rebalance) {
            setting_useRebalanceMod = false;
            logv_error("Failed to open patch zip %s", "app0:patch/patch_rebalance.zip");
        }
    }

    if (file_exists("app0:patch/patch_user.zip")) {
        zip_patch_user = zip_open("app0:patch/patch_user.zip", 0, NULL);
        if (!zip_patch_user) {
            logv_error("Failed to open patch zip %s", "app0:patch/patch_user.zip");
        }
    }

    zip_fopen_hook = hook_addr(so_symbol(&so_mod, "zip_fopen"), (uintptr_t)zip_fopen);
}
