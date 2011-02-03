#include "voxelize.h"
#include "vectormath.h"
#include "intersection.h"
#include "error.h"
#include "common.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>


/* Used to add triangle to given voxel, reallocating memory if needed. Returns
 * 1 on success or 0 on failure (that means memory allocation error). */
static int rtVoxelAddTriangle(RT_Voxel *v, RT_Triangle *t, int32_t bufsize) {
  /* Array has not been allocated yet - do this now. */
  if(!v->t) {
    v->t = malloc(bufsize*sizeof(RT_Triangle*));
    if(!v->t) {
      errno = E_MEMORY;
      return 0;
    }
    memset(v->t, 0, bufsize*sizeof(RT_Triangle*));
    v->p = bufsize;  // number of space left

  /* There is no more place to keep another triangle - reallocate memory by
   * `bufsize` step and make `bufsize` number of place. */
  } else if(v->p == 0) {
    // create new buffer
    RT_Triangle **tmp = malloc((v->nt+bufsize)*sizeof(RT_Triangle*));
    if(!tmp) {
      errno = E_MEMORY;
      return 0;
    }
    memset(tmp, 0, (v->nt+bufsize)*sizeof(RT_Triangle*));

    // copy triangle list to newly allocated buffer and destroy old one
    memcpy(tmp, v->t, v->nt*sizeof(RT_Triangle*));
    free(v->t);

    // update attributes
    v->t = tmp;
    v->p = bufsize;
  }

  /* At this point triangle list is ready to keep another triangle. Add given
   * triangle to list of triangles of given voxel then. */
  v->t[v->nt++] = t;
  v->p--;  // decrease number of space left (if it reaches 0 memory is reallocated)

  return 1;
}


