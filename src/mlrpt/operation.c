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

#include "operation.h"

#include "../common/common.h"
#include "../common/shared.h"
#include "../decoder/medet.h"
#include "../demodulator/demod.h"
#include "../sdr/airspy.h"
#include "../sdr/rtlsdr.h"
#include "utils.h"

#include <semaphore.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/*****************************************************************************/

static bool Init_Reception(void);
static bool Initialize_All(void);
static void Decode_Images(void);

/*****************************************************************************/

/* Init_Reception()
 *
 * Initialize Reception of signal from Satellite
 */
static bool Init_Reception(void) {
  /* Initialize semaphore */
  sem_init( &demod_semaphore, 0, 0 );

  /* Initialize RTLSDR device */
  switch( rc_data.sdr_rx_type )
  {
    case SDR_TYPE_RTLSDR:
      if( !RtlSdr_Initialize() )
      {
        Cleanup();
        Print_Message( "Failed to Initialize RTL-SDR", ERROR_MESG );
        return( false );
      }
      Print_Message( "Decoding from RTL-SDR Receiver", INFO_MESG );
      break;

    case SDR_TYPE_AIRSPY:
      if( !Airspy_Initialize() )
      {
        Cleanup();
        Print_Message( "Failed to Initialize Airspy", ERROR_MESG );
        return( false );
      }
      Print_Message( "Decoding from Airspy Receiver", INFO_MESG );
      break;

    default:
      Print_Message( "SDR Device Type Invalid - Exiting", ERROR_MESG );
      exit( -1 );

  } /* switch( rc_data.sdr_rx_type ) */

  return( true );
}

/*****************************************************************************/

/* Initialize_All()
 *
 * Initializes all needed to receive and decode images
 */
static bool Initialize_All(void) {
  /* Initialize Reception, abort on error */
  if( !Init_Reception() ) return( false );

  /* Create Demodulator object */
  Demod_Init();

  /* Initialize Meteor Image Decoder */
  Medet_Init();

  SetFlag( ALL_INITIALIZED );
  return( true );
}

/*****************************************************************************/

/* Decode_Images()
 *
 * Starts the LRPT decoder
 */
static void Decode_Images(void) {
  /* Message string buffer */
  char mesg[MESG_SIZE];

  snprintf( mesg, MESG_SIZE,
      "Operation Timer Started: %u sec", rc_data.operation_time );
  Print_Message( mesg, INFO_MESG );
  alarm( rc_data.operation_time );

  Print_Message( "Decoding of LRPT Images Started", INFO_MESG );
  SetFlag( ACTION_DECODE_IMAGES );
}

/*****************************************************************************/

/* Start_Receiver()
 *
 * Starts reception from the SDR receiver
 */
bool Start_Receiver(void) {
  /* Start SDR Receiver and Demodulator */
  if( !Initialize_All() ) return( false );
  SetFlag( ACTION_RECEIVER_ON );
  ClearFlag( ALARM_ACTION_START );
  Decode_Images();
  Demodulator_Run();
  return( true );
}

/*****************************************************************************/

/* Alarm_Action()
 *
 * Handles the SIGALRM timer signal
 */
void Alarm_Action(void) {
  /* Start Receive & Decode Operation */
  if( isFlagSet(ALARM_ACTION_START) )
    Print_Message( "Pause Timer Expired", INFO_MESG );
  else
  {
    Print_Message( "Operation Timer Expired", INFO_MESG );
    if( rc_data.psk_mode == IDOQPSK )
      SetFlag( ACTION_IDOQPSK_STOP );
    else
      ClearFlag( ACTION_FLAGS_ALL );
  }
}

/*****************************************************************************/

/* Oper_Timer_Setup()
 *
 * Handles the -t <timeout> option
 */
