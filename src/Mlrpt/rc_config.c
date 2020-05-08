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

#include "rc_config.h"
#include "../common/shared.h"

/*------------------------------------------------------------------------*/

/*  Load_Line()
 *
 *  Loads a line from a file, aborts on failure. Lines beginning
 *  with a '#' are ignored as comments. At the end of file EOF
 *  is returned. Lines assumed maximum 80 characters long.
 */

  static int
Load_Line( char *buff, FILE *pfile, const char *message )
{
  int
    num_chr, /* Number of characters read, excluding lf/cr */
    chr;     /* Character read by getc() */
  char mesg[MESG_SIZE];

  /* Prepare error message */
  snprintf( mesg, MESG_SIZE,
      _("Error reading %s: Premature EOF (End Of File)"), message );

  /* Clear buffer at start */
  buff[0] = '\0';
  num_chr = 0;

  /* Get next character, return error if chr = EOF */
  if( (chr = fgetc(pfile)) == EOF )
  {
    fclose( pfile );
    Print_Message( mesg, ERROR_MESG );
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
        Print_Message( mesg, ERROR_MESG );
        return( EOF );
      }

    /* Dump any CR/LF remaining */
    while( (chr == CR) || (chr == LF) )
      /* Get next character, return error if chr = EOF */
      if( (chr = fgetc(pfile)) == EOF )
      {
        fclose( pfile );
        Print_Message( mesg, ERROR_MESG );
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
      snprintf( mesg, MESG_SIZE,
          _("Error reading %s: Line longer than 80 characters"), message );
      fclose( pfile );
      Print_Message( mesg, ERROR_MESG );
      return( ERROR );
    }

  } /* End of while( num_chr < max_chr ) */

  /* Terminate buffer as a string */
  buff[num_chr] = '\0';

  /* Report configuration item (only if Verbose) */
  snprintf( mesg, MESG_SIZE, _("%s: %s"), message, buff );
  Print_Message( mesg, INFO_MESG );

  return( SUCCESS );

} /* End of Load_Line() */

/*------------------------------------------------------------------------*/

/*  Load_Config()
 *
 *  Loads the mlrptrc configuration file
 */

  BOOLEAN
