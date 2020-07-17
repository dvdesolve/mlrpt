/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details:
 *
 *  http://www.gnu.org/copyleft/gpl.txt
 */

/*****************************************************************************/

#include "rc_config.h"

#include "../common/common.h"
#include "../common/shared.h"
#include "../decoder/rectify_meteor.h"
#include "../demodulator/pll.h"
#include "utils.h"

#include <libconfig.h>

#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/*****************************************************************************/

/* Working config file */
char mlrpt_cfg[PATH_MAX + 1];

/* Cache directory */
char mlrpt_img_dir[PATH_MAX + 1];

/*****************************************************************************/

/* loadConfig()
 *
 * Loads the mlrptrc configuration file
 * TODO more detailed error messages (using mesg)
 * TODO use DEFINEd default values
 */
bool loadConfig(void) {
    char mesg[MESG_SIZE];

    /* Initialize string config values */
    memset(rc_data.sat_name, '\0', CFG_STRLEN_MAX);
    memset(rc_data.comment, '\0', CFG_STRLEN_MAX);
    memset(rc_data.device_driver, '\0', CFG_STRLEN_MAX);

    /* Initialize main config object and allow int <-> double convertion */
    config_t cfg;

    config_init(&cfg);
    config_set_options(&cfg, CONFIG_OPTION_AUTOCONVERT);

    /* Try to parse config file */
    if (!config_read_file(&cfg, mlrpt_cfg)) {
        snprintf(mesg, sizeof(mesg),
                "Failed to parse config file!\n%s:%d - %s\n",
                config_error_file(&cfg), config_error_line(&cfg),
                config_error_text(&cfg));

        Print_Message(mesg, ERROR_MESG);

        config_destroy(&cfg);

        return false;
    }

    /* Begin settings readout. Raw values are checked against valid ranges.
     * Default values are substitued right here if no others provided */
    config_setting_t *set_v;
    config_setting_t *arr_v;
    const char *str_v;
    int int_v;
    double flt_v;

    /* Common settings */
    if (config_lookup_string(&cfg, "sat_name", &str_v))
        strncpy(rc_data.sat_name, str_v, CFG_STRLEN_MAX);

    if (config_lookup_string(&cfg, "comment", &str_v))
        strncpy(rc_data.comment, str_v, CFG_STRLEN_MAX);

    /* SDR device settings */
    set_v = config_lookup(&cfg, "device");

    if (set_v && config_setting_is_group(set_v)) {
        if (config_setting_lookup_string(set_v, "driver", &str_v))
            strncpy(rc_data.device_driver, str_v, CFG_STRLEN_MAX);
        else
            strncpy(rc_data.device_driver, "auto", CFG_STRLEN_MAX);

        if (strncasecmp(rc_data.device_driver, "auto", 4) == 0)
            SetFlag(AUTO_DETECT_SDR);
        else
            ClearFlag(AUTO_DETECT_SDR);

        if (config_setting_lookup_int(set_v, "index", &int_v) &&
                (int_v >= 0) && (int_v <= 255))
            rc_data.device_index = (uint8_t)int_v;
        else
            rc_data.device_index = 0;
    }
    else {
        strncpy(rc_data.device_driver, "auto", CFG_STRLEN_MAX);
        SetFlag(AUTO_DETECT_SDR);

        rc_data.device_index = 0;
    }

    /* SDR receiver settings */
    set_v = config_lookup(&cfg, "receiver");

    if (set_v && config_setting_is_group(set_v)) {
        if (config_setting_lookup_int(set_v, "freq", &int_v) &&
                (int_v >= 136000) && (int_v <= 138000))
            rc_data.sdr_center_freq = (uint32_t)int_v * 1000;
        else {
            Print_Message("Can't find valid receiver frequency!", ERROR_MESG);

            return false;
        }

        if (config_setting_lookup_int(set_v, "bw", &int_v) &&
                (int_v >= 90000) && (int_v <= 210000))
            rc_data.sdr_filter_bw = (uint32_t)int_v;
        else
            rc_data.sdr_filter_bw = 120000;

        if (config_setting_lookup_float(set_v, "gain", &flt_v) &&
                (flt_v >= 0.0) && (flt_v <= 100.0))
            rc_data.tuner_gain = flt_v;
        else
            rc_data.tuner_gain = 0.0;

        if (config_setting_lookup_float(set_v, "corr_f", &flt_v) &&
                (flt_v >= -100.0) && (flt_v <= 100.0))
            rc_data.freq_correction = flt_v;
        else
            rc_data.freq_correction = 0.0;
    }
    else {
        Print_Message("Can't find SDR receiver settings!", ERROR_MESG);

        return false;
    }

    /* Demodulator settings */
    set_v = config_lookup(&cfg, "demodulator");

    if (set_v && config_setting_is_group(set_v)) {
        if (config_setting_lookup_int(set_v, "rrc_order", &int_v) &&
                (int_v >= 0))
            rc_data.rrc_order = (uint32_t)int_v;
        else
            rc_data.rrc_order = 32;

        if (config_setting_lookup_float(set_v, "rrc_alpha", &flt_v) &&
                (flt_v >= 0.0) && (flt_v <= 1.0))
            rc_data.rrc_alpha = flt_v;
        else
            rc_data.rrc_alpha = 0.6;

        if (config_setting_lookup_int(set_v, "interp_f", &int_v) &&
                (int_v >= 0))
            rc_data.interp_factor = (uint32_t)int_v;
        else
            rc_data.interp_factor = 4;

        if (config_setting_lookup_float(set_v, "pll_bw", &flt_v) &&
                (flt_v >= 0.0))
            rc_data.costas_bandwidth = flt_v;
        else
            rc_data.costas_bandwidth = 100.0;

        if (config_setting_lookup_float(set_v, "pll_thresh", &flt_v) &&
                (flt_v >= 0.0))
            rc_data.pll_locked = flt_v;
        else
            rc_data.pll_locked = 0.8;

        rc_data.pll_unlocked = 1.03 * rc_data.pll_locked;

        if (config_setting_lookup_string(set_v, "mode", &str_v)) {
            if (strncasecmp(str_v, "QPSK", 4) == 0)
                rc_data.psk_mode = QPSK;
            else if (strncasecmp(str_v, "DOQPSK", 6) == 0)
                rc_data.psk_mode = DOQPSK;
            else if (strncasecmp(str_v, "IDOQPSK", 7) == 0)
                rc_data.psk_mode = IDOQPSK;
            else {
                Print_Message("QPSK mode is invalid!", ERROR_MESG);

                return false;
            }
        }
        else {
            Print_Message("Can't find QPSK mode!", ERROR_MESG);

            return false;
        }

        if (config_setting_lookup_int(set_v, "rate", &int_v) &&
                (int_v >= 50000) && (int_v <= 100000))
            rc_data.symbol_rate = (uint32_t)int_v;
        else {
            Print_Message("Can't find valid QPSK symbol rate!",
                    ERROR_MESG);

            return false;
        }
    }
    else {
        Print_Message("Can't find demodulator settings!", ERROR_MESG);

        return false;
    }

    /* Decoder settings */
    set_v = config_lookup(&cfg, "decoder");

    if (set_v && config_setting_is_group(set_v)) {
        arr_v = config_setting_lookup(set_v, "apids");

        if (arr_v && config_setting_is_array(arr_v) &&
                (config_setting_length(arr_v) == CHANNEL_IMAGE_NUM)) {
            for (uint8_t idx = 0; idx < CHANNEL_IMAGE_NUM; idx++) {
                uint8_t apid =
                    (uint8_t)config_setting_get_int_elem(arr_v, idx);

                if ((apid < 64) || (apid > 69)) {
                    Print_Message("APIDs are incorrect!", ERROR_MESG);

                    return false;
                }
                else
                    rc_data.apid[idx] = apid;
            }
        }
        else {
            Print_Message("Can't find valid APIDs!", ERROR_MESG);

            return false;
        }

        arr_v = config_setting_lookup(set_v, "apids_invert");

        if (arr_v && config_setting_is_array(arr_v) &&
                (config_setting_length(arr_v) == 3)) {
            for (uint8_t idx = 0; idx < 3; idx++) {
                uint32_t apid =
                    (uint32_t)config_setting_get_int_elem(arr_v, idx);

                if ((apid != 0) && ((apid < 64) || (apid > 69)))
                    rc_data.invert_palette[idx] = 67 + idx;
                else
                    rc_data.invert_palette[idx] = apid;
            }
        }
        else {
            rc_data.invert_palette[0] = 67;
            rc_data.invert_palette[1] = 68;
            rc_data.invert_palette[2] = 69;
        }

        arr_v = config_setting_lookup(set_v, "rgb_chans");

        if (arr_v && config_setting_is_array(arr_v) &&
                (config_setting_length(arr_v) == 3)) {
            for (uint8_t idx = 0; idx < 3; idx++) {
                uint8_t chan =
                    (uint8_t)config_setting_get_int_elem(arr_v, idx);

                if (chan > 2)
                    rc_data.color_channel[idx] = idx;
                else
                    rc_data.color_channel[idx] = chan;
            }
        }
        else {
            rc_data.color_channel[0] = 0;
            rc_data.color_channel[1] = 1;
            rc_data.color_channel[2] = 2;
        }

        if (config_setting_lookup_int(set_v, "duration", &int_v) &&
                (int_v >= 0) && (int_v <= 1200))
            rc_data.default_timer = (uint32_t)int_v;
        else
            rc_data.default_timer = 900;

        if (!rc_data.decode_timer)
            rc_data.decode_timer = rc_data.default_timer;
    }
    else {
        Print_Message("Can't find decoder settings!", ERROR_MESG);

        return false;
    }

    /* Post-processing settings */
    set_v = config_lookup(&cfg, "postproc");

    if (set_v && config_setting_is_group(set_v)) {
        if (config_setting_lookup_bool(set_v, "colorize", &int_v)) {
            if (int_v)
                SetFlag(IMAGE_COLORIZE);
            else
                ClearFlag(IMAGE_COLORIZE);
        }
        else
            SetFlag(IMAGE_COLORIZE);

        arr_v = config_setting_lookup(set_v, "R_rng");

        if (arr_v && config_setting_is_array(arr_v) &&
                (config_setting_length(arr_v) == 2)) {
            uint8_t rng_min =
                (uint8_t)config_setting_get_int_elem(arr_v, 0);
            uint8_t rng_max =
                (uint8_t)config_setting_get_int_elem(arr_v, 1);

            if (rng_min > rng_max) {
                rc_data.norm_range[RED][NORM_RANGE_BLACK] = 0;
                rc_data.norm_range[RED][NORM_RANGE_WHITE] = 240;
            }
            else {
                rc_data.norm_range[RED][NORM_RANGE_BLACK] = rng_min;
                rc_data.norm_range[RED][NORM_RANGE_WHITE] = rng_max;
            }
        }
        else {
            rc_data.norm_range[RED][NORM_RANGE_BLACK] = 0;
            rc_data.norm_range[RED][NORM_RANGE_WHITE] = 240;
        }

        arr_v = config_setting_lookup(set_v, "G_rng");

        if (arr_v && config_setting_is_array(arr_v) &&
                (config_setting_length(arr_v) == 2)) {
            uint8_t rng_min =
                (uint8_t)config_setting_get_int_elem(arr_v, 0);
            uint8_t rng_max =
                (uint8_t)config_setting_get_int_elem(arr_v, 1);

            if (rng_min > rng_max) {
                rc_data.norm_range[GREEN][NORM_RANGE_BLACK] = 0;
                rc_data.norm_range[GREEN][NORM_RANGE_WHITE] = 255;
            }
            else {
                rc_data.norm_range[GREEN][NORM_RANGE_BLACK] = rng_min;
                rc_data.norm_range[GREEN][NORM_RANGE_WHITE] = rng_max;
            }
        }
        else {
            rc_data.norm_range[GREEN][NORM_RANGE_BLACK] = 0;
            rc_data.norm_range[GREEN][NORM_RANGE_WHITE] = 255;
        }

        arr_v = config_setting_lookup(set_v, "B_rng");

        if (arr_v && config_setting_is_array(arr_v) &&
                (config_setting_length(arr_v) == 2)) {
            uint8_t rng_min =
                (uint8_t)config_setting_get_int_elem(arr_v, 0);
            uint8_t rng_max =
                (uint8_t)config_setting_get_int_elem(arr_v, 1);

            if (rng_min > rng_max) {
                rc_data.norm_range[BLUE][NORM_RANGE_BLACK] = 60;
                rc_data.norm_range[BLUE][NORM_RANGE_WHITE] = 255;
            }
            else {
                rc_data.norm_range[BLUE][NORM_RANGE_BLACK] = rng_min;
                rc_data.norm_range[BLUE][NORM_RANGE_WHITE] = rng_max;
            }
        }
        else {
            rc_data.norm_range[BLUE][NORM_RANGE_BLACK] = 60;
            rc_data.norm_range[BLUE][NORM_RANGE_WHITE] = 255;
        }

        arr_v = config_setting_lookup(set_v, "B_water_rng");

        if (arr_v && config_setting_is_array(arr_v) &&
                (config_setting_length(arr_v) == 2)) {
            uint8_t rng_min =
                (uint8_t)config_setting_get_int_elem(arr_v, 0);
            uint8_t rng_max =
                (uint8_t)config_setting_get_int_elem(arr_v, 1);

            if (rng_min > rng_max) {
                rc_data.colorize_blue_min = 60;
                rc_data.colorize_blue_max = 80;
            }
            else {
                rc_data.colorize_blue_min = rng_min;
                rc_data.colorize_blue_max = rng_max;
            }
        }
        else {
            rc_data.colorize_blue_min = 60;
            rc_data.colorize_blue_max = 80;
        }

        if (config_setting_lookup_int(set_v, "B_clouds_thresh", &int_v) &&
                (int_v >= 0) && (int_v <= 255))
            rc_data.clouds_threshold = (uint8_t)int_v;
        else
            rc_data.clouds_threshold = 210;

        if (config_setting_lookup_bool(set_v, "normalize", &int_v)) {
            if (int_v)
                SetFlag(IMAGE_NORMALIZE);
            else
                ClearFlag(IMAGE_NORMALIZE);
        }
        else
            SetFlag(IMAGE_NORMALIZE);

        /* TODO deal with CLAHE enabling more carefully */
        if (config_setting_lookup_bool(set_v, "clahe", &int_v)) {
            if (int_v)
                SetFlag(IMAGE_CLAHE);
            else
                ClearFlag(IMAGE_CLAHE);
        }
        else
            SetFlag(IMAGE_CLAHE);

        if (config_setting_lookup_string(set_v, "rectify", &str_v)) {
            if (strncasecmp(str_v, "no", 2) == 0)
                rc_data.rectify_function = R_NO;
            else if (strncasecmp(str_v, "W2RG", 4) == 0)
                rc_data.rectify_function = R_W2RG;
            else if (strncasecmp(str_v, "5B4AZ", 5) == 0)
                rc_data.rectify_function = R_5B4AZ;
            else
                rc_data.rectify_function = R_5B4AZ;
        }
        else
            rc_data.rectify_function = R_5B4AZ;

        if (rc_data.rectify_function)
            SetFlag(IMAGE_RECTIFY);
        else
            ClearFlag(IMAGE_RECTIFY);
    }
    else {
        SetFlag(IMAGE_COLORIZE);

        rc_data.norm_range[RED][NORM_RANGE_BLACK] = 0;
        rc_data.norm_range[RED][NORM_RANGE_WHITE] = 240;
        rc_data.norm_range[GREEN][NORM_RANGE_BLACK] = 0;
        rc_data.norm_range[GREEN][NORM_RANGE_WHITE] = 255;
        rc_data.norm_range[BLUE][NORM_RANGE_BLACK] = 60;
        rc_data.norm_range[BLUE][NORM_RANGE_WHITE] = 255;

        rc_data.colorize_blue_min = 60;
        rc_data.colorize_blue_max = 80;

        rc_data.clouds_threshold = 210;

        SetFlag(IMAGE_NORMALIZE);
        SetFlag(IMAGE_CLAHE);

        rc_data.rectify_function = R_5B4AZ;
        SetFlag(IMAGE_RECTIFY);
    }

    /* Output settings */
    set_v = config_lookup(&cfg, "output");

    if (set_v && config_setting_is_group(set_v)) {
        if (config_setting_lookup_string(set_v, "type", &str_v)) {
            if (strncasecmp(str_v, "combo", 5) == 0) {
                SetFlag(IMAGE_OUT_COMBO);
                ClearFlag(IMAGE_OUT_SPLIT);
            }
            else if (strncasecmp(str_v, "chan", 4) == 0) {
                ClearFlag(IMAGE_OUT_COMBO);
                SetFlag(IMAGE_OUT_SPLIT);
            }
            else if (strncasecmp(str_v, "all", 3) == 0) {
                SetFlag(IMAGE_OUT_COMBO);
                SetFlag(IMAGE_OUT_SPLIT);
            }
            else {
                SetFlag(IMAGE_OUT_COMBO);
                SetFlag(IMAGE_OUT_SPLIT);
            }
        }
        else {
            SetFlag(IMAGE_OUT_COMBO);
            SetFlag(IMAGE_OUT_SPLIT);
        }

        if (config_setting_lookup_string(set_v, "format", &str_v)) {
            if (strncasecmp(str_v, "JPEG", 4) == 0) {
                SetFlag(IMAGE_SAVE_JPEG);
                ClearFlag(IMAGE_SAVE_PPGM);
            }
            else if (strncasecmp(str_v, "PGM", 3) == 0) {
                ClearFlag(IMAGE_SAVE_JPEG);
                SetFlag(IMAGE_SAVE_PPGM);
            }
            else if (strncasecmp(str_v, "all", 3) == 0) {
                SetFlag(IMAGE_SAVE_JPEG);
                SetFlag(IMAGE_SAVE_PPGM);
            }
            else {
                SetFlag(IMAGE_SAVE_JPEG);
                ClearFlag(IMAGE_SAVE_PPGM);
            }
        }
        else {
            SetFlag(IMAGE_SAVE_JPEG);
            ClearFlag(IMAGE_SAVE_PPGM);
        }

        if (config_setting_lookup_int(set_v, "jpeg_qual", &int_v) &&
                (int_v >= 0) && (int_v <= 100))
            rc_data.jpeg_quality = int_v;
        else
            rc_data.jpeg_quality = 100;

        /* TODO review how raw images are saved */
        if (config_setting_lookup_bool(set_v, "save_raw", &int_v)) {
            if (int_v)
                SetFlag(IMAGE_RAW);
            else
                ClearFlag(IMAGE_RAW);
        }
        else
            ClearFlag(IMAGE_RAW);
    }
    else {
        SetFlag(IMAGE_OUT_COMBO);
        SetFlag(IMAGE_OUT_SPLIT);

        SetFlag(IMAGE_SAVE_JPEG);
        ClearFlag(IMAGE_SAVE_PPGM);

        rc_data.jpeg_quality = 100;

        ClearFlag(IMAGE_RAW);
    }

    /* Cleanup */
    config_destroy(&cfg);

    /* Set Gain control buttons and slider */
    if (rc_data.tuner_gain != 0.0)
        ClearFlag(TUNER_GAIN_AUTO);
    else
        SetFlag(TUNER_GAIN_AUTO);

    return true;
}
