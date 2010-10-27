#ifndef __STRINGTOOLS_H
#define __STRINGTOOLS_H

#include "types.h"

/* Create empty string of given length and return pointer to it. */
char* str_string_create(uint32_t size);

/* Create copy of string `source` and return pointer to newly allocated copy. */
char* str_string_copy(const char* source);

/* Free memory occupied by string `self` and zero the pointer. */
void str_string_destroy(char** self);

/* Returns 1 if string `self` starts with string `needle` or 0 otherwise. */
int str_string_startswith(const char *self, const char *needle);

/* Creates new string that is concatenation of string `self` and string `str`. */
char* str_string_concat(const char* self, const char *str);

#endif
