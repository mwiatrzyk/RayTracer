/*
  Module that performs uniform domain division of scene.
*/
#ifndef __DOMDIVISION_H
#define __DOMDIVISION_H

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
  float si, sj, sk;     // size of single voxel (x, y, z)
  int32_t ni, nj, nk;   // size of voxel grid in x, y and z directions
  int32_t nv;           // number of voxels in `v` array
  RT_Voxel *v;          // array of voxels mapped from 3D array to 1D array
} RT_Udd;


//// FUNCTIONS ////////////////////////////////////////////////

/* Initializes Uniform Domain Division structure. */
RT_Udd* rtUddCreate(RT_Scene* scene);

#endif

// vim: tabstop=2 shiftwidth=2 softtabstop=2
