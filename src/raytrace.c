#include <float.h>
#include <errno.h>
#include "error.h"
#include "voxelize.h"
#include "raytrace.h"
#include "vectormath.h"
#include "rdtsc.h"
#include "common.h"


/* Implementation of RayTracing algorithm.

:param: scene: pointer to scene object
:param: udd: pointer to uniform domain division structure 
:param: t: pointer to first triangle
:param: maxt: limit of `t` pointer 
:param: current: actual nearest triangle found
:param: l: pointer to first light
:param: maxl: limit of `l` pointer
:param: o: ray origin
:param: r: normalized ray direction
:param: total_flux: sum of all lights flux, used to calculate ambient light
:param: level: recurrency level (when reaches 0, function returns immediately) */
static RT_Color rtRayTrace(
    RT_Scene *scene, RT_Udd *udd,
    RT_Triangle *t, RT_Triangle *maxt, RT_Triangle *current, 
    RT_Light *l, RT_Light *maxl,
    float *o, float *r, 
    float total_flux, uint32_t level,
    int32_t i, int32_t j, int32_t k) 
{
  RT_Color res={{0.0f, 0.0f, 0.0f, 0.0f}}, tmp, rcolor;
  RT_Vertex4f onew, rnew, rray, tmpv, norm;
  float dmin, df, rf, n_dot_lo, dm;

  /* Terminate if we reached limit of recurrency level. */
  if(level == 0) {
    return res;
  }
  
  /* Traverse through grid of voxels to find nearest triangle for further
   * shading processing. */
  RT_Triangle *nearest = rtUddFindNearestTriangle(udd, scene, current, onew, &dmin, o, r, &i, &j, &k);
  if(!nearest) {
    return res;
  }

  // initialize result color with ambient color
  if(nearest->s->ka > 0.0f) {
    rtVectorMul(res.c, nearest->s->color.c, nearest->s->ka * total_flux);
  }

  // rtRayTrace reflected ray
  if(nearest->s->kr > 0.0f) {
    rtVectorRayReflected(rray, nearest->n, rtVectorInverse(tmpv, r));
    rcolor = rtRayTrace(scene, udd, t, maxt, nearest, l, maxl, onew, rray, total_flux, level-1, i, j, k);
    rtVectorAdd(res.c, res.c, rtVectorMul(rcolor.c, rcolor.c, nearest->s->kr));
  }

  // rtRayTrace refracted ray
  if(nearest->s->kt > 0.0f) {
    rtVectorRayRefracted(rray, nearest->n, rtVectorInverse(tmpv, r), nearest->s->eta);
    rcolor = rtRayTrace(scene, udd, t, maxt, nearest, l, maxl, onew, rray, total_flux, level-1, i, j, k);
    rtVectorAdd(res.c, res.c, rtVectorMul(rcolor.c, rcolor.c, nearest->s->kt));
  }

  // calculate color at intersection point
  while(l < maxl) {
    df = rf = 0.0f;
    rtVectorRay(rnew, onew, l->p);

    //if(!is_shadow(t, maxt, nearest, onew, rnew, l->p)) {
    if(!rtUddFindShadow(udd, scene, nearest, onew, l->p)) {
      n_dot_lo = rtVectorDotp(nearest->n, rnew);

      // diffusion factor
      df = nearest->s->kd * n_dot_lo;

      // reflection factor
      if(nearest->s->ks > 0.0f) {
        rf = nearest->s->ks * pow(rtVectorDotp(r, rtVectorRayReflected2(tmpv, nearest->n, rnew, n_dot_lo)), nearest->s->g);
        if(rf < 0.0f) {
          rf = -rf;
        }
      }

      // calculate color
      dm = rtVectorDistance(onew, l->p);
      rtVectorAdd(tmp.c, l->color.c, nearest->s->color.c);
      rtVectorMul(tmp.c, tmp.c, l->flux*(df+rf)/(dm+0.001f));
      rtVectorAdd(res.c, res.c, tmp.c);
    }
    l++;
  }

  return res;
}


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
RT_VisualizedScene* rtVisualizedSceneRaytrace(RT_Scene *scene, RT_Camera *camera) {
  int32_t i, j, k;
  int32_t x, y, w=camera->sw, h=camera->sh;
  float h_inv=1.0f/h, w_inv=1.0f/w;
  RT_Vertex4f ray;
  RT_Color color;
  
  /* Create result object that will hold processed scene in unnormalized
   * format. */
  RT_VisualizedScene *res = malloc(sizeof(RT_VisualizedScene));
  if(res) {
    res->width = w;
    res->height = h;
    for(k=0; k<4; k++) {
      res->min.c[k] = FLT_MAX;
      res->max.c[k] = FLT_MIN;
    }
    res->map = malloc(camera->sw*camera->sh*sizeof(RT_Color));
    if(!res->map) {
      rtVisualizedSceneDestroy(&res);
      errno = E_MEMORY;
      return NULL;
    }
  } else {
    errno = E_MEMORY;
    return NULL;
  }

  /* At this point constant triangle coefficients are
   * calculated and correct ray->triangle intersection function is assigned. */
  rtScenePreprocess(scene, camera);

  /* At this step scene is divided into voxels and each triangle in scene is
   * assigned to all voxels it belongs to. */
  RT_Udd *udd = rtUddCreate(scene);
  if(!udd) {
    rtVisualizedSceneDestroy(&res);
    errno = E_MEMORY;
    return NULL;
  }
  
  RT_IINFO("starting voxelization...");
  rtUddVoxelize(udd, scene);
  RT_IINFO("...voxelization finished");
  

  /* Generate primary rays and execute rtRayTrace procedure for each of
   * generated primary rays. */
  for(y=0; y<h; y++) {
    for(x=0; x<w; x++) {
      // calculate primary ray direction vector
      rtVectorPrimaryRay(
          ray,
          camera->ul, camera->ur, camera->bl, camera->ob,
          x, y, w_inv, h_inv
      );
      
      // calculate startup/entry voxel for primary ray
      if(!rtUddFindStartupVoxel(udd, scene, camera->ob, ray, &i, &j, &k))
        continue;

      // trace current ray and calculate color of current pixel.
      color = rtRayTrace(
        scene, udd,
        scene->t, (RT_Triangle*)(scene->t+scene->nt), NULL,
        scene->l, (RT_Light*)(scene->l+scene->nl),
        camera->ob, ray, 3000.0f, 10,
        i, j, k
      );
      
      // update minimal and maximal color
      for(k=0; k<3; k++) {
        if(color.c[k] > res->max.c[k]) res->max.c[k]=color.c[k];
        if(color.c[k] < res->min.c[k]) res->min.c[k]=color.c[k];
      }

      // save pixel color (not normalized)
      rtVisualizedSceneSetPixel(res, x, y, &color);
    }
  }
  
  RT_INFO("minimal color (not normalized): R=%.3f, G=%.3f, B=%.3f", res->min.c[0], res->min.c[1], res->min.c[2])
  RT_INFO("maximal color (not normalized): R=%.3f, G=%.3f, B=%.3f", res->max.c[0], res->max.c[1], res->max.c[2])

  // release memory occupied by domain division structures
  rtUddDestroy(&udd);

  return res;
}
///////////////////////////////////////////////////////////////
void rtVisualizedSceneDestroy(RT_VisualizedScene **self) {
  RT_VisualizedScene *ptr=*self;
  if(ptr) {
    if(ptr->map) free(ptr->map);
    free(ptr);
    *self = NULL;
  }
}
///////////////////////////////////////////////////////////////
RT_Bitmap* rtVisualizedSceneToBitmap(RT_VisualizedScene *s) {
  RT_Bitmap *res = rtBitmapCreate(s->width, s->height, 0);
  if(!res)
    return NULL;

  int k;
  float r, g, b, delta[3];
  uint32_t *p=res->pixels, *maxp=(res->pixels + s->width*s->height);
  RT_Color *m=s->map;

  // calculate differences between minimal and maximal colors
  for(k=0; k<3; k++) {
    delta[k] = 255.0f / (s->max.c[k] - s->min.c[k]);
  }

  // iterate through each pixel of image and calculate color by scaling to
  // 0..255 range
  while(p < maxp) {
    r = (m->c[0] - s->min.c[0]) * delta[0];
    g = (m->c[1] - s->min.c[1]) * delta[1];
    b = (m->c[2] - s->min.c[2]) * delta[2];
    *p = rtColorBuildRGBA(r, g, b, 0);
    p++; m++;
  }

  return res;
}

// vim: tabstop=2 shiftwidth=2 softtabstop=2
