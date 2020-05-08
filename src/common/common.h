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


#ifndef COMMON_H
#define COMMON_H    1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <math.h>
#include <semaphore.h>
#include <pthread.h>
#include <complex.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#ifdef HAVE_LIBRTLSDR
  #include <rtl-sdr.h>
#endif
#ifdef HAVE_LIBAIRSPY
  #include <libairspy/airspy.h>
#endif

/*---------------------------------------------------------------------------*/

/* Flow control flags */
#define ACTION_RECEIVER_ON      0x000001 /* Start SDR Rx and demodulator  */
#define ACTION_DECODE_IMAGES    0x000002 /* Decode images from satellite  */
#define ACTION_IDOQPSK_STOP     0x000004 /* Stop buffering in Demod_IDOQPSK() */
#define ACTION_FLAGS_ALL        0x000007 /* All action flags (clearing)   */
#define IMAGE_RAW               0x000008 /* Save image in raw decoded state */
#define IMAGE_NORMALIZE         0x000010 /* Histogram normalize wx image    */
#define IMAGE_CLAHE             0x000020 /* CLAHE image contrast enhance  */
#define IMAGE_COLORIZE          0x000040 /* Pseudo colorize wx image      */
#define IMAGE_INVERT            0x000080 /* Rotate wx image 180 degrees   */
#define IMAGE_OUT_SPLIT         0x000100 /* Save individual channel image */
#define IMAGE_OUT_COMBO         0x000200 /* Combine & save channel images */
#define ALL_INITIALIZED         0x000400 /* All mlrpt systems initialized */
#define ALARM_ACTION_START      0x000800 /* Start Operation on SIGALRM    */
#define IMAGE_SAVE_JPEG         0x001000 /* Save channel images as JPEG   */
#define IMAGE_SAVE_PPGM         0x002000 /* Save channel image as PGM|PPM */
#define IMAGE_RECTIFY           0x004000 /* Images have been rectified */
#define VERBOSE_MODE            0x008000 /* Run verbose (print messages)  */

/*---------------------------------------------------------------------------*/

/* Size of char arrays (strings) for error messages etc */
#define MESG_SIZE   128

/* Maximum time duration in sec
 * of satellite signal processing */
#define MAX_OPERATION_TIME  1000

#define TRUE    1
#define FALSE   0

/* DSP Filter Parameters */
#define FILTER_RIPPLE   5.0
#define FILTER_POLES    6

/* Return values */
#define ERROR       1
#define SUCCESS     0

/* Generel definitions for image processing */
#define MAX_FILE_NAME   128 /* Max length for filenames */

/* Maximum length of strings in config file */
#define MAX_CONFIG_STRLEN   80

/* Maximum length of config file names */
#define MAX_CFG_STRLEN   32

/* Number of APID image channels */
#define CHANNEL_IMAGE_NUM   3

/* Indices to Normalization Range black and white values */
#define NORM_RANGE_BLACK    0
#define NORM_RANGE_WHITE    1

/* Should have been in math.h */
#ifndef M_2PI
  #define M_2PI     6.28318530717958647692
#endif

#define BOOLEAN     uint8_t

/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  define Q_(String) g_strip_context ((String), gettext (String))
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define Q_(String) g_strip_context ((String), (String))
#  define N_(String) (String)
#endif

/* CLAHE definitions */
#define BYTE_IMAGE  1

#ifdef BYTE_IMAGE /* for 8 bit-per-pixel images */
  typedef unsigned char kz_pixel_t;
  #define uiNR_OF_GREY ( 256 )
#else /* for 12 bit-per-pixel images (default) */
  typedef unsigned short kz_pixel_t;
  #define uiNR_OF_GREY ( 4096 )
#endif

/*---------------------------------------------------------------------*/

