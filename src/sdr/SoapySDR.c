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

#include "SoapySDR.h"

#include "../common/common.h"
#include "../common/shared.h"
#include "../mlrpt/utils.h"

#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>

#include <complex.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*****************************************************************************/

/* Range of gain slider */
#define GAIN_SCALE  100.0

#define DATA_SCALE  10.0

/* DSP filter parameters */
#define FILTER_RIPPLE   5.0
#define FILTER_POLES    6

/*****************************************************************************/

static void SoapySDR_Close_Device(void);
static void *SoapySDR_Stream(void *pid);

/*****************************************************************************/

static SoapySDRDevice *sdr = NULL;
static SoapySDRStream *rxStream    = NULL;
static complex short  *stream_buff = NULL;
static double  *data_buf_i[2] = { NULL, NULL };
static double  *data_buf_q[2] = { NULL, NULL };
static size_t   stream_mtu;
static uint32_t sdr_decimate;
static double   data_scale;
static uint32_t sdr_samplerate, sdr_buf_length;
double demod_samplerate;

/*****************************************************************************/

/* SoapySDR_Close_Device()
 *
 * Closes thr RTL-SDR device, if open
 */
static void SoapySDR_Close_Device(void) {
  int ret;

  /* Deactivate and close the stream */
  ClearFlag( STATUS_SOAPYSDR_INIT );
  if( (rxStream != NULL) && (sdr != NULL) )
  {
    ret = SoapySDRDevice_deactivateStream( sdr, rxStream, 0, 0 );
    if( ret != SUCCESS )
    {
      Print_Message( "Failed to Deactivate Stream", ERROR_MESG );
      Print_Message( SoapySDRDevice_lastError(), ERROR_MESG );
    }

    ret = SoapySDRDevice_closeStream( sdr, rxStream );
    if( ret != SUCCESS )
    {
      Print_Message( "Failed to Close Stream", ERROR_MESG );
      Print_Message( SoapySDRDevice_lastError(), ERROR_MESG );
    }

    rxStream = NULL;
  } /* if( rxStream != NULL ) */

  /* Close the SDR device */
  if( sdr != NULL )
  {
    ret = SoapySDRDevice_unmake( sdr );
    if( ret != SUCCESS )
    {
      Print_Message( "Failed to Close SDR device", ERROR_MESG );
      Print_Message( SoapySDRDevice_lastError(), ERROR_MESG );
    }
    sdr = NULL;
  }

  /* Free the samples buffer */
  free_ptr( (void **)&stream_buff );
  free_ptr( (void **)&data_buf_i[0] );
  free_ptr( (void **)&data_buf_q[0] );
  free_ptr( (void **)&data_buf_i[1] );
  free_ptr( (void **)&data_buf_q[1] );

  /* De-initialize Low Pass filter */
  Deinit_Chebyshev_Filter( &filter_data_i );
  Deinit_Chebyshev_Filter( &filter_data_q );

  ClearFlag( STATUS_STREAMING );
}

/*****************************************************************************/

/* SoapySDR_Stream()
 *
 * Runs in a thread of its own and loops around the
 * SoapySDRDevice_readStream() streaming function
 */
