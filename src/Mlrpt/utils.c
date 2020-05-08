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

#include "utils.h"
#include "../common/shared.h"

/*------------------------------------------------------------------*/

/* File_Name()
 *
 * Prepare a file name, use date and time if null argument
 */
  void
File_Name( char *file_name, uint chn, const char *ext )
{
  int len; /* String length of file_name */

  /* If file_name is null, use date and time as file name */
  if( strlen(file_name) == 0 )
  {
    /* Variables for reading time (UTC) */
    time_t tp;
    struct tm utc;
    char tim[20];

    /* Prepare file name as UTC date-time. Default path is images/ */
    time( &tp );
    utc = *gmtime( &tp );
    strftime( tim, sizeof(tim), "%d%b%Y-%H%M", &utc );

    /* Combination pseudo-color image */
    if( chn == 3 )
    {
      if( strlen(rc_data.images_dir) )
        snprintf( file_name, MAX_FILE_NAME-1,
            "%s%s-Combo%s", rc_data.images_dir, tim, ext );
      else
        snprintf( file_name, MAX_FILE_NAME-1,
            "%simages/%s-Combo%s", rc_data.mlrpt_dir, tim, ext );
    }
    else /* Channel image */
    {
      if( strlen(rc_data.images_dir) )
        snprintf( file_name, MAX_FILE_NAME-1,
            "%s%s-Ch%d%s", rc_data.images_dir, tim, chn, ext );
      else
        snprintf( file_name, MAX_FILE_NAME-1,
            "%simages/%s-Ch%d%s", rc_data.mlrpt_dir, tim, chn, ext );
    }
  }
  else /* Remove leading spaces from file_name */
  {
    int idx = 0;
    len = (int)strlen( file_name );
    while( (file_name[idx] == ' ') && (idx < len) )
    {
      file_name++;
      idx++;
    }
  }

} /* End of File_Name() */

/*------------------------------------------------------------------------*/

/* Fname()
 *
 * Finds file name in a file path
 */

  char *
Fname( char *fpath )
{
  int idx;

  idx = (int)strlen( fpath );
  while( (--idx >= 0) && (fpath[idx] != '/') );
  return( &fpath[++idx] );

} /* Fname() */

/*------------------------------------------------------------------------*/

/* Print_Message()
 *
 * Prints out messages to either stdout or stderr
 * but only if verbose maode is enabled by user
 */
  void
Print_Message( const char *mesg, char type )
{
  if( isFlagSet(VERBOSE_MODE) )
  {
    if( type == ERROR_MESG )
      fprintf( stderr, "!! mlrpt: %s\n", mesg );
    else if( type == INFO_MESG )
      printf( "mlrpt: %s\n", mesg );
  }
} /* Print_Message() */

/*------------------------------------------------------------------------*/

/*  Usage()
 *
 *  Prints usage information
 */

  void
Usage( void )
{
  fprintf( stderr,
      _("Usage:  mlrpt -[c<config-file-name> f<frequency> s<hhmm-hhmm> t<sec> -qihv]\n"
        "       -c: Specify configuration file name in ~/mlrpt/\n"
        "       -f: Specify SDR Receiver Frequency (in kHz).\n"
        "       -r: Specify Image Rectification: 0 = None, 1 = W2RG, 2 = 5B4AZ.\n"
        "       -s: Start and Stop Time in hhmm of Operation.\n"
        "       -t: Duration in min of Operation.\n"
        "       -q: Run in Quiet mode (no messages printed).\n"
        "       -i: Invert (flip) Images. Useful for South to North passes.\n"
        "       -h: Print this usage information and exit.\n"
        "       -v: Print version number and exit.\n") );

} /* End of Usage() */

/*------------------------------------------------------------------------*/

/***  Memory allocation/freeing utils ***/
void mem_alloc( void **ptr, size_t req )
{
  *ptr = malloc( req );
  if( *ptr == NULL )
  {
    perror( _("mlrpt: A memory allocation request failed") );
    exit( -1 );
  }
  memset( *ptr, 0, req );
} /* End of void mem_alloc() */