/* Runtime config data */
typedef struct
{
  char
    mlrpt_cfg[MAX_CFG_STRLEN],      /* mlrpt configuration file name */
    mlrpt_dir[MAX_FILE_NAME],       /* mlrpt working directory */
    images_dir[MAX_CONFIG_STRLEN];  /* mlrpt images saving directory */

  uint
    invert_palette[3], /* Image APID to invert palette (black<-->white) */
    operation_time,    /* Time duration in sec of an operation (decode etc) */
    default_oper_time; /* Default timer length in sec for image decoding */

  /* RTL-SDR Configuration */
  uint32_t rtlsdr_dev_index; /* librtlsdr Device index  */
  int rtlsdr_freq_corr;      /* Freq correction factor in ppm for RTL-SDR */

  /* SDR Receiver configuration */
  uint
    sdr_center_freq, /* Center Frequency for SDR Tuner */
    sdr_rx_type,     /* SDR Receiver type */
    sdr_samplerate,  /* SDR RX ADC sample rate */
    sdr_filter_bw,   /* SDR Low Pass Filter Bandwidth */
    tuner_gain;      /* Manual Gain of SDR receiver's tuner */

  /* LRPT Demodulator Parameters */
  double
    rrc_alpha,          /* Raised Root Cosine alpha factor */
    costas_bandwidth,   /* Costas PLL Loop Bandwidth */
    pll_locked,         /* Lower phase error threshold for PLL lock */
    pll_unlocked;       /* Upper phase error threshold for PLL unlock */

  uint
    rrc_order,          /* Raised Root Cosine filter order */
    demod_samplerate,   /* SDR Rx I/Q Sample Rate S/s */
    symbol_rate,        /* Transmitted QPSK Symbol Rate Sy/s */
    interp_factor;      /* Demodulator Interpolation Multiplier */

  /* LRPT Decoder Parameters (channel apid) */
  uint apid[CHANNEL_IMAGE_NUM];

  /* Image Normalization pixel value ranges */
  uint8_t norm_range[CHANNEL_IMAGE_NUM][2];

  uint8_t
    psk_mode,          /* Select QPSK or OQPSK mode */
    rectify_function,  /* Select rectification function, either W2RG or 5B4AZ */
    clouds_threshold,  /* Pixel values above which we assumed it is cloudy areas */
    colorize_blue_max, /* Max value of blue pixels to enhance in pseudo-colorized image */
    colorize_blue_min; /* Min value of blue pixels in pseudo-colorized image */

  float jpeg_quality;  /* JPEG compressor quality factor */

} rc_data_t;

/* SDR Receiver types for above */
enum
{
  SDR_TYPE_RTLSDR = 1,
  SDR_TYPE_AIRSPY,
  SDR_TYPE_NUM
};

/* Actions for above */
enum
{
  INIT_BWORTH_LPF = 1,
  RTL_DAGC_ENABLE
};

/* Low pass filter data */
typedef struct
{
  double
    cutoff, /* Cutoff frequency as fraction of sample rate */
    ripple; /* Passband ripple as a percentage */

  uint
    npoles, /* Number of poles, must be even */
    type;   /* Filter type as below */

  /* a and b Coefficients of the filter */
  double *a, *b;

  /* Saved input and output values */
  double *x, *y;

  /* Ring buffer index for above */
  uint ring_idx;

  /* Input samples buffer and its length */
  double *samples_buf;
  uint samples_buf_len;

} filter_data_t;

/* Filter type for above struct */
enum
{
  FILTER_LOWPASS = 1,
  FILTER_HIGHPASS,
  FILTER_BANDPASS
};

/* Message types */
enum
{
  ERROR_MESG = 1,
  INFO_MESG
};

/* Image channels (0-2) */
enum
{
  RED = 0,
  GREEN,
  BLUE
};

/*---------------------------------------------------------------------*/

/* LRPT Demodulator data */
typedef struct
{
  double   average;
  double   gain;
  double   target_ampl;
  complex double bias;
} Agc_t;

typedef enum
{
  QPSK = 1,
  DOQPSK,
  IDOQPSK
} ModScheme;

