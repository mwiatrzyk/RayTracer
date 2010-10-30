#ifndef __TYPES_H
#define __TYPES_H

#define DEBUGMODE 1

#ifndef NULL
#define NULL (void*)0
#endif

#define __ALIGN_16 __attribute__((aligned(16)))

typedef enum _bool {
    false = 0,
    true = 1
} bool;

typedef short int int16_t;
typedef unsigned short int uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long int int64_t;
typedef unsigned long int uint64_t;

#endif
