#include <errno.h>
#include <string.h>
#include "error.h"
#include "stringtools.h"


char* str_string_create(uint32_t size) {
    char *res=malloc(size*sizeof(char)+1);
    if(!res) {
        errno = E_MEMORY;
        return NULL;
    }
    memset(res, 0, size*sizeof(char)+1);
    return res;
}


char* str_string_copy(const char* source) {
    char *res=str_string_create(strlen(source));
    if(!res)
        return NULL;  // errno=E_MEMORY
    strcpy(res, source);
    return res;
}


void str_string_destroy(char** self) {
    if(*self) {
        free(*self);
        *self = NULL;
    }
}


int str_string_startswith(const char *self, const char *needle) {
    uint32_t sl=strlen(self), nl=strlen(needle);
    if(nl > sl) {
        return 0;
    }
    const char *t1=needle, *t2=self, *maxt1=(const char*)(needle+nl);
    while(t1 < maxt1) {
        if(*t1 != *t2) {
            return 0;
        }
        t1++; t2++;
    }
    return 1;
}


char* str_string_concat(const char* self, const char *str) {
    char *res=str_string_create(strlen(self)+strlen(str));
    strcpy(res, self);
    return strcat(res, str);
}