/*------------------------------------------------------------------------*/

void mem_realloc( void **ptr, size_t req )
{
  *ptr = realloc( *ptr, req );
  if( *ptr == NULL )
  {
    perror( _("mlrpt: A memory allocation request failed") );
    exit( -1 );
  }
} /* End of void mem_realloc() */

/*------------------------------------------------------------------------*/

void free_ptr( void **ptr )
{
  if( *ptr != NULL ) free( *ptr );
  *ptr = NULL;
} /* End of void free_ptr() */

/*------------------------------------------------------------------------*/

/* Open_File()
 *
 * Opens a file, aborts on error
 */

  BOOLEAN
Open_File( FILE **fp, char *fname, const char *mode )
{
  /* Message buffer */
  char mesg[64];

  /* Open Channel A image file */
  *fp = fopen( fname, mode );
  if( *fp == NULL )
  {
    perror( fname );
    snprintf( mesg, sizeof(mesg), _("Failed to open file: %s"), Fname(fname) );
    Print_Message( mesg, ERROR_MESG );
    return( FALSE );
  }

  return( TRUE );
} /* End of Open_File() */

/*------------------------------------------------------------------------*/

/* Save_Image_JPEG()
 *
 * Save an image buffer to file in jpeg format
 */
  void
Save_Image_JPEG(
    char *file_name,
    int width, int height,
    int num_channels,
    const uint8_t *pImage_data,
    compression_params_t *comp_params )
{
  char mesg[MESG_SIZE];
  BOOLEAN ret;

  /* Report files saved */
  snprintf( mesg, sizeof(mesg),
      _("Saving Image: %s"), Fname(file_name) );
  Print_Message( mesg, INFO_MESG );

  /* Compress image data to jpeg file, report failure */
  ret = jepg_encoder_compress_image_to_file(
      file_name, width, height, num_channels, pImage_data, comp_params );
  if( !ret )
  {
    snprintf( mesg, sizeof(mesg),
        _("Failed saving image: %s"), Fname(file_name) );
    Print_Message( mesg, ERROR_MESG );
  }

  return;
} /* Save_Image_JPEG() */

/*------------------------------------------------------------------------*/

/* Save_Image_Raw()
 *
 * Save an image buffer to file in raw pgm or ppm format
 */

  void
Save_Image_Raw(
    char *fname, const char *type,
    uint width, uint height,
    uint max_val, uint8_t *buffer )
{
  size_t size, fw;
  int ret;
  FILE *fp = 0;
  char mesg[MESG_SIZE];


  /* Open image file, abort on error */
  snprintf( mesg, sizeof(mesg), _("Saving Image: %s"), Fname(fname) );
  Print_Message( mesg, INFO_MESG );
  if( !Open_File(&fp, fname, "w") ) return;

  /* Write header in Ch-A output PPM files */
  ret = fprintf( fp, "%s\n%s\n%d %d\n%d\n", type,
      _("# Created by mlrpt"), width, height, max_val );
  if( ret < 0 )
  {
    fclose( fp );
    perror( _("mlrpt: Error writing image to file") );
    Print_Message( _("Error writing image to file"), ERROR_MESG );
    return;
  }

  /* P6 type (PPM) files are 3* size in pixels */
  if( strcmp(type, "P6") == 0 )
    size = (size_t)( 3 * width * height );
  else
    size = (size_t)( width * height );

  /* Write image buffer to file, abort on error */
  fw = fwrite( buffer, 1, size, fp );
  if( fw != size )
  {
    fclose( fp );
    perror( _("mlrpt: Error writing image to file") );
    Print_Message( _("Error writing image to file"), ERROR_MESG );
    return;
  }

  fclose( fp );
  return;

} /* Save_Image_Raw() */

/*------------------------------------------------------------------------*/

/*  Cleanup()
 *
 *  Cleanup before quitting or stopping action
 */

  void
