/* 
  Set of common macros & inline functions. 
*/
#ifndef __COMMON_H
#define __COMMON_H

#define MIN(a,b,c) ((c)<((a)<(b)? (a): (b))? (c): ((a)<(b)? (a): (b)))
#define MAX(a,b,c) ((c)>((a)>(b)? (a): (b))? (c): ((a)>(b)? (a): (b)))

#endif

// vim: tabstop=2 shiftwidth=2 softtabstop=2
