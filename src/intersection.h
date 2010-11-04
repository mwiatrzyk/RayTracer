#ifndef __INTERSECTION_H
#define __INTERSECTION_H

#include "scene.h"

/* Precalculates coefficients of intersection test algorithms for given
 * triangle `t`. */
void rtIntCoeffsPrecalc(RT_Triangle *t);

#endif

// vim: tabstop=2 shiftwidth=2 softtabstop=2