typedef struct
{
  double  nco_phase, nco_freq;
  double  alpha, beta;
  double  damping, bandwidth;
  uint8_t locked;
  double  moving_average;
  ModScheme mode;
} Costas_t;

typedef struct
{
  complex double *restrict memory;
  uint32_t fwd_count;
  uint32_t stage_no;
  double  *restrict fwd_coeff;
} Filter_t;

typedef struct
{
  Agc_t    *agc;
  Costas_t *costas;
  double    sym_period;
  uint32_t  sym_rate;
  ModScheme mode;
  Filter_t *rrc;
} Demod_t;

/*---------------------------------------------------------------------------*/

/* LRPT Decoder data */
#define PATTERN_SIZE        64
#define PATTERN_CNT         8
#define CORR_LIMIT          55
#define HARD_FRAME_LEN      1024
#define FRAME_BITS          8192
#define SOFT_FRAME_LEN      16384
#define MIN_CORRELATION     45

#define MIN_TRACEBACK       35      // 5*7
#define TRACEBACK_LENGTH    105     // 15*7
#define NUM_STATES          128

/* My addition, width (pixels per line) of image (MCU_PER_LINE * 8) */
#define METEOR_IMAGE_WIDTH 1568

/* Flags to select images to output */
enum
{
  OUT_COMBO = 1,
  OUT_SPLIT,
  OUT_BOTH
};

/* Flags to indicate image file type to save as */
enum
{
  SAVEAS_JPEG = 1,
  SAVEAS_PGM,
  SAVEAS_BOTH
};

/* Flags to indicate image to be saved */
enum
{
  CH0 = 0,
  CH1,
  CH2,
  COMBO
};

typedef struct bit_io_rec
{
  uint8_t *p;
  int pos, len;
  uint8_t cur;
  int cur_len;
} bit_io_rec_t;

typedef struct corr_rec
{
  uint8_t patts[PATTERN_SIZE][PATTERN_SIZE];

  int
    correlation[PATTERN_CNT],
    tmp_corr[PATTERN_CNT],
    position[PATTERN_CNT];
} corr_rec_t;

typedef struct ac_table_rec
{
  int run, size, len;
  uint32_t mask, code;
} ac_table_rec_t;

typedef struct viterbi27_rec
{
  int BER;

  uint16_t dist_table[4][65536];
  uint8_t  table[NUM_STATES];
  uint16_t distances[4];

  bit_io_rec_t bit_writer;

  //pair_lookup
  uint32_t pair_keys[64];      //1 shl (order-1)
  uint32_t *pair_distances;
  size_t   pair_distances_len; // My addition, size of above pointer's alloc
  uint32_t pair_outputs[16];   //1 shl (2*rate)
  uint32_t pair_outputs_len;

  uint8_t history[MIN_TRACEBACK + TRACEBACK_LENGTH][NUM_STATES];
  uint8_t fetched[MIN_TRACEBACK + TRACEBACK_LENGTH];
  int hist_index, len, renormalize_counter;

  int err_index;
  uint16_t errors[2][NUM_STATES];
  uint16_t *read_errors, *write_errors;
} viterbi27_rec_t;

typedef struct mtd_rec
{
  corr_rec_t c;
  viterbi27_rec_t v;

  int pos, prev_pos;
  uint8_t ecced_data[HARD_FRAME_LEN];

  uint32_t word, cpos, corr, last_sync;
  int r[4], sig_q;
} mtd_rec_t;

/*-------------------------------------------------------------------*/

// JPEG chroma subsampling factors. Y_ONLY (grayscale images)
// and H2V2 (color images) are the most common.
enum subsampling_t
{
  Y_ONLY = 0,
  H1V1   = 1,
  H2V1   = 2,
  H2V2   = 3
};

