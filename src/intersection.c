#include "intersection.h"
#include "vectormath.h"


///////////////////////////////////////////////////////////////
void rtIntCoeffsPrecalc(RT_Triangle *t) {
  // calculate d coefficient of plane equation
  t->d = -rtVectorDotp(t->i, t->n);

  // project triangle onto coordinate system (one of XOY, XOZ, ZOY) that
  // won't cause reduction of triangle to segment
  if(is_projection_possible(t, PP_XOY)) {
    t->pplane = PP_XOY;
    t->minx = MIN(t->i[0], t->j[0], t->k[0]);
    t->miny = MIN(t->i[1], t->j[1], t->k[1]);
    t->maxx = MAX(t->i[0], t->j[0], t->k[0]);
    t->maxy = MAX(t->i[1], t->j[1], t->k[1]);
  } else if(is_projection_possible(t, PP_XOZ)) {
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
  calc_line_coefficients(t->i, t->j, t->k, t->pplane, &t->A[0], &t->B[0], &t->C[0]);
  calc_line_coefficients(t->j, t->k, t->i, t->pplane, &t->A[1], &t->B[1], &t->C[1]);
  calc_line_coefficients(t->i, t->k, t->j, t->pplane, &t->A[2], &t->B[2], &t->C[2]);
}

// vim: tabstop=2 shiftwidth=2 softtabstop=2
