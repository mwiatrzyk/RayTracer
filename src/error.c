#include <errno.h>
#include "types.h"
#include "error.h"


/* Textual description of errors. 
 * TODO: add description once another error code is added */
static struct ERR_Description errdesc[] = {
    //common error codes
    {E_IO,                  "unable to read and/or write to file"},
    {E_MEMORY,              "not enough memory"},

    //error codes used by imagelib.c
    {E_INVALID_FILE,        "not a BMP file"},
    {E_INVALID_FILE_FORMAT, "BMP format not supported"},
    {E_INVALID_BPP,         "invalid number of bits per pixel"},
    
    //error codes used by scene.c
    {E_NOT_ENOUGH_SURFACES, "scene requires more surfaces to be defined"},

    //no error, unexpected error (array end signature)
    {0,                     "success"},
    {-1,                    "unexpected error"}
};


char* err_desc() {
    int16_t i=0;
    while(1) {
        if (errno == errdesc[i].eid) {
            return errdesc[i].desc;
        } else if (errdesc[i].eid == -1) {
            return errdesc[i].desc;
        }
        i++;
    }
}


