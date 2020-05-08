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

#include "image.h"
#include "../common/shared.h"

/*------------------------------------------------------------------------*/

/*  Normalize_Image()
 *
 *  Does histogram (linear) normalization of a pgm (P5) image file
 */

  void
Normalize_Image(
    uint8_t *image_buffer,
    uint image_size,
    uint8_t range_low,
    uint8_t range_high )
{
  uint
    hist[MAX_WHITE+1],  /* Intensity histogram of pgm image file  */
    pixel_cnt,          /* Total pixels counter for cut-off point */
    black_cutoff,       /* Count of pixels for black cutoff value */
    white_cutoff,       /* Count of pixels for white cutoff value */
    idx;

  uint8_t
    pixel_val_in,       /* Used for calculating normalized pixels */
    black_min_in,       /* Black cut-off pixel intensity value */
    white_max_in,       /* White cut-off pixel intensity value */
    val_range_in,       /* Range of intensity values in input image  */
    val_range_out;      /* Range of intensity values in output image */

  /* Abort for "empty" image buffers */
  if( image_size <= 0 )
  {
    Print_Message(
        _("Image buffer empty? Normalization not performed"), ERROR_MESG );
    return;
  }

  /* Clear histogram */
  for( idx = 0; idx <= MAX_WHITE; idx++ )
    hist[ idx ] = 0;

  /* Build image intensity histogram */
  for( idx = 0; idx < image_size; idx++ )
    hist[ image_buffer[idx] ]++;

  /* Determine black/white cut-off counts */
  black_cutoff = (image_size * BLACK_CUT_OFF) / 100;
  white_cutoff = (image_size * WHITE_CUT_OFF) / 100;

  /* Find black cut-off intensity value. Values below
   * MIN_BLACK are ignored to leave behind the black stripes
   * that seem to be sent by the satellite occasionally */
  pixel_cnt = 0;
  for( black_min_in = MIN_BLACK; black_min_in != MAX_WHITE; black_min_in++ )
  {
    pixel_cnt += hist[ black_min_in ];
    if( pixel_cnt >= black_cutoff ) break;
  }

  /* Find white cut-off intensity value */
  pixel_cnt = 0;
  for( white_max_in = MAX_WHITE; white_max_in != 0; white_max_in-- )
  {
    pixel_cnt += hist[ white_max_in ];
    if( pixel_cnt >= white_cutoff ) break;
  }

  /* Rescale pixels in image for required intensity range */
  val_range_in = white_max_in - black_min_in;
  if( val_range_in == 0 )
  {
    Print_Message(
        _("Image buffer flat? Normalization not performed"), ERROR_MESG );
    return;
  }

  /* Perform histogram normalization on images */
  Print_Message( _("Performing histogram normalization"), INFO_MESG );

  val_range_out = range_high - range_low;
  for( pixel_cnt = 0; pixel_cnt < image_size; pixel_cnt++ )
  {
    /* Input image pixel values relative to input black cut off.
     * Clamp pixel values within black and white cut off values */
    pixel_val_in  = (uint8_t)
      iClamp( image_buffer[pixel_cnt], black_min_in, white_max_in );
    pixel_val_in -= black_min_in;

    /* Normalized pixel values are scaled according to the ratio
     * of required pixel value range to input pixel value range */
    image_buffer[ pixel_cnt ] =
      range_low + ( pixel_val_in * val_range_out ) / val_range_in;
  }

} /* End of Normalize_Image() */

/*------------------------------------------------------------------------*/

/*  Flip_Image()
 *
 *  Flips a pgm (P5) image by 180 degrees
 */

  void
Flip_Image( unsigned char *image_buffer, uint image_size )
{
  uint idx; /* Index for loops etc */

  unsigned char
    *idx_temp,  /* Buffer location to be saved in temp    */
    *idx_swap;  /* Buffer location to be swaped with temp */

  /* Holds a pixel value temporarily */
  unsigned char temp;

  if( image_size <= 0 )
  {
    Print_Message(
        _("Image buffer empty? Rotation not performed"), ERROR_MESG );
    return;
  }

  /* Rotate image 180 degrees */
  Print_Message( _("Rotating image by 180 degrees"), INFO_MESG );
  for( idx = 0; idx < image_size / 2; idx++ )
  {
    idx_temp = image_buffer + idx;
    idx_swap = image_buffer - 1 + image_size - idx;
    temp = *( idx_temp );
    *( idx_temp ) = *( idx_swap );
    *( idx_swap ) = temp;
  }

} /* End of Flip_Image() */

