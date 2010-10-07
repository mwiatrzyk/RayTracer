#ifndef __BITMAP_H
#define __BITMAP_H

#include "types.h"

//// GLOBALS //////////////////////////////////////////////////

extern uint32_t il_error;

//// ENUMERATIONS /////////////////////////////////////////////

/* List of imagelib's errors that may occur. */
typedef enum _IL_Error {
    IL_IO_ERROR,             //unable to read from file
    IL_INVALID_FILE_FORMAT,  //trying to load file of invalid format
    IL_FORMAT_NOT_SUPPORTED, //image file format is not supported
    IL_NOT_ENOUGH_MEMORY     //not enough memory to hold entire image 
} IL_Error;

/* Enumeration of DIB header types. */
typedef enum _IL_DibType {
    DIB_UNKNOWN = 0,
    DIB_OS2_V1  = 1,
    DIB_WIN_V3  = 2
} IL_DibType;

//// STRUCTURES ///////////////////////////////////////////////

/* BMP file header. */
typedef struct _IL_BmpHeader {
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
} IL_BmpHeader;

/* An object that holds bitmap data. All image operations uses this object as
 * first argument. */
typedef struct _IL_Bitmap {
    int32_t width;          //bitmap width
    int32_t height;         //bitmap height
    uint32_t background;    //bitmap background
    uint32_t *pixels;       //bitmap pixel array
} IL_Bitmap;

//// INLINE FUNCTIONS /////////////////////////////////////////

/* Sets pixel at given (x,y) coords to value specified in `color` without
 * buffer overrun checking. 

 @param: self: pointer to bitmap
 @param: x: x coordinate
 @param: y: y coordinate
 @param: color: pixel color */
inline void set_pixel(IL_Bitmap* self, int32_t x, int32_t y, uint32_t color) {
    *(self->pixels+(y*self->width)+x) = color;
}

/* Builds singe 32bit value representing RGBA color using its components.

 @param: r: red component
 @param: g: green component
 @param: b: blue component
 @param: a: alpha component (actually not used - this value is ignored) */
inline uint32_t rgba(uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
    return ((r&0xff)<<24) | ((g&0xff)<<16) | ((b&0xff)<<8) | (a&0xff);
}

//// FUNCTIONS ////////////////////////////////////////////////

/* Creates bitmap of given size and background color. Returns pointer to
 * IL_Bitmap object or NULL if bitmap could not be created.

 @param: width: bitmap width
 @param: height: bitmap height 
 @param: background: bitmap background */
IL_Bitmap* il_create_bitmap(int32_t width, int32_t height, uint32_t background);

/* Releases memory occupied by given IL_Bitmap object. */
void il_destroy_bitmap(IL_Bitmap* self);

/* Loads bitmap image from given file and returns it as buffer of bytes. Bitmap
 * is decoded to RGBA format used by IL_Bitmap object.
 
 @param: filename: path to image file */
IL_Bitmap* il_load(const char* filename);

#endif
