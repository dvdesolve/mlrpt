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

#ifndef COMMON_SHARED_H
#define COMMON_SHARED_H

/*****************************************************************************/

#include "../decoder/huffman.h"
#include "../decoder/met_to_data.h"
#include "../mlrpt/rc_config.h"
#include "../sdr/filters.h"
#include "common.h"

#include <semaphore.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*****************************************************************************/

/* Runtime config data */
extern rc_data_t rc_data;

/* Chebyshev filter data I/Q */
extern filter_data_t filter_data_i;
extern filter_data_t filter_data_q;

/* Demodulator control semaphore */
extern sem_t demod_semaphore;

/* Meteor decoder variables */
extern bool no_time_yet;
extern int last_time, first_time;
extern ac_table_rec_t *ac_table;
extern size_t ac_table_len;
extern mtd_rec_t mtd_record;

/* Channel images and sizes */
extern uint8_t *channel_image[CHANNEL_IMAGE_NUM];
extern size_t   channel_image_size;
extern uint32_t channel_image_width, channel_image_height;

/*****************************************************************************/

#endif
