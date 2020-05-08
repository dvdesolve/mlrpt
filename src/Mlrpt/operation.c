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

#include "operation.h"
#include "../common/shared.h"

/*------------------------------------------------------------------------*/

/* Init_Reception()
 *
 * Initialize Reception of signal from Satellite
 */
  static BOOLEAN
Init_Reception( void )
{
  /* Initialize semaphore */
  sem_init( &demod_semaphore, 0, 0 );

  /* Initialize RTLSDR device */
  switch( rc_data.sdr_rx_type )
  {
#ifdef HAVE_LIBRTLSDR
    case SDR_TYPE_RTLSDR:
      if( !RtlSdr_Initialize() )
      {
        Cleanup();
        Print_Message( _("Failed to Initialize RTL-SDR"), ERROR_MESG );
        return( FALSE );
      }
      Print_Message( _("Decoding from RTL-SDR Receiver"), INFO_MESG );
      break;
#endif

#ifdef HAVE_LIBAIRSPY
    case SDR_TYPE_AIRSPY:
      if( !Airspy_Initialize() )
      {
        Cleanup();
        Print_Message( _("Failed to Initialize Airspy"), ERROR_MESG );
        return( FALSE );
      }
      Print_Message( _("Decoding from Airspy Receiver"), INFO_MESG );
      break;
#endif

    default:
      Print_Message( _("SDR Device Type Invalid - Exiting"), ERROR_MESG );
      exit( -1 );

  } /* switch( rc_data.sdr_rx_type ) */

  return( TRUE );
} /* Init_Reception() */

/*------------------------------------------------------------------------*/

/* Initialize_All()
 *
 * Initializes all needed to receive and decode images
 */
  static BOOLEAN
Initialize_All( void )
{
  /* Initialize Reception, abort on error */
  if( !Init_Reception() ) return( FALSE );

  /* Create Demodulator object */
  Demod_Init();

  /* Initialize Meteor Image Decoder */
  Medet_Init();

  SetFlag( ALL_INITIALIZED );
  return( TRUE );

} /* Initialize_All( void ) */

/*------------------------------------------------------------------------*/

/* Start_Receiver()
 *
 * Starts reception from the SDR receiver
 */
  BOOLEAN
Start_Receiver( void )
{
  /* Start SDR Receiver and Demodulator */
  if( !Initialize_All() ) return( FALSE );
  SetFlag( ACTION_RECEIVER_ON );
  ClearFlag( ALARM_ACTION_START );
  Decode_Images();
  Demodulator_Run();
  return( TRUE );
} /* Start_Receiver() */

/*------------------------------------------------------------------------*/

/* Decode_Images()
 *
 * Starts the LRPT decoder
 */
  void
Decode_Images( void )
{
  /* Message string buffer */
  char mesg[MESG_SIZE];

  snprintf( mesg, MESG_SIZE,
      _("Operation Timer Started: %d sec"), rc_data.operation_time );
  Print_Message( mesg, INFO_MESG );
  alarm( rc_data.operation_time );

  Print_Message( _("Decoding of LRPT Images Started"), INFO_MESG );
  SetFlag( ACTION_DECODE_IMAGES );
} /* Decode_Images() */

/*------------------------------------------------------------------------*/

/* Alarm_Action()
 *
 * Handles the SIGALRM timer signal
 */
  void
Alarm_Action( void )
{
  /* Start Receive & Decode Operation */
  if( isFlagSet(ALARM_ACTION_START) )
    Print_Message( _("Pause Timer Expired"), INFO_MESG );
  else
  {
    Print_Message( _("Operation Timer Expired"), INFO_MESG );
    if( rc_data.psk_mode == IDOQPSK )
      SetFlag( ACTION_IDOQPSK_STOP );
    else
      ClearFlag( ACTION_FLAGS_ALL );
  }
} /* Alarm_Action() */

/*------------------------------------------------------------------------*/

/* Oper_Timer_Setup()
 *
 * Handles the -t <timeout> option
 */
  void
Oper_Timer_Setup( char *arg )
{
  /* Message string buffer */
  char mesg[MESG_SIZE];

  rc_data.operation_time = (uint)( atoi(arg) * 60 );
  if( rc_data.operation_time > MAX_OPERATION_TIME )
  {
    snprintf( mesg, MESG_SIZE,
        _("Operation Timer (%d sec) excessive?"), rc_data.operation_time );
    Print_Message( mesg, ERROR_MESG );
  }

  snprintf( mesg, MESG_SIZE,
      _("Operation Timer set to %d sec"),
      rc_data.operation_time );
  Print_Message( mesg, INFO_MESG );

} /* Oper_Timer_Setup() */

/*------------------------------------------------------------------------*/

/* Auto_Timer_Setup()
 *
 * Handles the -s <start-stop-time> option
 *
 */
  void
Auto_Timer_Setup( char *arg )
{
  uint
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

  BOOLEAN test = TRUE;
  int idx;

  /* Do some timer data sanity checks.
   * Times format should be hhmm-hhmm */
  if( strlen(optarg) != 9 ) test = FALSE;
  if( optarg[4] != '-' ) test = FALSE;
  for( idx = 0; idx <= 3; idx++ )
    if( (optarg[idx] < '0') || (optarg[idx] > '9') )
      test = FALSE;
  for( idx = 5; idx <= 8; idx++ )
    if( (optarg[idx] < '0') || (optarg[idx] > '9') )
      test = FALSE;
  if( !test )
  {
    Print_Message( _("Start-Stop times invalid - exiting"), ERROR_MESG );
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
  time_sec = (uint)
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
    Print_Message( _("Stop Time ahead of Start Time - exiting"), ERROR_MESG );
    exit( -1 );
  }

  /* Difference between start-stop times in sec */
  rc_data.operation_time = (uint)( stop_sec - sleep_sec );

  /* Data sanity check */
  if( rc_data.operation_time > MAX_OPERATION_TIME )
  {
    snprintf( mesg, MESG_SIZE,
        _("Operation Time (%d sec) excessive?"), rc_data.operation_time );
    Print_Message( mesg, ERROR_MESG );
  }

  /* Notify sleeping */
  snprintf( mesg, MESG_SIZE,
      _("Paused till %02d:%02d UTC. Operation Timer set to %d sec"),
      start_hrs, start_min, rc_data.operation_time );
  Print_Message( mesg, INFO_MESG );

  /* Set sleep flag and wakeup action */
  SetFlag( ALARM_ACTION_START );
  alarm( sleep_sec );

  return;
} /* Auto_Timer_Setup() */

/*------------------------------------------------------------------------*/

