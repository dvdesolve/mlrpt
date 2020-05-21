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

#include "utils.h"

#include "../common/common.h"
#include "../common/shared.h"
#include "../demodulator/demod.h"
#include "../sdr/airspy.h"
#include "../sdr/rtlsdr.h"
#include "jpeg.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/*****************************************************************************/

static bool MkdirRecurse(const char *path);
static char *Filename(char *fpath);

/*****************************************************************************/

/* An int variable holding the single-bit flags */
static int Flags = 0;

/*****************************************************************************/

/* PrepareCacheDirectory
 *
 * Find and create (if necessary) dirs with configs and final pictures.
 * XDG specs are supported:
 * https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
 */
bool PrepareCacheDirectory(void) {
    char *var_ptr;

    /* cache for image storage is mandatory */
    /* TODO allow user to select his own directory */
    if ((var_ptr = getenv("XDG_CACHE_HOME")))
        snprintf(rc_data.mlrpt_imgs, sizeof(rc_data.mlrpt_imgs),
                "%s/%s", var_ptr, PACKAGE_NAME);
    else
        snprintf(rc_data.mlrpt_imgs, sizeof(rc_data.mlrpt_imgs),
                "%s/.cache/%s", getenv("HOME"), PACKAGE_NAME);

    if (!MkdirRecurse(rc_data.mlrpt_imgs)) {
        fprintf(stderr, "mlrpt: %s\n",
                "can't access/create images cache directory");

        return false;
    }

    return true;
}

/*****************************************************************************/

/* MkdirRecurse
 *
 * Create directory and all ancestors.
 * Adapted from:
 * https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
 */
static bool MkdirRecurse(const char *path) {
    const size_t len = strlen(path);
    char _path[MAX_FILE_NAME];
    char *p;

    /* save directory path in a mutable var */
    if (len > sizeof(_path) - 1) {
        errno = ENAMETOOLONG;
        return false;
    }

    strcpy(_path, path);

    /* walk through the path string */
    for (p = _path + 1; *p; p++) {
        if (*p == '/') {
            /* temporarily truncate path */
            *p = '\0';

            if (mkdir(_path, S_IRWXU) != 0) {
                if (errno != EEXIST)
                    return false;
            }

            /* restore path */
            *p = '/';
        }
    }

    if (mkdir(_path, S_IRWXU) != 0) {
        if (errno != EEXIST)
            return false;
    }

    return true;
}

/*****************************************************************************/

/* File_Name()
 *
 * Prepare a file name, use date and time if null argument
 */
void File_Name(char *file_name, uint32_t chn, const char *ext) {
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

    /* TODO possibly dangerous because of system string length limits */
    /* Combination pseudo-color image */
    if( chn == 3 )
      snprintf( file_name, MAX_FILE_NAME-1,
        "%s/%s-Combo%s", rc_data.mlrpt_imgs, tim, ext );
    else /* Channel image */
      snprintf( file_name, MAX_FILE_NAME-1,
        "%s/%s-Ch%u%s", rc_data.mlrpt_imgs, tim, chn, ext );
  }
  else /* Remove leading spaces from file_name */
  {
    int idx = 0;
    len = (int)strlen( file_name );
    while( (idx < len) && (file_name[idx] == ' ') )
    {
      file_name++;
      idx++;
    }
  }
}

/*****************************************************************************/

/* Filename()
 *
 * Finds file name in a file path
 * TODO may be it worth to use standard library routine
 */
static char *Filename(char *fpath) {
  int idx;

  idx = (int)strlen( fpath );
  while( (--idx >= 0) && (fpath[idx] != '/') );
  return( &fpath[++idx] );
}

/*****************************************************************************/

/* Usage()
 *
 * Prints usage information
 */
void Usage(void) {
  fprintf( stderr,
      "Usage:  mlrpt -[c <config-file> r <algorithm> f <frequency> s <HHMM-HHMM> t <min> -qihv]\n"
        "       -c: configuration file\n"
        "       -f: SDR receiver frequency (in kHz)\n"
        "       -r: image rectification algorithm: 0 = none, 1 = W2RG, 2 = 5B4AZ\n"
        "       -s: start and stop operation time in HHMM format\n"
        "       -t: operation duration in minutes\n"
        "       -q: run in quiet mode (no messages printed)\n"
        "       -i: flip images (useful for South to North passes)\n"
        "       -h: print this usage information and exit\n"
        "       -v: print version information and exit\n" );
}

/*****************************************************************************/

/* Print_Message()
 *
 * Prints out messages to either stdout or stderr
 * but only if verbose maode is enabled by user
 */
void Print_Message(const char *mesg, char type) {
  if( isFlagSet(VERBOSE_MODE) )
  {
    if( type == ERROR_MESG )
      fprintf( stderr, "!! mlrpt: %s\n", mesg );
    else if( type == INFO_MESG )
      printf( "mlrpt: %s\n", mesg );
  }
}

/*****************************************************************************/

