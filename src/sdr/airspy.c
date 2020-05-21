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

#include "airspy.h"

#include "../common/common.h"
#include "../common/shared.h"
#include "../mlrpt/utils.h"
#include "filters.h"

#include <libairspy/airspy.h>

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*****************************************************************************/

#define AIRSPY_DECIMATE     8
#define AIRSPY_GAIN_AUTO    1
#define AIRSPY_GAIN_MANUAL  0
#define MAX_LINEARITY_GAIN  21.0
#define MAX_GAIN            45.0

/*****************************************************************************/

static int Airspy_Data_Cb(airspy_transfer *transfer);
static void *Airspy_Read_Async(void *arg);
static void Airspy_Set_Center_Frequency(uint32_t center_freq);
static bool Airspy_Set_Tuner_Gain_Mode(int mode);
static bool Airspy_Set_Tuner_Gain(uint32_t gain);
static bool Airspy_Set_Sample_Rate(void);
static bool Airspy_Open_Device(void);

/*****************************************************************************/

static struct airspy_device *device = NULL;
static pthread_t airspy_pthread_id;
static sem_t airspy_semaphore;
static double
  *buf_i[2] = { NULL, NULL },
  *buf_q[2] = { NULL, NULL };
static uint8_t buf_cnt = 0;

/*****************************************************************************/

/* Airspy_Data_Cb()
 *
 * Callback function for airspy_read_async()
 */
static int Airspy_Data_Cb(airspy_transfer *transfer) {
  static uint32_t buf_len = 0, decim_len;
  uint32_t idx, ids, dcnt;
  size_t mreq;

  /* Initialize on first call */
  if( !buf_len )
  {
    /* Create local I/Q data buffers */
    buf_len   = (uint32_t)( transfer->sample_count ) * 2;
    decim_len = (uint32_t)( transfer->sample_count ) / AIRSPY_DECIMATE;

    /* Allocate I/Q data buffers */
    mreq  = (size_t)decim_len * sizeof( double );
    for( buf_cnt = 0; buf_cnt < 2; buf_cnt++ )
    {
      mem_alloc( (void **)&buf_i[buf_cnt], mreq );
      mem_alloc( (void **)&buf_q[buf_cnt], mreq );
    }
    buf_cnt = 0;

    /* Init I and Q Low Pass Filter */
    Init_Chebyshev_Filter(
        &filter_data_i,
        decim_len,
        rc_data.sdr_filter_bw,
        rc_data.demod_samplerate,
        FILTER_RIPPLE,
        FILTER_POLES,
        FILTER_LOWPASS );

    Init_Chebyshev_Filter(
        &filter_data_q,
        decim_len,
        rc_data.sdr_filter_bw,
        rc_data.demod_samplerate,
        FILTER_RIPPLE,
        FILTER_POLES,
        FILTER_LOWPASS );
  } /* if( !buf_len ) */

  /* Copy I/Q data into the samples buffer */
  idx = 0;
  ids = 0;
  int16_t *data = (int16_t *)( transfer->samples );
  while( idx < buf_len )
  {
    /* Decimate samples down to managable rates */
    dcnt = 0;
    buf_i[buf_cnt][ids] = 0.0;
    buf_q[buf_cnt][ids] = 0.0;
    while( dcnt < AIRSPY_DECIMATE )
    {
      buf_i[buf_cnt][ids] += (double)data[idx];
      idx++;
      buf_q[buf_cnt][ids] += (double)data[idx];
      idx++;
      dcnt++;
    }

    buf_i[buf_cnt][ids] /= (double)AIRSPY_DECIMATE;
    buf_q[buf_cnt][ids] /= (double)AIRSPY_DECIMATE;
    ids++;
  } /* while( idx < buf_len ) */

  // Writes IQ samples to file, for testing only
  /*{
    static FILE *fdi = NULL, *fdq = NULL;
    if( fdi == NULL ) fdi = fopen( "i.s", "w" );
    if( fdq == NULL ) fdq = fopen( "q.s", "w" );
    fwrite( buf_i[buf_cnt], sizeof(double), (size_t)decim_len, fdi );
    fwrite( buf_q[buf_cnt], sizeof(double), (size_t)decim_len, fdq );
    }*/

  // Reads IQ samples from file, for testing only
  /*{
    static FILE *fdi = NULL, *fdq = NULL;
    if( fdi == NULL ) fdi = fopen( "i.s", "r" );
    if( fdq == NULL ) fdq = fopen( "q.s", "r" );
    fread( buf_i[buf_cnt], sizeof(double), (size_t)decim_len, fdi );
    fread( buf_q[buf_cnt], sizeof(double), (size_t)decim_len, fdq );
  }*/

  // Writes the phase angle of samples, for testing only
  /* if( isFlagSet(ACTION_DECODE_IMAGES) )
  {
    static double prev = 0.0;
    double phase, delta, x, y;
    for( idx = 0; idx < decim_len; idx++ )
    {
      x = (double)(buf_i[buf_cnt][idx]);
      y = (double)(buf_q[buf_cnt][idx]);
      phase = atan2( fabs(x), fabs(y) ) * 57.3;
      if( (x > 0.0) && (y < 0.0) ) phase = 360.0 - phase;
      if( (x < 0.0) && (y > 0.0) ) phase = 180.0 - phase;
      if( (x < 0.0) && (y < 0.0) ) phase = 180.0 + phase;
      delta = phase - prev;
      prev  = phase;
      printf( "%6.1f  %6.1f\n", phase, delta );
    }
  } */

  /* Post to semaphore that DSP data is ready */
  /* Link low pass filter I and Q data to local buffers */
  filter_data_i.samples_buf = buf_i[buf_cnt];
  filter_data_q.samples_buf = buf_q[buf_cnt];
  buf_cnt++;
  if( buf_cnt >= 2 ) buf_cnt = 0;

  int sval;
  sem_getvalue( &demod_semaphore, &sval );
  if( !sval ) sem_post( &demod_semaphore );

  return( AIRSPY_SUCCESS );
}