static void *SoapySDR_Stream(void *pid) {
  /* Soapy streaming buffers */
  void *buffs[] = { stream_buff };
  int flags = 0;
  long long timeNs = 0;
  long timeout;

  double temp_i, temp_q;

  uint32_t
    sdr_decim_cnt = 0,  /* Samples decimation counter */
    buf_cnt       = 0,  /* Index to current data ring buffer */
    samp_buf_idx  = 0,  /* Output samples buffer index */
    strm_buf_idx  = sdr_buf_length; /* Streaming buffer index */


  /* Data transfer timeout in uSec,
   * 10x longer to avoid dropped samples */
  timeout  = (long)stream_mtu * 10000000;
  timeout /= (long)sdr_samplerate;

  /* Loop around SoapySDRDevice_readStream()
   * till reception stopped by the user */
  while( isFlagSet(STATUS_RECEIVING) )
  {
    /* We need sdr_decimate summations to decimate samples */
    while( samp_buf_idx < sdr_buf_length )
    {
      /* Summate sdr_decimate samples into one samples buffer */
      temp_i = 0.0;
      temp_q = 0.0;
      for( sdr_decim_cnt = 0; sdr_decim_cnt < sdr_decimate; sdr_decim_cnt++ )
      {
        /* Read new data from the sample stream when exhausted */
        if( strm_buf_idx >= sdr_buf_length )
        {
          /* Read stream I/Q data from SDR device */
          SoapySDRDevice_readStream(
              sdr, rxStream, buffs, stream_mtu, &flags, &timeNs, timeout );
          strm_buf_idx = 0;
        }

        /* Summate samples to decimate */
        temp_i += creal( (complex double)stream_buff[strm_buf_idx] );
        temp_q += cimag( (complex double)stream_buff[strm_buf_idx] );
        strm_buf_idx++;
      }

      /* Top up Chebyshev LP filter buffers */
      data_buf_i[buf_cnt][samp_buf_idx] = temp_i / data_scale;
      data_buf_q[buf_cnt][samp_buf_idx] = temp_q / data_scale;

      samp_buf_idx++;
    }
    samp_buf_idx = 0;

    // Writes IQ samples to file, for testing only
    /*{
      static FILE *fdi = NULL, *fdq = NULL;
      if( fdi == NULL ) fdi = fopen( "i.s", "w" );
      if( fdq == NULL ) fdq = fopen( "q.s", "w" );
      fwrite( data_buf_i[buf_cnt],
          sizeof(double), (size_t)sdr_buf_length, fdi );
      fwrite( data_buf_q[buf_cnt],
          sizeof(double), (size_t)sdr_buf_length, fdq );
    }*/

    // Reads IQ samples from file, for testing only
    /* {
      static FILE *fdi = NULL, *fdq = NULL;
      if( fdi == NULL ) fdi = fopen( "i.s", "r" );
      if( fdq == NULL ) fdq = fopen( "q.s", "r" );
      fread( data_buf_i[buf_cnt],
          sizeof(double), (size_t)sdr_buf_length, fdi );
      fread( data_buf_q[buf_cnt],
          sizeof(double), (size_t)sdr_buf_length, fdq );
    }*/

    /* // Writes the phase angle of samples, for testing only
       if( isFlagSet(STATUS_DECODING) )
       {
       static double prev = 0.0;
       double phase, delta, x, y;
       for( uint32_t idx = 0; idx < sdr_buf_length; idx++ )
       {
        x = (double)(data_buf_i[buf_cnt][idx]);
        y = (double)(data_buf_q[buf_cnt][idx]);
        phase = atan2( fabs(x), fabs(y) ) * 57.3;
        if( (x > 0.0) && (y < 0.0) ) phase = 360.0 - phase;
        if( (x < 0.0) && (y > 0.0) ) phase = 180.0 - phase;
        if( (x < 0.0) && (y < 0.0) ) phase = 180.0 + phase;
        delta = phase - prev;
        prev  = phase;
        printf( "%6.1f  %6.1f\n", phase, delta );
      }
    } */

    /* Link local buffer to filter's data buffer */
    filter_data_i.samples_buf = data_buf_i[buf_cnt];
    filter_data_q.samples_buf = data_buf_q[buf_cnt];
    buf_cnt++;
    if( buf_cnt >= 2 ) buf_cnt = 0;

    int sval;
    sem_getvalue( &demod_semaphore, &sval );
    if( !sval ) sem_post( &demod_semaphore );
  } /* while( isFlagSet(STATUS_RECEIVING) ) */

  /* Close device when streaming is stopped */
  SoapySDR_Close_Device();

  /* Will de-initialize systems and free
   * buffers only if (hopefully) its safe */
  Cleanup();

  return( NULL );
}

/*****************************************************************************/

/* SoapySDR_Set_Center_Freq()
 *
 * Sets the Center Frequency of the RTL-SDR Tuner
 */
