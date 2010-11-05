/*
  Set of helper tools to work with `char*` strings.
*/
#ifndef __STRINGTOOLS_H
#define __STRINGTOOLS_H

#include "types.h"

/* Create empty string of given `length` and return pointer to it. */
char* rtStringCreate(uint32_t length);

/* Create copy of string `source` and return pointer to newly allocated copy. */
char* rtStringCopy(const char* source);

/* Free memory occupied by string `s` (prevoiusly created by `rtStringCreate`
 * or `rtStringCopy` functions) and zero the pointer. */
void rtStringDestroy(char** s);

/* Returns 1 if string `s` starts with string `needle` or 0 otherwise. */
int rtStringStartsWith(const char *s, const char *needle);

/* Creates new string that is concatenation of string `s1` and string `s2`. */
char* rtStringConcat(const char* s1, const char *s2);

#endif

// vim: tabstop=2 shiftwidth=2 softtabstop=2