/*** Memory allocation/freeing utils ***/
void mem_alloc(void **ptr, size_t req) {
  *ptr = malloc( req );
  if( *ptr == NULL )
  {
    perror( "mlrpt: A memory allocation request failed" );
    exit( -1 );
  }
  memset( *ptr, 0, req );
}

/*****************************************************************************/

void mem_realloc(void **ptr, size_t req) {
  *ptr = realloc( *ptr, req );
  if( *ptr == NULL )
  {
    perror( "mlrpt: A memory allocation request failed" );
    exit( -1 );
  }
}

/*****************************************************************************/

void free_ptr(void **ptr) {
  if( *ptr != NULL ) free( *ptr );
  *ptr = NULL;
}

/*****************************************************************************/

/* Open_File()
 *
 * Opens a file, aborts on error
 */
bool Open_File(FILE **fp, char *fname, const char *mode) {
  /* Message buffer */
  char mesg[64];

  /* Open Channel A image file */
  *fp = fopen( fname, mode );
  if( *fp == NULL )
  {
    perror( fname );
    snprintf( mesg, sizeof(mesg), "Failed to open file: %s", Filename(fname) );
    Print_Message( mesg, ERROR_MESG );
    return( false );
  }

  return( true );
}

/*****************************************************************************/

/* Save_Image_JPEG()
 *
 * Write an image buffer to file
 */
void Save_Image_JPEG(
        char *file_name,
        int width,
        int height,
        int num_channels,
        const uint8_t *pImage_data,
        compression_params_t *comp_params) {
  char mesg[MESG_SIZE];
  bool ret;

  /* Open image file, abort on error */
  snprintf( mesg, sizeof(mesg),
      "Saving Image: %s", Filename(file_name) );
  Print_Message( mesg, INFO_MESG );

  /* Compress image data to jpeg file, report failure */
  ret = jpeg_encoder_compress_image_to_file(
      file_name, width, height, num_channels, pImage_data, comp_params );
  if( !ret )
  {
    snprintf( mesg, sizeof(mesg),
        "Failed saving image: %s", Filename(file_name) );
    Print_Message( mesg, ERROR_MESG );
  }

  return;
}

/*****************************************************************************/

/* Save_Image_Raw()
 *
 * Write an image buffer to file
 */
void Save_Image_Raw(
        char *fname,
        const char *type,
        uint32_t width,
        uint32_t height,
        uint32_t max_val,
        uint8_t *buffer) {
  size_t size, fw;
  int ret;
  FILE *fp = 0;
  char mesg[MESG_SIZE];


  /* Open image file, abort on error */
  snprintf( mesg, sizeof(mesg), "Saving Image: %s", Filename(fname) );
  Print_Message( mesg, INFO_MESG );
  if( !Open_File(&fp, fname, "w") ) return;

  /* Write header in Ch-A output PPM files */
  ret = fprintf( fp, "%s\n%s\n%u %u\n%u\n", type,
      "# Created by mlrpt", width, height, max_val );
  if( ret < 0 )
  {
    fclose( fp );
    perror( "mlrpt: Error writing image to file" );
    Print_Message( "Error writing image to file", ERROR_MESG );
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
    perror( "mlrpt: Error writing image to file" );
    Print_Message( "Error writing image to file", ERROR_MESG );
    return;
  }

  fclose( fp );
  return;
}

/*****************************************************************************/

/* Cleanup()
 *
 * Cleanup before quitting or stopping action
 */
void Cleanup(void) {
  ClearFlag( ACTION_FLAGS_ALL );

  switch( rc_data.sdr_rx_type )
  {
    case SDR_TYPE_RTLSDR:
      RtlSdr_Close_Device();
      break;

    case SDR_TYPE_AIRSPY:
      Airspy_Close_Device();
      break;
  }

  Demod_Deinit();

  free_ptr( (void **)&(mtd_record.v.pair_distances) );
  free_ptr( (void **)&(filter_data_i.samples_buf) );
  free_ptr( (void **)&(filter_data_q.samples_buf) );

  ClearFlag( ALL_INITIALIZED );

  /* Cancel any alarms */
  alarm( 0 );
}

/*****************************************************************************/

/* Functions for testing and setting/clearing flags */

int isFlagSet(int flag) {
  return( Flags & flag );
}

/*****************************************************************************/

int isFlagClear(int flag) {
  return( !(Flags & flag) );
}

/*****************************************************************************/

void SetFlag(int flag) {
  Flags |= flag;
}

/*****************************************************************************/

void ClearFlag(int flag) {
  Flags &= ~flag;
}

/*****************************************************************************/

/* Strlcpy()
 *
 * Copies n-1 chars from src string into dest string. Unlike other
 * such library fuctions, this makes sure that the dest string is
 * null terminated by copying only n-1 chars to leave room for the
 * terminating char. n would normally be the sizeof(dest) string but
 * copying will not go beyond the terminating null of src string
 */
void Strlcpy(char *dest, const char *src, size_t n) {
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
}
