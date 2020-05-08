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

#include "rtlsdr.h"
#include "../common/shared.h"

/* rtlsdr device handle */
static rtlsdr_dev_t *dev = NULL;
static double
  *buf_i[2] = { NULL, NULL },
  *buf_q[2] = { NULL, NULL };
static uint8_t buf_cnt;

/*-------------------------------------------------------------------*/

/* RtlSdr_Cb()
 *
 * Callback function for rtlsdr_read_async()
 */
  static void
RtlSdr_Cb( unsigned char *buf, uint32_t len, void *ctx )
{
  uint32_t ids = 0, idx = 0;

  /* Convert sample values to range of int8_t */
  while( idx < len )
  {
    buf_i[buf_cnt][ids] = (double)( buf[idx] - 127 ) * 4.0;
    idx++;
    buf_q[buf_cnt][ids] = (double)( buf[idx] - 127 ) * 4.0;
    idx++;
    ids++;
  }

  /* Link low pass filter I and Q data to local buffers */
  filter_data_i.samples_buf = buf_i[buf_cnt];
  filter_data_q.samples_buf = buf_q[buf_cnt];
  buf_cnt++;
  if( buf_cnt >= 2 ) buf_cnt = 0;

  /* Post to semaphore that DSP data is ready */
  int sval;
  sem_getvalue( &demod_semaphore, &sval );
  if( !sval ) sem_post( &demod_semaphore );

} /* RtlSdr_Cb() */

/*-----------------------------------------------------------------------*/

/* RtlSdr_Read_Async()
 *
 * Pthread function for async reading of RTL I/Q samples
 */
  static void *
RtlSdr_Read_Async( void *pid )
{
  rtlsdr_read_async(
      dev, RtlSdr_Cb, pid, NUM_ASYNC_BUF, RTLSDR_BUF_LEN );

  return( NULL );
} /* RtlSdr_Read_Async() */

/*-----------------------------------------------------------------------*/

/* RtlSdr_Set_Center_Freq()
 *
 * Sets the Center Frequency of the RTL-SDR Tuner
 */
  static BOOLEAN
RtlSdr_Set_Center_Freq( uint32_t center_freq )
{
  uint32_t ret;
  char mesg[64];


  /* Set the Center Frequency of the RTL_SDR Device */
  if( rtlsdr_set_center_freq(dev, center_freq) != SUCCESS )
  {
    Print_Message( _("Failed to set SDR Frequency"), ERROR_MESG );
    return( FALSE );
  }

  /* Get the Center Frequency of the RTL_SDR Device */
  ret = rtlsdr_get_center_freq( dev );
  if( (ret != center_freq) || (ret == 0) )
  {
    Print_Message( _("Failed to set SDR Frequency"), ERROR_MESG );
    return( FALSE );
  }

  /* Display center frequency in messages */
  snprintf( mesg, sizeof(mesg),
      _("Set SDR Frequency to %0.1fkHz"),
      (double)center_freq / 1000.0 );
  Print_Message( mesg, INFO_MESG );

  return( TRUE );
} /* RtlSdr_Set_Center_Freq() */

/*-----------------------------------------------------------------------*/

/* RtlSdr_Set_Tuner_Gain_Mode()
 *
 * Sets the Tuner Gain mode to Auto or Manual
 */
  static BOOLEAN
RtlSdr_Set_Tuner_Gain_Mode( int mode )
{
  /* Set Tuner Gain Mode */
  int ret = rtlsdr_set_tuner_gain_mode( dev, mode );
  if( ret != SUCCESS )
  {
    Print_Message( _("Failed to set Tuner Gain Mode"), ERROR_MESG );
    return( FALSE );
  }

  /* Set RTL2832 Digital AGC */
  if( mode == TUNER_GAIN_AUTO )
  {
    Print_Message( _("Setting Tuner Gain Mode to Auto"), INFO_MESG );
    mode = 1;
  }
  else
  {
    Print_Message( _("Setting Tuner Gain Mode to Manual"), INFO_MESG );
    mode = 0;
  }
  ret = rtlsdr_set_agc_mode( dev, mode);
  if( ret != SUCCESS )
  {
    Print_Message( _("Failed to set RTL2832 Digital AGC"), ERROR_MESG );
    return( FALSE );
  }

  return( TRUE );
} /* RtlSdr_Set_Tuner_Gain_Mode() */

/*----------------------------------------------------------------------*/

/* RtlSdr_Set_Tuner_Gain()
 *
 * Set the Tuner Gain if in Manual mode
 */
  static BOOLEAN