/*****************************************************************************/

/* Airspy_Read_Async()
 *
 * Pthread function for async reading of RTLSDR I/Q samples
 */
static void *Airspy_Read_Async(void *arg) {
  int ret = airspy_start_rx( device, Airspy_Data_Cb, NULL );

  if( ret != SUCCESS )
    fprintf( stderr, "airspy_read_async() returned %d\n", ret );

  sem_wait( &airspy_semaphore );

  return( NULL );
}

/*****************************************************************************/

/* Airspy_Set_Center_Frequency()
 *
 * Sets the Center Frequency of the Airspy Tuner
 */
static void Airspy_Set_Center_Frequency(uint32_t center_freq) {
  int ret;
  char mesg[MESG_SIZE];

  /* Set the Center Frequency of the Airspy Device */
  ret = airspy_set_freq( device, center_freq );
  if( ret != AIRSPY_SUCCESS )
  {
    Print_Message( "Failed to set SDR Frequency", ERROR_MESG );
    return;
  }

  /* Print out center frequency */
  center_freq /= 1000;
  snprintf( mesg, sizeof(mesg),
      "Set Center Frequency to %ukHz", center_freq );
  Print_Message( mesg, INFO_MESG );
}

/*****************************************************************************/

/* Airspy_Set_Tuner_Gain_Mode()
 *
 * Sets the Tuner Gain mode to Auto or Manual
 */
static bool Airspy_Set_Tuner_Gain_Mode(int mode) {
  int ret;

  /* Set Tuner LNA Gain Mode */
  ret = airspy_set_lna_agc( device, (uint8_t)mode );
  if( ret != AIRSPY_SUCCESS )
  {
    Print_Message(
        "Failed to set LNA Gain Mode", ERROR_MESG );
    return( false );
  }

  /* Set Tuner Mixer Gain Mode */
  ret = airspy_set_mixer_agc( device, (uint8_t)mode );
  if( ret != AIRSPY_SUCCESS )
  {
    Print_Message(
        "Failed to set Mixer Gain Mode", ERROR_MESG );
    return( false );
  }

  return( true );
}

