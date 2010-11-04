#include "bitmap.h"
#include "error.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>


///////////////////////////////////////////////////////////////
RT_Bitmap* rtBitmapCreate(int32_t width, int32_t height, uint32_t background) {
  uint32_t *p, *maxp;
  RT_Bitmap* res = malloc(sizeof(RT_Bitmap));
  if(!res) {
    return NULL;  // allocation failed
  }
  res->width = width;
  res->height = height;
  res->background = background;
  res->pixels = malloc(width*height*sizeof(uint32_t));
  p = res->pixels;
  maxp = (uint32_t*)(p+width*height);
  while(p < maxp) {
    *(p++) = background;
  }
  if(!res->pixels) {
    free(res);
    return NULL;  // allocation failed - free previously allocated bitmap and return
  }
  return res;
}


///////////////////////////////////////////////////////////////
void rtBitmapDestroy(RT_Bitmap* self) {
  free(self->pixels);
  free(self);
}


///////////////////////////////////////////////////////////////
RT_Bitmap* rtBitmapLoad(const char* filename) {
  int32_t r=0;
  RT_BmpHeader hdr;
  RT_DibType dib_type;
  RT_Bitmap *res=NULL;

  /* dynamically allocated resources to be released at the end of function
   * call */
  FILE *fd=NULL;
  unsigned char *buffer=NULL;
  unsigned char *palette=NULL;

  memset(&hdr, 0, sizeof(hdr));

  //open file
  fd = fopen(filename, "rb");
  if (!fd) {
    errno = E_IO;  //unable to read from file
    goto garbage_collect;
  }

  //read file header and check if this is real BMP file
  r += fread(&hdr.bfType, sizeof(hdr.bfType), 1, fd);
  r += fread(&hdr.bfSize, sizeof(hdr.bfSize), 1, fd);
  r += fread(&hdr.bfReserved1, sizeof(hdr.bfReserved1), 1, fd);
  r += fread(&hdr.bfReserved2, sizeof(hdr.bfReserved2), 1, fd);
  r += fread(&hdr.bfOffBits, sizeof(hdr.bfOffBits), 1, fd);
  if(r != 5) {
  }
  if (hdr.bfType[0] != 'B' || hdr.bfType[1] != 'M') {
    errno = E_INVALID_FILE;  //BM signature check failed
    goto garbage_collect;
  }

  //read DIB part of file header
  r += fread(&hdr.biSize, sizeof(hdr.biSize), 1, fd);
  r += fread(&hdr.biWidth, sizeof(hdr.biWidth), 1, fd);
  r += fread(&hdr.biHeight, sizeof(hdr.biHeight), 1, fd);
  r += fread(&hdr.biPlanes, sizeof(hdr.biPlanes), 1, fd);
  r += fread(&hdr.biBitCount, sizeof(hdr.biBitCount), 1, fd);
  if (hdr.biSize == 40) {
    r += fread(&hdr.biCompression, sizeof(hdr.biCompression), 1, fd);
    r += fread(&hdr.biSizeImage, sizeof(hdr.biSizeImage), 1, fd);
    r += fread(&hdr.biXPelsPerMeter, sizeof(hdr.biXPelsPerMeter), 1, fd);
    r += fread(&hdr.biYPelsPerMeter, sizeof(hdr.biYPelsPerMeter), 1, fd);
    r += fread(&hdr.biClrUsed, sizeof(hdr.biClrUsed), 1, fd);
    r += fread(&hdr.biClrImportant, sizeof(hdr.biClrImportant), 1, fd);
    r += fread(&hdr.biClrRotation, sizeof(hdr.biClrRotation), 1, fd);
    r += fread(&hdr.biReserved, sizeof(hdr.biReserved), 1, fd);
    if (hdr.biCompression != 0) {
      errno = E_INVALID_FILE_FORMAT;
      goto garbage_collect;
    }
    dib_type = DIB_WIN_V3;
  } else {
    dib_type = DIB_OS2_V1;
  }

  //create result bitmap
  res = rtBitmapCreate(hdr.biWidth, hdr.biHeight>0 ? hdr.biHeight : -hdr.biHeight, rtColorBuildRGBA(0,0,0,0));
  if (!res) {
    errno = E_MEMORY;
    goto garbage_collect;
  }

  //make scanline buffer - temporary storage of lines loaded from file
  uint32_t buf_size = (uint32_t)(4*ceil(hdr.biBitCount*hdr.biWidth/32.0));
  buffer = malloc(buf_size*sizeof(unsigned char)); 
  if (!buffer) {
    errno = E_MEMORY;
    goto garbage_collect;
  }

  switch(hdr.biBitCount) {
    /* with palette */
    case 1:
    case 4:
    case 8: {
      //allocate memory for palette
      uint32_t pal_size = hdr.biClrUsed!=0 ? hdr.biClrUsed : (dib_type==DIB_OS2_V1 ? 3 : 4)*(uint32_t)pow(2.0f, hdr.biBitCount);
      palette = malloc(pal_size*sizeof(unsigned char));
      if (!palette) {
        errno = E_MEMORY;
        goto garbage_collect;
      }

      //read palette from file
      r += fread(palette, pal_size, 1, fd);

      switch(hdr.biBitCount) {
        //monochrome
        case 1: {
          int32_t x, y, w=hdr.biWidth, h=hdr.biHeight;
          uint32_t i;
          int16_t k;
          unsigned char *ptr, p;

          for(y=h>0? h-1: 0; h>0? y>=0: y<-h; h>0? y--: y++) {
            r += fread(buffer, buf_size, 1, fd);
            for(x=0, i=0; x<w; x+=8, i++) {
              p=*(buffer+i);
              for(k=0; k<8; k++) {  //iteration over bits in buffer[i]
                if (x+k >= w)
                  break;
                ptr = palette+(dib_type==DIB_OS2_V1 ? ((p>>(7-k))&1)*3 : ((p>>(7-k))&1)<<2);
                rtBitmapSetPixel(res, x+k, y, rtColorBuildRGBA(*(ptr+2), *(ptr+1), *(ptr), 0));
              }
            }
          }
          break;
        }

        //16 colors
        case 4: {
          int32_t x, y, w=hdr.biWidth, h=hdr.biHeight;
          uint32_t i;
          int16_t k;
          unsigned char *ptr;
          unsigned char p[2];

          for(y=h>0? h-1: 0; h>0? y>=0: y<-h; h>0? y--: y++) {
            r += fread(buffer, buf_size, 1, fd);
            for(x=0, i=0; x<w; x+=2, i++) {
              p[0] = *(buffer+i);   //buffer[i] przechowuje 2 piksele
              p[1] = *(p)&15;       //4 najmlodsze bity do p[1] (drugi piksel)
              p[0] = *(p)>>4;       //4 najstarsze bity do p[0] (pierwszy piksel)
              for(k=0; k<2; k++) {
                if (x+k >= w)
                  break;
                ptr = palette+(dib_type==DIB_OS2_V1 ? p[k]*3 : p[k]<<2);
                rtBitmapSetPixel(res, x+k, y, rtColorBuildRGBA(*(ptr+2), *(ptr+1), *(ptr), 0));
              }
            }
          }
          break;
        }

        //256 colors
        case 8: {
          int32_t x, y, w=hdr.biWidth, h=hdr.biHeight;
          uint32_t i, dib_size=hdr.biSize;
          unsigned char *ptr;
          unsigned short p;

          for(y=h>0? h-1: 0; h>0? y>=0: y<-h; h>0? y--: y++) {
            r += fread(buffer, buf_size, 1, fd);
            for(x=0, i=0; x<w; x++, i++) {
              p=*(buffer+i);
              ptr=palette+(dib_size==12 ? p*3 : p<<2);
              rtBitmapSetPixel(res, x, y, rtColorBuildRGBA(*(ptr+2), *(ptr+1), *(ptr), 0));
            }
          }
          break;
        }
      }
      break;
    }

    /* without palette */
    case 16: {
      int32_t x, y, w=hdr.biWidth, h=hdr.biHeight;
      uint32_t i;
      uint16_t p;

      for(y=h>0? h-1: 0; h>0? y>=0: y<-h; h>0? y--: y++) {
        r += fread(buffer, buf_size, 1, fd);
        for(x=0, i=0; x<w; x++, i+=2) {
          p = (*(buffer+i+1)<<8) | *(buffer+i);
          rtBitmapSetPixel(res, x, y, rtColorBuildRGBA(((p>>10)&0x1f)<<3, ((p>>5)&0x1f)<<3, (p&0x1f)<<3, 0));
        }
      }
      break;
    }

    case 24:
    case 32: {
      int32_t x, y, w=hdr.biWidth, h=hdr.biHeight;
      uint32_t i;
      uint16_t bpp=hdr.biBitCount;

      for(y=h>0? h-1: 0; h>0? y>=0: y<-h; h>0? y--: y++) {
        r += fread(buffer, buf_size, 1, fd);
        for(x=0, i=0; x<w; x++, i+=bpp==24? 3: 4) {
          rtBitmapSetPixel(res, x, y, rtColorBuildRGBA(*(buffer+i+2), *(buffer+i+1), *(buffer+i), 0));
        }
      }
      break;
    }
  }

garbage_collect:
  if(fd)
    fclose(fd);
  if(buffer)
    free(buffer);
  if(palette)
    free(palette);

  return res;
}


