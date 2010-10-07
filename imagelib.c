#include "imagelib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>


uint32_t iml_errno = 0;


inline void iml_bitmap_setpixel(IML_Bitmap* self, int32_t x, int32_t y, uint32_t color) {
    *(self->pixels+(y*self->width)+x) = color;
}


inline uint32_t iml_bitmap_getpixel(const IML_Bitmap* self, int32_t x, int32_t y) {
    return *(self->pixels+(y*self->width)+x);
}


inline uint32_t iml_rgba(uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
    return ((r&0xff)<<24) | ((g&0xff)<<16) | ((b&0xff)<<8) | (a&0xff);
}


inline uint32_t iml_getr(uint32_t color) {
    return (color>>24)&0xff;
}


inline uint32_t iml_getg(uint32_t color) {
    return (color>>16)&0xff;
}


inline uint32_t iml_getb(uint32_t color) {
    return (color>>8)&0xff;
}


inline uint32_t iml_geta(uint32_t color) {
    return color&0xff;
}


int iml_error(void) {
    return iml_errno;
}


char* iml_error_string(int errno) {
    int32_t i=0;
    while(1) {
        if (errno == iml_errdesc[i].errno) {
            return iml_errdesc[i].desc;
        } else if (iml_errdesc[i].errno == -1) {
            return iml_errdesc[i].desc;
        }
        i++;
    }
}


