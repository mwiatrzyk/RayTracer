#ifndef __BITMAP_H
#define __BITMAP_H

#include "types.h"

//// ENUMERATIONS /////////////////////////////////////////////

/* Enumeration of DIB header types. */
typedef enum _IML_DibType {
    DIB_UNKNOWN = 0,
    DIB_OS2_V1  = 1,
    DIB_WIN_V3  = 2
} IML_DibType;

//// STRUCTURES ///////////////////////////////////////////////

/* BMP file header. */
typedef struct _IML_BmpHeader {
	char		    bfType[2];			//signature (must be: BM)
	uint32_t	    bfSize;				//length of entire file (in bytes)
	uint16_t	    bfReserved1;		//reserved (0 by default)
	uint16_t	    bfReserved2;		//like above
	uint32_t	    bfOffBits;			//bitmap data offset
    //DIB Header
	uint32_t	    biSize;				//DIB header size (40 bytes, sometimes less)
	int32_t			biWidth;			//image width
	int32_t			biHeight;			//image height
	uint16_t	    biPlanes;			//number of color layers (always 1)
	uint16_t	    biBitCount;			//number of bits per pixel (1, 4, 8, 16, 24 lub 32)
	uint32_t	    biCompression;		//compression algorithm (0 - without compression)
	uint32_t        biSizeImage;		//size of image data only (in bytes)
	int32_t			biXPelsPerMeter;	//X resolution
	int32_t			biYPelsPerMeter;	//Y resolution
	uint32_t	    biClrUsed;			//number of colors in pallette
	unsigned char	biClrImportant;		//number of important colors in pallette (0 - all important)
	unsigned char	biClrRotation;		//pallette rotation flag (0 - without rotation)
	char			biReserved[2];		//reserved
} IML_BmpHeader;

/* An object that holds bitmap data. All image operations uses this object as
 * first argument. */
typedef struct _IML_Bitmap {
    int32_t width;          //bitmap width
    int32_t height;         //bitmap height
    uint32_t background;    //bitmap background
    uint32_t *pixels;       //bitmap pixel array
} IML_Bitmap;

/* Object that represents non normalized color. */
typedef struct _IML_Color {
    float r, g, b, a;
} IML_Color;

//// INLINE BITMAP FUNCTIONS //////////////////////////////////

/* Sets pixel at given (x,y) coords to value specified in `color` without
 * buffer overrun checking. 

 @param: self: pointer to bitmap
 @param: x: x coordinate
 @param: y: y coordinate
 @param: color: pixel color */
static inline void iml_bitmap_setpixel(IML_Bitmap* self, int32_t x, int32_t y, uint32_t color) {
    *(self->pixels+(y*self->width)+x) = color;
}

/* Gets pixel value at given bitmap position without bound check.

 @param: self: pointer to bitmap
 @param: x: x coordinate of pixel
 @param: y: y coordinate of pixel */
static inline uint32_t iml_bitmap_getpixel(const IML_Bitmap* self, int32_t x, int32_t y) {
    return *(self->pixels+(y*self->width)+x);
}

/* Builds singe 32bit value representing RGBA color using its components.

 @param: r: red component
 @param: g: green component
 @param: b: blue component
 @param: a: alpha component (actually not used - this value is ignored) */
static inline uint32_t iml_rgba(uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
    return ((r&0xff)<<24) | ((g&0xff)<<16) | ((b&0xff)<<8) | (a&0xff);
}

/* Returns "red" component value of given color. 

 @param: color: RGBA color value */
static inline uint32_t iml_getr(uint32_t color) {
    return (color>>24)&0xff;
}

/* Returns "green" component value of given color. 

 @param: color: RGBA color value */
static inline uint32_t iml_getg(uint32_t color) {
    return (color>>16)&0xff;
}

/* Returns "blue" component value of given color. 

 @param: color: RGBA color value */
static inline uint32_t iml_getb(uint32_t color) {
    return (color>>8)&0xff;
}

/* Returns "alpha" component value of given color. 

 @param: color: RGBA color value */
static inline uint32_t iml_geta(uint32_t color) {
    return color&0xff;
}

//// FUNCTIONS ////////////////////////////////////////////////

/* Creates bitmap of given size and background color. Returns pointer to
 * IML_Bitmap object or NULL if bitmap could not be created.

 @param: width: bitmap width
 @param: height: bitmap height 
 @param: background: bitmap background */
IML_Bitmap* iml_bitmap_create(int32_t width, int32_t height, uint32_t background);

/* Releases memory occupied by given IML_Bitmap object. */
void iml_bitmap_destroy(IML_Bitmap* self);

/* Loads bitmap image from given file and returns it as buffer of bytes. Bitmap
 * is decoded to RGBA format used by IML_Bitmap object.
 
 @param: filename: path to image file */
IML_Bitmap* iml_bitmap_load(const char* filename);

/* Saves given IML_Bitmap object into specified BMP file with specified bit
 * count. 
 
 @param: self: pointer to IML_Bitmap object
 @param: filename: pointer to file name where bitmap will be stored
 @param: bpp: bit count (1, 4, 8, 16, 24 or 32) */
void iml_bitmap_save(const IML_Bitmap* self, const char* filename, uint16_t bpp);

#endif
