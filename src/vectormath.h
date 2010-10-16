#ifndef __VECTORMATH_H
#define __VECTORMATH_H

#include <math.h>
#include "scene.h"

/* Creates vector pointing from point `a` towards point `b`. */
inline SCN_Vertex vec_make_vector(SCN_Vertex *a, SCN_Vertex *b) {
    SCN_Vertex res = {
        b->x - a->x,
        b->y - a->y,
        b->z - a->z
    };
    return res;
}

/* Returns length of given vector. */
inline float vec_length(SCN_Vertex *v) {
    return sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
}

/* Calculates Euclidean distance between given two vectors. */
inline float vec_distance(SCN_Vertex *v1, SCN_Vertex *v2) {
    float dx=v1->x-v2->x, dy=v1->y-v2->y, dz=v1->z-v2->z;
    return sqrt(dx*dx + dy*dy + dz*dz);
}

/* Normalizes given vector and returns normalized vector. */
inline SCN_Vertex vec_normalize(SCN_Vertex *v) {
    float len=sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
    SCN_Vertex res = {
        v->x / len,
        v->y / len,
        v->z / len
    };
    return res;
}

/* Multiplicates given vector by given constant and returns resulting vector. */
inline SCN_Vertex vec_mul(SCN_Vertex *v, float t) {
    SCN_Vertex res = {
        v->x * t,
        v->y * t,
        v->z * t
    };
    return res;
}

/* Calculates and returns vector that is sum of given two vectors. If first
 * vector is interpreted as point this function performs translation of point
 * `v1` by vector `v2`. */
inline SCN_Vertex vec_sum(SCN_Vertex *v1, SCN_Vertex *v2) {
    SCN_Vertex res = {
        v1->x + v2->x,
        v1->y + v2->y,
        v1->z + v2->z
    };
    return res;
}

/* Calculates crossproduct of two vectors. */
inline SCN_Vertex vec_crossproduct(SCN_Vertex *v1, SCN_Vertex *v2) {
    SCN_Vertex res = {
        v1->y*v2->z - v1->z*v2->y,
        v1->z*v2->x - v1->x*v2->z,
        v1->x*v2->y - v1->y*v2->x
    };
    return res;
}

/* Calculates dotproduct of two vectors. */
inline float vec_dotproduct(SCN_Vertex *v1, SCN_Vertex *v2) {
    return v1->x*v2->x + v1->y*v2->y + v1->z*v2->z;
}

#endif
