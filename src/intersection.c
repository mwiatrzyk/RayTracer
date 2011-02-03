#include "intersection.h"
#include "vectormath.h"
#include "common.h"

#define EPSILON 0.000001f


/* Calculates coefficients of triangle's bounding lines after projection onto
 * 2D plane. */
static void rtInt1CalcLineCoeffs(float *a, float *b, float *c, 
                                 int xi, int yi, 
                                 float *A, float *B, float *C) 
{
  float x1=a[xi], x2=b[xi], x3=c[xi];
  float y1=a[yi], y2=b[yi], y3=c[yi];
  
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
static int rtInt1CanProject(RT_Triangle *t, int xi, int yi) {
  float x1=t->i[xi], x2=t->j[xi], x3=t->k[xi];
  float y1=t->i[yi], y2=t->j[yi], y3=t->k[yi];
  float p1, q1, p2, q2, l1, l2;
  
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
int rtInt0Test(RT_Triangle *t, float *o, float *r, float *d, float *dmin, float *u, float *v) {
  RT_Vertex4f pvec, tvec, qvec;
  float det, inv_det;

  rtVectorCrossp(pvec, r, t->ik);
  det = rtVectorDotp(t->ij, pvec);
  if(det > -EPSILON && det < EPSILON) {
    return 0;
  }

  inv_det = 1.0f / det;
  rtVectorMake(tvec, t->i, o);
  *u = rtVectorDotp(tvec, pvec) * inv_det;
  if(*u < 0.0f || *u > 1.0f) {
    return 0;
  }

  rtVectorCrossp(qvec, tvec, t->ij);
  *v = rtVectorDotp(r, qvec) * inv_det;
  if(*v < 0.0f || *u + *v > 1.0f) {
    return 0;
  }

  *d = rtVectorDotp(t->ik, qvec) * inv_det;
  if(*d < 0.0f)
    return 0;

  return 1;
}
///////////////////////////////////////////////////////////////
int rtInt1Test(RT_Triangle *t, float *o, float *r, float *d, float *dmin, float *u, float *v) {
  float rdn = rtVectorDotp(r, t->n);
  if(rdn == 0.0f)
    return 0;

  RT_Int1Coeffs *cf=&t->ic.i1;

  *d = -(rtVectorDotp(o, t->n) + t->d) / rdn;
  if(*d < 0.0f || *d > *dmin)
    return 0;

  float x = o[cf->xi] + (*d)*r[cf->xi];
  float y = o[cf->yi] + (*d)*r[cf->yi];

  if(x < cf->minx) return 0;
  if(x > cf->maxx) return 0;
  if(y < cf->miny) return 0;
  if(y > cf->maxy) return 0;

  //#ifndef __SSE__
  if(cf->A[0]*x + cf->B[0]*y < -cf->C[0]) return 0;
  if(cf->A[1]*x + cf->B[1]*y < -cf->C[1]) return 0;
  if(cf->A[2]*x + cf->B[2]*y < -cf->C[2]) return 0;
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
int rtInt1TestPoint(RT_Triangle *t, float *p) {
  RT_Int1Coeffs *cf=&t->ic.i1;

  float x = p[cf->xi];
  float y = p[cf->yi];

  if(cf->A[0]*x + cf->B[0]*y < -cf->C[0]) return 0;
  if(cf->A[1]*x + cf->B[1]*y < -cf->C[1]) return 0;
  if(cf->A[2]*x + cf->B[2]*y < -cf->C[2]) return 0;

  return 1;
}
///////////////////////////////////////////////////////////////
void rtInt1CoeffsPrecalc(RT_Triangle *t) {
  RT_Int1Coeffs *cf=&t->ic.i1;

  // project triangle onto coordinate system (one of XOY, XOZ, ZOY) that
  // won't cause reduction of triangle to segment
  if(rtInt1CanProject(t, 0, 1)) { // XOY
    cf->xi = 0;
    cf->yi = 1;
  } else if(rtInt1CanProject(t, 0, 2)) {  // XOZ
    cf->xi = 0; 
    cf->yi = 2;
  } else {  // ZOY
    cf->xi = 2;
    cf->yi = 1;
  }

  // calculate bounding box
  cf->minx = MIN(t->i[cf->xi], t->j[cf->xi], t->k[cf->xi]);
  cf->miny = MIN(t->i[cf->yi], t->j[cf->yi], t->k[cf->yi]);
  cf->maxx = MAX(t->i[cf->xi], t->j[cf->xi], t->k[cf->xi]);
  cf->maxy = MAX(t->i[cf->yi], t->j[cf->yi], t->k[cf->yi]);

  // calculate segment coefficients for each of 3 triangle's segments
  rtInt1CalcLineCoeffs(t->i, t->j, t->k, cf->xi, cf->yi, &cf->A[0], &cf->B[0], &cf->C[0]);
  rtInt1CalcLineCoeffs(t->j, t->k, t->i, cf->xi, cf->yi, &cf->A[1], &cf->B[1], &cf->C[1]);
  rtInt1CalcLineCoeffs(t->i, t->k, t->j, cf->xi, cf->yi, &cf->A[2], &cf->B[2], &cf->C[2]);
  
  // assign intersection test function
  t->isint = &rtInt0Test;
}

// vim: tabstop=2 shiftwidth=2 softtabstop=2
