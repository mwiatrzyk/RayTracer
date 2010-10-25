#ifndef __VECTORMATH_H
#define __VECTORMATH_H

#include <math.h>
#include "scene.h"

/* Copies given vector `self` and stores its copy in vector pointed by `dest`.
 * Returns `dest`. */
static inline SCN_Vertex* vec_vector_copy(SCN_Vertex *self, SCN_Vertex *dest) {
    dest->x = self->x;
    dest->y = self->y;
    dest->z = self->z;
    return dest;
}

/* Subtracts vector `b` from vector `a` and stores result in vector `self`.
 * When `a` and `b` are interpreted as points, not vectors, this function will
 * calculate vector from point `b` towards point `a`.  Returns `self`. */
static inline SCN_Vertex* vec_vector_sub(SCN_Vertex *self, SCN_Vertex *a, SCN_Vertex *b) {
    self->x = a->x - b->x;
    self->y = a->y - b->y;
    self->z = a->z - b->z;
    return self;
}

/* Alias for `vec_vector_sub`. Used to calculate vector from point `a` towards
 * point `b`. */
static inline SCN_Vertex* vec_vector_make(SCN_Vertex *self, SCN_Vertex *a, SCN_Vertex *b) {
    return vec_vector_sub(self, b, a);
}

/* Returns length of given vector `self`. */
static inline float vec_vector_length(SCN_Vertex *self) {
    return sqrt(self->x*self->x + self->y*self->y + self->z*self->z);
}

/* Calculates Euclidean distance between given vector `self` and vector `v`. */
static inline float vec_vector_distance(SCN_Vertex *self, SCN_Vertex *v) {
    float dx=self->x-v->x, dy=self->y-v->y, dz=self->z-v->z;
    return sqrt(dx*dx + dy*dy + dz*dz);
}

/* Changes direction of given vector. */
static inline SCN_Vertex* vec_vector_inverse(SCN_Vertex *self, SCN_Vertex *v) {
    self->x = -v->x;
    self->y = -v->y;
    self->z = -v->z;
    return self;
}

/* Normalizes given vector "inplace". Returns `self`. */
static inline SCN_Vertex* vec_vector_normalize(SCN_Vertex *self) {
    float len=sqrt(self->x*self->x + self->y*self->y + self->z*self->z);
    self->x /= len;
    self->y /= len;
    self->z /= len;
    return self;
}

/* Multiplicates vector `v` by scalar `t` and stores result in `self`. Returns
 * `self`. */
static inline SCN_Vertex* vec_vector_mul(SCN_Vertex *self, SCN_Vertex *v, float t) {
    self->x = v->x * t;
    self->y = v->y * t;
    self->z = v->z * t;
    return self;
}

/* Adds vector `b` to vector `a` and stores result in `self`. Returns `self`. */
static inline SCN_Vertex* vec_vector_add(SCN_Vertex *self, SCN_Vertex *a, SCN_Vertex *b) {
    self->x = a->x + b->x;
    self->y = a->y + b->y;
    self->z = a->z + b->z;
    return self;
}

/* Calculates cross product of vectors `a` and `b` and stores result in `self`.
 * Returns `self`. */
static inline SCN_Vertex* vec_vector_crossp(SCN_Vertex *self, SCN_Vertex *a, SCN_Vertex *b) {
    self->x = a->y*b->z - a->z*b->y;
    self->y = a->z*b->x - a->x*b->z;
    self->z = a->x*b->y - a->y*b->x;
    return self;
}

/* Calculates dot product of vector `self` and vector `v`. Returns dot product
 * value. */
static inline float vec_vector_dotp(SCN_Vertex *self, SCN_Vertex *v) {
    return self->x*v->x + self->y*v->y + self->z*v->z;
}

/* Calculates normalized ray vector pointing from `a` towards `b`. */
static inline SCN_Vertex* vec_vector_ray(SCN_Vertex *self, SCN_Vertex *a, SCN_Vertex *b) {
    self->x = b->x - a->x;
    self->y = b->y - a->y;
    self->z = b->z - a->z;
    return vec_vector_normalize(self);
}

/* Solves parametric ray equation R(t)=O+tD for given params and returns
 * resulting point. Used to calculate intersection point coordinates. 
 
 @param: self: pointer to result point
 @param: o: ray origin
 @param: r: ray direction vector (must be normalized)
 @param: d: distance parameter */
static inline SCN_Vertex* vec_vector_raypoint(SCN_Vertex *self, SCN_Vertex *o, SCN_Vertex *r, float d) {
    self->x = o->x + d*r->x;
    self->y = o->y + d*r->y;
    self->z = o->z + d*r->z;
    return self;
}

/* Calculate normalized vector representing reflected ray by solving equation:
 * Z=2N(N*L)-L. 
 
 @param: self: result vector pointer
 @param: n: surface's normal vector 
 @param: l: normalized vector from light towards current intersection point */
static inline SCN_Vertex* vec_vector_ray_reflected(SCN_Vertex *self, SCN_Vertex *n, SCN_Vertex *l) {
    float n_dot_l = vec_vector_dotp(n, l);
    self->x = 2.0f * n->x * n_dot_l - l->x;
    self->y = 2.0f * n->y * n_dot_l - l->y;
    self->z = 2.0f * n->z * n_dot_l - l->z;
    return vec_vector_normalize(self);
}

#endif
