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

#include "doqpsk.h"
#include "../common/shared.h"

/*------------------------------------------------------------------------*/

static uint8_t *isqrt_table = NULL;

/*------------------------------------------------------------------------*/

/* Assembles a byte by thresholding (<|> 128) 8 soft symbols at
 * an offset in the raw soft symbols buffer, to find a sync byte */
  static uint8_t
Byte_at_Offset( uint8_t *data )
{
  uint8_t result = 0, test, idx;

  /* Do thresholding of 8 consecutive symbols */
  for( idx = 0; idx <= 7; idx++ )
  {
    if( data[idx] < 128 )
      test = 1;
    else
      test = 0;

    /* Assemble a sync byte candidate */
    result |= test << idx;
  }

  return( result );
}

/*------------------------------------------------------------------------*/

/* The sync word could be in any of 8 different orientations, so we
 * will look for a repeating bit pattern the right distance apart.
 * The sync sequence is 00100111 repeating every 80 symbols in stream */
  static BOOLEAN
Find_Sync(
    uint8_t *data, int block_siz,
    int step, int depth,
    int *offset, uint8_t *sync )
{
  int idx, jdx, limit;
  BOOLEAN result;
  uint8_t temp;


  *offset = 0;
  result  = FALSE;

  /* Sync byte search limit 80 symbols * depth */
  limit = block_siz - step * depth;

  /* Search for a sync byte at the beginning of block */
  for( idx = 0; idx < limit; idx++ )
  {
    result = TRUE;

    /* Assemble a sync byte candidate */
    *sync = Byte_at_Offset( &data[idx] );

    /* Look for "depth" identical sync bytes
     * at "step" (80 syms) buffer offsets */
    for( jdx = 1; jdx <= depth; jdx++ )
    {
      temp = Byte_at_Offset( &data[idx + jdx * step] );
      if( *sync != temp )
      {
        result = FALSE;
        break;
      }
    } /* for( jdx = 1; jdx <= depth; jdx++ ) */

    if( result )
    {
      *offset = idx;
      break;
    }
  } /* for( idx = 0; idx < lim; idx++ ) */

  return( result );
}

/*------------------------------------------------------------------------*/

//80k stream: 00100111 36 bits 36 bits 00100111 36 bits 36 bits 00100111 ...
  static void
Resync_Stream( uint8_t *raw, int raw_siz, int *resync_siz )
{
  uint8_t *src = NULL;
  int
    temp,
    posn   = 0,
    offset = 0,
    limit1 = raw_siz - SYNCD_BUF_MARGIN,
    limit2 = raw_siz - INTER_SYNCDATA;

  uint8_t sync = 0, test = 0;
  BOOLEAN ok;

  mem_alloc( (void **)&src, (size_t)raw_siz );
  memcpy( src, raw, (size_t)raw_siz );

  *resync_siz = 0;
  while( posn < limit1 )
  {
    if( !Find_Sync( &src[posn], SYNCD_BLOCK_SIZ,
          INTER_SYNCDATA, SYNCD_DEPTH, &offset, &sync ) )
    {
      posn += SYNCD_BUF_STEP;
      continue;
    }
    posn += offset;

    while( posn < limit2 )
    {
      //Look ahead to prevent it losing sync on weak signal
      ok = FALSE;
      for( int idx = 0; idx < 128; idx++ )
      {
        temp = posn + idx * INTER_SYNCDATA;
        if( temp < limit2 )
        {
          test = Byte_at_Offset( &src[temp] );
          if( sync == test )
          {
            ok = TRUE;
            break;
          }
        } /* if( pos + tmp < lim2 ) */
      } /* for( idx = 0; idx <= 127; idx++ ) */
      if( !ok ) break;

      memcpy( &raw[*resync_siz], &src[posn + 8], INTER_DATA_LEN );
      posn += INTER_SYNCDATA;
      *resync_siz += INTER_DATA_LEN;

    } /* while( pos < lim2 ) */
  } /* while( pos < lim1 ) */

  free_ptr( (void **)&src );
}

/*------------------------------------------------------------------------*/

/* Resynching and de-interleaving the raw symbols buffer */
  void
De_Interleave(
    uint8_t *raw, int raw_siz,
    uint8_t **resync, int *resync_siz )
{
  int resync_buf_idx, raw_buf_idx;


  /* Re-synchronize the new raw data at the bottom of the raw
   * buffer after the INTER_MESG_LEN point and to the end */
  Resync_Stream( raw, raw_siz, resync_siz );

  /* Allocate the resynced and deinterleaved buffer */
  if( *resync_siz && (*resync_siz < raw_siz) )
    mem_alloc( (void **)resync, (size_t)*resync_siz );
  else
  {
    Print_Message( _("Resync_Stream() failed"), ERROR_MESG );
    exit( -1 );
  }

  /* We de-interleave INTER_BASE_LEN number of symbols, so that
   * all symbols in raw buffer up to this length are used up. */
  for( resync_buf_idx = 0; resync_buf_idx < *resync_siz; resync_buf_idx++ )
  {
    /* This is the convolutional interleaving
     * algorithm, used in reverse to de-interleave */
    raw_buf_idx =
      resync_buf_idx + (resync_buf_idx % INTER_BRANCHES) * INTER_BASE_LEN;
    if( raw_buf_idx < *resync_siz )
      (*resync)[resync_buf_idx] = raw[raw_buf_idx];
  }
}

/*------------------------------------------------------------------------*/

/* Make_Isqrt_Table()
 *
 * Makes the Integer square root table
 */
  void
Make_Isqrt_Table( void )
{
  uint16_t idx;

  mem_alloc( (void **)&isqrt_table, sizeof(uint8_t) * 16385 );
  for( idx = 0; idx < 16385; idx++ )
    isqrt_table[idx] = (uint8_t)( sqrt( (double)idx ) );

} /* Make_Isqrt_Table() */

/*------------------------------------------------------------------------*/

  void
Free_Isqrt_Table( void )
{
  free_ptr( (void **)&isqrt_table );
}

/*------------------------------------------------------------------------*/

/* Isqrt()
 *
 * Integer square root function
 */
  static inline int8_t
Isqrt( int a )
{
  if( a >= 0 )
    return( (int8_t)isqrt_table[a] );
  else
    return( -((int8_t)isqrt_table[-a]) );
} /* Isqrt() */

/*------------------------------------------------------------------------*/

/* De_Diffcode()
 *
 * "Fixes" a Differential Offset QPSK soft symbols
 * buffer so that it can be decoded by the LRPT decoder
 */
  void
De_Diffcode( int8_t *buff, uint32_t length )
{
  uint32_t idx;
  int x, y;
  int tmp1, tmp2;
  static int prev_i = 0;
  static int prev_q = 0;

  tmp1 = buff[0];
  tmp2 = buff[1];

  buff[0] = Isqrt(  buff[0] * prev_i );
  buff[1] = Isqrt( -buff[1] * prev_q );

  length -= 2;
  for( idx = 2; idx <= length; idx += 2 )
  {
    x = buff[idx];
    y = buff[idx+1];

    buff[idx]   = Isqrt(  buff[idx]   * tmp1 );
    buff[idx+1] = Isqrt( -buff[idx+1] * tmp2 );

    tmp1 = x;
    tmp2 = y;
  }


  prev_i = tmp1;
  prev_q = tmp2;

  return;
} /* De_Diffcode */

/*------------------------------------------------------------------------*/