/* Initializes parameters used to traverse through voxel grid array. */
static inline void rtUddTraverseInitialize(
    RT_Udd *self, RT_Scene *scene, 
    float *o, float *r, 
    int32_t i, int32_t j, int32_t k,
    float *dtx, float *tx, int32_t *di,
    float *dty, float *ty, int32_t *dj,
    float *dtz, float *tz, int32_t *dk) 
{
  // calculate voxel's planes
  float x1 = scene->dmin[0] + i*self->s[0];
  float x2 = x1 + self->s[0];
  float y1 = scene->dmin[1] + j*self->s[1];
  float y2 = y1 + self->s[1];
  float z1 = scene->dmin[2] + k*self->s[2];
  float z2 = z1 + self->s[2];
  
  // calculate dtx and tx
  if(r[0] == 0.0f) {
    *dtx = FLT_MAX;
    *tx = 0.0f;
  } else {
    float dtx1 = (x1-o[0]) / r[0];
    float dtx2 = (x2-o[0]) / r[0];
    *dtx = rtAbs(dtx2 - dtx1);
    *tx = dtx1<dtx2? dtx1: dtx2;
  }
  
  // calculate dty and ty
  if(r[1] == 0.0f) {
    *dty = FLT_MAX;
    *ty = 0.0f;
  } else {
    float dty1 = (y1-o[1]) / r[1]; 
    float dty2 = (y2-o[1]) / r[1];
    *dty = rtAbs(dty2 - dty1);
    *ty = dty1<dty2? dty1: dty2;
  }

  //calculate dtz and tz
  if(r[2] == 0.0f) {
    *dtz = FLT_MAX;
    *tz = 0.0f;
  } else {
    float dtz1 = (z1-o[2]) / r[2]; 
    float dtz2 = (z2-o[2]) / r[2];
    *dtz = rtAbs(dtz2 - dtz1);
    *tz = dtz1<dtz2? dtz1: dtz2;
  }
  
  // calculate di, dj and dk
  if(r[0] > 0.0f) {
    *di = 1;
  } else {
    *di = -1;
  }
  if(r[1] > 0.0f) {
    *dj = 1;
  } else {
    *dj = -1;
  }
  if(r[2] > 0.0f) {
    *dk = 1;
  } else {
    *dk = -1;
  }
}


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
RT_Udd* rtUddCreate(RT_Scene* scene) {
  int k;
  float ds[3], v, tmp;
  RT_Udd *res=NULL;

  // create result object
  res = malloc(sizeof(RT_Udd));
  if(!res) {
    errno = E_MEMORY;
    return NULL;
  }

  // calculate domain size
  for(k=0; k<3; k++) {
    scene->dmin[k] -= 0.001f; 
    scene->dmax[k] += 0.001f;
    ds[k] = scene->dmax[k] - scene->dmin[k] + 0.001;
  }
  RT_INFO("domain size: x=%.3f, y=%.3f, z=%.3f", ds[0], ds[1], ds[2]);
  RT_INFO("domain size min: x=%.3f, y=%.3f, z=%.3f", scene->dmin[0], scene->dmin[1], scene->dmin[2]);
  RT_INFO("domain size max: x=%.3f, y=%.3f, z=%.3f", scene->dmax[0], scene->dmax[1], scene->dmax[2]);
  
  // calculate number of voxels depending on current voxelization mode
  switch(scene->cfg.vmode) {
    /* Default voxelization mode. Number of voxels in this mode is as close to
     * number of triangles as possible. */
    case VOX_DEFAULT:
      v = pow(scene->nt/(ds[0]*ds[1]*ds[2]), 0.33333f);
      for(k=0; k<3; k++) {
        tmp = ceil(ds[k]*v);  // number of grid elements in k-direction
        res->nv[k] = tmp;
        res->s[k] = ds[k]/tmp;  // size of voxel in k-direction
      }
      break;

    /* In this mode number of voxels is calculated like in method above, but
     * can also be modified by i,j,k coeffs. If coeff is less than 1, number
     * of voxels will be less than default in direction specified by coeff. If
     * coeff is greater than 1, number of voxels will be greater than default
     * in direction specified by coeff. If all coeffs are set to 1 this mode
     * will give same results as previous one. */
    case VOX_MODIFIED_DEFAULT:
      for(k=0; k<3; k++) {
        if(scene->cfg.vcoeff[k] <= 0.0f) {
          RT_EERROR("none of voxelization coeffs can be <= 0 in VOX_MODIFIED_DEFAULT voxelization mode")
          free(res);
          errno = E_INVALID_PARAM_VALUE;
          return NULL;
        }
      }
      v = pow(scene->nt/(ds[0]*ds[1]*ds[2]), 0.33333f);
      for(k=0; k<3; k++) {
        tmp = ceil(ds[k]*v*scene->cfg.vcoeff[k]);  // number of grid elements in k-direction
        res->nv[k] = tmp;
        res->s[k] = ds[k]/tmp;  // size of voxel in k-direction
      }
      break;
    /* In this mode voxelization coeffs are simply used as number of voxels in
     * i,j,k directions, correspondingly. */
    case VOX_FIXED:
      for(k=0; k<3; k++) {
        if(scene->cfg.vcoeff[k] <= 0.0f) {
          RT_EERROR("none of voxelization coeffs can be <= 0 in VOX_MODIFIED_DEFAULT voxelization mode")
          free(res);
          errno = E_INVALID_PARAM_VALUE;
          return NULL;
        }
      }
      for(k=0; k<3; k++) {
        tmp = ceil(scene->cfg.vcoeff[k]);  // number of grid elements in k-direction
        res->nv[k] = tmp;
        res->s[k] = ds[k]/tmp;  // size of voxel in k-direction
      }
      break;
  }

  RT_INFO("number of voxels: i=%d, j=%d, k=%d", res->nv[0], res->nv[1], res->nv[2]);
  RT_INFO("total number of voxels: %d", res->nv[0]*res->nv[1]*res->nv[2]);
  RT_INFO("total number of triangles: %d", scene->nt);
  RT_INFO("total number of lights: %d", scene->nl);
  RT_INFO("size of single voxel: i=%.3f, j=%.3f, k=%.3f", res->s[0], res->s[1], res->s[2]);

  // create voxel grid array
  tmp = res->nv[0] * res->nv[1] * res->nv[2];
  res->v = malloc(tmp*sizeof(RT_Voxel));
  if(!res->v) {
    free(res);
    errno = E_MEMORY;
    return NULL;
  }
  memset(res->v, 0, tmp*sizeof(RT_Voxel));

  return res;
}
///////////////////////////////////////////////////////////////
void rtUddDestroy(RT_Udd **self) {
  RT_Udd *ptr = *self;
  int32_t k, s=ptr->nv[0]*ptr->nv[1]*ptr->nv[2];
  if(ptr->v) {
    for(k=0; k<s; k++) {
      if(ptr->v[k].t) {
        free(ptr->v[k].t);
      }
    }
    free(ptr->v);
  }
  free(ptr);
  *self = NULL;
}
///////////////////////////////////////////////////////////////
void rtUddVoxelize(RT_Udd *self, RT_Scene *scene) {
  const int32_t BUFSIZE = 10;  // number of space to add after each reallocation
  int32_t i, j, k;
  RT_Vertex4f p;
  RT_Voxel *vptr=NULL;
  RT_Triangle *t=scene->t, *maxt=(RT_Triangle*)(scene->t + scene->nt), **tmp;

  // make temporary storage for assigned triangles
  tmp = malloc(scene->nt*sizeof(RT_Triangle*));
  if(!tmp) {
    errno = E_MEMORY;
    return;
  }
  memset(tmp, 0, scene->nt*sizeof(RT_Triangle*));
  
  // iterate through array of triangles
  while(t < maxt) {
    //if(t->sid == 34) {t++; continue;}
    // calculate indices of voxels containing current triangle's vertices
    int32_t iidx[3], jidx[3], kidx[3];
    for(k=0; k<3; k++) {
      iidx[k] = (t->i[k] - scene->dmin[k]) / self->s[k];
      jidx[k] = (t->j[k] - scene->dmin[k]) / self->s[k];
      kidx[k] = (t->k[k] - scene->dmin[k]) / self->s[k];
    }
    
    // now calculate minimal and maximal indices of voxels that must be checked
    int32_t min[3], max[3];
    for(k=0; k<3; k++) {
      min[k] = MIN(iidx[k], jidx[k], kidx[k]);
      max[k] = MAX(iidx[k], jidx[k], kidx[k]);
    }

    // if minimal and maximal are equal, triangle is added to exactly one voxel
    if(min[0]==max[0] && min[1]==max[1] && min[2]==max[2]) {
      vptr = (RT_Voxel*)(self->v + rtVoxelArrayOffset(self, min[0], min[1], min[2]));
      rtVoxelAddTriangle(vptr, t, BUFSIZE);
      t++;
      continue;
    }

    // loop through grid array
    for(i=min[0]; i<=max[0]; i++) {
      for(j=min[1]; j<=max[1]; j++) {
        for(k=min[2]; k<=max[2]; k++) {
          // add triangle to current voxel
          vptr = (RT_Voxel*)(self->v + rtVoxelArrayOffset(self, i, j, k));
          rtVoxelAddTriangle(vptr, t, BUFSIZE);
          continue;

          /* Triangle is included in the voxel if at least one of following
           * alternatives is true:
           * 1) At least one of its vertices is inside voxel
           * 2) At least one of triangle segments intersects voxel
           * 3) Triangle plane intersects voxel and voxel is inside triangle */
          float x1 = scene->dmin[0] + i*self->s[0];
          float x2 = x1 + self->s[0];
          float y1 = scene->dmin[1] + j*self->s[1];
          float y2 = y1 + self->s[1];
          float z1 = scene->dmin[2] + k*self->s[2];
          float z2 = z1 + self->s[2];

          /* Calculate intersection points between triangle's plane and voxel's
           * edges (x and z planes). */
          float y11, y12, y21, y22;
          if(t->n[1] != 0.0f) {
            y11 = (-t->d - t->n[0]*x1 - t->n[2]*z1) / t->n[1];
            y12 = (-t->d - t->n[0]*x1 - t->n[2]*z2) / t->n[1];
            y21 = (-t->d - t->n[0]*x2 - t->n[2]*z1) / t->n[1];
            y22 = (-t->d - t->n[0]*x2 - t->n[2]*z2) / t->n[1];
          } else {
            y11 = y12 = y21 = y22 = FLT_MAX;
          }

          /* Now do the same, but for y and z planes. */
          float x11, x12, x21, x22;
          if(t->n[0] != 0.0f) {
            x11 = (-t->d - t->n[1]*y1 - t->n[2]*z1) / t->n[0];
            x12 = (-t->d - t->n[1]*y1 - t->n[2]*z2) / t->n[0];
            x21 = (-t->d - t->n[1]*y2 - t->n[2]*z1) / t->n[0];
            x22 = (-t->d - t->n[1]*y2 - t->n[2]*z2) / t->n[0];
          } else {
            x11 = x12 = x21 = x22 = FLT_MAX;
          }

          /* And once again - for x and y planes. */
          float z11, z12, z21, z22;
          if(t->n[2] != 0.0f) {
            z11 = (-t->d - t->n[0]*x1 - t->n[1]*y1) / t->n[2];
            z12 = (-t->d - t->n[0]*x1 - t->n[1]*y2) / t->n[2];
            z21 = (-t->d - t->n[0]*x2 - t->n[1]*y1) / t->n[2];
            z22 = (-t->d - t->n[0]*x2 - t->n[1]*y2) / t->n[2];
          } else {
            z11 = z12 = z21 = z22 = FLT_MAX;
          }
          
          /* Now if all found intersection points does not satisfy voxel's
           * bounds, plane does not intersect voxel. */
          if((x11 < x1 || x11 > x2) && (x12 < x1 || x12 > x2) && (x21 < x1 || x21 > x2) && (x22 < x1 || x22 > x2) &&
             (y11 < y1 || y11 > y2) && (y12 < y1 || y12 > y2) && (y21 < y1 || y21 > y2) && (y22 < y1 || y22 > y2) &&
             (z11 < z1 || z11 > z2) && (z12 < z1 || z12 > z2) && (z21 < z1 || z21 > z2) && (z22 < z1 || z22 > z2))
          {
            continue;
          }
          
          /* At this point we know, that triangle's plane intersects current
           * voxel. Now using points calculated in previous step check if
           * triangle really intersects current voxel. */
          int further_test = 0;
          rtVectorCreate(p, x1, y11, z1);
          if(!rtInt1TestPoint(t, p)) {
            rtVectorCreate(p, x1, y12, z2);
            if(!rtInt1TestPoint(t, p)) {
              rtVectorCreate(p, x2, y21, z1);
              if(!rtInt1TestPoint(t, p)) {
                rtVectorCreate(p, x2, y22, z2);
                if(!rtInt1TestPoint(t, p)) {
                  rtVectorCreate(p, x11, y1, z1);
                  if(!rtInt1TestPoint(t, p)) {
                    rtVectorCreate(p, x12, y1, z2);
                    if(!rtInt1TestPoint(t, p)) {
                      rtVectorCreate(p, x21, y2, z1);
                      if(!rtInt1TestPoint(t, p)) {
                        rtVectorCreate(p, x22, y2, z2);
                        if(!rtInt1TestPoint(t, p)) {
                          rtVectorCreate(p, x1, y1, z11);
                          if(!rtInt1TestPoint(t, p)) {
                            rtVectorCreate(p, x1, y2, z12);
                            if(!rtInt1TestPoint(t, p)) {
                              rtVectorCreate(p, x2, y1, z21);
                              if(!rtInt1TestPoint(t, p)) {
                                rtVectorCreate(p, x2, y2, z22);
                                if(!rtInt1TestPoint(t, p)) {
                                  further_test = 1;
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
          
          /* One more test - find intersection point between triangle edges and
           * voxel edges. */
          if(further_test) {
            if((t->i[0] < x1 || t->i[0] > x2 || t->i[1] < y1 || t->i[1] > y2 || t->i[2] < z1 || t->i[2] > z2) &&
               (t->j[0] < x1 || t->j[0] > x2 || t->j[1] < y1 || t->j[1] > y2 || t->j[2] < z1 || t->j[2] > z2) &&
               (t->k[0] < x1 || t->k[0] > x2 || t->k[1] < y1 || t->k[1] > y2 || t->k[2] < z1 || t->k[2] > z2))
            {
              RT_Vertex4f ij, ik, jk;

              rtVectorRay(ij, t->i, t->j);
              rtVectorRay(ik, t->i, t->k);
              rtVectorRay(jk, t->j, t->k);

              if(!rtUddCheckVoxelIntersection(self, scene, t->i, ij, i, j, k)) {
                if(!rtUddCheckVoxelIntersection(self, scene, t->i, ik, i, j, k)) {
                  if(!rtUddCheckVoxelIntersection(self, scene, t->j, jk, i, j, k)) {
                    continue;
                  }
                }
              }
            }
          }

          // add triangle to current voxel
          vptr = (RT_Voxel*)(self->v + rtVoxelArrayOffset(self, i, j, k));
          rtVoxelAddTriangle(vptr, t, BUFSIZE);
        }
      }
    }

    t++;
  }
}
///////////////////////////////////////////////////////////////
int rtUddFindStartupVoxel(
    RT_Udd *self, RT_Scene *scene, 
    float *o, float *r, 
    int32_t *i, int32_t *j, int32_t *k) 
{
  // check if we are already inside domain
  if(rtVertexGetVoxel(scene, self, o, i, j, k))
    return 1;

  // define some more variables
  RT_Vertex4f tmpv;
  float dmin1=FLT_MAX, dmin2=FLT_MAX;
  float d;
  int a;

  /* Calculate distance from ray origin to all walls by solving simplified
   * ray->plane equation. Get two minimal distances (because only two are
   * needed to test whether ray enters domain). */
  for(a=0; a<3; a++) {
    if(r[a] != 0.0f) {
      d = (scene->dmin[a] - o[a]) / r[a];
      if(d > 0.0f) {
        if(d < dmin1) {
          dmin2 = dmin1;
          dmin1 = d;
        } else if(d < dmin2) {
          dmin2 = d;
        }
      }
      d = (scene->dmax[a] - o[a]) / r[a];
      if(d > 0.0f) {
        if(d < dmin1) {
          dmin2 = dmin1;
          dmin1 = d;
        } else if(d < dmin2) {
          dmin2 = d;
        }
      }
    }
  }
  
  /* Calculate intersection point at first minimal distance and check whether
   * it belongs to domain. */
  rtVectorRaypoint(tmpv, o, r, dmin1);
  if(rtVertexGetVoxel(scene, self, tmpv, i, j, k))
    return 1;

  /* Calculate intersection point at second minimal distance - if this check
   * fails, ray is not entering domain. */
  rtVectorRaypoint(tmpv, o, r, dmin2);
  if(rtVertexGetVoxel(scene, self, tmpv, i, j, k))
    return 1;

  return 0;
}
///////////////////////////////////////////////////////////////
int rtUddCheckVoxelIntersection(
    RT_Udd *self, RT_Scene *scene,
    float *o, float *r,
    int32_t i_, int32_t j_, int32_t k_)
{
  RT_Vertex4f tmpv;
  float dmin1=FLT_MAX, dmin2=FLT_MAX;
  float dmin[3], dmax[3];
  float d;
  int a;
  int32_t i, j, k, ijk[3]={i_, j_, k_};

  /* Calculate voxel bounds. */
  for(a=0; a<3; a++) {
    dmin[a] = scene->dmin[a] + ijk[a]*self->s[a];
    dmax[a] = dmin[a] + self->s[a];
  }

  /* Calculate distance from ray origin to all walls by solving simplified
   * ray->plane equation. Get two minimal distances (because only two are
   * needed to test whether ray enters domain). */
  for(a=0; a<3; a++) {
    if(r[a] != 0.0f) {
      d = (dmin[a] - o[a]) / r[a];
      if(d > 0.0f) {
        if(d < dmin1) {
          dmin2 = dmin1;
          dmin1 = d;
        } else if(d < dmin2) {
          dmin2 = d;
        }
      }
      d = (dmax[a] - o[a]) / r[a];
      if(d > 0.0f) {
        if(d < dmin1) {
          dmin2 = dmin1;
          dmin1 = d;
        } else if(d < dmin2) {
          dmin2 = d;
        }
      }
    }
  }
  
  /* Calculate intersection point at first minimal distance and check whether
   * it belongs to domain. */
  rtVectorRaypoint(tmpv, o, r, dmin1);
  if(rtVertexGetVoxel(scene, self, tmpv, &i, &j, &k)) {
    if(i==i_ && j==j_ && k==k_)
      return 1;
  }

  /* Calculate intersection point at second minimal distance - if this check
   * fails, ray is not entering domain. */
  rtVectorRaypoint(tmpv, o, r, dmin2);
  if(rtVertexGetVoxel(scene, self, tmpv, &i, &j, &k)) {
    if(i==i_ && j==j_ && k==k_)
      return 1;
  }

  return 0;
}
///////////////////////////////////////////////////////////////
RT_Triangle* rtUddFindNearestTriangle(
  RT_Udd *self, RT_Scene *scene, 
  RT_Triangle *current,
  float *ipoint,
  float *dmin,
  float *o, float *r, 
  int32_t *i_, int32_t *j_, int32_t *k_,
  float *u, float *v)
{
  float dtx, dty, dtz, tx, ty, tz;
  float tx_n, ty_n, tz_n;
  float d;
  float utmp, vtmp;
  int32_t di, dj, dk;
  int32_t i=*i_, j=*j_, k=*k_;
  int32_t c, nidx=0;
  RT_Triangle *t, *nearest, *tmp;
  
  /* Initialize traversal algorithm. */
  rtUddTraverseInitialize(
      self, scene,
      o, r,
      i, j, k,
      &dtx, &tx, &di,
      &dty, &ty, &dj,
      &dtz, &tz, &dk
  );

  /* Traverse through grid array. */
  while(1) {
    // check intersections in current voxel
    RT_Voxel *voxel = (RT_Voxel*)(self->v + rtVoxelArrayOffset(self, i, j, k));
    if(voxel->nt > 0) {
      *dmin = MIN(tx+dtx, ty+dty, tz+dtz);
      nearest = NULL;
      for(c=0; c<voxel->nt; c++) {
        t = voxel->t[c]; 
        if(t->isint(t, o, r, &d, dmin, &utmp, &vtmp)) {
          if(t != current && d < *dmin) {
            *dmin = d;
            nearest = t;
            nidx = c;
            *u = utmp;
            *v = vtmp;
          }
        }
      }
      if(nearest) {
        //tmp = voxel->t[0];
        //voxel->t[0] = nearest;
        //voxel->t[nidx] = tmp;
        rtVectorRaypoint(ipoint, o, r, *dmin); //FIXME: move calculation of intersection point to intersection test function
        *i_=i; *j_=j; *k_=k;
        return nearest;
      }
    }

    // proceed to next voxel
    if ((tx_n=tx+dtx) < (ty_n=ty+dty)) {
      if (tx_n < (tz_n=tz+dtz)) { 
        i+=di; tx=tx_n;
      } else {
        k+=dk; tz=tz_n;
      }
    } else {
      if (ty_n < (tz_n=tz+dtz)) {
        j+=dj; ty=ty_n;
      } else {
        k+=dk; tz=tz_n;
      }
    }

    // termination check
    if(i<0 || i>=self->nv[0]) return NULL;
    if(j<0 || j>=self->nv[1]) return NULL;
    if(k<0 || k>=self->nv[2]) return NULL;
  }
}
///////////////////////////////////////////////////////////////
RT_Triangle* rtUddFindShadow(
  RT_Udd *self, RT_Scene *scene,
  RT_Triangle *current,
  float *a, RT_Light *l, int32_t lindex, float *ts)
{
  int32_t aidx[3], bidx[3];
  int32_t min[3], max[3];
  float tx, dtx, ty, dty, tz, dtz, u, v;
  float tx_n, ty_n, tz_n;
  float *b = l->p;
  int32_t di, dj, dk;
  int32_t i, j, k;
  int32_t c;
  float d, dmin=FLT_MAX, dmax;
  RT_Vertex4f r;
  RT_Triangle *t;
  
  // initialize ts
  *ts = 1.0f;

  // calculate normalized ray vector from vertex `a` to vertex `b`
  rtVectorRay(r, a, b);
  
  // check if light is beyond current surface (100% sure that light is not
  // visible from such surface if so)
  if(current->s->kt == 0.0f) {
    if(rtVectorDotp(r, current->n) <= 0.0f) {
      return current;
    }
  }
  
  // check if ray intersects cached object
  if(lindex >= 0) {
    RT_Triangle *cache = current->shadow_cache[lindex];
    if(cache != NULL) {
      if(cache->isint(cache, a, r, &d, &dmin, &u, &v)) {
        return cache;
      }
      current->shadow_cache[lindex] = NULL;
    }
  }

  // calculate distance between points
  dmax = rtVectorDistance(a, b);

  // find voxel for point `a`
  if(!rtVertexGetVoxel(scene, self, a, &aidx[0], &aidx[1], &aidx[2])) {
    RT_ERROR("rtUddFindShadow(): vertex `a` outside of domain: x=%.3f, y=%.3f, z=%.3f", a[0], a[1], a[2])
    return NULL;
  }

  // find voxel for point `b`
  if(!rtVertexGetVoxel(scene, self, b, &bidx[0], &bidx[1], &bidx[2])) {
    RT_ERROR("rtUddFindShadow(): vertex `b` outside of domain: x=%.3f, y=%.3f, z=%.3f", b[0], b[1], b[2])
    return NULL;
  }

  // calculate minimal and maximal voxel
  for(c=0; c<3; c++) {
    if(aidx[c] < bidx[c]) {
      min[c] = aidx[c];
      max[c] = bidx[c];
    } else {
      min[c] = bidx[c];
      max[c] = aidx[c];
    }
  }

  // initialize traversal grid
  i=aidx[0]; j=aidx[1]; k=aidx[2];
  rtUddTraverseInitialize(
      self, scene,
      a, r,
      i, j, k,
      &dtx, &tx, &di,
      &dty, &ty, &dj,
      &dtz, &tz, &dk
  );

  // traverse
  while(1) {
    // check intersections in current voxel
    RT_Voxel *voxel = (RT_Voxel*)(self->v + rtVoxelArrayOffset(self, i, j, k));
    if(voxel->nt > 0) {
      for(c=0; c<voxel->nt; c++) {
        t = voxel->t[c];
        if(t->isint(t, a, r, &d, &dmin, &u, &v)) {
          if(t != current) {
            if(t->s->kt > 0.0f) {  // found transparent or semi-transparent triangle
              *ts *= t->s->kt;
              continue;
            }
            if(d > 0.00001f && d < dmax) {
              if(lindex >= 0) {
                current->shadow_cache[lindex] = t;
              }
              return t;
            }
          }
        }
      }
    }

    // proceed to next voxel
    if ((tx_n=tx+dtx) < (ty_n=ty+dty)) {
      if (tx_n < (tz_n=tz+dtz)) { 
        i+=di; tx=tx_n;
      } else {
        k+=dk; tz=tz_n;
      }
    } else {
      if (ty_n < (tz_n=tz+dtz)) {
        j+=dj; ty=ty_n;
      } else {
        k+=dk; tz=tz_n;
      }
    }

    // termination check
    if(i<min[0] || i>max[0]) return NULL;
    if(j<min[1] || j>max[1]) return NULL;
    if(k<min[2] || k>max[2]) return NULL;
  }
}

// vim: tabstop=2 shiftwidth=2 softtabstop=2
