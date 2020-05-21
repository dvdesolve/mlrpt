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

#ifndef MLRPT_RC_CONFIG_H
#define MLRPT_RC_CONFIG_H

/*****************************************************************************/

#include "../common/common.h"

#include <stdbool.h>
#include <stdint.h>

/*****************************************************************************/

/* Runtime config data storage type */
typedef struct rc_data_t {
    /* mlrpt config directory and pictures directory */
    char mlrpt_cfg[64], mlrpt_imgs[64];

    /* Timers: time duration (sec) for image decoding,
     * default timer duration value
     */
    uint32_t operation_time, default_oper_time;

    /* RTL-SDR configuration: device index */
    uint32_t rtlsdr_dev_index;

    /* Frequency correction factor in ppm for RTL-SDR */
    int rtlsdr_freq_corr;

    /* SDR receiver configuration: RX frequency, receiver type,
     * RX ADC sampling rate, low pass filter bandwidth and tuner gain
     */
    uint32_t sdr_center_freq, sdr_rx_type, sdr_samplerate, sdr_filter_bw, tuner_gain;

    /* I/Q sampling rate (sym/sec), QPSK symbol rate (sym/sec) */
    uint32_t demod_samplerate, symbol_rate;

    /* Demodulator type (QPSK/OQPSK) */
    uint8_t psk_mode;

    /* Raised root cosine settings: alpha factor, filter order */
    double rrc_alpha;
    uint32_t rrc_order;

    /* Costas PLL parameters: bandwidth,
     * lower phase error threshold, upper phase error threshold
     */
    double costas_bandwidth;
    double pll_locked, pll_unlocked;

    /* Demodulator interpolation multiplier */
    uint32_t interp_factor;

    /* Channels APID */
    uint8_t apid[CHANNEL_IMAGE_NUM];

    /* Image normalization pixel value ranges */
    uint8_t norm_range[CHANNEL_IMAGE_NUM][2];

    /* Image APIDs to invert palette (black <--> white) */
    uint32_t invert_palette[3];

    /* Image rectification algorithm (W2RG/5B4AZ) */
    uint8_t rectify_function;

    /* Image pixel values above which we assume it is cloudy areas */
    uint8_t clouds_threshold;

    /* Max and min value of blue pixels during pseudo-colorization enhancement */
    uint8_t colorize_blue_max, colorize_blue_min;

    /* JPEG image quality */
    float jpeg_quality;
} rc_data_t;

/*****************************************************************************/

bool Load_Config(void);

/*****************************************************************************/

#endif
