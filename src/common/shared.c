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

#include "shared.h"

/* Runtime config data */
rc_data_t rc_data;

/* Demodulator object */
Demod_t *demodulator = NULL;

/* Chebyshev filter data I/Q */
filter_data_t filter_data_i;
filter_data_t filter_data_q;

/* Playback control semaphore */
sem_t demod_semaphore;

/* Meteor Image Decoder objects */
BOOLEAN no_time_yet = TRUE;
int last_time, first_time;

ac_table_rec_t *ac_table = NULL;
size_t ac_table_len;

/* Channel images and sizes */
uint8_t *channel_image[CHANNEL_IMAGE_NUM];
size_t   channel_image_size;
uint32_t channel_image_height, channel_image_width;

mtd_rec_t mtd_record;

/*------------------------------------------------------------------------*/

