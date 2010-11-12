#ifndef __RAYTRACE_H
#define __RAYTRACE_H

#include "scene.h"
#include "bitmap.h"


//// STRUCTURES ///////////////////////////////////////////////

typedef struct _RT_VisualizedScene {
  int32_t width;
  int32_t height;
  RT_Color min, max;
  RT_Color *map;
} RT_VisualizedScene;


//// INLINE FUNCTIONS /////////////////////////////////////////

/* Sets not normalized pixel at given coords. */
static inline void rtVisualizedSceneSetPixel(RT_VisualizedScene *s, int32_t x, int32_t y, RT_Color *c) {
  *(s->map + (y*s->width) + x) = *c;
}


//// FUNCTIONS ////////////////////////////////////////////////

/* Performs visualization of given `scene` from viewpoint set in `camera`
 * object using raytracing algorithm.*/
RT_VisualizedScene* rtVisualizedSceneRaytrace(RT_Scene *scene, RT_Camera *camera);

/* Releases memory occupied by given RT_VisualizedScene object. */
void rtVisualizedSceneDestroy(RT_VisualizedScene **self);

RT_Bitmap* rtVisualizedSceneToBitmap(RT_VisualizedScene *s);

#endif

// vim: tabstop=2 shiftwidth=2 softtabstop=2