IML_Bitmap* iml_bitmap_create(int32_t width, int32_t height, uint32_t background) {
    uint32_t *p, *maxp;
    IML_Bitmap* res = malloc(sizeof(IML_Bitmap));
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


void iml_bitmap_destroy(IML_Bitmap* self) {
    free(self->pixels);
    free(self);
}


IML_Bitmap* iml_bitmap_load(const char* filename) {
    IML_BmpHeader hdr;
    IML_DibType dib_type;

    memset(&hdr, 0, sizeof(hdr));

    //open file
    FILE *fd = fopen(filename, "rb");
    if (!fd) {
        iml_errno = IML_IO_ERROR;  //unable to read from file
        return NULL;
    }

    //read file header and check if this is real BMP file
    fread(&hdr.bfType, sizeof(hdr.bfType), 1, fd);
    fread(&hdr.bfSize, sizeof(hdr.bfSize), 1, fd);
    fread(&hdr.bfReserved1, sizeof(hdr.bfReserved1), 1, fd);
    fread(&hdr.bfReserved2, sizeof(hdr.bfReserved2), 1, fd);
    fread(&hdr.bfOffBits, sizeof(hdr.bfOffBits), 1, fd);
    if (hdr.bfType[0] != 'B' || hdr.bfType[1] != 'M') {
        iml_errno = IML_NOT_A_BMP_FILE;  //BM signature check failed
        return NULL;
    }

    //read DIB part of file header
    fread(&hdr.biSize, sizeof(hdr.biSize), 1, fd);
    fread(&hdr.biWidth, sizeof(hdr.biWidth), 1, fd);
    fread(&hdr.biHeight, sizeof(hdr.biHeight), 1, fd);
    fread(&hdr.biPlanes, sizeof(hdr.biPlanes), 1, fd);
    fread(&hdr.biBitCount, sizeof(hdr.biBitCount), 1, fd);
    if (hdr.biSize == 40) {
        fread(&hdr.biCompression, sizeof(hdr.biCompression), 1, fd);
        fread(&hdr.biSizeImage, sizeof(hdr.biSizeImage), 1, fd);
        fread(&hdr.biXPelsPerMeter, sizeof(hdr.biXPelsPerMeter), 1, fd);
        fread(&hdr.biYPelsPerMeter, sizeof(hdr.biYPelsPerMeter), 1, fd);
        fread(&hdr.biClrUsed, sizeof(hdr.biClrUsed), 1, fd);
        fread(&hdr.biClrImportant, sizeof(hdr.biClrImportant), 1, fd);
        fread(&hdr.biClrRotation, sizeof(hdr.biClrRotation), 1, fd);
        fread(&hdr.biReserved, sizeof(hdr.biReserved), 1, fd);
        if (hdr.biCompression != 0) {
            iml_errno = IML_FORMAT_NOT_SUPPORTED;
            return NULL;
        }
        dib_type = DIB_WIN_V3;
    } else {
        dib_type = DIB_OS2_V1;
    }
    
    //create result bitmap
    IML_Bitmap* res = iml_bitmap_create(hdr.biWidth, hdr.biHeight>0 ? hdr.biHeight : -hdr.biHeight, iml_rgba(0,0,0,0));
    if (!res) {
        iml_errno = IML_NOT_ENOUGH_MEMORY;
        return NULL;
    }

    //make scanline buffer - temporary storage of lines loaded from file
    uint32_t buf_size = (uint32_t)(4*ceil(hdr.biBitCount*hdr.biWidth/32.0));
    unsigned char *buffer = malloc(buf_size*sizeof(unsigned char)); 
    if (!buffer) {
        iml_errno = IML_NOT_ENOUGH_MEMORY;
        return NULL;
    }

    switch(hdr.biBitCount) {
        /* with palette */
        case 1:
        case 4:
        case 8: {
            //allocate memory for palette
            uint32_t pal_size = hdr.biClrUsed!=0 ? hdr.biClrUsed : (dib_type==DIB_OS2_V1 ? 3 : 4)*(uint32_t)pow(2.0f, hdr.biBitCount);
            unsigned char *palette = malloc(pal_size*sizeof(unsigned char));
            if (!palette) {
                iml_errno = IML_NOT_ENOUGH_MEMORY;
                return NULL;
            }
            
            //read palette from file
            fread(palette, pal_size, 1, fd);
            
            switch(hdr.biBitCount) {
                //monochrome
                case 1: {
                    int32_t x, y, w=hdr.biWidth, h=hdr.biHeight;
                    uint32_t i;
                    int16_t k;
                    unsigned char *ptr, p;

                    for(y=h>0? h-1: 0; h>0? y>=0: y<-h; h>0? y--: y++) {
                        fread(buffer, buf_size, 1, fd);
                        for(x=0, i=0; x<w; x+=8, i++) {
                            p=*(buffer+i);
                            for(k=0; k<8; k++) {  //iteration over bits in buffer[i]
                                if (x+k >= w)
                                    break;
                                ptr = palette+(dib_type==DIB_OS2_V1 ? ((p>>(7-k))&1)*3 : ((p>>(7-k))&1)<<2);
                                iml_bitmap_setpixel(res, x+k, y, iml_rgba(*(ptr+2), *(ptr+1), *(ptr), 0));
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
                        fread(buffer, buf_size, 1, fd);
                        for(x=0, i=0; x<w; x+=2, i++) {
                            p[0] = *(buffer+i);   //buffer[i] przechowuje 2 piksele
                            p[1] = *(p)&15;       //4 najmlodsze bity do p[1] (drugi piksel)
                            p[0] = *(p)>>4;       //4 najstarsze bity do p[0] (pierwszy piksel)
                            for(k=0; k<2; k++) {
                                if (x+k >= w)
                                    break;
                                ptr = palette+(dib_type==DIB_OS2_V1 ? p[k]*3 : p[k]<<2);
                                iml_bitmap_setpixel(res, x+k, y, iml_rgba(*(ptr+2), *(ptr+1), *(ptr), 0));
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
                        fread(buffer, buf_size, 1, fd);
                        for(x=0, i=0; x<w; x++, i++) {
                            p=*(buffer+i);
                            ptr=palette+(dib_size==12 ? p*3 : p<<2);
                            iml_bitmap_setpixel(res, x, y, iml_rgba(*(ptr+2), *(ptr+1), *(ptr), 0));
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
                fread(buffer, buf_size, 1, fd);
                for(x=0, i=0; x<w; x++, i+=2) {
                    p = (*(buffer+i+1)<<8) | *(buffer+i);
                    iml_bitmap_setpixel(res, x, y, iml_rgba(((p>>10)&0x1f)<<3, ((p>>5)&0x1f)<<3, (p&0x1f)<<3, 0));
                }
            }
            break;
        }

        case 24:
        case 32:
        {
            int32_t x, y, w=hdr.biWidth, h=hdr.biHeight;
            uint32_t i;
            uint16_t bpp=hdr.biBitCount;

            for(y=h>0? h-1: 0; h>0? y>=0: y<-h; h>0? y--: y++) {
                fread(buffer, buf_size, 1, fd);
                for(x=0, i=0; x<w; x++, i+=bpp==24? 3: 4) {
                    iml_bitmap_setpixel(res, x, y, iml_rgba(*(buffer+i+2), *(buffer+i+1), *(buffer+i), 0));
                }
            }
            break;
        }
    }

    fclose(fd);

    return res;
}


void iml_bitmap_save(const IML_Bitmap* self, const char* filename, uint16_t bpp) {
    IML_BmpHeader hdr;
    uint32_t palette_size=4*(uint32_t)pow(2.0f, bpp);

    memset(&hdr, 0, sizeof(hdr));

    //validate bpp parameter
    if (bpp!=1 && bpp!=4 && bpp!=8 && bpp!=16 && bpp!=24 && bpp!=32) {
        iml_errno = IML_INVALID_BPP;
        return;
    }
    
    //open file for binary write mode
    FILE *fd=fopen(filename, "wb");
    if (!fd) {
        iml_errno = IML_IO_ERROR;  //unable to read from file
        return;
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
    fwrite(&hdr, sizeof(hdr), 1, fd);

    //create scanline buffer
    uint32_t buf_size=(uint32_t)(4*ceil(bpp*self->width/32.0));
    unsigned char *buffer=malloc(buf_size*sizeof(unsigned char));
    if(!buffer) {
        iml_errno = IML_NOT_ENOUGH_MEMORY;
        return;
    }

    switch(bpp) {
        case 1:
        case 4:
        case 8: {
            //create palette buffer
            unsigned char *palette=malloc(palette_size*sizeof(unsigned char));
            if (!palette) {
                iml_errno = IML_NOT_ENOUGH_MEMORY;
                return;
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
                                p = iml_bitmap_getpixel(self, x+k, y);
                                p = (iml_getr(p)+iml_getg(p)+iml_getb(p))/3;  //grayscale (0..255)
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
                            p1 = iml_bitmap_getpixel(self, x, y);
                            if (x+1 < w) {
                                p2 = iml_bitmap_getpixel(self, x+1, y);
                            } else {
                                p2=0;
                            }
                            *(buffer+i) = (iml_getr(p1)>>7)<<7 | (iml_getg(p1)>>6)<<5 | (iml_getb(p1)>>7)<<4 | 
                                          (iml_getr(p2)>>7)<<3 | (iml_getg(p2)>>6)<<1 | iml_getb(p2)>>7;
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
                            p = iml_bitmap_getpixel(self, x, y);
                            *(buffer+i) = (iml_getr(p)>>5)<<5 | (iml_getg(p)>>5)<<2 | iml_getb(p)>>6;
                        }
                        fwrite(buffer, buf_size, 1, fd);
                    }
                    break;
                }
            }
        }

        //16bit color (R -> 5 bits, G -> 5 bits, B -> 5 bits)
        case 16: {
            int32_t x, y, w=self->width, h=self->height;
            uint32_t i, p, col16;

            for(y=h-1; y>=0; y--) {
                memset(buffer, 0, buf_size*sizeof(unsigned char));
                for(x=0, i=0; x<w; x++, i+=2) {
                    p = iml_bitmap_getpixel(self, x, y);
                    col16 = (iml_getr(p)>>3)<<10 | (iml_getg(p)>>3)<<5 | iml_getb(p)>>3;
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
                memset(buffer, 0, buf_size*sizeof(unsigned char));
                for(x=0, i=0; x<w; x++, i+=bpp==24? 3: 4) {
                    p = iml_bitmap_getpixel(self, x, y);
                    *(buffer+i) = iml_getb(p);
                    *(buffer+i+1) = iml_getg(p);
                    *(buffer+i+2) = iml_getr(p);
                }
                fwrite(buffer, buf_size, 1, fd);
            }
            break;
        }
    }
    
    fclose(fd);
}
