#ifndef __VECTORMATH_H
#define __VECTORMATH_H

#include "scene.h"

/* Creates vector pointing from point `a` towards point `b`. */
inline SCN_Vertex vec_make_vector(SCN_Vertex *a, SCN_Vertex *b);

/* Normalizes given vector and returns normalized vector. */
inline SCN_Vertex vec_normalize(SCN_Vertex *v);

/* Multiplicates given vector by given constant and returns resulting vector. */
inline SCN_Vertex vec_mul(SCN_Vertex *v, float t);

/* Calculates and returns vector that is sum of given two vectors. If first
 * vector is interpreted as point this function performs translation of point
 * `v1` by vector `v2`. */
inline SCN_Vertex vec_sum(SCN_Vertex *v1, SCN_Vertex *v2);

/* Calculates crossproduct of two vectors. */
inline SCN_Vertex vec_crossproduct(SCN_Vertex *v1, SCN_Vertex *v2);

/* Calculates dotproduct of two vectors. */
inline float vec_dotproduct(SCN_Vertex *v1, SCN_Vertex *v2);

#endif