bool SoapySDR_Set_Center_Freq(uint32_t center_freq) {
  /* Set the Center Frequency of the Tuner */
  int ret = SoapySDRDevice_setFrequency(
      sdr, SOAPY_SDR_RX, 0, (double)center_freq, NULL );
  if( ret != SUCCESS )
  {
    Print_Message( "Failed to set Center Frequency", ERROR_MESG );
    Print_Message( SoapySDRDevice_lastError(), ERROR_MESG );
    return( false );
  }

  return( true );
}

/*****************************************************************************/

/* SoapySDR_Set_Tuner_Gain_Mode()
 *
 * Sets the Tuner Gain mode to Auto or Manual
 */
void SoapySDR_Set_Tuner_Gain_Mode(void) {
  int ret;

  if( isFlagSet(TUNER_GAIN_AUTO) )
  {
    /* Check for support of auto gain control */
    if( !SoapySDRDevice_hasGainMode(sdr, SOAPY_SDR_RX, 0) )
    {
      Print_Message( "Device has no Auto Gain Control", ERROR_MESG );
      return;
    }

    /* Set auto gain control */
    SoapySDR_Set_Tuner_Gain( 100.0 );
    ret = SoapySDRDevice_setGainMode( sdr, SOAPY_SDR_RX, 0, true );
    if( ret != SUCCESS )
    {
      Print_Message( "Failed to Set Auto Gain Control", ERROR_MESG );
      Print_Message( SoapySDRDevice_lastError(), ERROR_MESG );
      return;
    }

    Print_Message( "Set Auto Gain Control", INFO_MESG );
  }
  else
  {
    /* Set manual gain control */
    ret = SoapySDRDevice_setGainMode( sdr, SOAPY_SDR_RX, 0, false );
    if( ret != SUCCESS )
    {
      Print_Message( "Failed to Set Manual Gain Control", ERROR_MESG );
      Print_Message( SoapySDRDevice_lastError(), ERROR_MESG );
      return;
    }

    Print_Message( "Set Manual Gain Control", INFO_MESG );
    SoapySDR_Set_Tuner_Gain( rc_data.tuner_gain );
  }
}

/*****************************************************************************/

/* SoapySDR_Set_Tuner_Gain()
 *
 * Set the Tuner Gain if in Manual mode
 */
void SoapySDR_Set_Tuner_Gain(double gain) {
  /* Get range of available gains */
  SoapySDRRange range =
    SoapySDRDevice_getGainRange( sdr, SOAPY_SDR_RX, 0 );

  /* Scale gain request to range of available gains */
  gain *= range.maximum - range.minimum;
  gain /= GAIN_SCALE;
  gain += range.minimum;

  /* Set device receiver gain */
  int ret = SoapySDRDevice_setGain( sdr, SOAPY_SDR_RX, 0, gain );
  if( ret != SUCCESS )
  {
    Print_Message( "Failed to set Tuner Gain", ERROR_MESG );
    Print_Message( SoapySDRDevice_lastError(), ERROR_MESG );
    return;
  }
}

/*****************************************************************************/

/* SoapySDR_Init()
 *
 * Initialize SoapySDR by finding the specified SDR device,
 * instance it and setting up its working parameters */
