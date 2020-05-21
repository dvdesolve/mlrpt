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

#include "rc_config.h"

#include "../common/common.h"
#include "../common/shared.h"
#include "utils.h"

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/

/* Special characters */
#define LF   0x0A /* Line Feed */
#define CR   0x0D /* Carriage Return */
#define HT   0x09 /* Horizontal Tab  */

/* Max length of lines in config file */
#define MAX_CONFIG_STRLEN   80

/*****************************************************************************/

static int Load_Line(char *buff, FILE *pfile, const char *mesg);

/*****************************************************************************/

/* Load_Line()
 *
 * Loads a line from a file, aborts on failure. Lines beginning
 * with a '#' are ignored as comments. At the end of file EOF
 * is returned. Lines assumed maximum 80 characters long.
 */
static int Load_Line(char *buff, FILE *pfile, const char *mesg) {
  int
    num_chr, /* Number of characters read, excluding lf/cr */
    chr;     /* Character read by getc() */
  char error_mesg[MESG_SIZE];

  /* Prepare error message */
  snprintf( error_mesg, MESG_SIZE,
      "Error reading %s: Premature EOF (End Of File)", mesg );

  /* Clear buffer at start */
  buff[0] = '\0';
  num_chr = 0;

  /* Get next character, return error if chr = EOF */
  if( (chr = fgetc(pfile)) == EOF )
  {
    fclose( pfile );
    Print_Message( error_mesg, ERROR_MESG );
    return( EOF );
  }

  /* Ignore commented lines and eol/cr and tab */
  while(
      (chr == '#') ||
      (chr == HT ) ||
      (chr == CR ) ||
      (chr == LF ) )
  {
    /* Go to the end of line (look for LF or CR) */
    while( (chr != CR) && (chr != LF) )
      /* Get next character, return error if chr = EOF */
      if( (chr = fgetc(pfile)) == EOF )
      {
        fclose( pfile );
        Print_Message( error_mesg, ERROR_MESG );
        return( EOF );
      }

    /* Dump any CR/LF remaining */
    while( (chr == CR) || (chr == LF) )
      /* Get next character, return error if chr = EOF */
      if( (chr = fgetc(pfile)) == EOF )
      {
        fclose( pfile );
        Print_Message( error_mesg, ERROR_MESG );
        return( EOF );
      }

  } /* End of while( (chr == '#') || ... */

  /* Continue reading characters from file till
   * number of characters = 80 or EOF or CR/LF */
  while( num_chr < MAX_CONFIG_STRLEN )
  {
    /* If LF/CR reached before filling buffer, return line */
    if( (chr == LF) || (chr == CR) ) break;

    /* Enter new character to line buffer */
    buff[num_chr++] = (char)chr;

    /* Get next character */
    if( (chr = fgetc(pfile)) == EOF )
    {
      /* Terminate buffer as a string if chr = EOF */
      buff[num_chr] = '\0';
      return( SUCCESS );
    }

    /* Abort if end of line not reached at 80 char. */
    if( (num_chr == MAX_CONFIG_STRLEN) &&
        (chr != LF) && (chr != CR) )
    {
      /* Terminate buffer as a string */
      buff[num_chr] = '\0';
      snprintf( error_mesg, MESG_SIZE,
          "Error reading %s: Line longer than 80 characters", mesg );
      fclose( pfile );
      Print_Message( error_mesg, ERROR_MESG );
      return( ERROR );
    }

  } /* End of while( num_chr < max_chr ) */

  /* Terminate buffer as a string */
  buff[num_chr] = '\0';

  /* Report configuration item (only if Verbose) */
  snprintf( error_mesg, MESG_SIZE, "%s: %s", mesg, buff );
  Print_Message( error_mesg, INFO_MESG );

  return( SUCCESS );
}

/*****************************************************************************/

/* Load_Config()
 *
 * Loads the mlrptrc configuration file
 */