///////////////////////////////////////////////////////////////
void rtBitmapSave(const RT_Bitmap* self, const char* filename, uint16_t bpp) {
  RT_BmpHeader hdr;
  uint32_t palette_size=bpp<=8? 4*(uint32_t)pow(2.0f, bpp): 0;  //palette is used only in 1, 4 and 8 bpp bitmaps

  /* dynamically allocated variables 
   * (released at the end of function call) */
  FILE *fd=NULL;
  unsigned char *buffer=NULL;
  unsigned char *palette=NULL;

  memset(&hdr, 0, sizeof(hdr));

  //validate bpp parameter
  if (bpp!=1 && bpp!=4 && bpp!=8 && bpp!=16 && bpp!=24 && bpp!=32) {
    errno = E_INVALID_BPP;
    return;
  }

  //open file for binary write mode
  fd=fopen(filename, "wb");
  if (!fd) {
    errno = E_IO;  //unable to read from file
    goto garbage_collect;
  }

  //prepare header
  hdr.bfType[0] = 'B';
  hdr.bfType[1] = 'M';
  hdr.bfSize = (uint32_t)(54+4*ceil(bpp*self->width/32.0)*self->height+palette_size);
  hdr.bfOffBits = 54+palette_size;
  hdr.biSize = 40;
  hdr.biWidth = self->width;
  hdr.biHeight = self->height;
  hdr.biPlanes = 1;
  hdr.biBitCount = bpp;
  hdr.biSizeImage = hdr.bfSize-54-palette_size;

  //write header to file
  fwrite(&hdr.bfType, sizeof(hdr.bfType), 1, fd);
  fwrite(&hdr.bfSize, sizeof(hdr.bfSize), 1, fd);
  fwrite(&hdr.bfReserved1, sizeof(hdr.bfReserved1), 1, fd);
  fwrite(&hdr.bfReserved2, sizeof(hdr.bfReserved2), 1, fd);
  fwrite(&hdr.bfOffBits, sizeof(hdr.bfOffBits), 1, fd);
  fwrite(&hdr.biSize, sizeof(hdr.biSize), 1, fd);
  fwrite(&hdr.biWidth, sizeof(hdr.biWidth), 1, fd);
  fwrite(&hdr.biHeight, sizeof(hdr.biHeight), 1, fd);
  fwrite(&hdr.biPlanes, sizeof(hdr.biPlanes), 1, fd);
  fwrite(&hdr.biBitCount, sizeof(hdr.biBitCount), 1, fd);
  fwrite(&hdr.biCompression, sizeof(hdr.biCompression), 1, fd);
  fwrite(&hdr.biSizeImage, sizeof(hdr.biSizeImage), 1, fd);
  fwrite(&hdr.biXPelsPerMeter, sizeof(hdr.biXPelsPerMeter), 1, fd);
  fwrite(&hdr.biYPelsPerMeter, sizeof(hdr.biYPelsPerMeter), 1, fd);
  fwrite(&hdr.biClrUsed, sizeof(hdr.biClrUsed), 1, fd);
  fwrite(&hdr.biClrImportant, sizeof(hdr.biClrImportant), 1, fd);
  fwrite(&hdr.biClrRotation, sizeof(hdr.biClrRotation), 1, fd);
  fwrite(&hdr.biReserved, sizeof(hdr.biReserved), 1, fd);

  //create scanline buffer
  uint32_t buf_size=(uint32_t)(4*ceil(bpp*self->width/32.0));
  buffer=malloc(buf_size);
  if(!buffer) {
    errno = E_MEMORY;
    goto garbage_collect;
  }

  switch(bpp) {
    case 1:
    case 4:
    case 8: {
      //create palette buffer
      palette=malloc(palette_size);
      if (!palette) {
        errno = E_MEMORY;
        goto garbage_collect;
      }

      switch(bpp) {
        case 1: {
          int32_t x, y, w=self->width, h=self->height;
          uint32_t p, i;
          uint16_t k;

          //create palette containing two colors - black and white
          palette[0] = palette[1] = palette[2] = 0;
          palette[3] = 0;
          palette[4] = palette[5] = palette[6] = 255;
          palette[7] = 0;

          //write palette to file
          fwrite(palette, palette_size, 1, fd);

          for(y=h-1; y>=0; y--) {
            memset(buffer, 0, buf_size);
            for(x=0, i=0; x<w; x+=8, i++) {
              for(k=0; k<8; k++) {
                if (x+k >= w)
                  break;
                p = rtBitmapGetPixel(self, x+k, y);
                p = (rtColorGetR(p)+rtColorGetG(p)+rtColorGetB(p))/3;  //grayscale (0..255)
                p >>= 7;  //p>127 -> p=1, p<=127 -> p=0
                *(buffer+i) |= p<<(7-k);
              }
            }
            //write image data scanline to file
            fwrite(buffer, buf_size, 1, fd);
          }
          break;
        }

        case 4: {
          int32_t x, y, w=self->width, h=self->height;
          uint32_t i, j, p1, p2;

          /* create palette. The idea behind is that we treat each
           * 0..16 number as RGB color, where 1 bit is used for R
           * component, 2 bits for G component and 1 bit for B
           * component. After splitting each number, each bits are
           * scaled to 0..255 range by shifting left to oldest
           * possible bit positions */
          for(j=0, i=0; j<15; j++, i+=4) {
            *(palette+i)=(j&1)<<7;
            *(palette+i+1)=((j>>1)&3)<<6;
            *(palette+i+2)=((j>>3)&1)<<7;
            *(palette+i+3)=0;
          }

          //add white color to palette
          palette[i]=palette[i+1]=palette[i+2]=255;
          palette[i+3]=0;

          //save palette to file
          fwrite(palette, palette_size, 1, fd);

          for(y=h-1; y>=0; y--) {
            memset(buffer, 0, buf_size);
            for(x=0, i=0; x<w; x+=2, i++) {
              p1 = rtBitmapGetPixel(self, x, y);
              if (x+1 < w) {
                p2 = rtBitmapGetPixel(self, x+1, y);
              } else {
                p2=0;
              }
              *(buffer+i) = (rtColorGetR(p1)>>7)<<7 | (rtColorGetG(p1)>>6)<<5 | (rtColorGetB(p1)>>7)<<4 | 
                (rtColorGetR(p2)>>7)<<3 | (rtColorGetG(p2)>>6)<<1 | (rtColorGetB(p2)>>7);
            }
            fwrite(buffer, buf_size, 1, fd);
          }
          break;
        }

        case 8: {
          int32_t x, y, w=self->width, h=self->height;
          uint32_t i, j, p;

          /* create palette. Idea is the same as for 4bit color.
           * However, since we have more bits we can assign 3 bits
           * for "red" and "green" and 2 bits for "blue" */
          for(j=0, i=0; j<255; j++, i+=4) {
            *(palette+i)=(j&3)<<6;
            *(palette+i+1)=((j>>2)&7)<<5;
            *(palette+i+2)=((j>>5)&7)<<5;
            *(palette+i+3)=0;
          }

          //add white color to palette
          palette[i]=palette[i+1]=palette[i+2]=255;
          palette[i+3]=0;

          //save palette to file
          fwrite(palette, palette_size, 1, fd);

          for(y=h-1; y>=0; y--) {
            memset(buffer, 0, buf_size);
            for(x=0, i=0; x<w; x++, i++) {
              p = rtBitmapGetPixel(self, x, y);
              *(buffer+i) = (rtColorGetR(p)>>5)<<5 | (rtColorGetG(p)>>5)<<2 | rtColorGetB(p)>>6;
            }
            fwrite(buffer, buf_size, 1, fd);
          }
          break;
        }
      }
      break;
    }

    //16bit color (R -> 5 bits, G -> 5 bits, B -> 5 bits)
    case 16: {
      int32_t x, y, w=self->width, h=self->height;
      uint32_t i, p, col16;

      for(y=h-1; y>=0; y--) {
        memset(buffer, 0, buf_size);
        for(x=0, i=0; x<w; x++, i+=2) {
          p = rtBitmapGetPixel(self, x, y);
          col16 = ((rtColorGetR(p)>>3)<<10) | ((rtColorGetG(p)>>3)<<5) | (rtColorGetB(p)>>3);
          *(buffer+i+1) = (col16>>8)&0xff;
          *(buffer+i) = col16&0xff;
        }
        fwrite(buffer, buf_size, 1, fd);
      }
      break;
    }

    //True color
    case 24:
    case 32: {
      int32_t x, y, w=self->width, h=self->height;
      uint32_t i, p;

      for(y=h-1; y>=0; y--) {
        memset(buffer, 0, buf_size);
        for(x=0, i=0; x<w; x++, i+=bpp==24? 3: 4) {
          p = rtBitmapGetPixel(self, x, y);
          *(buffer+i) = rtColorGetB(p);
          *(buffer+i+1) = rtColorGetG(p);
          *(buffer+i+2) = rtColorGetR(p);
        }
        fwrite(buffer, buf_size, 1, fd);
      }
      break;
    }
  }

garbage_collect:
  if(fd)
    fclose(fd);
  if(buffer)
    free(buffer);
  if(palette)
    free(palette);
}

// vim: tabstop=2 shiftwidth=2 softtabstop=2
