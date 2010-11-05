#include "intersection.h"
#include "vectormath.h"
#include "common.h"

#define EPSILON 0.000001f


/* Calculates coefficients of triangle's bounding lines after projection onto
 * 2D plane. */
static void rtInt1CalcLineCoeffs(float *a, float *b, float *c, 
    RT_ProjectionPlane pplane, 
    float *A, float *B, float *C) {
  float x1, x2, y1, y2, x3, y3;
  switch(pplane) {
    case PP_XOY:
      x1 = a[0]; x2 = b[0]; x3 = c[0];
      y1 = a[1]; y2 = b[1]; y3 = c[1];
      break;
    case PP_XOZ:
      x1 = a[0]; x2 = b[0]; x3 = c[0];
      y1 = a[2]; y2 = b[2]; y3 = c[2];
      break;
    default:  //PP_ZOY
      x1 = a[2]; x2 = b[2]; x3 = c[2];
      y1 = a[1]; y2 = b[1]; y3 = c[1];
      break;
  }
  *A = y2 - y1;
  *B = x1 - x2;    // equals to: -(x2 - x1)
  *C = -((*A)*x1 + (*B)*y1);  // Ax+By+C=0 => C=-(Ax+By)
  if((*A)*x3 + (*B)*y3 + *C < 0.0f) {
    *A = -(*A);
    *B = -(*B);
    *C = -(*C);
  }
}


/* Tests if given triangle can be projected onto given plane without being
 * reduced to segment. This is helper funtion, executed once for each triangle
 * in preprocessing step. */
static int rtInt1CanProject(RT_Triangle *t, RT_ProjectionPlane pplane) {
  float x1, x2, x3, y1, y2, y3, p1, q1, p2, q2, l1, l2;
  switch(pplane) {
    case PP_XOY:
      x1 = t->i[0]; x2 = t->j[0]; x3 = t->k[0];
      y1 = t->i[1]; y2 = t->j[1]; y3 = t->k[1];
      break;
    case PP_XOZ:
      x1 = t->i[0]; x2 = t->j[0]; x3 = t->k[0];
      y1 = t->i[2]; y2 = t->j[2]; y3 = t->k[2];
      break;
    default:  //PP_ZOY
      x1 = t->i[2]; x2 = t->j[2]; x3 = t->k[2];
      y1 = t->i[1]; y2 = t->j[1]; y3 = t->k[1];
      break;
  }
  p1 = x2 - x1; q1 = y2 - y1;  // build vector from (x1, y1) to (x2, y2)
  p2 = x3 - x1; q2 = y3 - y1;  // build vector from (x1, y1) to (x2, y2)
  if((p1 == 0.0f && q1 == 0.0f) || (p2 == 0.0f && q2 == 0.0f))  // [0,0] vectors - projection not possible
    return 0;
  l1 = sqrt(p1*p1 + q1*q1);   
  l2 = sqrt(p2*p2 + q2*q2);
  p1 /= l1; q1 /= l1;  // normalize vectors
  p2 /= l2; q2 /= l2;
  if((p1 == p2 && q1 == q2) || (p1+p2 == 0.0f && q1+q2 == 0.0f)) {
    return 0;
  } else {
    return 1;
  }
}


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
int rtInt0Test(RT_Triangle *t, float *o, float *r, float *d, float *dmin) {
  RT_Vertex4f pvec, tvec, qvec;
  float det, inv_det, u, v;

  rtVectorCrossp(pvec, r, t->ik);
  det = rtVectorDotp(t->ij, pvec);
  if(det > -EPSILON && det < EPSILON) {
    return 0;
  }

  inv_det = 1.0f / det;
  rtVectorMake(tvec, t->i, o);
  u = rtVectorDotp(tvec, pvec) * inv_det;
  if(u < 0.0f || u > 1.0f) {
    return 0;
  }

  rtVectorCrossp(qvec, tvec, t->ij);
  v = rtVectorDotp(r, qvec) * inv_det;
  if(v < 0.0f || u + v > 1.0f) {
    return 0;
  }

  *d = rtVectorDotp(t->ik, qvec) * inv_det;
  if(*d < 0.0f)
    return 0;

  return 1;
}


