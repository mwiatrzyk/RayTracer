#ifndef __ERROR_H
#define __ERROR_H

#include "types.h"
#include <errno.h>

//// STRUCTURES ///////////////////////////////////////////////

/* Structure containing single errno -> errdesc mapping. */
struct ERR_Description {
    int32_t eid; 
    char desc[256];
};

//// ENUMERATIONS /////////////////////////////////////////////

/* List of imagelib's errors that may occur. 
 * TODO: add more error codes here */
typedef enum _ERR_Code {
    /* common */
    E_IO=1,         //unable to read or write from/to file
    E_MEMORY,       //unable to allocate memory

    /* used by: imagelib.c */
    E_INVALID_FILE,         //trying to load file of invalid format
    E_INVALID_FILE_FORMAT,  //image file format is not supported
    E_INVALID_BPP           //bits per pixel argument is invalid (correct ones are: 1, 4, 8, 16, 24, 32)
} ERR_Code;

//// FUNCTIONS ////////////////////////////////////////////////

/* Gets text description of currently set `errno` variable. */
char* err_desc();

#endif
