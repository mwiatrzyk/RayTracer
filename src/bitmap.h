#ifndef __BITMAP_H
#define __BITMAP_H

#include "types.h"

//// ENUMERATIONS /////////////////////////////////////////////

/* Enumeration of DIB header types. */
typedef enum _RT_DibType {
  DIB_UNKNOWN = 0,
  DIB_OS2_V1  = 1,
  DIB_WIN_V3  = 2
} RT_DibType;


//// STRUCTURES ///////////////////////////////////////////////

/* BMP file header. */
typedef struct _RT_BmpHeader {
  char		      bfType[2];			  // signature (must be: BM)
  uint32_t	    bfSize;				    // length of entire file (in bytes)
  uint16_t	    bfReserved1;		  // reserved (0 by default)
  uint16_t	    bfReserved2;		  // like above
  uint32_t	    bfOffBits;			  // bitmap data offset
  //DIB Header
  uint32_t	    biSize;				    // DIB header size (40 bytes, sometimes less)
  int32_t			  biWidth;			    // image width
  int32_t			  biHeight;			    // image height
  uint16_t	    biPlanes;			    // number of color layers (always 1)
  uint16_t	    biBitCount;			  // number of bits per pixel (1, 4, 8, 16, 24 lub 32)
  uint32_t	    biCompression;	  // compression algorithm (0 - without compression)
  uint32_t      biSizeImage;      // size of image data only (in bytes)
  int32_t			  biXPelsPerMeter;	// X resolution
  int32_t			  biYPelsPerMeter;	// Y resolution
  uint32_t	    biClrUsed;			  // number of colors in pallette
  unsigned char	biClrImportant;		// number of important colors in pallette (0 - all important)
  unsigned char	biClrRotation;		// pallette rotation flag (0 - without rotation)
  char			    biReserved[2];		// reserved
} RT_BmpHeader;

/* An object that holds bitmap data. All image operations uses this object as
 * first argument. */
typedef struct _RT_Bitmap {
  int32_t width;          //bitmap width
  int32_t height;         //bitmap height
  uint32_t background;    //bitmap background
  uint32_t *pixels;       //bitmap pixel array
} RT_Bitmap;

/* Object that represents color in [0..1] float range. */
typedef struct _RT_Color {
  float r, g, b, a;
} RT_Color;


//// INLINE COLOR FUNCTIONS ///////////////////////////////////

/* Scale color by given constant and store result in `self`. 

   @param: self: pointer to result object
   @param: color: pointer to object to be scaled
   @param: scale: scale value */
static inline RT_Color* iml_color_scale(RT_Color *self, RT_Color *color, float scale) {
  self->r = scale * color->r;
  self->g = scale * color->g;
  self->b = scale * color->b;
  return self;
}

/* Add colors `a` and `b` and store result in color `self`. */
static inline RT_Color* iml_color_add(RT_Color *self, RT_Color *a, RT_Color *b) {
  self->r = a->r + b->r;
  self->g = a->g + b->g;
  self->b = a->b + b->b;
  return self;
}

//// INLINE BITMAP FUNCTIONS //////////////////////////////////

/* Pixel setter. */
static inline void rtBitmapSetPixel(RT_Bitmap* self, int32_t x, int32_t y, uint32_t color) {
  *(self->pixels+(y*self->width)+x) = color;
}

/* Pixel getter. */
static inline uint32_t rtBitmapGetPixel(const RT_Bitmap* self, int32_t x, int32_t y) {
  return *(self->pixels+(y*self->width)+x);
}

/* Build single 32bit value representing RGBA color using separate values for
 * each color channel. */
static inline uint32_t rtColorBuildRGBA(uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
  return ((r&0xff)<<24) | ((g&0xff)<<16) | ((b&0xff)<<8) | (a&0xff);
}

/* Get RED of given 32bit color index. */
static inline uint32_t rtColorGetR(uint32_t color) {
  return (color>>24)&0xff;
}

/* Get GREEN of given 32bit color index. */
static inline uint32_t rtColorGetG(uint32_t color) {
  return (color>>16)&0xff;
}

/* Get BLUE of given 32bit color index. */
static inline uint32_t rtColorGetB(uint32_t color) {
  return (color>>8)&0xff;
}

/* Get ALPHA of given 32bit color index. */
static inline uint32_t rtColorGetA(uint32_t color) {
  return color&0xff;
}

//// FUNCTIONS ////////////////////////////////////////////////

/* Creates bitmap of given size and background color. Returns pointer to
 * RT_Bitmap object or NULL if bitmap could not be created.

:param: width: bitmap width
:param: height: bitmap height 
:param: background: bitmap background */
RT_Bitmap* rtBitmapCreate(int32_t width, int32_t height, uint32_t background);

/* Releases memory occupied by given RT_Bitmap object. */
void rtBitmapDestroy(RT_Bitmap* self);

/* Loads bitmap image from given file and returns it as buffer of bytes. Bitmap
 * is decoded to RGBA format used by RT_Bitmap object.

:param: filename: path to image file */
RT_Bitmap* rtBitmapLoad(const char* filename);

/* Saves given RT_Bitmap object into specified BMP file with specified bit
 * count. 

:param: self: pointer to RT_Bitmap object
:param: filename: pointer to file name where bitmap will be stored
:param: bpp: bit count (1, 4, 8, 16, 24 or 32) */
void rtBitmapSave(const RT_Bitmap* self, const char* filename, uint16_t bpp);

#endif

// vim: tabstop=2 shiftwidth=2 softtabstop=2