///////////////////////////////////////////////////////////////
int rtInt1Test(RT_Triangle *t, float *o, float *r, float *d, float *dmin) {
  float rdn = rtVectorDotp(r, t->n);
  if(rdn == 0.0f)
    return 0;
  *d = -(rtVectorDotp(o, t->n) + t->d) / rdn;
  if(*d < 0.0f || *d > *dmin)
    return 0;
  float x, y;
  RT_ProjectionPlane pplane = t->pplane;
  if(pplane == PP_XOY) {
    x = o[0] + (*d)*r[0];
    y = o[1] + (*d)*r[1];
  } else if(pplane == PP_XOZ) {
    x = o[0] + (*d)*r[0];
    y = o[2] + (*d)*r[2];
  } else {  // PP_ZOY
    x = o[2] + (*d)*r[2];
    y = o[1] + (*d)*r[1];
  }
  if(x < t->minx) return 0;
  if(x > t->maxx) return 0;
  if(y < t->miny) return 0;
  if(y > t->maxy) return 0;
  //#ifndef __SSE__
  if(t->A[0]*x + t->B[0]*y < -t->C[0]) return 0;
  if(t->A[1]*x + t->B[1]*y < -t->C[1]) return 0;
  if(t->A[2]*x + t->B[2]*y < -t->C[2]) return 0;
  /*#else
    uint64_t start=rdtsc();
    __ALIGN_16 float xx[4]={x, x, x, x}, yy[4]={y, y, y, y}, res[4];
    __asm__ volatile(
    "movaps (%1), %%xmm0 \n\t"
    "movaps (%2), %%xmm1 \n\t"
    "mulps %%xmm1, %%xmm0 \n\t"  // xmm0 = t->A * xx
    "movaps (%3), %%xmm1 \n\t"
    "movaps (%4), %%xmm2 \n\t"
    "mulps %%xmm1, %%xmm2 \n\t"  // xmm2 = t->B * yy
    "addps %%xmm2, %%xmm0 \n\t"  // xmm0 = xmm0 + xmm2 = t->A*xx + t->B*yy
    "movaps %%xmm0, %0 \n\t"
    : "=m"(res) 
    : "r"(t->A), "r"(xx), "r"(t->B), "r"(yy)
    : "%xmm0", "%xmm1", "%xmm2"
    );
  //if(res[1] != t->A[1]*x + t->B[1]*y)
  //printf("%f %f %f\n", x, res[1], t->A[1]*x + t->B[1]*y);
  //printf("%f %f\n", res[1], t->A[1]*x + t->B[1]*y);
  //printf("%f %f\n\n", res[2], t->A[2]*x + t->B[2]*y);
  if(res[0] < -t->C[0]) return 0;
  if(res[1] < -t->C[1]) return 0;
  if(res[2] < -t->C[2]) return 0;
  printf("%lld\n", rdtsc()-start);
#endif*/
  return 1;
}


///////////////////////////////////////////////////////////////
void rtIntCoeffsPrecalc(RT_Triangle *t) {
  // calculate d coefficient of plane equation
  t->d = -rtVectorDotp(t->i, t->n);

  // project triangle onto coordinate system (one of XOY, XOZ, ZOY) that
  // won't cause reduction of triangle to segment
  if(rtInt1CanProject(t, PP_XOY)) {
    t->pplane = PP_XOY;
    t->minx = MIN(t->i[0], t->j[0], t->k[0]);
    t->miny = MIN(t->i[1], t->j[1], t->k[1]);
    t->maxx = MAX(t->i[0], t->j[0], t->k[0]);
    t->maxy = MAX(t->i[1], t->j[1], t->k[1]);
  } else if(rtInt1CanProject(t, PP_XOZ)) {
    t->pplane = PP_XOZ;
    t->minx = MIN(t->i[0], t->j[0], t->k[0]);
    t->miny = MIN(t->i[2], t->j[2], t->k[2]);
    t->maxx = MAX(t->i[0], t->j[0], t->k[0]);
    t->maxy = MAX(t->i[2], t->j[2], t->k[2]);
  } else {
    t->pplane = PP_ZOY;
    t->minx = MIN(t->i[2], t->j[2], t->k[2]);
    t->miny = MIN(t->i[1], t->j[1], t->k[1]);
    t->maxx = MAX(t->i[2], t->j[2], t->k[2]);
    t->maxy = MAX(t->i[1], t->j[1], t->k[1]);
  }
  rtInt1CalcLineCoeffs(t->i, t->j, t->k, t->pplane, &t->A[0], &t->B[0], &t->C[0]);
  rtInt1CalcLineCoeffs(t->j, t->k, t->i, t->pplane, &t->A[1], &t->B[1], &t->C[1]);
  rtInt1CalcLineCoeffs(t->i, t->k, t->j, t->pplane, &t->A[2], &t->B[2], &t->C[2]);

  // decide which algorithm to use
  t->isint = &rtInt1Test;
}

// vim: tabstop=2 shiftwidth=2 softtabstop=2
