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

/*****************************************************************************/

#ifndef DEMODULATOR_DEMOD_H
#define DEMODULATOR_DEMOD_H

/*****************************************************************************/

#include "agc.h"
#include "filters.h"
#include "pll.h"

#include <stdbool.h>
#include <stdint.h>

/*****************************************************************************/

typedef struct Demod_t {
    Agc_t    *agc;
    Costas_t *costas;
    double    sym_period;
    uint32_t  sym_rate;
    ModScheme mode;
    Filter_t *rrc;
} Demod_t;

/*****************************************************************************/

void Demod_Init(void);
void Demod_Deinit(void);
double Agc_Gain(double *gain);
double Signal_Level(uint32_t *level);
double Pll_Average(void);
void Demodulator_Run(void);

/*****************************************************************************/

#endif
