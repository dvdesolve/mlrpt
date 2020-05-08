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

#ifndef RTLSDR_H
#define RTLSDR_H    1

#include "../common/common.h"

/* Length of RTL-SDR I/Q buffer length */
#define RTLSDR_BUF_LEN  65536

/* Settings for various rtlsdr functions */
#define TUNER_GAIN_MANUAL   1
#define TUNER_GAIN_AUTO     0
#define RTL_DAGC_ON         1
#define RTL_DAGC_OFF        0
#define AGC_SCALE_RANGE     100

/* These are actually the default settings of
 * rtlsdr_read_async() but I need them to define
 * the size of the working samples_buf, which must
 * be the same as the async buffers */
#define NUM_ASYNC_BUF   15

#define TUNER_TYPES \
  "UNKNOWN", \
  "E4000", \
  "FC0012", \
  "FC0013", \
  "FC2580", \
  "R820T", \
  "R828D"

#endif

