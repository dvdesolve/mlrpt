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

#ifndef SDR_RTLSDR_H
#define SDR_RTLSDR_H

/*****************************************************************************/

#include <stdbool.h>

/*****************************************************************************/

bool RtlSdr_Initialize(void);
void RtlSdr_Close_Device(void);

/*****************************************************************************/

#endif
