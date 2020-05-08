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

#ifndef SHARED_H
#define SHARED_H    1

#include "common.h"


/* Runtime config data */
extern rc_data_t rc_data;

/* Demodulator object */
extern Demod_t *demodulator;

/* Chebyshev filter data I/Q */
extern filter_data_t filter_data_i;
extern filter_data_t filter_data_q;

/* Playback control semaphore */
extern sem_t demod_semaphore;

/* Meteor Image Decoder objects */
extern BOOLEAN no_time_yet;
extern int last_time, first_time;

extern ac_table_rec_t *ac_table;
extern size_t ac_table_len;

/* Channel images and sizes */
extern uint8_t *channel_image[CHANNEL_IMAGE_NUM];
extern size_t   channel_image_size;
extern uint32_t channel_image_height, channel_image_width;

extern mtd_rec_t mtd_record;

extern int corr_tab[256][256];
extern int ac_lookup[65536], dc_lookup[65536];

#endif

