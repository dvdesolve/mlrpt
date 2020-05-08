/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 3 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details:
 *
 *  http://www.gnu.org/copyleft/gpl.txt
 */

#ifndef DOQPSK_H
#define DOQPSK_H   1

#include "../common/common.h"
#include "demod.h"

//Interleaver parameters
#define INTER_BRANCHES      36
#define INTER_DELAY         2048
#define INTER_BASE_LEN      73728 //INTER_BRANCHES * INTER_DELAY
#define INTER_DATA_LEN      72    // Number of actual interleaved symbols
#define INTER_SYNCDATA      80    // Number of interleaved symbols + sync

#define SYNCD_DEPTH         4     // How many consecutive sync words to search for
#define SYNCD_BUF_MARGIN    320   // SYNCD_DEPTH  * INTER_SYNCDATA
#define SYNCD_BLOCK_SIZ     400   // (SYNCD_DEPTH + 1) * INTER_SYNCDATA
#define SYNCD_BUF_STEP      240   // (SYNCD_DEPTH - 1) * INTER_SYNCDATA


#endif
