/* 
  Set of common macros & inline functions. 
*/
#ifndef __COMMON_H
#define __COMMON_H

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

/* Shows debug messages. */
#if VERBOSE <= DEBUG
  #define RT_DEBUG(msg, _VA_ARGS_) printf("D: "msg"\n", _VA_ARGS_)
  #define RT_DDEBUG(msg) printf("D: "msg"\n")
#else
  #define RT_DEBUG(msg, _VA_ARGS_)
  #define RT_DDEBUG(msg, _VA_ARGS_)
#endif

/* Shows RT_INFO messages. */
#if VERBOSE <= INFO
  #define RT_INFO(msg, _VA_ARGS_) printf("I: "msg"\n", _VA_ARGS_)
  #define RT_IINFO(msg) printf("I: "msg"\n")
#else
  #define RT_INFO(msg, _VA_ARGS_)
  #define RT_IINFO(msg, _VA_ARGS_)
#endif

/* Shows warning messages. */
#if VERBOSE <= WARN
  #define RT_WARN(msg, _VA_ARGS_) printf("W: "msg"\n", _VA_ARGS_)
  #define RT_WWARN(msg) printf("W: "msg"\n")
#else
  #define RT_WARN(msg, _VA_ARGS_)
  #define RT_WWARN(msg, _VA_ARGS_)
#endif

/* Shows error messages. */
#if VERBOSE <= ERROR
  #define RT_ERROR(msg, _VA_ARGS_) printf("E: "msg"\n", _VA_ARGS_)
  #define RT_EERROR(msg) printf("E: "msg"\n")
#else
  #define RT_ERROR(msg, _VA_ARGS_)
  #define RT_EERROR(msg, _VA_ARGS_)
#endif

/* Shows critical error messages. */
#if VERBOSE <= CRITICAL
  #define RT_CRITICAL(msg, _VA_ARGS_) printf("C: "msg"\n", _VA_ARGS_)
  #define RT_CCRITICAL(msg) printf("C: "msg"\n")
#else
  #define RT_CRITICAL(msg, _VA_ARGS_)
  #define RT_CCRITICAL(msg, _VA_ARGS_)
#endif

#endif

// vim: tabstop=2 shiftwidth=2 softtabstop=2
