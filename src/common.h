/* 
  Set of common macros & inline functions. 
*/
#ifndef __COMMON_H
#define __COMMON_H

#include <time.h>


//// VERBOSITY LEVELS /////////////////////////////////////////

#define DEBUG     0   // debug messages
#define INFO      1   // RT_INFO messages
#define WARN      2   // warning messages
#define ERROR     3   // error messages
#define CRITICAL  4   // critical error messages


//// INLINE FUNCTIONS /////////////////////////////////////////

/* Returns minimal of given two float values. */
static inline float rtMinF(float a, float b) {
  return a<b? a: b;
}

/* Return absolute value of given float number `x`. */
static inline float rtAbs(float x) {
  return x>=0.0f? x: -x;
}


//// MACROS ///////////////////////////////////////////////////

/* Get minimal value of a, b and c. */
#define MIN(a,b,c) ((c)<((a)<(b)? (a): (b))? (c): ((a)<(b)? (a): (b)))

/* Get maximal value of a, b and c. */
#define MAX(a,b,c) ((c)>((a)>(b)? (a): (b))? (c): ((a)>(b)? (a): (b)))

/* `printf` wrapper. */
#define PRINT(prefix, msg, ...)  printf(prefix": %.3f sec: "msg"\n", (double)clock()/CLOCKS_PER_SEC, __VA_ARGS__);

/* Shows debug messages. */
#if VERBOSE <= DEBUG
  #define RT_DEBUG(msg, ...) PRINT("\033[94mD", msg"\033[0m", __VA_ARGS__)
#else
  #define RT_DEBUG(msg, ...)
#endif

/* Shows info messages. */
#if VERBOSE <= INFO
  #define RT_INFO(msg, ...) PRINT("I", msg, __VA_ARGS__)
#else
  #define RT_INFO(msg, ...)
#endif

/* Shows warning messages. */
#if VERBOSE <= WARN
  #define RT_WARN(msg, ...) PRINT("\033[93mW", msg"\033[0m", __VA_ARGS__)
#else
  #define RT_WARN(msg, ...)
#endif

/* Shows error messages. */
#if VERBOSE <= ERROR
  #define RT_ERROR(msg, ...) PRINT("\033[91mE", msg"\033[0m", __VA_ARGS__)
#else
  #define RT_ERROR(msg, ...)
#endif

/* Shows critical error messages. */
#if VERBOSE <= CRITICAL
  #define RT_CRITICAL(msg, ...) PRINT("\033[91mC", msg"\033[0m", __VA_ARGS__)
#else
  #define RT_CRITICAL(msg, ...)
#endif

/* Set of debug, info, warning, error and critical message logging macros with
 * message without arguments. */
#define RT_DDEBUG(msg) RT_DEBUG(msg"%s", "")
#define RT_IINFO(msg) RT_INFO(msg"%s", "")
#define RT_WWARN(msg) RT_WARN(msg"%s", "")
#define RT_EERROR(msg) RT_ERROR(msg"%s", "")
#define RT_CCRITICAL(msg) RT_CRITICAL(msg"%s", "")

#endif

// vim: tabstop=2 shiftwidth=2 softtabstop=2