RtlSdr_Set_Tuner_Gain( uint32_t gain )
{
  int
    idx,
    min,
    diff,
    igain,
    igx;

  int *gains = NULL, num_gains = 0;

  /* Get the available Tuner Gains */
  num_gains = rtlsdr_get_tuner_gains( dev, NULL );
  if( num_gains <= 0 )
  {
    Print_Message( _("Failed to get Tuner Number of Gains"), ERROR_MESG );
    return( FALSE );
  }

  /* Get the Gains List from the Device */
  mem_alloc( (void **)&gains, (size_t)num_gains * sizeof(int) );
  num_gains = rtlsdr_get_tuner_gains( dev, gains );
  if( num_gains <= 0 )
  {
    Print_Message( _("Failed to get Tuner Gains Array"), ERROR_MESG );
    return( FALSE );
  }

  /* Scale gain request to range of available gains */
  igain  = (int)gain * 10; /* Gains are in 1/10 dB */
  igain *= gains[num_gains - 1] - gains[0];
  igain /= AGC_SCALE_RANGE * 10; /* Gains are in 1/10 dB */
  igain -= gains[0];

  /* Find nearest available gain */
  min = 10000; igx = 0; /* Prime */
  for( idx = 0; idx < num_gains; idx++ )
  {
    diff = abs( gains[idx] - igain );
    if( diff < min )
    {
      min = diff;
      igx = idx;
    }
  }
  if( igx >= num_gains )
    igx = num_gains - 1;

  /* Try to set the Tuner Gain */
  char mesg[MESG_SIZE];
  snprintf( mesg, sizeof(mesg),
      _("Setting Tuner Gain to %ddB"), gains[igx] / 10 );
  Print_Message( mesg, INFO_MESG );
  int ret = rtlsdr_set_tuner_gain( dev, gains[igx] );
  if( ret != SUCCESS )
  {
    Print_Message( _("Failed to set Tuner Gain"), ERROR_MESG );
    return( FALSE );
  }
  free_ptr( (void **)&gains );

  return( TRUE );
} /* RtlSdr_Set_Tuner_Gain() */

/*----------------------------------------------------------------------*/

/* RtlSdr_Initialize()
 *
 * Initialize rtlsdr device status
 */
  BOOLEAN