void Oper_Timer_Setup(char *arg) {
  /* Message string buffer */
  char mesg[MESG_SIZE];

  rc_data.operation_time = (uint32_t)( atoi(arg) * 60 );
  if( rc_data.operation_time > MAX_OPERATION_TIME )
  {
    snprintf( mesg, MESG_SIZE,
        "Operation Timer (%u sec) excessive?", rc_data.operation_time );
    Print_Message( mesg, ERROR_MESG );
  }

  snprintf( mesg, MESG_SIZE,
      "Operation Timer set to %u sec",
      rc_data.operation_time );
  Print_Message( mesg, INFO_MESG );
}

/*****************************************************************************/

/* Auto_Timer_Setup()
 *
 * Handles the -s <start-stop-time> option
 *
 */
void Auto_Timer_Setup(char *arg) {
  uint32_t
    start_hrs, /* Start time hours   */
    start_min, /* Start time minutes */
    stop_hrs,  /* Stop time hours    */
    stop_min,  /* Stop time minutes  */
    sleep_sec, /* Sleep time in sec  */
    stop_sec,  /* Stop time in sec since 00:00 hrs  */
    start_sec, /* Start time in sec since 00:00 hrs */
    time_sec;  /* Time now in sec since 00:00 hrs   */

  /* Message string buffer */
  char mesg[MESG_SIZE];

  /* Used to read real time */
  struct tm time_now;
  time_t t;

  bool test = true;
  int idx;

  /* Do some timer data sanity checks.
   * Times format should be hhmm-hhmm */
  if( strlen(optarg) != 9 ) test = false;
  if( optarg[4] != '-' ) test = false;
  for( idx = 0; idx <= 3; idx++ )
    if( (optarg[idx] < '0') || (optarg[idx] > '9') )
      test = false;
  for( idx = 5; idx <= 8; idx++ )
    if( (optarg[idx] < '0') || (optarg[idx] > '9') )
      test = false;
  if( !test )
  {
    Print_Message( "Start-Stop times invalid - exiting", ERROR_MESG );
    exit( -1 );
  }

  /* Calculate start and stop times */
  start_hrs = (optarg[0] & 0x0f) * 10 + (optarg[1] & 0x0f);
  start_min = (optarg[2] & 0x0f) * 10 + (optarg[3] & 0x0f);
  stop_hrs  = (optarg[5] & 0x0f) * 10 + (optarg[6] & 0x0f);
  stop_min  = (optarg[7] & 0x0f) * 10 + (optarg[8] & 0x0f);

  /* Time now */
  t = time( &t );
  time_now = *gmtime( &t );
  time_sec = (uint32_t)
    (time_now.tm_hour * 3600 + time_now.tm_min * 60 + time_now.tm_sec);

  start_sec = start_hrs * 3600 + start_min * 60;
  if( start_sec < time_sec ) start_sec += 86400; /* Next day */
  sleep_sec = start_sec - time_sec;

  stop_sec  = stop_hrs * 3600 + stop_min * 60;
  if( stop_sec < time_sec ) stop_sec += 86400; /* Next day */
  stop_sec -= time_sec;

  /* Data sanity check */
  if( stop_sec <= sleep_sec )
  {
    Print_Message( "Stop Time ahead of Start Time - exiting", ERROR_MESG );
    exit( -1 );
  }

  /* Difference between start-stop times in sec */
  rc_data.operation_time = (uint32_t)( stop_sec - sleep_sec );

  /* Data sanity check */
  if( rc_data.operation_time > MAX_OPERATION_TIME )
  {
    snprintf( mesg, MESG_SIZE,
        "Operation Time (%u sec) excessive?", rc_data.operation_time );
    Print_Message( mesg, ERROR_MESG );
  }

  /* Notify sleeping */
  snprintf( mesg, MESG_SIZE,
      "Paused till %02d:%02d UTC. Operation Timer set to %u sec",
      start_hrs, start_min, rc_data.operation_time );
  Print_Message( mesg, INFO_MESG );

  /* Set sleep flag and wakeup action */
  SetFlag( ALARM_ACTION_START );
  alarm( sleep_sec );

  return;
}
