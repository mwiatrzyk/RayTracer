#ifndef __VECTORMATH_H
#define __VECTORMATH_H

#include <math.h>

/* Copies source vector to dest vector and returns dest vector. */
static inline float* rtVectorCopy(float *src, float *dest) {
  dest[0] = src[0];  //x
  dest[1] = src[1];  //y
  dest[2] = src[2];  //z
  return dest;
}

/* Subtracts vector `b` from vector `a` and stores result in vector `out`. */
static inline float* rtVectorSub(float *out, float *a, float *b) {
  out[0] = a[0] - b[0];
  out[1] = a[1] - b[1];
  out[2] = a[2] - b[2];
  return out;
}

/* Creates vector from vertex `a` towards vertex `b` and stores result in
 * `out`. Returns `out`. */
static inline float* rtVectorMake(float *out, float *a, float *b) {
  out[0] = b[0] - a[0];
  out[1] = b[1] - a[1];
  out[2] = b[2] - a[2];
  return out;
}

/* Returns length of given vector. */
static inline float rtVectorLength(float *v) {
  return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

/* Calculates distance between vertex `a` and `b`. */
static inline float rtVectorDistance(float *a, float *b) {
  float dx=a[0]-b[0], dy=a[1]-b[1], dz=a[2]-b[2];
  return sqrt(dx*dx + dy*dy + dz*dz);
}

/* Changes direction of given vector `v` and stores result in `out`. Returns
 * `out`. */
static inline float* rtVectorInverse(float *out, float *v) {
  out[0] = -v[0];
  out[1] = -v[1];
  out[2] = -v[2];
  return out;
}

/* Normalizes given vector "inplace". Returns `v`. */
static inline float* rtVectorNorm(float *v) {
  float inv_len=1.0f/sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
  v[0] *= inv_len;
  v[1] *= inv_len;
  v[2] *= inv_len;
  return v;
}

/* Multiplicates vector `v` by scalar `t` and stores result in `out`. Returns
 * `out`. */
static inline float* rtVectorMul(float *out, float *v, float t) {
  out[0] = v[0] * t;
  out[1] = v[1] * t;
  out[2] = v[2] * t;
  return out;
}

/* Adds vector `b` to vector `a` and stores result in `out`. Returns `out`. */
static inline float* rtVectorAdd(float *out, float *a, float *b) {
  out[0] = a[0] + b[0];
  out[1] = a[1] + b[1];
  out[2] = a[2] + b[2];
  return out;
}

/* Calculates cross product of vectors `a` and `b` and stores result in `out`.
 * Returns `out`. */
static inline float* rtVectorCrossp(float *out, float *a, float *b) {
  out[0] = a[1]*b[2] - a[2]*b[1];
  out[1] = a[2]*b[0] - a[0]*b[2];
  out[2] = a[0]*b[1] - a[1]*b[0];
  return out;
}

/* Calculates dot product of vectors `a` and `b`. */
static inline float rtVectorDotp(float *a, float *b) {
  return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

/* Calculates normalized ray vector pointing from `a` towards `b` and stores
 * result in `out`. Returns `out`. */
static inline float* rtVectorRay(float *out, float *a, float *b) {
  out[0] = b[0] - a[0];
  out[1] = b[1] - a[1];
  out[2] = b[2] - a[2];
  return rtVectorNorm(out);
}

/* Solves parametric ray equation out=o+dr for given `o`, `r` and `d` params
 * and stores result in `out`. Returns `out`. */
static inline float* rtVectorRaypoint(float *out, float *o, float *r, float d) {
  out[0] = o[0] + d*r[0];
  out[1] = o[1] + d*r[1];
  out[2] = o[2] + d*r[2];
  return out;
}

/* Calculate normalized vector representing reflected ray by solving equation:
 * Z=2N(N*L)-L. 

:param: out: result vector pointer
:param: n: surface's normal vector 
:param: l: normalized vector from light towards current intersection point */
static inline float* rtVectorRayReflected(float *out, float *n, float *l) {
  float n_dot_l = rtVectorDotp(n, l);
  out[0] = 2.0f * n[0] * n_dot_l - l[0];
  out[1] = 2.0f * n[1] * n_dot_l - l[1];
  out[2] = 2.0f * n[2] * n_dot_l - l[2];
  return rtVectorNorm(out);
}

/* Same as `vec_vector_ray_reflected` but requires dot product between `n` and
 * `l` to be calculated outside and to be passed as `n_dot_l`. */
static inline float* rtVectorRayReflected2(float *out, float *n, float *l, float n_dot_l) {
  out[0] = 2.0f * n[0] * n_dot_l - l[0];
  out[1] = 2.0f * n[1] * n_dot_l - l[1];
  out[2] = 2.0f * n[2] * n_dot_l - l[2];
  return rtVectorNorm(out);
}

/* Calculate normalized vector representing refracted ray. */
static inline float* rtVectorRayRefracted(float *out, float *n, float *l, float eta) {
  float n_dotp_l=rtVectorDotp(n, l);
  float f=eta*n_dotp_l - sqrt(1.0f - (eta*eta) * (1.0f - n_dotp_l*n_dotp_l));
  out[0] = f*n[0] - eta*l[0];
  out[1] = f*n[1] - eta*l[1];
  out[2] = f*n[2] - eta*l[2];
  return rtVectorNorm(out);
}

#endif

// vim: tabstop=2 shiftwidth=2 softtabstop=2
