#ifndef __INTERSECTION_H
#define __INTERSECTION_H

#include "scene.h"


//// INTERSECTION TEST ALGORITHMS /////////////////////////////

/* First algorithm. */
int rtInt0Test(RT_Triangle *t, float *o, float *r, float *d, float *dmin);

/* Second algorithm. */
int rtInt1Test(RT_Triangle *t, float *o, float *r, float *d, float *dmin);


//// OTHER FUNCTIONS //////////////////////////////////////////

/* Precalculates coefficients of intersection test algorithms for given
 * triangle `t`. */
void rtIntCoeffsPrecalc(RT_Triangle *t);

#endif

// vim: tabstop=2 shiftwidth=2 softtabstop=2
