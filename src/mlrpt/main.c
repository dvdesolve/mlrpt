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

#include "../common/common.h"
#include "../common/shared.h"
#include "../demodulator/pll.h"
#include "operation.h"
#include "utils.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*****************************************************************************/

/* Signal handler */
static void sig_handler(int signal);

/*****************************************************************************/

int main(int argc, char *argv[]) {
  /* Find and prepare program directories */
  if (!PrepareCacheDirectory()) {
    fprintf(stderr, "mlrpt: %s\n", "error during preparing cache directory");
    exit(-1);
  }

  /* Command line option returned by getopt() */
  int option;

  /* New and old actions for sigaction() */
  struct sigaction sa_new, sa_old;


  /* Initialize new actions */
  sa_new.sa_handler = sig_handler;
  sigemptyset( &sa_new.sa_mask );
  sa_new.sa_flags = 0;

  /* Register function to handle signals */
  sigaction( SIGINT,  &sa_new, &sa_old );
  sigaction( SIGSEGV, &sa_new, 0 );
  sigaction( SIGFPE,  &sa_new, 0 );
  sigaction( SIGTERM, &sa_new, 0 );
  sigaction( SIGABRT, &sa_new, 0 );
  sigaction( SIGCONT, &sa_new, 0 );
  sigaction( SIGALRM, &sa_new, 0 );

  /* Defaults/initialization */
  SetFlag( VERBOSE_MODE );

  /* Process command line options */
  rc_data.sdr_center_freq  = 0;
  rc_data.operation_time   = 0;
  rc_data.rectify_function = 0;

  /* Default config file */
  /* TODO recheck length */
  snprintf(rc_data.mlrpt_cfg, sizeof(rc_data.mlrpt_cfg),
              "%s/.mlrptrc", getenv("HOME"));

  while( (option = getopt(argc, argv, "c:f:r:s:t:qhiv") ) != -1 )
    switch( option )
    {
      case 'c': /* Config file name */
        /* TODO recheck length */
        snprintf(rc_data.mlrpt_cfg, sizeof(rc_data.mlrpt_cfg),
              "%s", optarg);
        break;

      case 'f': /* Receiver frequency in kHz */
        {
          double freq = atof( optarg ) * 1000.0;
          rc_data.sdr_center_freq = (uint32_t)freq;
        }
        break;

      case 's': /* Start and Stop times as hhmm-hhmm in UTC */
        Auto_Timer_Setup( optarg );
        pause(); /* Flow pauses here till start time */
        break;

      case 'r': /* Start and Stop times as hhmm-hhmm in UTC */
        rc_data.rectify_function = (uint8_t)atoi( optarg );
        break;

      case 't': /* Operation timer in min */
        Oper_Timer_Setup( optarg );
        break;

      case 'q': /* Disable running verbose */
        ClearFlag( VERBOSE_MODE );
        break;

      case 'h': /* Print usage and exit */
        Usage();
        exit(0);

      case 'i': /* Enable image flipping (invert images) */
        SetFlag( IMAGE_INVERT );
        break;

      case 'v': /* Print version  and exit*/
        puts( PACKAGE_STRING );
        exit(0);

      default: /* Print usage and exit */
        Usage();
        exit(-1);
    } /* End of switch( option ) */
 
  /* Load configuration data from mlrptrc */
  if( !Load_Config() )
  {
    Print_Message(
        "Failed to read mlrptrc configuration file - exiting",
        ERROR_MESG );
    exit( -1 );
  }
 
  /* Start receiver and decoder */
  if( !Start_Receiver() )
  {
    Print_Message(
        "Failed to start Receiver and Decoder - exiting",
        ERROR_MESG );
    exit( -1 );
  }

  return( 0 );
}

/*****************************************************************************/

/*  sig_handler()
 *
 *  Signal Action Handler function
 */

static void sig_handler( int signal )
{
  if( signal == SIGALRM )
  {
    Alarm_Action();
    return;
  }

  /* Internal wakeup call */
  if( signal == SIGCONT ) return;

  fprintf( stderr, "\n" );
  switch( signal )
  {
    case SIGINT:
      Print_Message( "Exiting via User Interrupt", ERROR_MESG );
      if( rc_data.psk_mode == IDOQPSK )
        SetFlag( ACTION_IDOQPSK_STOP );
      else
        ClearFlag( ACTION_FLAGS_ALL );
      return;

    case SIGSEGV:
      Print_Message( "Segmentation Fault - exiting", ERROR_MESG );
      exit( -1 );

    case SIGFPE:
      Print_Message( "Floating Point Exception - exiting", ERROR_MESG );
      exit( -1 );

    case SIGABRT:
      Print_Message( "Abort Signal received - exiting", ERROR_MESG );
      exit( -1 );

    case SIGTERM:
      Print_Message( "Termination Request received - exiting", ERROR_MESG );
      exit( -1 );
  }
}
