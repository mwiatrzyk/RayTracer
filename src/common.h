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


//// MACROS ///////////////////////////////////////////////////

/* Get minimal value of a, b and c. */
#define MIN(a,b,c) ((c)<((a)<(b)? (a): (b))? (c): ((a)<(b)? (a): (b)))

/* Get maximal value of a, b and c. */
#define MAX(a,b,c) ((c)>((a)>(b)? (a): (b))? (c): ((a)>(b)? (a): (b)))

/* `printf` wrapper. */
#define PRINT(prefix, msg, ...)  printf(prefix": %.3f sec: "msg"\n", (double)clock()/CLOCKS_PER_SEC, __VA_ARGS__)

/* Shows debug messages. */
#if VERBOSE <= DEBUG
  #define RT_DEBUG(msg, ...) PRINT("D", msg, __VA_ARGS__)
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
  #define RT_WARN(msg, ...) PRINT("W", msg, __VA_ARGS__)
#else
  #define RT_WARN(msg, ...)
#endif

/* Shows error messages. */
#if VERBOSE <= ERROR
  #define RT_ERROR(msg, ...) PRINT("E", msg, __VA_ARGS__)
#else
  #define RT_ERROR(msg, ...)
#endif

/* Shows critical error messages. */
#if VERBOSE <= CRITICAL
  #define RT_CRITICAL(msg, ...) PRINT("C", msg, __VA_ARGS__)
#else
  #define RT_CRITICAL(msg, ...)
#endif

/* Set of debug, info, warning, error and critical message logging macros with
 * message without arguments. */
#define RT_DDEBUG(msg) RT_DEBUG(msg"%s", "")
#define RT_IINFO(msg) RT_INFO(msg"%s", "")

#endif

// vim: tabstop=2 shiftwidth=2 softtabstop=2
