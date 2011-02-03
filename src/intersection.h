#ifndef __INTERSECTION_H
#define __INTERSECTION_H

#include "scene.h"


//// INTERSECTION TEST ALGORITHMS /////////////////////////////

/* First algorithm. Works by solving S+tR=u(A-B)+v(C-B) equation. */
int rtInt0Test(RT_Triangle *t, float *o, float *r, float *d, float *dmin, float *u, float *v);

/* Second algorithm. Projects 3D triangle onto 2D plane and then solves
 * intersection equation in 2D. */
int rtInt1Test(RT_Triangle *t, float *o, float *r, float *d, float *dmin, float *u, float *v);

/* Checks if given point belongs to given triangle using same methods as in
 * `rtInt1Test` function. */
int rtInt1TestPoint(RT_Triangle *t, float *p);


//// OTHER FUNCTIONS //////////////////////////////////////////

/* Precalculates coefficients of 2nd intersection test algorithm for given
 * triangle `t`. */
void rtInt1CoeffsPrecalc(RT_Triangle *t);

#endif

// vim: tabstop=2 shiftwidth=2 softtabstop=2
