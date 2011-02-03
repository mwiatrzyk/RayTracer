#ifndef __RAYTRACE_H
#define __RAYTRACE_H

#include "scene.h"
#include "bitmap.h"

//// rtVisualizedSceneToBitmap() FLAGS ////////////////////////

/* Use gamma corection to produce result image.

:param: param1: pointer to gamma value (float) */
#define F_GAMMA   1

/* Produce HDR image (use several gammas and combine resulting images). 

:param: param1: pointer to NULL-terminated array of gamma values (float). When
    set to NULL, default values are used (0.5, 1.5 and 2.5) */
#define F_HDR     2


//// STRUCTURES ///////////////////////////////////////////////

typedef struct _RT_VisualizedScenePixel {
  RT_Color c;
  RT_Triangle *t;
} RT_VisualizedScenePixel;

typedef struct _RT_VisualizedScene {
  int32_t width;
  int32_t height;
  float total_flux;
  float gamma;
  RT_Color min, max;
  RT_VisualizedScenePixel *map;
} RT_VisualizedScene;


//// INLINE FUNCTIONS /////////////////////////////////////////

/* Sets not normalized pixel at given coords. */
static inline void rtVisualizedSceneSetPixel(RT_VisualizedScene *s, int32_t x, int32_t y, RT_Color *c, RT_Triangle *t) {
  RT_VisualizedScenePixel *ptr = s->map + (y*s->width) + x;
  ptr->c = *c;
  ptr->t = t;
}

/* Gets pixel. */
static inline RT_VisualizedScenePixel* rtVisualizedSceneGetPixel(RT_VisualizedScene *s, int32_t x, int32_t y) {
  return s->map + (y*s->width) + x;
}


//// FUNCTIONS ////////////////////////////////////////////////

/* Performs visualization of given `scene` from viewpoint set in `camera`
 * object using raytracing algorithm.*/
RT_VisualizedScene* rtVisualizedSceneRaytrace(RT_Scene *scene, RT_Camera *camera);

/* Releases memory occupied by given RT_VisualizedScene object. */
void rtVisualizedSceneDestroy(RT_VisualizedScene **self);

/* Converts not normalized pixel values to RT_Bitmap representing result image
 * that can be saved to file. */
RT_Bitmap* rtVisualizedSceneToBitmap(RT_VisualizedScene *s, int flags, void* param1);

#endif

// vim: tabstop=2 shiftwidth=2 softtabstop=2
