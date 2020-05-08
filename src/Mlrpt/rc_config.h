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

#ifndef RC_CONFIG_H
#define RC_CONFIG_H     1

#include "../common/common.h"

/* Special characters */
#define LF      0x0A /* Line Feed */
#define CR      0x0D /* Carriage Return */
#define HT      0x09 /* Horizontal Tab  */

/* Min receiver bandwidth, 100kHz by user request */
#define MIN_BANDWIDTH   100000
#define MAX_BANDWIDTH   200000

#endif

