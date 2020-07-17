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

#ifndef MLRPT_OPERATION_H
#define MLRPT_OPERATION_H

/*****************************************************************************/

#include <stdbool.h>

/*****************************************************************************/

bool Start_Receiver(void);
void Alarm_Action(void);
void Auto_Timer_Setup(char *arg);

/*****************************************************************************/

#endif
