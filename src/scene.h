#ifndef __SCENE_H
#define __SCENE_H

#include "types.h"
#include "bitmap.h"
#include <stdio.h>

//// TYPES ////////////////////////////////////////////////////

typedef float RT_Vertex4f[4];
typedef float RT_Vertex2f[2];

typedef enum _RT_VoxelizationMode {
  VOX_DEFAULT,           // calculating number of voxels in default way
  VOX_MODIFIED_DEFAULT,  // calculating number of voxels in default way, but number of voxels can be modified by 3 constants
  VOX_FIXED              // fixed number of voxels in each direction (3 constants)
} RT_VoxelizationMode;


//// INTERSECTION TEST COEFFS STRUCTURES //////////////////////

typedef struct _RT_Int1Coeffs {
  int xi, yi;                   // indices of vector coords to use after projection
  float A[4], B[4], C[4];       // line coefficients
  float minx, maxx, miny, maxy; // bounding box coefficients
} RT_Int1Coeffs;


//// STRUCTURES ///////////////////////////////////////////////

/* Definition of surface properties. */
typedef struct _RT_Surface {
  RT_Color color;      // surface RGB color
  float kd, ks, g, ka;  // kd - diffusion factor, ks - specular factor, g - glitter factor, ka - ambient factor
  float kt, eta, kr;    // kt - refraction (transparency) factor, eta - refraction index, kr -reflection factor
} RT_Surface;


/* Definition of single triangle. */
typedef struct _RT_Triangle {
  RT_Vertex4f i, j, k;          // triangle's vertices
  RT_Vertex2f ti, tj, tk;       // texture coords
  RT_Bitmap* texture;           // pinter to texture image
  RT_Surface *s;                // pointer to surface properties of this triangle
  int (*isint)(struct _RT_Triangle*, float*, float*, float*, float*, float*, float*);  // pointer to intersection test function dedicated for this triangle
  /* helpers */
  int32_t sid;                  // surface index (used only to assign `s` pointer while loading surface description)
  RT_Vertex4f n;                // normal vector
  RT_Vertex4f ij, ik;           // vectors: i to j, i to k
  float d;                      // d parameter of plane equation: nx*x + ny*y + nz*z + d = 0
  struct _RT_Triangle **shadow_cache;  // array of previous found triangles at shadow test step (length matches number of lights)
  union {
    RT_Int1Coeffs i1;
  } ic;
} RT_Triangle;


/* Definition of single point light. */
typedef struct _RT_Light {
  RT_Vertex4f p;    // light position
  float flux;       // total flux
  RT_Color color;  // RGB color
} RT_Light;


/* Definition of single planar light. */
typedef struct _RT_PlanarLight {
  float flux;       // total flux
  RT_Color color;   // RGB light color
  RT_Vertex4f a, b, c;  // coordinates of light
  RT_Vertex4f ab, ac;  // a->b and a->c vectors
  RT_Vertex4f n;  // normal vector (direction of planar light)
} RT_PlanarLight;


/* Rendering process configuration. */
typedef struct _RT_SceneConfig {
  float epsilon;  // epislon parameter for simplified shadow calculation (0 for normal processing)
  float gamma;  // gamma correction parameter of resulting image
  float distmod;   // distance modifier used in light calculation
  RT_VoxelizationMode vmode;   // voxelization mode
  float vcoeff[3];   // voxelization coefficients (meaning depend on mode)
} RT_SceneConfig;


/* Definition of scene. This is a container that keeps entire scene data in one
 * place. */
typedef struct _RT_Scene {
  /* Rendering process configuration. */
  RT_SceneConfig cfg;
  /* Scene description variables. */
  float dmin[3];  // minimal values of x, y and z of all scene's triangles
  float dmax[3];  // like above, but maximal
  int32_t nt;     // number of triangles in scene
  int32_t nl;     // number of lights
  int32_t npl;    // number of planar lights
  int32_t ns;     // number of surfaces
  float *lbuf;    // luminance buffer
  float *tc;
  float *lc;
  RT_Triangle *t; // array of triangles
  RT_Light *l;    // array of lights
  RT_PlanarLight *pl;  // array of planar lights
  RT_Surface *s;  // array of surfaces
} RT_Scene;


/* Definition of camera. */
typedef struct _RT_Camera {
  RT_Vertex4f ob;           // observer location
  RT_Vertex4f ul, bl, ur;   // screen coords: ul - upper left, bl - bottom left, ur - upper right
  int32_t sw, sh;           // screen resolution: sw - width, sh - height
} RT_Camera;


//// FUNCTIONS ////////////////////////////////////////////////

/* Loads scene geometry description. 

:param: filename: path to geometry file */
RT_Scene* rtSceneLoad(const char *filename);

/* Loads scene rendering configuration for given scene from given file. 

:param: self: pointer to RT_Scene object
:param: filename: path to config file for scene */
RT_Scene* rtSceneConfigureRenderer(RT_Scene* self, const char *filename);

/* Sets surfaces array in given scene and applies surface pointers to all
 * triangles within that scene. 

:param: self: pointer to scene object
:param: s: array of surfaces
:param: ns: number of items in array of surfaces */
RT_Scene* rtSceneSetSurfaces(RT_Scene* self, RT_Surface* s, uint32_t ns);

/* Sets lights array in given scene. 

:param: self: pointer to scene object
:param: l: array of lights
:param: nl: number of items in array of lights */
RT_Scene* rtSceneSetLights(RT_Scene* self, RT_Light* l, uint32_t nl);

/* Assign planar lights to scene. */
RT_Scene* rtSceneSetPlanarLights(RT_Scene* self, RT_PlanarLight* l, uint32_t npl);

/* Releases memory occupied by given RT_Scene object.

:param: self: pointer to RT_Scene object */
void rtSceneDestroy(RT_Scene **self);

/* Loads lights data from given file and returns array of lights.

:param: filename: path to lights file
:param: n: pointer to variable that will hold number of returned array's
  items */
RT_Light* rtLightLoad(const char *filename, uint32_t *n);

/* Loads planar lights from file. */
RT_PlanarLight* rtPlanarLightLoad(const char *filename, uint32_t *n);

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

// vim: tabstop=2 shiftwidth=2 softtabstop=2
