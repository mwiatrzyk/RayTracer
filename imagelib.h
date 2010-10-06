#ifndef __BITMAP_H
#define __BITMAP_H

#include "types.h"

//// STRUCTURES ///////////////////////////////////////////////

/* BMP file header. */
typedef struct _IL_BmpHeader {
	char		    bfType[2];			//signature (must be: BM)
	uint32_t	    bfSize;				//length of entire file (in bytes)
	uint16_t	    bfReserved1;		//reserved (0 by default)
	uint16_t	    bfReserved2;		//like above
	uint32_t	    bfOffBits;			//bitmap data offset
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

//// FUNCTIONS ////////////////////////////////////////////////

/* Creates bitmap of given size and background color. Returns pointer to
 * IL_Bitmap object or NULL if bitmap could not be created.

 @param: width: bitmap width
 @param: height: bitmap height 
 @param: background: bitmap background */
IL_Bitmap* il_create_bitmap(int32_t width, int32_t height, uint32_t background);

/* Releases memory occupied by given IL_Bitmap object. */
void il_destroy_bitmap(IL_Bitmap* self);

#endif
