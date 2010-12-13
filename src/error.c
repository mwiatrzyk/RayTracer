#include <errno.h>
#include "error.h"


/* Textual description of errors. */
static struct RT_ErrorDescription errdesc[] = {
  /*--------------
    Common errors
   ---------------*/
  {E_IO,                  "unable to read and/or write to file"},
  {E_MEMORY,              "not enough memory"},

  /*---------------------------- 
    used by: `1imagelib` module 
   -----------------------------*/
  {E_INVALID_FILE,        "not a BMP file"},
  {E_INVALID_FILE_FORMAT, "BMP format not supported"},
  {E_INVALID_BPP,         "invalid number of bits per pixel"},

  /*------------------------
    used by: `scene` module 
   -------------------------*/
  {E_NOT_ENOUGH_SURFACES, "scene requires more surfaces to be defined"},

  /*---------------------------
    used by: `voxelize` module
   ----------------------------*/
  {E_INVALID_PARAM_VALUE, "parameter has invalid value"},

  /*-------
    others
   --------*/
  {0,                     "success"},
  {-1,                    "unexpected error"}
};


///////////////////////////////////////////////////////////////
char* rtGetErrorDesc() {
  int i=0;
  while(1) {
    if (errno == errdesc[i].eid) {
      return errdesc[i].desc;
    } else if (errdesc[i].eid == -1) {
      return errdesc[i].desc;
    }
    i++;
  }
}

// vim: tabstop=2 shiftwidth=2 softtabstop=2