// JPEG compression parameters structure.
typedef struct
{
  // Quality: 1-100, higher is better. Typical values are around 50-95.
  float m_quality;

  // m_subsampling:
  // 0 = Y (grayscale) only
  // 1 = YCbCr, no subsampling (H1V1, YCbCr 1x1x1, 3 blocks per MCU)
  // 2 = YCbCr, H2V1 subsampling (YCbCr 2x1x1, 4 blocks per MCU)
  // 3 = YCbCr, H2V2 subsampling (YCbCr 4x1x1, 6 blocks per MCU-- very common)
  enum subsampling_t m_subsampling;

  // Disables CbCr discrimination - only intended for testing.
  // If true, the Y quantization table is also used for the CbCr channels.
  BOOLEAN m_no_chroma_discrim_flag;

} compression_params_t;

/*-------------------------------------------------------------------*/

/* Function prototypes produced by cproto */
/* Mlrpt/clahe.c */
BOOLEAN CLAHE(kz_pixel_t *pImage, uint uiXRes, uint uiYRes, kz_pixel_t Min, kz_pixel_t Max, uint uiNrX, uint uiNrY, uint uiNrBins, double fCliplimit);
/* Mlrpt/image.c */
void Normalize_Image(uint8_t *image_buffer, uint image_size, uint8_t range_low, uint8_t range_high);
void Flip_Image(unsigned char *image_buffer, uint image_size);
void Create_Combo_Image(uint8_t *combo_image);
/* Mlrpt/jpeg.c */
BOOLEAN jepg_encoder_compression_parameters(compression_params_t *comp_params, float m_quality, enum subsampling_t m_subsampling, uint8_t m_no_chroma_discrim_flag);
BOOLEAN jepg_encoder_compress_image_to_file(char *file_name, int width, int height, int num_channels, const uint8_t *pImage_data, compression_params_t *comp_params);
/* Mlrpt/main.c */
int main(int argc, char *argv[]);
/* Mlrpt/operation.c */
BOOLEAN Start_Receiver(void);
void Decode_Images(void);
void Alarm_Action(void);
void Oper_Timer_Setup(char *arg);
void Auto_Timer_Setup(char *arg);
/* Mlrpt/rc_config.c */
uint8_t Load_Config(void);
/* Mlrpt/shared.c */
/* Mlrpt/utils.c */
void File_Name(char *file_name, uint chn, const char *ext);
char *Fname(char *fpath);
void Print_Message(const char *mesg, char type);
void Usage(void);
void mem_alloc(void **ptr, size_t req);
void mem_realloc(void **ptr, size_t req);
void free_ptr(void **ptr);
BOOLEAN Open_File(FILE **fp, char *fname, const char *mode);
void Save_Image_JPEG(char *file_name, int width, int height, int num_channels, const uint8_t *pImage_data, compression_params_t *comp_params);
void Save_Image_Raw(char *fname, const char *type, uint width, uint height, uint max_val, uint8_t *buffer);
void Cleanup(void);
int iClamp(int i, int min, int max);
double dClamp(double x, double min, double max);
int isFlagSet(int flag);
int isFlagClear(int flag);
void SetFlag(int flag);
void ClearFlag(int flag);
void ToggleFlag(int flag);
void Strlcpy(char *dest, const char *src, size_t n);
void Strlcat(char *dest, const char *src, size_t n);
/* sdr/filters.c */
BOOLEAN Init_Chebyshev_Filter(filter_data_t *filter_data, uint buf_len, uint filter_bw, uint sample_rate, double ripple, uint num_poles, uint type);
void DSP_Filter(filter_data_t *filter_data);
/* sdr/rtlsdr.c */
#ifdef HAVE_LIBRTLSDR
BOOLEAN RtlSdr_Initialize(void);
void RtlSdr_Close_Device(void);
#endif
/* sdr/airspy.c */
#ifdef HAVE_LIBAIRSPY
void Airspy_Close_Device(void);
BOOLEAN Airspy_Initialize(void);
#endif
/* lrpt_decode/alib.c */
int Count_Bits(uint32_t n);
uint32_t Bio_Peek_n_Bits(bit_io_rec_t *b, const int n);
void Bio_Advance_n_Bits(bit_io_rec_t *b, const int n);
uint32_t Bio_Fetch_n_Bits(bit_io_rec_t *b, const int n);
void Bit_Writer_Create(bit_io_rec_t *w, uint8_t *bytes, int len);
void Bio_Write_Bitlist_Reversed(bit_io_rec_t *w, uint8_t *l, int len);
int Ecc_Decode(uint8_t *idata, int pad);
void Ecc_Deinterleave(uint8_t *data, uint8_t *output, int pos, int n);
void Ecc_Interleave(uint8_t *data, uint8_t *output, int pos, int n);
/* lrpt_decode/correlator.c */
int Hard_Correlate(const uint8_t d, const uint8_t w);
void Init_Correlator_Tables(void);
void Fix_Packet(void *data, int len, int shift);
void Correlator_Init(corr_rec_t *c, uint64_t q);
int Corr_Correlate(corr_rec_t *c, uint8_t *data, uint32_t len);
/* lrpt_decode/dct.c */
void Flt_Idct_8x8(double *res, const double *inpt);
/* lrpt_decode/huffman.c */
int Get_AC(const uint16_t w);
int Get_DC(const uint16_t w);
int Map_Range(const int cat, const int vl);
void Default_Huffman_Table(void);
/* lrpt_decode/medet.c */
void Medet_Init(void);
void Decode_Image(uint8_t *in_buffer, int buf_len);
/* lrpt_decode/met_jpg.c */
void Mj_Dump_Image(void);
void Mj_Dec_Mcus(uint8_t *p, uint apid, int pck_cnt, int mcu_id, uint8_t q);
void Mj_Init(void);
/* lrpt_decode/met_packet.c */
void Parse_Cvcdu(uint8_t *p, int len);
/* lrpt_decode/met_to_data.c */
void Mtd_Init(mtd_rec_t *mtd);
BOOLEAN Mtd_One_Frame(mtd_rec_t *mtd, uint8_t *raw);
/* lrpt_decode/rectify_meteor.c */
void Rectify_Images(void);
/* lrpt_decode/viterbi27.c */
double Vit_Get_Percent_BER(const viterbi27_rec_t *v);
void Vit_Decode(viterbi27_rec_t *v, uint8_t *input, uint8_t *output);
void Mk_Viterbi27(viterbi27_rec_t *v);
/* agc.c */
Agc_t *Agc_Init(void);
_Complex double Agc_Apply(Agc_t *self, _Complex double sample);
void Agc_Free(Agc_t *self);
/* demod.c */
void Demod_Init(void);
void Demod_Deinit(void);
double Agc_Gain(double *gain);
double Signal_Level(uint32_t *level);
double Pll_Average(void);
void Demodulator_Run(void);
/* doqpsk.c */
void De_Interleave(uint8_t *raw, int raw_siz, uint8_t **resync, int *resync_siz);
void Make_Isqrt_Table(void);
void Free_Isqrt_Table(void);
void De_Diffcode(int8_t *buff, uint32_t length);
/* filters.c */
Filter_t *Filter_New(uint32_t fwd_count, double *fwd_coeff);
Filter_t *Filter_RRC(uint32_t order, uint32_t factor, double osf, double alpha);
_Complex double Filter_Fwd(Filter_t *const self, _Complex double in);
void Filter_Free(Filter_t *self);
/* pll.c */
Costas_t *Costas_Init(double bw, ModScheme mode);
_Complex double Costas_Mix(Costas_t *self, _Complex double samp);
void Costas_Correct_Phase(Costas_t *self, double err);
void Costas_Recompute_Coeffs(Costas_t *self, double damping, double bw);
void Costas_Free(Costas_t *self);
double Costas_Delta(_Complex double sample, _Complex double cosample);
/* utils.c */
int8_t Clamp_Int8(double x);
double Clamp_Double(double x, double max_abs);
#endif