/*****************************************************************************/

/* Airspy_Set_Tuner_Gain()
 *
 * Set the Tuner Gain if in Manual mode
 */
static bool Airspy_Set_Tuner_Gain(uint32_t gain) {
  char mesg[MESG_SIZE];

  /* Try to set the Tuner Gain */

  double value = MAX_LINEARITY_GAIN * (double)gain / 100.0 + 0.5;
  snprintf( mesg, sizeof(mesg), "Setting Tuner Gain to %ddB",
      (int)(value * MAX_GAIN / MAX_LINEARITY_GAIN) );
  Print_Message( mesg, INFO_MESG );

  int ret = airspy_set_linearity_gain( device, (uint8_t)value );
  if( ret != AIRSPY_SUCCESS )
  {
    Print_Message( "Failed to set Tuner Gain", ERROR_MESG );
    return( false );
  }

  return( true );
}

/*****************************************************************************/

/* Airspy_Set_Sample_Rate()
 *
 * Sets the Airspy Sample Rate
 */
static bool Airspy_Set_Sample_Rate(void) {
  /* Set Airspy Sample Rate */
  int ret = airspy_set_samplerate( device, rc_data.sdr_samplerate );
  if( ret != AIRSPY_SUCCESS )
  {
    Print_Message( "Failed to set ADC Sample Rate", ERROR_MESG );
    return( false );
  }

  return( true );
}

/*****************************************************************************/

/* Airspy_Open_Device()
 *
 * Opens an Airspy SDR device for use
 */
static bool Airspy_Open_Device(void) {
  /* Open Airspy Device */
  int ret = airspy_open( &device );
  if( ret != AIRSPY_SUCCESS )
  {
    Print_Message( "Failed to open Airspy device", ERROR_MESG );
    return( false );
  }

  return( true );
}

/*****************************************************************************/

/* Airspy_Initialize()
 *
 * Initializes an Airspy SDR device for use
 */
bool Airspy_Initialize(void) {
  int ret;

  /* Open Airspy Device */
  if( !Airspy_Open_Device() )
    return( false );

  /* Set the Airspy ADC/DSP sample rate */
  if( !Airspy_Set_Sample_Rate() )
    return( false );

  /* Demodulator effective sample rate */
  rc_data.demod_samplerate = rc_data.sdr_samplerate / AIRSPY_DECIMATE;

  /* Set sample type */
  airspy_set_sample_type( device, AIRSPY_SAMPLE_INT16_IQ );

  /* Set Tuner Gain Mode to Auto */
  if( rc_data.tuner_gain == 0 )
  {
    Print_Message( "Setting Tuner Gain Mode to Auto", INFO_MESG );
    if( !Airspy_Set_Tuner_Gain_Mode(AIRSPY_GAIN_AUTO) )
      return( false );
  }
  else
  {
    /* Set Tuner Gain Mode to Manual setting */
    Print_Message( "Setting Tuner Gain Mode to Manual", INFO_MESG );
    if( !Airspy_Set_Tuner_Gain_Mode(AIRSPY_GAIN_MANUAL) )
      return( false );
    if( !Airspy_Set_Tuner_Gain(rc_data.tuner_gain) )
      return( false );
  }

  /* Set the Center Frequency of the Airspy Device */
  Airspy_Set_Center_Frequency( rc_data.sdr_center_freq );

  /* Create a thread for async read from Airspy device */
  ret = pthread_create( &airspy_pthread_id, NULL, Airspy_Read_Async, NULL );
  if( ret != SUCCESS )
  {
    Print_Message( "Failed to create Airspy Streaming Thread", ERROR_MESG );
    return( false );
  }
  sleep( 1 );

  sem_init( &airspy_semaphore,  0, 0 );

  return( true );
}

/*****************************************************************************/

/* Airspy_Close_Device()
 *
 * Closes and frees an open Airspy device
 */
void Airspy_Close_Device(void) {
  if( device != NULL )
  {
    if( airspy_is_streaming(device) )
      airspy_stop_rx( device );
    airspy_close( device );
    device = NULL;
  }
}