Load_Config( void )
{
  char
    mesg[MESG_SIZE],
    rc_fpath[MAX_FILE_NAME],           /* File path to mlrptrc */
    line[MAX_CONFIG_STRLEN + 1] = {0}; /* Buffer for Load_Line */

  /* Config file pointer */
  FILE *mlrptrc;

  int idx;


  /* Setup file path to mlrptrc and working dir */
  snprintf( rc_fpath,
      sizeof(rc_fpath), "%s/mlrpt/%s", getenv("HOME"), rc_data.mlrpt_cfg );
  snprintf( rc_data.mlrpt_dir,
      sizeof(rc_data.mlrpt_dir), "%s/mlrpt/", getenv("HOME") );

  /* Open mlrptrc file */
  snprintf( mesg, sizeof(mesg), _("Opening Config file %s"), rc_fpath );
  Print_Message( mesg, INFO_MESG );
  mlrptrc = fopen( rc_fpath, "r" );
  if( mlrptrc == NULL )
  {
    perror( rc_fpath );
    Print_Message( _("Failed to open mlrptrc file"), ERROR_MESG );
    return( FALSE );
  }

  /*** Read runtime configuration data ***/

  /*** SDR Receiver onfiguration data ***/
  /* Read SDR Receiver Type to use */
  if( Load_Line(line, mlrptrc, "SDR Receiver Type") != SUCCESS )
    return( FALSE );
  if( strcmp(line, "RTL-SDR") == 0 )
    rc_data.sdr_rx_type = SDR_TYPE_RTLSDR;
  else if( strcmp(line, "AIRSPY") == 0 )
    rc_data.sdr_rx_type = SDR_TYPE_AIRSPY;
  else
  {
    rc_data.sdr_rx_type = SDR_TYPE_RTLSDR;
    Print_Message(
        _("Invalid SDR Radio Receiver type - Assuming RTL-SDR"), ERROR_MESG );
  }

  /* Read Device Index, abort if EOF */
  if( Load_Line(line, mlrptrc, "SDR Device Index") != SUCCESS )
    return( FALSE );
  idx = atoi( line );
  if( (idx < 0) || (idx > 8) )
  {
    idx = 0;
    Print_Message( _("Invalid librtlsdr Device Index - Assuming 0"), ERROR_MESG );
  }
  rc_data.rtlsdr_dev_index = (uint32_t)idx;

  /* Read SDR Receiver I/Q Sample Rate, abort if EOF */
  if( Load_Line(line, mlrptrc, _("SDR Receiver I/Q Sample Rate")) != SUCCESS )
    return( FALSE );
  rc_data.sdr_samplerate = (uint)( atoi(line) );

  /* Read Low Pass Filter Bandwidth, abort if EOF */
  if( Load_Line(line, mlrptrc, "Roofing Filter Bandwidth") != SUCCESS )
    return( FALSE );
  rc_data.sdr_filter_bw = (uint)atoi( line );

  /* Read Manual AGC Setting, abort if EOF */
  if( Load_Line(line, mlrptrc, _("Manual Gain Setting")) != SUCCESS )
    return( FALSE );
  rc_data.tuner_gain = (uint32_t)( atoi(line) );
  if( rc_data.tuner_gain > 100 )
  {
    rc_data.tuner_gain = 100;
    Print_Message(
        _("Invalid Manual Gain Setting. Assuming a value of 100%"), ERROR_MESG );
  }

  /* Read Frequency Correction Factor, abort if EOF */
  if( Load_Line(line, mlrptrc, "Frequency Correction Factor") != SUCCESS )
    return( FALSE );
  rc_data.rtlsdr_freq_corr = atoi( line );
  if( abs(rc_data.rtlsdr_freq_corr) > 100 )
  {
    rc_data.rtlsdr_freq_corr = 0;
    Print_Message( _("Invalid Frequency Correction Factor - Assuming 0"), ERROR_MESG );
  }

  /* Read Satellite Frequency in kHz, abort if EOF */
  if( Load_Line(line, mlrptrc, _("Satellite Frequency kHz")) != SUCCESS )
    return( FALSE );
  if( !rc_data.sdr_center_freq )
  {
    double freq = atof( line ) * 1000.0;
    rc_data.sdr_center_freq = (uint)freq;
  }

  /*** Image Decoding configuration data ***/
  /* Read default decode duration, abort if EOF */
  if( Load_Line(line, mlrptrc, _("Decoding Duration")) != SUCCESS )
    return( FALSE );
  rc_data.default_oper_time = (uint)atoi( line );
  if( !rc_data.operation_time )
    rc_data.operation_time = rc_data.default_oper_time;

  /* Warn if decoding duration is too long */
  if( rc_data.operation_time > MAX_OPERATION_TIME )
  {
    snprintf( mesg, sizeof(mesg),
        _("Decoding Duration in mlrptrc (%d sec) excessive?\n"),
        rc_data.operation_time );
    Print_Message( mesg, ERROR_MESG );
  }

  /* LRPT Demodulator Parameters */
  /* Read RRC Filter Order, abort if EOF */
  if( Load_Line(line, mlrptrc, _("RRC Filter Order")) != SUCCESS )
    return( FALSE );
  rc_data.rrc_order = (uint)( atoi(line) );

  /* Read RRC Filter alpha factor, abort if EOF */
  if( Load_Line(line, mlrptrc, _("RRC Filter alpha factor")) != SUCCESS )
    return( FALSE );
  rc_data.rrc_alpha = atof( line );

  /* Read Costas PLL Loop Bandwidth, abort if EOF */
  if( Load_Line(line, mlrptrc, _("Costas PLL Loop Bandwidth")) != SUCCESS )
    return( FALSE );
  rc_data.costas_bandwidth = atof( line );

  /* Read Costas PLL Locked Threshold, abort if EOF */
  if( Load_Line(line, mlrptrc, _("Costas PLL Locked Threshold")) != SUCCESS )
    return( FALSE );
  rc_data.pll_locked   = atof( line );
  rc_data.pll_unlocked = rc_data.pll_locked * 1.05;

  /* Read Transmitter Modulation Mode, abort if EOF */
  if( Load_Line(line, mlrptrc, _("Transmitter Modulation Mode")) != SUCCESS )
    return( FALSE );
  rc_data.psk_mode = (uint8_t)( atoi(line) );

  /* Read Transmitter QPSK Symbol Rate, abort if EOF */
  if( Load_Line(line, mlrptrc, _("Transmitter QPSK Symbol Rate")) != SUCCESS )
    return( FALSE );
  rc_data.symbol_rate = (uint)( atoi(line) );

  /* Read Demodulator Interpolation Factor, abort if EOF */
  if( Load_Line(line, mlrptrc, _("Demodulator Interpolation Factor")) != SUCCESS )
    return( FALSE );
  rc_data.interp_factor = (uint)( atoi(line) );

  /* Read LRPT Decoder Output Mode, abort if EOF */
  if( Load_Line(line, mlrptrc, _("LRPT Decoder Output Mode")) != SUCCESS )
    return( FALSE );
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
          _("Image Output Mode option invalid"), ERROR_MESG );
      return( FALSE );
  }

  /* Read LRPT Image Save file type, abort if EOF */
  if( Load_Line(line, mlrptrc, _("Save As image file type")) != SUCCESS )
    return( FALSE );
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
          _("Image Save As option invalid"), ERROR_MESG );
      return( FALSE );
  }

  /* Read JPEG Quality Factor, abort if EOF */
  if( Load_Line(line, mlrptrc, _("JPEG Quality Factor")) != SUCCESS )
    return( FALSE );
  rc_data.jpeg_quality = (float)atof( line );

  /* Read LRPT Decoder Save Raw Images flag, abort if EOF */
  if( Load_Line(line, mlrptrc, _("LRPT Decoder Save Raw Images flag")) != SUCCESS )
    return( FALSE );
  if( atoi(line) ) SetFlag( IMAGE_RAW );

  /* Read LRPT Decoder Image Normalize flag, abort if EOF */
  if( Load_Line(line, mlrptrc, _("LRPT Decoder Image Normalize flag")) != SUCCESS )
    return( FALSE );
  if( atoi(line) ) SetFlag( IMAGE_NORMALIZE );

  /* Read LRPT Decoder Image CLAHE Enhance flag, abort if EOF */
  if( Load_Line(line, mlrptrc, _("LRPT Decoder Image CLAHE Enhance flag")) != SUCCESS )
    return( FALSE );
  if( atoi(line) ) SetFlag( IMAGE_CLAHE );

  /* Read LRPT Decoder Image Rectify flag, abort if EOF */
  if( Load_Line(line, mlrptrc, _("LRPT Decoder Image Rectify flag")) != SUCCESS )
    return( FALSE );
  if( !rc_data.rectify_function )
    rc_data.rectify_function = (uint8_t)atoi( line );
  if( rc_data.rectify_function > 2 )
  {
    rc_data.rectify_function = 1;
    Print_Message( _("Invalid Rectify Function index - assuming 1"), ERROR_MESG );
  }
  if( rc_data.rectify_function )
    SetFlag( IMAGE_RECTIFY );

  /* Read LRPT Decoder Image Colorize flag, abort if EOF */
  if( Load_Line(line, mlrptrc, _("LRPT Decoder Image Colorize flag")) != SUCCESS )
    return( FALSE );
  if( atoi(line) ) SetFlag( IMAGE_COLORIZE );

  /* Read LRPT Decoder Red APID, abort if EOF */
  if( Load_Line(line, mlrptrc, _("LRPT Decoder Red APID")) != SUCCESS )
    return( FALSE );
  rc_data.apid[0] = (uint)atoi( line );

  /* Read LRPT Decoder Green APID, abort if EOF */
  if( Load_Line(line, mlrptrc, _("LRPT Decoder Green APID")) != SUCCESS )
    return( FALSE );
  rc_data.apid[1] = (uint)atoi( line );

  /* Read LRPT Decoder Blue APID, abort if EOF */
  if( Load_Line(line, mlrptrc, _("LRPT Decoder Blue APID")) != SUCCESS )
    return( FALSE );
  rc_data.apid[2] = (uint)atoi( line );

  /* Read image APIDs to invert palette, abort if EOF */
  if( Load_Line(line, mlrptrc, _("Invert Palette APID")) != SUCCESS )
    return( FALSE );
  char *nptr = line, *endptr = NULL;
  for( idx = 0; idx < 3; idx++ )
  {
    rc_data.invert_palette[idx] = (uint32_t)( strtol(nptr, &endptr, 10) );
    nptr = ++endptr;
  }

  /* Read Red Channel Normalization Range, abort if EOF */
  if( Load_Line(line, mlrptrc, _("Red Channel Normalization Range")) != SUCCESS )
    return( FALSE );
  rc_data.norm_range[RED][NORM_RANGE_BLACK] = (uint8_t)atoi( line );
  idx = 0;
  while( line[idx++] != '-' );
  rc_data.norm_range[RED][NORM_RANGE_WHITE] = (uint8_t)atoi( &line[idx] );

  /* Read Green Channel Normalization Range, abort if EOF */
  if( Load_Line(line, mlrptrc, _("Green Channel Normalization Range")) != SUCCESS )
    return( FALSE );
  rc_data.norm_range[GREEN][NORM_RANGE_BLACK] = (uint8_t)atoi( line );
  idx = 0;
  while( line[idx++] != '-' );
  rc_data.norm_range[GREEN][NORM_RANGE_WHITE] = (uint8_t)atoi( &line[idx] );

  /* Read Blue Channel Normalization Range, abort if EOF */
  if( Load_Line(line, mlrptrc, _("Blue Channel Normalization Range")) != SUCCESS )
    return( FALSE );
  rc_data.norm_range[BLUE][NORM_RANGE_BLACK] = (uint8_t)atoi( line );
  idx = 0;
  while( line[idx++] != '-' );
  rc_data.norm_range[BLUE][NORM_RANGE_WHITE] = (uint8_t)atoi( &line[idx] );

  /* Read Blue Channel min pixel value in pseudo-color image */
  if( Load_Line(line, mlrptrc,
        _("Blue Channel min pixel value in pseudo-color image")) != SUCCESS )
    return( FALSE );
  rc_data.colorize_blue_min = (uint8_t)atoi( line );

  /* Read Blue Channel max pixel value to enhance in pseudo-color image */
  if( Load_Line(line, mlrptrc,
        _("Blue Channel max pixel value to enhance in pseudo-color image")) != SUCCESS )
    return( FALSE );
  rc_data.colorize_blue_max = (uint8_t)atoi( line );

  /* Read Blue Channel pixel value above which we assume it is a cloudy area */
  if( Load_Line(line, mlrptrc,
        _("Blue Channel cloud area pixel value threshold")) != SUCCESS )
    return( FALSE );
  rc_data.clouds_threshold = (uint8_t)atoi( line );

  /* Read directory to save images in. If "--" then zero string */
  if( Load_Line(line, mlrptrc, _("Directory to save images in")) != SUCCESS )
    return( FALSE );
  if( strstr(line, "Default") != NULL )
    rc_data.images_dir[0] = '\0';
  else
    Strlcpy( rc_data.images_dir, line, sizeof(rc_data.images_dir) );

  /* Check low pass filter bandwidth. It should be at least
   * 100kHz and no more than the SDR sampling rate */
  if( (rc_data.sdr_filter_bw < MIN_BANDWIDTH) ||
      (rc_data.sdr_filter_bw > MAX_BANDWIDTH) )
  {
    rc_data.sdr_filter_bw = 110000;
    Print_Message( _("Invalid Roofing Filter Bandwidth - Assuming 110000"), ERROR_MESG );
  }

  fclose( mlrptrc );

  return( TRUE );
} /* End of Load_Config() */

/*------------------------------------------------------------------*/

