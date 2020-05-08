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

#include "medet.h"
#include "../common/shared.h"

static int ok_cnt, total_cnt;

/*------------------------------------------------------------------------*/

  void
Medet_Init( void )
{
  int idx;

  /* Initialize things */
  Init_Correlator_Tables();
  Mj_Init();
  Mtd_Init( &mtd_record );

  for( idx = 0; idx < CHANNEL_IMAGE_NUM; idx++ )
    channel_image[idx] = NULL;
  channel_image_size = 0;

  ok_cnt    = 0;
  total_cnt = 0;
}

/*------------------------------------------------------------------------*/

  void
Decode_Image( uint8_t *in_buffer, int buf_len )
{
  BOOLEAN ok;

  while( mtd_record.pos < buf_len )
  {
    ok = Mtd_One_Frame( &mtd_record, in_buffer );
    if( ok )
    {
      Parse_Cvcdu( mtd_record.ecced_data, HARD_FRAME_LEN - 132 );
      ok_cnt++;
    }

    total_cnt++;
  } /* while( mtd_rec.pos < buf_len ) */
}

/*------------------------------------------------------------------------*/