bool Load_Config(void) {
  char
    mesg[MESG_SIZE],
    line[MAX_CONFIG_STRLEN + 1] = {0}; /* Buffer for Load_Line */

  /* Config file pointer */
  FILE *mlrptrc;

  int idx;

  /* Open mlrptrc file */
  /* TODO recheck for possible overflow */
  snprintf( mesg, sizeof(mesg), "Opening Config file %s", rc_data.mlrpt_cfg );
  Print_Message( mesg, INFO_MESG );
  mlrptrc = fopen( rc_data.mlrpt_cfg, "r" );
  if( mlrptrc == NULL )
  {
    perror( rc_data.mlrpt_cfg );
    Print_Message( "Failed to open mlrptrc file", ERROR_MESG );
    return( false );
  }

  /*** Read runtime configuration data ***/

  /*** SDR Receiver configuration data ***/
  /* Read SDR Receiver Type to use */
  if( Load_Line(line, mlrptrc, "SDR Receiver Type") != SUCCESS )
    return( false );
  if( strcmp(line, "RTL-SDR") == 0 )
    rc_data.sdr_rx_type = SDR_TYPE_RTLSDR;
  else if( strcmp(line, "AIRSPY") == 0 )
    rc_data.sdr_rx_type = SDR_TYPE_AIRSPY;
  else
  {
    rc_data.sdr_rx_type = SDR_TYPE_RTLSDR;
    Print_Message(
        "Invalid SDR Radio Receiver type - Assuming RTL-SDR", ERROR_MESG );
  }

  /* Read Device Index, abort if EOF */
  if( Load_Line(line, mlrptrc, "SDR Device Index") != SUCCESS )
    return( false );
  idx = atoi( line );
  if( (idx < 0) || (idx > 8) )
  {
    idx = 0;
    Print_Message( "Invalid librtlsdr Device Index - Assuming 0", ERROR_MESG );
  }
  rc_data.rtlsdr_dev_index = (uint32_t)idx;

  /* Read SDR Receiver I/Q Sample Rate, abort if EOF */
  if( Load_Line(line, mlrptrc, "SDR Receiver I/Q Sample Rate") != SUCCESS )
    return( false );
  rc_data.sdr_samplerate = (uint32_t)( atoi(line) );

  /* Read Low Pass Filter Bandwidth, abort if EOF */
  if( Load_Line(line, mlrptrc, "Roofing Filter Bandwidth") != SUCCESS )
    return( false );
  rc_data.sdr_filter_bw = (uint32_t)atoi( line );

  /* Read Manual AGC Setting, abort if EOF */
  if( Load_Line(line, mlrptrc, "Manual Gain Setting") != SUCCESS )
    return( false );
  rc_data.tuner_gain = (uint32_t)( atoi(line) );
  if( rc_data.tuner_gain > 100 )
  {
    rc_data.tuner_gain = 100;
    Print_Message(
        "Invalid Manual Gain Setting. Assuming a value of 100%", ERROR_MESG );
  }

  /* Read Frequency Correction Factor, abort if EOF */
  if( Load_Line(line, mlrptrc, "Frequency Correction Factor") != SUCCESS )
    return( false );
  rc_data.rtlsdr_freq_corr = atoi( line );
  if( abs(rc_data.rtlsdr_freq_corr) > 100 )
  {
    rc_data.rtlsdr_freq_corr = 0;
    Print_Message( "Invalid Frequency Correction Factor - Assuming 0", ERROR_MESG );
  }

  /* Read Satellite Frequency in kHz, abort if EOF */
  if( Load_Line(line, mlrptrc, "Satellite Frequency kHz") != SUCCESS )
    return( false );
  if( !rc_data.sdr_center_freq )
  {
    double freq = atof( line ) * 1000.0;
    rc_data.sdr_center_freq = (uint32_t)freq;
  }

  /*** Image Decoding configuration data ***/
  /* Read default decode duration, abort if EOF */
  if( Load_Line(line, mlrptrc, "Decoding Duration") != SUCCESS )
    return( false );
  rc_data.default_oper_time = (uint32_t)atoi( line );
  if( !rc_data.operation_time )
    rc_data.operation_time = rc_data.default_oper_time;

  /* Warn if decoding duration is too long */
  if( rc_data.operation_time > MAX_OPERATION_TIME )
  {
    snprintf( mesg, sizeof(mesg),
        "Decoding Duration in mlrptrc (%u sec) excessive?\n",
        rc_data.operation_time );
    Print_Message( mesg, ERROR_MESG );
  }

  /* LRPT Demodulator Parameters */
  /* Read RRC Filter Order, abort if EOF */
  if( Load_Line(line, mlrptrc, "RRC Filter Order") != SUCCESS )
    return( false );
  rc_data.rrc_order = (uint32_t)( atoi(line) );

  /* Read RRC Filter alpha factor, abort if EOF */
  if( Load_Line(line, mlrptrc, "RRC Filter alpha factor") != SUCCESS )
    return( false );
  rc_data.rrc_alpha = atof( line );

  /* Read Costas PLL Loop Bandwidth, abort if EOF */
  if( Load_Line(line, mlrptrc, "Costas PLL Loop Bandwidth") != SUCCESS )
    return( false );
  rc_data.costas_bandwidth = atof( line );

  /* Read Costas PLL Locked Threshold, abort if EOF */
  if( Load_Line(line, mlrptrc, "Costas PLL Locked Threshold") != SUCCESS )
    return( false );
  rc_data.pll_locked   = atof( line );
  rc_data.pll_unlocked = rc_data.pll_locked * 1.05;

  /* Read Transmitter Modulation Mode, abort if EOF */
  if( Load_Line(line, mlrptrc, "Transmitter Modulation Mode") != SUCCESS )
    return( false );
  rc_data.psk_mode = (uint8_t)( atoi(line) );

  /* Read Transmitter QPSK Symbol Rate, abort if EOF */
  if( Load_Line(line, mlrptrc, "Transmitter QPSK Symbol Rate") != SUCCESS )
    return( false );
  rc_data.symbol_rate = (uint32_t)( atoi(line) );

  /* Read Demodulator Interpolation Factor, abort if EOF */
  if( Load_Line(line, mlrptrc, "Demodulator Interpolation Factor") != SUCCESS )
    return( false );
  rc_data.interp_factor = (uint32_t)( atoi(line) );

  /* Read LRPT Decoder Output Mode, abort if EOF */
  if( Load_Line(line, mlrptrc, "LRPT Decoder Output Mode") != SUCCESS )
    return( false );
  switch( atoi(line) )
  {
    case OUT_COMBO:
      SetFlag( IMAGE_OUT_COMBO );
      break;

    case OUT_SPLIT:
      SetFlag( IMAGE_OUT_SPLIT );
      break;

    case OUT_BOTH:
      SetFlag( IMAGE_OUT_COMBO );
      SetFlag( IMAGE_OUT_SPLIT );
      break;

    default:
      Print_Message(
          "Image Output Mode option invalid", ERROR_MESG );
      return( false );
  }

  /* Read LRPT Image Save file type, abort if EOF */
  if( Load_Line(line, mlrptrc, "Save As image file type") != SUCCESS )
    return( false );
  switch( atoi(line) )
  {
    case SAVEAS_JPEG:
      SetFlag( IMAGE_SAVE_JPEG );
      break;

    case SAVEAS_PGM:
      SetFlag( IMAGE_SAVE_PPGM );
      break;

    case SAVEAS_BOTH:
      SetFlag( IMAGE_SAVE_JPEG );
      SetFlag( IMAGE_SAVE_PPGM );
      break;

    default:
      Print_Message(
          "Image Save As option invalid", ERROR_MESG );
      return( false );
  }

  /* Read JPEG Quality Factor, abort if EOF */
  if( Load_Line(line, mlrptrc, "JPEG Quality Factor") != SUCCESS )
    return( false );
  rc_data.jpeg_quality = (float)atof( line );

  /* Read LRPT Decoder Save Raw Images flag, abort if EOF */
  if( Load_Line(line, mlrptrc, "LRPT Decoder Save Raw Images flag") != SUCCESS )
    return( false );
  if( atoi(line) ) SetFlag( IMAGE_RAW );

  /* Read LRPT Decoder Image Normalize flag, abort if EOF */
  if( Load_Line(line, mlrptrc, "LRPT Decoder Image Normalize flag") != SUCCESS )
    return( false );
  if( atoi(line) ) SetFlag( IMAGE_NORMALIZE );

  /* Read LRPT Decoder Image CLAHE Enhance flag, abort if EOF */
  if( Load_Line(line, mlrptrc, "LRPT Decoder Image CLAHE Enhance flag") != SUCCESS )
    return( false );
  if( atoi(line) ) SetFlag( IMAGE_CLAHE );

  /* Read LRPT Decoder Image Rectify flag, abort if EOF */
  if( Load_Line(line, mlrptrc, "LRPT Decoder Image Rectify flag") != SUCCESS )
    return( false );
  if( !rc_data.rectify_function )
    rc_data.rectify_function = (uint8_t)atoi( line );
  if( rc_data.rectify_function > 2 )
  {
    rc_data.rectify_function = 1;
    Print_Message( "Invalid Rectify Function index - assuming 1", ERROR_MESG );
  }
  if( rc_data.rectify_function )
    SetFlag( IMAGE_RECTIFY );

  /* Read LRPT Decoder Image Colorize flag, abort if EOF */
  if( Load_Line(line, mlrptrc, "LRPT Decoder Image Colorize flag") != SUCCESS )
    return( false );
  if( atoi(line) ) SetFlag( IMAGE_COLORIZE );

  /* Read LRPT Decoder Red APID, abort if EOF */
  if( Load_Line(line, mlrptrc, "LRPT Decoder Red APID") != SUCCESS )
    return( false );
  rc_data.apid[0] = (uint32_t)atoi( line );

  /* Read LRPT Decoder Green APID, abort if EOF */
  if( Load_Line(line, mlrptrc, "LRPT Decoder Green APID") != SUCCESS )
    return( false );
  rc_data.apid[1] = (uint32_t)atoi( line );

  /* Read LRPT Decoder Blue APID, abort if EOF */
  if( Load_Line(line, mlrptrc, "LRPT Decoder Blue APID") != SUCCESS )
    return( false );
  rc_data.apid[2] = (uint32_t)atoi( line );

  /* Read image APIDs to invert palette, abort if EOF */
  if( Load_Line(line, mlrptrc, "Invert Palette APID") != SUCCESS )
    return( false );
  char *nptr = line, *endptr = NULL;
  for( idx = 0; idx < 3; idx++ )
  {
    rc_data.invert_palette[idx] = (uint32_t)( strtol(nptr, &endptr, 10) );
    nptr = ++endptr;
  }

  /* Read Red Channel Normalization Range, abort if EOF */
  if( Load_Line(line, mlrptrc, "Red Channel Normalization Range") != SUCCESS )
    return( false );
  rc_data.norm_range[RED][NORM_RANGE_BLACK] = (uint8_t)atoi( line );
  idx = 0;
  while( line[idx++] != '-' );
  rc_data.norm_range[RED][NORM_RANGE_WHITE] = (uint8_t)atoi( &line[idx] );

  /* Read Green Channel Normalization Range, abort if EOF */
  if( Load_Line(line, mlrptrc, "Green Channel Normalization Range") != SUCCESS )
    return( false );
  rc_data.norm_range[GREEN][NORM_RANGE_BLACK] = (uint8_t)atoi( line );
  idx = 0;
  while( line[idx++] != '-' );
  rc_data.norm_range[GREEN][NORM_RANGE_WHITE] = (uint8_t)atoi( &line[idx] );

  /* Read Blue Channel Normalization Range, abort if EOF */
  if( Load_Line(line, mlrptrc, "Blue Channel Normalization Range") != SUCCESS )
    return( false );
  rc_data.norm_range[BLUE][NORM_RANGE_BLACK] = (uint8_t)atoi( line );
  idx = 0;
  while( line[idx++] != '-' );
  rc_data.norm_range[BLUE][NORM_RANGE_WHITE] = (uint8_t)atoi( &line[idx] );

  /* Read Blue Channel min pixel value in pseudo-color image */
  if( Load_Line(line, mlrptrc,
        "Blue Channel min pixel value in pseudo-color image") != SUCCESS )
    return( false );
  rc_data.colorize_blue_min = (uint8_t)atoi( line );

  /* Read Blue Channel max pixel value to enhance in pseudo-color image */
  if( Load_Line(line, mlrptrc,
        "Blue Channel max pixel value to enhance in pseudo-color image") != SUCCESS )
    return( false );
  rc_data.colorize_blue_max = (uint8_t)atoi( line );

  /* Read Blue Channel pixel value above which we assume it is a cloudy area */
  if( Load_Line(line, mlrptrc,
        "Blue Channel cloud area pixel value threshold") != SUCCESS )
    return( false );
  rc_data.clouds_threshold = (uint8_t)atoi( line );

  /* Check low pass filter bandwidth. It should be at least
   * 100kHz and no more than the SDR sampling rate */
  if( (rc_data.sdr_filter_bw < MIN_BANDWIDTH) ||
      (rc_data.sdr_filter_bw > MAX_BANDWIDTH) )
  {
    rc_data.sdr_filter_bw = 110000;
    Print_Message( "Invalid Roofing Filter Bandwidth - Assuming 110000", ERROR_MESG );
  }

  fclose( mlrptrc );

  return( true );
}
