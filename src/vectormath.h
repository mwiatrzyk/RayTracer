#ifndef __VECTORMATH_H
#define __VECTORMATH_H

#include <math.h>
#include "scene.h"

/* Copies given vector `self` and stores its copy in vector pointed by `dest`.
 * Returns `dest`. */
inline SCN_Vertex* vec_vector_copy(SCN_Vertex *self, SCN_Vertex *dest) {
    dest->x = self->x;
    dest->y = self->y;
    dest->z = self->z;
    return dest;
}

/* Subtracts vector `b` from vector `a` and stores result in vector `self`.
 * When `a` and `b` are interpreted as points, not vectors, this function will
 * calculate vector from point `b` towards point `a`.  Returns `self`. */
inline SCN_Vertex* vec_vector_sub(SCN_Vertex *self, SCN_Vertex *a, SCN_Vertex *b) {
    self->x = a->x - b->x;
    self->y = a->y - b->y;
    self->z = a->z - b->z;
    return self;
}

/* Alias for `vec_vector_sub`. Used to calculate vector from point `a` towards
 * point `b`. */
inline SCN_Vertex* vec_vector_make(SCN_Vertex *self, SCN_Vertex *a, SCN_Vertex *b) {
    return vec_vector_sub(self, b, a);
}

/* Returns length of given vector `self`. */
inline float vec_vector_length(SCN_Vertex *self) {
    return sqrt(self->x*self->x + self->y*self->y + self->z*self->z);
}

/* Calculates Euclidean distance between given vector `self` and vector `v`. */
inline float vec_vector_distance(SCN_Vertex *self, SCN_Vertex *v) {
    float dx=self->x-v->x, dy=self->y-v->y, dz=self->z-v->z;
    return sqrt(dx*dx + dy*dy + dz*dz);
}

/* Normalizes given vector "inplace". Returns `self`. */
inline SCN_Vertex* vec_vector_normalize(SCN_Vertex *self) {
    float len=sqrt(self->x*self->x + self->y*self->y + self->z*self->z);
    self->x /= len;
    self->y /= len;
    self->z /= len;
    return self;
}

/* Multiplicates vector `v` by scalar `t` and stores result in `self`. Returns
 * `self`. */
inline SCN_Vertex* vec_vector_mul(SCN_Vertex *self, SCN_Vertex *v, float t) {
    self->x = v->x * t;
    self->y = v->y * t;
    self->z = v->z * t;
    return self;
}

/* Adds vector `b` to vector `a` and stores result in `self`. Returns `self`. */
inline SCN_Vertex* vec_vector_add(SCN_Vertex *self, SCN_Vertex *a, SCN_Vertex *b) {
    self->x = a->x + b->x;
    self->y = a->y + b->y;
    self->z = a->z + b->z;
    return self;
}

/* Calculates cross product of vectors `a` and `b` and stores result in `self`.
 * Returns `self`. */
inline SCN_Vertex* vec_vector_crossp(SCN_Vertex *self, SCN_Vertex *a, SCN_Vertex *b) {
    self->x = a->y*b->z - a->z*b->y;
    self->y = a->z*b->x - a->x*b->z;
    self->z = a->x*b->y - a->y*b->x;
    return self;
}

/* Calculates dot product of vector `self` and vector `v`. Returns dot product
 * value. */
inline float vec_vector_dotp(SCN_Vertex *self, SCN_Vertex *v) {
    return self->x*v->x + self->y*v->y + self->z*v->z;
}

#endif
