#include "imagelib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>


uint32_t il_error = 0;


IL_Bitmap* il_create_bitmap(int32_t width, int32_t height, uint32_t background) {
    IL_Bitmap* res = malloc(sizeof(IL_Bitmap));
    if(!res) {
        return NULL;  // allocation failed
    }
    res->width = width;
    res->height = height;
    res->background = background;
    res->pixels = malloc(width*height*sizeof(uint32_t));
    if(!res->pixels) {
        free(res);
        return NULL;  // allocation failed - free previously allocated bitmap and return
    }
    return res;
}


void il_destroy_bitmap(IL_Bitmap* self) {
    free(self->pixels);
    free(self);
}


IL_Bitmap* il_load(const char* filename) {
    IL_BmpHeader hdr;
    IL_DibType dib_type;

    memset(&hdr, 0, sizeof(hdr));

    //open file
    FILE *fd = fopen(filename, "rb");
    if (!fd) {
        il_error = IL_IO_ERROR;  //unable to read from file
        return NULL;
    }

    //read file header and check if this is real BMP file
    fread(&hdr.bfType, sizeof(hdr.bfType), 1, fd);
    fread(&hdr.bfSize, sizeof(hdr.bfSize), 1, fd);
    fread(&hdr.bfReserved1, sizeof(hdr.bfReserved1), 1, fd);
    fread(&hdr.bfReserved2, sizeof(hdr.bfReserved2), 1, fd);
    fread(&hdr.bfOffBits, sizeof(hdr.bfOffBits), 1, fd);
    if (hdr.bfType[0] != 'B' || hdr.bfType[1] != 'M') {
        il_error = IL_INVALID_FILE_FORMAT;  //BM signature check failed
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
            il_error = IL_FORMAT_NOT_SUPPORTED;
            return NULL;
        }
        dib_type = DIB_WIN_V3;
    } else {
        dib_type = DIB_OS2_V1;
    }
    
    //create result bitmap
    IL_Bitmap* res = il_create_bitmap(hdr.biWidth, hdr.biHeight>0 ? hdr.biHeight : -hdr.biHeight, rgba(0,0,0,0));
    if (!res) {
        il_error = IL_NOT_ENOUGH_MEMORY;
        return NULL;
    }

    //make scanline buffer - temporary storage of lines loaded from file
    uint32_t buf_size = (uint32_t)(4*ceil(hdr.biBitCount*hdr.biWidth/32.0));
    unsigned char *buffer = malloc(buf_size*sizeof(unsigned char)); 
    if (!buffer) {
        il_error = IL_NOT_ENOUGH_MEMORY;
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
                il_error = IL_NOT_ENOUGH_MEMORY;
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
                                set_pixel(res, x+k, y, rgba(*(ptr+2), *(ptr+1), *(ptr), 0));
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
                                set_pixel(res, x+k, y, rgba(*(ptr+2), *(ptr+1), *(ptr), 0));
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
                            set_pixel(res, x, y, rgba(*(ptr+2), *(ptr+1), *(ptr), 0));
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
                    set_pixel(res, x, y, rgba(((p>>10)&0x1f)<<3, ((p>>5)&0x1f)<<3, (p&0x1f)<<3, 0));
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
                    set_pixel(res, x, y, rgba(*(buffer+i+2), *(buffer+i+1), *(buffer+i), 0));
                }
            }
            break;
        }
    }

    fclose(fd);

    return res;
}