/*------------------------------------------------------------------------*/

  void
Create_Combo_Image( uint8_t *combo_image )
{
  /* Color channels are 0 = red, 1 = green, 2 = blue
   * but it all depends on the APID options in mlrptrc */
  uint idx = 0, cnt;
  uint8_t range_red, range_green, range_blue;

  /* Perform speculative enhancement of watery areas and clouds */
  if( isFlagSet(IMAGE_COLORIZE) )
  {
    /* The Red channel image from the Meteor M2 satellite seems
     * to have some excess luminance after Normalization so here
     * the pixel value range is reduced to that specified in the
     * ~/mlrpt/mlrptrc configuration file */
    range_red =
      rc_data.norm_range[RED][NORM_RANGE_WHITE] -
      rc_data.norm_range[RED][NORM_RANGE_BLACK];

    /* The Blue channel image from the Meteor M2 satellite looses
     * pixel values (luminance) in the watery areas (seas and lakes)
     * after Normalization. Here the pixel value range in the dark
     * areas is enhanced according to values specified in the 
     * ~/mlrpt/mlrptrc configuration file */
    range_blue = rc_data.colorize_blue_max - rc_data.colorize_blue_min;

    for( cnt = 0; cnt < channel_image_size; cnt++ )
    {
      /* Progressively raise the value of blue channel
       * pixels in the dark areas to counteract the 
       * effects of histogram equalization, which darkens
       * the parts of the image that are watery areas */
      if( channel_image[BLUE][cnt] < rc_data.colorize_blue_min )
      {
        channel_image[BLUE][cnt] =
          rc_data.colorize_blue_min +
          ( channel_image[BLUE][cnt] * range_blue ) /
          rc_data.colorize_blue_max;
      }

      /* Colorize cloudy areas white pseudocolor. This helps
       * because the red channel does not render clouds right */
      if( channel_image[BLUE][cnt] > rc_data.clouds_threshold )
      {
        combo_image[idx++] = channel_image[BLUE][cnt];
        combo_image[idx++] = channel_image[BLUE][cnt];
        combo_image[idx++] = channel_image[BLUE][cnt];
      }
      else /* Just combine channels */
      {
        /* Reduce Red channel luminance as specified in config file */
        combo_image[idx++] = rc_data.norm_range[RED][NORM_RANGE_BLACK] +
          ( channel_image[RED][cnt] * range_red ) / MAX_WHITE;
        combo_image[idx++] = channel_image[GREEN][cnt];
        combo_image[idx++] = channel_image[BLUE][cnt];
      }
    } /* for( cnt = 0; cnt < (int)channel_image_size; cnt++ ) */

  } /* if( isFlagSet(IMAGE_COLORIZE) ) */
  else
  {
    /* Else combine channel images after changing pixel
     * value range to that specified in the config file */
    range_red =
      rc_data.norm_range[RED][NORM_RANGE_WHITE] -
      rc_data.norm_range[RED][NORM_RANGE_BLACK];
    range_green =
      rc_data.norm_range[GREEN][NORM_RANGE_WHITE] -
      rc_data.norm_range[GREEN][NORM_RANGE_BLACK];
    range_blue =
      rc_data.norm_range[BLUE][NORM_RANGE_WHITE] -
      rc_data.norm_range[BLUE][NORM_RANGE_BLACK];

    for( cnt = 0; cnt < channel_image_size; cnt++ )
    {
      combo_image[idx++] = rc_data.norm_range[RED][NORM_RANGE_BLACK] +
        ( channel_image[RED][cnt] * range_red ) / MAX_WHITE;
      combo_image[idx++] = rc_data.norm_range[GREEN][NORM_RANGE_BLACK] +
        ( channel_image[GREEN][cnt] * range_green ) / MAX_WHITE;
      combo_image[idx++] = rc_data.norm_range[BLUE][NORM_RANGE_BLACK] +
        ( channel_image[BLUE][cnt] * range_blue ) / MAX_WHITE;
    } /* for( cnt = 0; cnt < (int)channel_image_size; cnt++ ) */
  }

} /* Create_Combo_Image() */

/*------------------------------------------------------------------------*/

