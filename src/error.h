// vim: tabstop=2 shiftwidth=2 softtabstop=2
#ifndef __ERROR_H
#define __ERROR_H

#include "types.h"
#include <errno.h>

//// STRUCTURES ///////////////////////////////////////////////

/* Structure containing single errno -> errdesc mapping. */
struct RT_ErrorDescription {
  int32_t eid;      // error id
  char desc[256];   // error desc
};

//// ENUMERATIONS /////////////////////////////////////////////

/* List of error codes that may occur during raytrace process. */
typedef enum _RT_ErrorCode {
  /*--------------
    Common errors
   ---------------*/
  E_IO=1,         //unable to read or write from/to file
  E_MEMORY,       //unable to allocate memory

  /*---------------------------- 
    used by: `1imagelib` module 
   -----------------------------*/
  E_INVALID_FILE,         //trying to load file of invalid format
  E_INVALID_FILE_FORMAT,  //image file format is not supported
  E_INVALID_BPP,          //bits per pixel argument is invalid (correct ones are: 1, 4, 8, 16, 24, 32)

  /*------------------------
    used by: `scene` module 
   -------------------------*/
  E_NOT_ENOUGH_SURFACES,   //not enough surfaces to cover entire scene

  /*---------------------------
    used by: `voxelize` module
   ----------------------------*/
  E_INVALID_PARAM_VALUE   // parameter has invalid value
} RT_ErrorCode;

//// FUNCTIONS ////////////////////////////////////////////////

/* Gets text description of currently set `errno` variable. */
char* rtGetErrorDesc();

#endif
