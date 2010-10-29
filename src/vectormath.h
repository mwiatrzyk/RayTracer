#ifndef __VECTORMATH_H
#define __VECTORMATH_H

#include <math.h>
#include "scene.h"

/* Copies given vector `self` and stores its copy in vector pointed by `dest`.
 * Returns `dest`. */
static inline float* vec_vector_copy(float *self, float *dest) {
    dest[0] = self[0];  //x
    dest[1] = self[1];  //y
    dest[2] = self[2];  //z
    return dest;
}

/* Subtracts vector `b` from vector `a` and stores result in vector `self`.
 * When `a` and `b` are interpreted as points, not vectors, this function will
 * calculate vector from point `b` towards point `a`.  Returns `self`. */
static inline float* vec_vector_sub(float *self, float *a, float *b) {
    self[0] = a[0] - b[0];
    self[1] = a[1] - b[1];
    self[2] = a[2] - b[2];
    return self;
}

/* Same as `vec_vector_sub` but this one subtracts vector `a` from vector `b`,
 * creating vector `a->b`. */
static inline float* vec_vector_make(float *self, float *a, float *b) {
    self[0] = b[0] - a[0];
    self[1] = b[1] - a[1];
    self[2] = b[2] - a[2];
    return self;
}

/* Returns length of given vector `self`. */
static inline float vec_vector_length(float *self) {
    return sqrt(self[0]*self[0] + self[1]*self[1] + self[2]*self[2]);
}

/* Calculates Euclidean distance between given vector `self` and vector `v`.
 * Can also be used to calculate distance between two points in 3D space. */
static inline float vec_vector_distance(float *self, float *v) {
    float dx=self[0]-v[0], dy=self[1]-v[1], dz=self[2]-v[2];
    return sqrt(dx*dx + dy*dy + dz*dz);
}

/* Changes direction of given vector. */
static inline float* vec_vector_inverse(float *self, float *v) {
    self[0] = -v[0];
    self[1] = -v[1];
    self[2] = -v[2];
    return self;
}

/* Normalizes given vector "inplace". Returns `self`. */
static inline float* vec_vector_normalize(float *self) {
    float len=sqrt(self[0]*self[0] + self[1]*self[1] + self[2]*self[2]);
    self[0] /= len;
    self[1] /= len;
    self[2] /= len;
    return self;
}

/* Multiplicates vector `v` by scalar `t` and stores result in `self`. Returns
 * `self`. */
static inline float* vec_vector_mul(float *self, float *v, float t) {
    self[0] = v[0] * t;
    self[1] = v[1] * t;
    self[2] = v[2] * t;
    return self;
}

/* Adds vector `b` to vector `a` and stores result in `self`. Returns `self`. */
static inline float* vec_vector_add(float *self, float *a, float *b) {
    self[0] = a[0] + b[0];
    self[1] = a[1] + b[1];
    self[2] = a[2] + b[2];
    return self;
}

/* Calculates cross product of vectors `a` and `b` and stores result in `self`.
 * Returns `self`. */
static inline float* vec_vector_crossp(float *self, float *a, float *b) {
    self[0] = a[1]*b[2] - a[2]*b[1];
    self[1] = a[2]*b[0] - a[0]*b[2];
    self[2] = a[0]*b[1] - a[1]*b[0];
    return self;
}

/* Calculates dot product of vector `self` and vector `v`. Returns dot product
 * value. */
static inline float vec_vector_dotp(float *self, float *v) {
    return self[0]*v[0] + self[1]*v[1] + self[2]*v[2];
}

/* Calculates normalized ray vector pointing from `a` towards `b`. */
static inline float* vec_vector_ray(float *self, float *a, float *b) {
    self[0] = b[0] - a[0];
    self[1] = b[1] - a[1];
    self[2] = b[2] - a[2];
    return vec_vector_normalize(self);
}

/* Solves parametric ray equation R(t)=O+tD for given params and returns
 * resulting point. Used to calculate intersection point coordinates. 
 
 @param: self: pointer to result point
 @param: o: ray origin
 @param: r: ray direction vector (must be normalized)
 @param: d: distance parameter */
static inline float* vec_vector_raypoint(float *self, float *o, float *r, float d) {
    self[0] = o[0] + d*r[0];
    self[1] = o[1] + d*r[1];
    self[2] = o[2] + d*r[2];
    return self;
}

/* Calculate normalized vector representing reflected ray by solving equation:
 * Z=2N(N*L)-L. 
 
 @param: self: result vector pointer
 @param: n: surface's normal vector 
 @param: l: normalized vector from light towards current intersection point */
static inline float* vec_vector_ray_reflected(float *self, float *n, float *l) {
    float n_dot_l = vec_vector_dotp(n, l);
    self[0] = 2.0f * n[0] * n_dot_l - l[0];
    self[1] = 2.0f * n[1] * n_dot_l - l[1];
    self[2] = 2.0f * n[2] * n_dot_l - l[2];
    return vec_vector_normalize(self);
}

/* Same as `vec_vector_ray_reflected` but requires dot product between `n` and
 * `l` to be calculated outside and to be passed as `n_dot_l`. */
static inline float* vec_vector_ray_reflected2(float *self, float *n, float *l, float n_dot_l) {
    self[0] = 2.0f * n[0] * n_dot_l - l[0];
    self[1] = 2.0f * n[1] * n_dot_l - l[1];
    self[2] = 2.0f * n[2] * n_dot_l - l[2];
    return vec_vector_normalize(self);
}

/* Calculate normalized vector representing refracted ray. */
static inline float* vec_vector_ray_refracted(float *self, float *n, float *l, float eta) {
    float n_dotp_l=vec_vector_dotp(n, l);
    float f=eta*n_dotp_l - sqrt(1.0f - (eta*eta) * (1.0f - n_dotp_l*n_dotp_l));
    self[0] = f*n[0] - eta*l[0];
    self[1] = f*n[1] - eta*l[1];
    self[2] = f*n[2] - eta*l[2];
    return self; //TODO: normalize
}

#endif
