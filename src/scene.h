// vim: tabstop=2 shiftwidth=2 softtabstop=2
#ifndef __SCENE_H
#define __SCENE_H

#include "types.h"
#include "imagelib.h"
#include <stdio.h>

//// TYPES ////////////////////////////////////////////////////

typedef float RT_Vertex4f[4];


//// ENUMS ////////////////////////////////////////////////////

typedef enum _RT_ProjectionPlane {
  PP_XOY,
  PP_XOZ,
  PP_ZOY
} RT_ProjectionPlane;


//// STRUCTURES ///////////////////////////////////////////////

/* Definition of surface properties. */
typedef struct _RT_Surface {
  IML_Color color;      // surface RGB color
  float kd, ks, g, ka;  // kd - diffusion factor, ks - specular factor, g - glitter factor, ka - ambient factor
  float kt, eta, kr;    // kt - refraction (transparency) factor, eta - refraction index, kr -reflection factor
} RT_Surface;


/* Definition of single triangle. */
typedef struct _RT_Triangle {
  RT_Vertex4f i, j, k;          // triangle's vertices
  RT_Surface *s;                // pointer to surface properties of this triangle
  /* helpers */
  int32_t sid;                  // surface index (used only to assign `s` pointer while loading surface description)
  RT_Vertex4f n;                // normal vector
  RT_Vertex4f ij, ik;           // vectors: i to j, i to k
#if INT_ALG == 2
  float d;                      // d coefficient of triangle's plane equation: (i dotp n) + d = 0
  RT_ProjectionPlane pplane;   // decides which plane is used while performing 3D -> 2D projection of triangle
  float A[4], B[4], C[4];       // line coefficients
  float minx, maxx, miny, maxy; // bounding box coefficients
#endif
} RT_Triangle;


/* Definition of single light. */
typedef struct _RT_Light {
  RT_Vertex4f p;    // light position
  float flux;       // total flux
  IML_Color color;  // RGB color
} RT_Light;


/* Definition of scene. This is a container that keeps entire scene data in one
 * place. */
typedef struct _RT_Scene {
  int32_t nt;     // number of triangles in scene
  int32_t nl;     // number of lights
  int32_t ns;     // number of surfaces
  RT_Triangle *t; // array of triangles
  RT_Light *l;    // array of lights
  RT_Surface *s;  // array of surfaces
} RT_Scene;


/* Definition of camera. */
typedef struct _RT_Camera {
  RT_Vertex4f ob;           // observer location
  RT_Vertex4f ul, bl, ur;   // screen coords: ul - upper left, bl - bottom left, ur - upper right
  int32_t sw, sh;           // screen resolution: sw - width, sh - height
} RT_Camera;


//// INLINE FUNCTIONS /////////////////////////////////////////

/* Sets lights array in given scene. 

:param: self: pointer to scene object
:param: l: array of lights
:param: nl: number of items in array of lights */
static inline RT_Scene* rtSceneSetLights(RT_Scene* self, RT_Light* l, uint32_t nl) {
  self->nl = nl;
  self->l = l;
  return self;
}


//// FUNCTIONS ////////////////////////////////////////////////

/* Loads scene geometry description. 

:param: filename: path to geometry file */
RT_Scene* rtSceneLoad(const char *filename);

/* Sets surfaces array in given scene and applies surface pointers to all
 * triangles within that scene. 

:param: self: pointer to scene object
:param: s: array of surfaces
:param: ns: number of items in array of surfaces */
RT_Scene* rtSceneSetSurfaces(RT_Scene* self, RT_Surface* s, uint32_t ns);

/* Releases memory occupied by given RT_Scene object.

:param: self: pointer to RT_Scene object */
void rtSceneDestroy(RT_Scene **self);

/* Loads lights data from given file and returns array of lights.

:param: filename: path to lights file
:param: n: pointer to variable that will hold number of returned array's
  items */
RT_Light* rtLightLoad(const char *filename, uint32_t *n);

/* Loads surface description from given file and returns array of surfaces.

:param: filename: path to surface file
:param: n: pointer to variable that will hold number of returned array's
  items */
RT_Surface* rtSurfaceLoad(const char *filename, uint32_t *n);

/* Loads camera configuration from given file. 

:param: filename: path to camera file */
RT_Camera* rtCameraLoad(const char *filename);

/* Releases memory occupied by given camera object. 

:param: self: pointer to RT_Camera object */
void rtCameraDestroy(RT_Camera **self);

#endif
