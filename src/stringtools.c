#include <errno.h>
#include <string.h>
#include "error.h"
#include "stringtools.h"


///////////////////////////////////////////////////////////////
char* rtStringCreate(uint32_t length) {
  char *res=malloc(length*sizeof(char)+1);
  if(!res) {
    errno = E_MEMORY;
    return NULL;
  }
  memset(res, 0, length*sizeof(char)+1);
  return res;
}


///////////////////////////////////////////////////////////////
char* rtStringCopy(const char* source) {
  char *res=rtStringCreate(strlen(source));
  if(!res)
    return NULL;  // errno=E_MEMORY
  strcpy(res, source);
  return res;
}


///////////////////////////////////////////////////////////////
void rtStringDestroy(char** s) {
  if(*s) {
    free(*s);
    *s = NULL;
  }
}


///////////////////////////////////////////////////////////////
int rtStringStartsWith(const char *s, const char *needle) {
  uint32_t sl=strlen(s), nl=strlen(needle);
  if(nl > sl) {
    return 0;
  }
  const char *t1=needle, *t2=s, *maxt1=(const char*)(needle+nl);
  while(t1 < maxt1) {
    if(*t1 != *t2) {
      return 0;
    }
    t1++; t2++;
  }
  return 1;
}


///////////////////////////////////////////////////////////////
char* rtStringConcat(const char* s1, const char *s2) {
  char *res=rtStringCreate(strlen(s1)+strlen(s2));
  strcpy(res, s1);
  return strcat(res, s2);
}

// vim: tabstop=2 shiftwidth=2 softtabstop=2