RtlSdr_Initialize( void )
{
  /* rtlsdr device sample rate */
  uint32_t sample_rate;

  /* Check function return values */
  int ret = -1;

  /* Device USB strings */
  char
    manufact[256],
    product[256],
    serial[256],
    mesg[512];

  const char *tuner_types[] = { TUNER_TYPES };

  /* rtlsdr tuner handle */
  enum rtlsdr_tuner tuner;

  /* Device name */
  const char *dev_name = NULL;

  /* Thread ID for the newly created thread */
  pthread_t pthread_id;


  /* Open RTL-SDR Device */
  ret = rtlsdr_open( &dev, rc_data.rtlsdr_dev_index);
  if( ret != SUCCESS )
  {
    Print_Message( _("Failed to open RTL-SDR device"), ERROR_MESG );
    return( FALSE );
  }

  /* Get RTL Device Name */
  Print_Message( _("Initializing RTLSDR Device "), INFO_MESG );
  Print_Message( _("RTLSDR Device Information:"),  INFO_MESG );
  dev_name = rtlsdr_get_device_name( rc_data.rtlsdr_dev_index );

  /* Get device USB strings */
  ret = rtlsdr_get_device_usb_strings(
      rc_data.rtlsdr_dev_index, manufact, product, serial );
  if( ret != SUCCESS )
  {
    Print_Message( _("Failed to get device usb strings"), ERROR_MESG );
    return( FALSE );
  }

  /* Display device name and USB strings */
  snprintf( mesg, sizeof(mesg),
      _("Device Index: %d Name: %s\n"\
        "Manufacturer: %s Product: %s Serial: %s"),
      rc_data.rtlsdr_dev_index, dev_name, manufact, product, serial );
  Print_Message( mesg, INFO_MESG );

  /* Get the RTL_SDR Tuner type */
  tuner = rtlsdr_get_tuner_type( dev );
  if( tuner == RTLSDR_TUNER_UNKNOWN )
  {
    Print_Message( _("Failed to get Tuner type"), ERROR_MESG );
    return( FALSE );
  }
  snprintf( mesg, sizeof(mesg),
      _("Tuner Type: %s"), tuner_types[tuner] );
  Print_Message( mesg, INFO_MESG );

  /* Set the Center Frequency of the RTL_SDR Device */
  ret = RtlSdr_Set_Center_Freq( rc_data.sdr_center_freq );
  if( !ret ) return( FALSE );

  /* Set the Frequency Correction factor for the device */
  if( rc_data.rtlsdr_freq_corr )
  {
    ret = rtlsdr_set_freq_correction( dev, rc_data.rtlsdr_freq_corr );
    if( ret != SUCCESS )
    {
      Print_Message(
          _("Failed to set Frequency Correction factor"), ERROR_MESG );
      return( FALSE );
    }
  }

  /* Get the Frequency Correction factor from the device */
  rc_data.rtlsdr_freq_corr = rtlsdr_get_freq_correction( dev );
  snprintf( mesg, sizeof(mesg),
      _("Frequency Correction: %d ppm"), rc_data.rtlsdr_freq_corr );
  Print_Message( mesg, INFO_MESG );

  /* Set Tuner Gain Mode to Auto */
  if( rc_data.tuner_gain == 0 )
  {
    if( !RtlSdr_Set_Tuner_Gain_Mode(TUNER_GAIN_AUTO) )
      return( FALSE );
  }
  else
  {
    /* Set Tuner Gain Mode to Manual */
    if( !RtlSdr_Set_Tuner_Gain_Mode(TUNER_GAIN_MANUAL) )
      return( FALSE );
    if( !RtlSdr_Set_Tuner_Gain(rc_data.tuner_gain) )
      return( FALSE );
  }

  /* Set RTL Sample Rate */
  ret = rtlsdr_set_sample_rate( dev, rc_data.sdr_samplerate );
  if( (ret != SUCCESS) || (ret == -EINVAL) )
  {
    Print_Message( _("Failed to set ADC Sample Rate"), ERROR_MESG );
    return( FALSE );
  }

  /* Demodulator effective sample rate */
  rc_data.demod_samplerate = rc_data.sdr_samplerate;

  /* Get RTL Sample Rate */
  sample_rate = rtlsdr_get_sample_rate( dev );
  if( sample_rate == 0 )
  {
    Print_Message( _("Failed to get ADC Sample Rate"), ERROR_MESG );
    return( FALSE );
  }
  snprintf( mesg, sizeof(mesg),
      _("ADC Sample Rate: %d S/s"), sample_rate );
  Print_Message( mesg, INFO_MESG );

  /* Reset RTL data buffer */
  ret = rtlsdr_reset_buffer( dev );
  if( ret != SUCCESS )
  {
    Print_Message( _("Failed to Reset sampling Buffer"), ERROR_MESG );
    return( FALSE );
  }

  /* Allocate I/Q data buffers */
  size_t mreq = RTLSDR_BUF_LEN / 2 * sizeof( double );
  for( buf_cnt = 0; buf_cnt < 2; buf_cnt++ )
  {
    mem_alloc( (void **)&buf_i[buf_cnt], mreq );
    mem_alloc( (void **)&buf_q[buf_cnt], mreq );
  }
  buf_cnt = 0;

  /* Init Chebyshev I/Q data Low Pass Filters */
  Init_Chebyshev_Filter(
      &filter_data_i,
      RTLSDR_BUF_LEN / 2,
      rc_data.sdr_filter_bw,
      rc_data.sdr_samplerate,
      FILTER_RIPPLE,
      FILTER_POLES,
      FILTER_LOWPASS );

  Init_Chebyshev_Filter(
      &filter_data_q,
      RTLSDR_BUF_LEN / 2,
      rc_data.sdr_filter_bw,
      rc_data.sdr_samplerate,
      FILTER_RIPPLE,
      FILTER_POLES,
      FILTER_LOWPASS );

  /* Create a thread for async read from RTL device */
  ret = pthread_create( &pthread_id, NULL, RtlSdr_Read_Async, NULL );
  if( ret != SUCCESS )
  {
    Print_Message( _("Failed to create async read thread"), ERROR_MESG );
    return( FALSE );
  }
  sleep( 1 );

  Print_Message( _("RTLSDR Device Initialized OK"), INFO_MESG );

  return( TRUE );
} /* RtlSdr_Initialize() */

/*-----------------------------------------------------------------------*/

/* RtlSdr_Close_Device()
 *
 * Closes thr RTL-SDR device, if open
 */
  void
RtlSdr_Close_Device( void )
{
  if( dev != NULL )
  {
    rtlsdr_cancel_async( dev );
    rtlsdr_close( dev );
    dev = NULL;
  }

} /* RtlSdr_Close_Device() */

/*-----------------------------------------------------------------------*/

