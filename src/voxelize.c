#include "voxelize.h"
#include "vectormath.h"
#include "error.h"
#include "common.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>


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
    ds[k] = scene->dmax[k] - scene->dmin[k];
  }
  RT_DEBUG("domain size: x=%.3f, y=%.3f, z=%.3f", ds[0], ds[1], ds[2]);
  
  // calculate grid size and size of single element of grid
  v = pow(scene->nt/(ds[0]*ds[1]*ds[2]), 0.33333f);
  for(k=0; k<3; k++) {
    tmp = ceil(ds[k]*v);  // number of grid elements in k-direction
    res->nv[k] = tmp;
    res->s[k] = ds[k]/tmp;  // size of voxel in k-direction
  }
  RT_DEBUG("number of voxels: i=%d, j=%d, k=%d", res->nv[0], res->nv[1], res->nv[2]);
  RT_DEBUG("total number of voxels: %d", res->nv[0]*res->nv[1]*res->nv[2]);
  RT_DEBUG("total number of triangles: %d", scene->nt);
  RT_DEBUG("size of single voxel: i=%.3f, j=%.3f, k=%.3f", res->s[0], res->s[1], res->s[2]);

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
  if(ptr->v) {
    free(ptr->v);
  }
  free(ptr);
  *self = NULL;
}
///////////////////////////////////////////////////////////////
void rtUddVoxelize(RT_Udd *self, RT_Scene *scene) {
  int32_t i, j, k;
  int32_t ni=30, nj=30, nk=30;//self->nv[0], nj=self->nv[1], nk=self->nv[2];
  int32_t itmp=0;
  float s1, s2;
  RT_Triangle *t=scene->t, **tmp;

  // make temporary storage for assigned triangles
  tmp = malloc(scene->nt*sizeof(RT_Triangle*));
  if(!tmp) {
    errno = E_MEMORY;
    return;
  }
  memset(tmp, 0, scene->nt*sizeof(RT_Triangle*));

  // loop through grid array
  for(i=0; i<ni; i++) {
    for(j=0; j<nj; j++) {
      for(k=0; k<nk; k++) {

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

        /* Vertices of current voxel can now be calculated as follows:
         * 1) Bottom vertices:
         *    (x1,y1,z1) - close left
         *    (x2,y1,z1) - close right
         *    (x1,y1,z2) - far left
         *    (x2,y1,z2) - far right 
         * 2) Upper vertices:
         *    Like above, but with y2 instead of y1. */
        RT_Vertex4f bcl={x1, y1, z1, 0.0f}, bcr={x2, y1, z1, 0.0f};
        RT_Vertex4f bfl={x1, y1, x2, 0.0f}, bfr={x2, y1, z2, 0.0f};
        RT_Vertex4f ucl={x1, y2, z1, 0.0f}, ucr={x2, y2, z1, 0.0f};
        RT_Vertex4f ufl={x1, y2, x2, 0.0f}, ufr={x2, y2, z2, 0.0f};
        
        // initialize buffer indexer
        itmp = 0;

        // loop through array of triangles to assign triangles for current voxel
        RT_Triangle *t=scene->t, *maxt=(RT_Triangle*)(scene->t + scene->nt);
        while(t < maxt) {
          /* Apply voxel's corner nodes to triangle's plane equation and check
           * signs of result. If at least one corner have different sign that
           * any other then triangle's plane intersects voxel. */
          s1 = rtVectorDotp(t->n, bcl) + t->d;
          s2 = rtVectorDotp(t->n, bcr) + t->d;
          if(s1*s2 > 0.0f) {  // if sign for both vertices is the same
            s2 = rtVectorDotp(t->n, bfl) + t->d;
            if(s1*s2 > 0.0f) {
              s2 = rtVectorDotp(t->n, bfr) + t->d;
              if(s1*s2 > 0.0f) {
                s2 = rtVectorDotp(t->n, ucl) + t->d;
                if(s1*s2 > 0.0f) {
                  s2 = rtVectorDotp(t->n, ucr) + t->d;
                  if(s1*s2 > 0.0f) {
                    s2 = rtVectorDotp(t->n, ufl) + t->d;
                    if(s1*s2 > 0.0f) {
                      s2 = rtVectorDotp(t->n, ufr) + t->d;
                      if(s1*s2 > 0.0f) {
                        /* If we are here, all corners of voxel are on the same
                         * side of triangle's plane. That means that current
                         * triangle is not added to vertex. */
                        t++;
                        continue;
                      }
                    }
                  }
                }
              }
            }
          }
          
          // add triangle pointer to buffer
          tmp[itmp++] = t;

          // proceed to next triangle
          t++;
        }
      }
    }
  }
}

// vim: tabstop=2 shiftwidth=2 softtabstop=2