bool SoapySDR_Init(void) {
  int ret = 0;
  size_t length, key, idx, mreq;
  SoapySDRKwargs *results;
  SoapySDRRange *range;

  uint32_t temp;

  /* Abort if already init */
  if( isFlagSet(STATUS_SOAPYSDR_INIT) )
    return( true );

  /* Enumerate SDR devices, abort if no devices found */
  results = SoapySDRDevice_enumerate( NULL, &length );
  if( length == 0 )
  {
    Print_Message( "No SoapySDR Device found", ERROR_MESG );
    return( false );
  }

  /* Use SDR device specified by index alone
   * if "auto" driver name specified in config */
  if (isFlagSet(AUTO_DETECT_SDR)) {
      Print_Message("Will use Auto-Detected SDR Device", INFO_MESG);
      idx = rc_data.device_index;
  }
  else {
      /* Look for device matching specified driver */

      for (idx = 0; idx < length; idx++) {
          for (key = 0; key < results[idx].size; key++) {
              /* Look for "driver" key */
              ret = strcmp(results[idx].keys[key], "driver");

              if (ret == 0) {
                  /* Match driver to one specified in config */
                  ret = strcmp(results[idx].vals[key], rc_data.device_driver);

                  if ((ret == 0) && (idx == rc_data.device_index))
                      break;
              }
          }

          if ((ret == 0) && (idx == rc_data.device_index))
              break;
      }

      /* If no matching device found, abort */
      if (idx == length) {
          Print_Message( "No specified device was found", ERROR_MESG );
          return false;
      }
  }

  /* Find SDR driver name for auto detect case */
  if (isFlagSet(AUTO_DETECT_SDR)) {
      for (key = 0; key < results[idx].size; key++) {
          /* Look for "driver" key */
          ret = strcmp(results[idx].keys[key], "driver");

          if (ret == 0)
              Strlcpy(rc_data.device_driver, results[idx].vals[key],
                      sizeof(rc_data.device_driver));
      }
  }

  /* Create device instance, abort on error */
  sdr = SoapySDRDevice_make( &results[idx] );
  if( sdr == NULL )
  {
    Print_Message( "SoapySDRDevice_make() failed:", ERROR_MESG );
    Print_Message( SoapySDRDevice_lastError(), ERROR_MESG );
    return( false );
  }
  SoapySDRKwargsList_clear( results, length );

  /* Set the Center Frequency of the RTL_SDR Device */
  if( !SoapySDR_Set_Center_Freq( rc_data.sdr_center_freq ) )
    return( false );

  /* Set the Frequency Correction factor for the device */
  if( SoapySDRDevice_hasFrequencyCorrection(sdr, SOAPY_SDR_RX, 0) )
  {
    Print_Message( "Device has Frequency Correction", INFO_MESG );
    if (rc_data.freq_correction != 0.0) {
      ret = SoapySDRDevice_setFrequencyCorrection(
          sdr, SOAPY_SDR_RX, 0, rc_data.freq_correction );
      if( ret != SUCCESS )
      {
        Print_Message( "Failed to set Frequency Correction", ERROR_MESG );
        return( false );
      }
    }
  }

  /* Get the range of available sample rates */
  range = SoapySDRDevice_getSampleRateRange( sdr, SOAPY_SDR_RX, 0, &length );

  /*** List sampling rates and select lowest available ***/
  /* Prime this to high value to look for a minimum */
  sdr_samplerate = 100000000;

  /* TODO handle this more accurately */
  /* This is the minimum prefered value for the demodulator
   * effective sample rate. It could have been 2 * symbol_rate
   * but this can result in unfavorable sampling rates */
  temp = 4 * rc_data.symbol_rate;

  /* Select lowest sampling rate above minimum demod sampling rate */
  for( idx = 0; idx < length; idx++ )
  {
    /* Select the lowest sample rate above demod sample rate */
    uint32_t min = (uint32_t)range[idx].minimum;
    if( (sdr_samplerate > min) && (temp <= min) )
      sdr_samplerate = (uint32_t)range[idx].minimum;

  } /* for( idx = 0; idx < length; idx++ ) */
  free_ptr( (void **)&range );

  /* Set SDR Sample Rate */
  ret = SoapySDRDevice_setSampleRate(
      sdr, SOAPY_SDR_RX, 0, (double)sdr_samplerate );
  if( ret != SUCCESS )
  {
    Print_Message( "Failed to set ADC Sample Rate", ERROR_MESG );
    Print_Message( SoapySDRDevice_lastError(), ERROR_MESG );
    return( false );
  }

  /* Get actual sample rate and display */
  sdr_samplerate =
    (uint32_t)( SoapySDRDevice_getSampleRate(sdr, SOAPY_SDR_RX, 0) );

  /* Find sample rate decimation factor which
   * is the nearest power of 2, up to 32 */
  sdr_decimate = (uint32_t)sdr_samplerate / temp;
  int sav = 1;
  int min = 64; /* Prime to find a min */
  for( int i = 0; i <= 5; i++ )
  {
    int diff = abs( (int)sdr_decimate - (1 << i) );
    if( min > diff )
    {
      min = diff;
      sav = i;
    }
  }
  sdr_decimate = (uint32_t)( 1 << sav );
  data_scale   = (double)sdr_decimate * DATA_SCALE;

  /* We now need to calculate the sample rate decimation factor for
   * high sample rates and the new effective demodulator sample rate */
  demod_samplerate =
    (double)sdr_samplerate / (double)sdr_decimate;

  /* Set Tuner Gain Mode to auto or manual as per config file */
  SoapySDR_Set_Tuner_Gain_Mode();

  /* Set up receiving stream */
  Print_Message( "Setting up Receive Stream", INFO_MESG );
  rxStream = SoapySDRDevice_setupStream(
      sdr, SOAPY_SDR_RX, SOAPY_SDR_CS16, NULL, 0, NULL );
  if(!rxStream)
  {
    Print_Message( "Failed to set up Receive Stream", ERROR_MESG );
    Print_Message( SoapySDRDevice_lastError(), ERROR_MESG );
    return( false );
  }
  Print_Message( "Receive Stream set up OK", INFO_MESG );

  /* Find stream MTU and use as read buffer size */
  stream_mtu = SoapySDRDevice_getStreamMTU( sdr, rxStream );
  sdr_buf_length = (uint32_t)stream_mtu;

  /* Allocate stream buffer */
  mreq = stream_mtu * sizeof( complex short );
  mem_alloc( (void **)&stream_buff, mreq );

  /* Allocate local data buffers */
  mreq = stream_mtu * sizeof( double );
  for( idx = 0; idx < 2; idx++ )
  {
    mem_alloc( (void **)&data_buf_i[idx], mreq );
    mem_alloc( (void **)&data_buf_q[idx], mreq );
  }

  /* Init Chebyshev I/Q data Low Pass Filters */
  Init_Chebyshev_Filter(
      &filter_data_i,
      sdr_buf_length,
      rc_data.sdr_filter_bw,
      demod_samplerate,
      FILTER_RIPPLE,
      FILTER_POLES,
      FILTER_LOWPASS );

  Init_Chebyshev_Filter(
      &filter_data_q,
      sdr_buf_length,
      rc_data.sdr_filter_bw,
      demod_samplerate,
      FILTER_RIPPLE,
      FILTER_POLES,
      FILTER_LOWPASS );

  /* Wait a little for things to settle and set init OK flag */
  sleep( 1 );
  SetFlag( STATUS_SOAPYSDR_INIT );
  Print_Message( "SoapySDR Initialized OK", INFO_MESG );

  return( true );
}

/*****************************************************************************/

/* SoapySDR_Activate_Stream()
 *
 * Closes thr RTL-SDR device, if open
 */
bool SoapySDR_Activate_Stream(void) {
  /* Thread ID for the newly created thread */
  pthread_t pthread_id;

  /* Create a thread for async  read from SDR device */
  int ret = pthread_create( &pthread_id, NULL, SoapySDR_Stream, NULL );
  if( ret != SUCCESS )
  {
    Print_Message( "Failed to create Streaming thread", ERROR_MESG );
    Print_Message( SoapySDRDevice_lastError(), ERROR_MESG );
    ClearFlag( STATUS_SOAPYSDR_INIT );
    return( false );
  }

  /* Activate receive stream */
  ret = SoapySDRDevice_activateStream( sdr, rxStream, 0, 0, 0 );
  if( ret != SUCCESS )
  {
    Print_Message( "Failed to activate Receive Stream", ERROR_MESG );
    Print_Message( SoapySDRDevice_lastError(), ERROR_MESG );
    return( false );
  }
  Print_Message( "Receive Stream activated OK", INFO_MESG );
  SetFlag( STATUS_STREAMING );

  return( true );
}
