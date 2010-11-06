/*
  Module that performs uniform domain division of scene.
*/
#ifndef __VOXELIZE_H
#define __VOXELIZE_H

#include "scene.h"


//// STRUCTURES ///////////////////////////////////////////////

/* Structure that represents single voxel. */
typedef struct _RT_Voxel {
  int32_t nt;       // number of triangles in this voxel
  RT_Triangle **t;  // array of triangle pointers
} RT_Voxel;

/* Structure that groups all voxels in one place. "UDD" stands for "Uniform
 * Domain Division". */
typedef struct _RT_Udd {
  float s[3];     // size of single voxel (x, y, z)
  int32_t nv[3];  // voxel grid size (nv[0]*nv[1]*nv[2] is number of items in `v` array)
  RT_Voxel *v;    // array of voxels mapped from 3D array to 1D array
} RT_Udd;


//// INLINE FUNCTIONS /////////////////////////////////////////

/* Calculates mapping from 3D coords into 1D array offset value. Used to get 1D
 * index of voxel at position (i,j,k).

:param: s: pointer to RT_Udd object
:param: i, j, k: voxel coordinates */
static inline int32_t rtVoxelArrayOffset(const RT_Udd *s, int32_t i, int32_t j, int32_t k) {
  return (i*s->nv[1] + j)*s->nv[2] + k;
}


//// FUNCTIONS ////////////////////////////////////////////////

/* Initializes Uniform Domain Division structure. */
RT_Udd* rtUddCreate(RT_Scene *scene);

/* Releases memory occupied by RT_Udd object. */
void rtUddDestroy(RT_Udd **self);

/* Performs scene voxelization (fills voxels with triangles). */
void rtUddVoxelize(RT_Udd *self, RT_Scene *scene);

#endif

// vim: tabstop=2 shiftwidth=2 softtabstop=2
