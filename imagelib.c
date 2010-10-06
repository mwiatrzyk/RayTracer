#include "imagelib.h"
#include <stdlib.h>


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