Cleanup( void )
{
  ClearFlag( ACTION_FLAGS_ALL );

  switch( rc_data.sdr_rx_type )
  {
#ifdef USE_LIBRTLSDR
    case SDR_TYPE_RTLSDR:
      RtlSdr_Close_Device();
      break;
#endif

#ifdef USE_LIBAIRSPY
    case SDR_TYPE_AIRSPY:
      Airspy_Close_Device();
      break;
#endif
  }

  if( demodulator )
  {
    Agc_Free( demodulator->agc );
    Costas_Free( demodulator->costas );
    free_ptr( (void **)&demodulator );
  }

  free_ptr( (void **)&(mtd_record.v.pair_distances) );
  free_ptr( (void **)&(filter_data_i.samples_buf) );
  free_ptr( (void **)&(filter_data_q.samples_buf) );

  ClearFlag( ALL_INITIALIZED );

  /* Cancel any alarms */
  alarm( 0 );

} /* Cleanup() */

/*------------------------------------------------------------------------*/

/* Functions for testing and setting/clearing flags */

/* An int variable holding the single-bit flags */
static int Flags = 0;

  int
isFlagSet( int flag )
{
  return( Flags & flag );
}

  int
isFlagClear( int flag )
{
  return( !(Flags & flag) );
}

  void
SetFlag( int flag )
{
  Flags |= flag;
}

  void
ClearFlag( int flag )
{
  Flags &= ~flag;
}

  void
ToggleFlag( int flag )
{
  Flags ^= flag;
}

/*------------------------------------------------------------------*/

/* Strlcpy()
 *
 * Copies n-1 chars from src string into dest string. Unlike other
 * such library fuctions, this makes sure that the dest string is
 * null terminated by copying only n-1 chars to leave room for the
 * terminating char. n would normally be the sizeof(dest) string but
 * copying will not go beyond the terminating null of src string
 */
  void
Strlcpy( char *dest, const char *src, size_t n )
{
  char ch = src[0];
  int idx = 0;

  /* Leave room for terminating null in dest */
  n--;

  /* Copy till terminating null of src or to n-1 */
  while( (ch != '\0') && (n > 0) )
  {
    dest[idx] = src[idx];
    idx++;
    ch = src[idx];
    n--;
  }

  /* Terminate dest string */
  dest[idx] = '\0';

} /* Strlcpy() */

/*------------------------------------------------------------------*/

/* Strlcat()
 *
 * Concatenates at most n-1 chars from src string into dest string.
 * Unlike other such library fuctions, this makes sure that the dest
 * string is null terminated by copying only n-1 chars to leave room
 * for the terminating char. n would normally be the sizeof(dest)
 * string but copying will not go beyond the terminating null of src

 */
  void
Strlcat( char *dest, const char *src, size_t n )
{
  char ch = dest[0];
  int idd = 0; /* dest index */
  int ids = 0; /* src  index */

  /* Find terminating null of dest */
  while( (n > 0) && (ch != '\0') )
  {
    idd++;
    ch = dest[idd];
    n--; /* Count remaining char's in dest */
  }

  /* Copy n-1 chars to leave room for terminating null */
  n--;
  ch = src[ids];
  while( (n > 0) && (ch != '\0') )
  {
    dest[idd] = src[ids];
    ids++;
    ch = src[ids];
    idd++;
    n--;
  }

  /* Terminate dest string */
  dest[idd] = '\0';

} /* Strlcat() */

/*------------------------------------------------------------------*/

/* Clamps a integer value between min and max */
  inline int
iClamp( int i, int min, int max )
{
  int ret = i;
  if( i < min ) ret = min;
  else if( i > max ) ret = max;
  return( ret );
}

/*------------------------------------------------------------------*/

/* Clamps a double value between min and max */
  inline double
dClamp( double x, double min, double max )
{
  double ret = x;
  if( x < min ) ret = min;
  else if( x > max ) ret = max;
  return( ret );
}

/*------------------------------------------------------------------*/

