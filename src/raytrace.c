#include <float.h>
#include <errno.h>
#include <stdlib.h>
#include "error.h"
#include "voxelize.h"
#include "raytrace.h"
#include "vectormath.h"
#include "rdtsc.h"
#include "common.h"


static inline int lbuf_cmp(const void *a_, const void *b_) {
  float *a=(float*)a_, *b=(float*)b_;
  if(*a < *b) {
    return 1;
  } else if(*a == *b) {
    return 0;
  } else {
    return -1;
  }
}


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
  RT_Color res={{0.0f, 0.0f, 0.0f, 0.0f}}, rcolor;
  RT_Vertex4f onew, rnew, rray, tmpv, norm;
  float dmin, df, rf, n_dot_lo, dm, ts=1.0f;
  int32_t c;

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
  
  // point normal towards current observer
  rtVectorCopy(nearest->n, norm);
  if(rtVectorDotp(r, norm) > 0.0f) {
    rtVectorInverse(norm, norm);
  }

  // initialize result color with ambient color
  if(nearest->s->ka > 0.0f) {
    rtVectorMul(res.c, nearest->s->color.c, nearest->s->ka * total_flux);
  }

  // rtRayTrace reflected ray
  if(nearest->s->kr > 0.0f) {
    rtVectorRayReflected(rray, norm, rtVectorInverse(tmpv, r));
    rcolor = rtRayTrace(scene, udd, t, maxt, nearest, l, maxl, onew, rray, total_flux, level-1, i, j, k);
    rtVectorAdd(res.c, res.c, rtVectorMul(rcolor.c, rcolor.c, nearest->s->kr));
  }

  // rtRayTrace refracted ray
  if(nearest->s->kt > 0.0f) {
    rtVectorRayRefracted(rray, norm, rtVectorInverse(tmpv, r), nearest->s->eta);
    rcolor = rtRayTrace(scene, udd, t, maxt, nearest, l, maxl, onew, rray, total_flux, level-1, i, j, k);
    rtVectorAdd(res.c, res.c, rtVectorMul(rcolor.c, rcolor.c, nearest->s->kt));
  }
  
  // some variables
  RT_Color tmp={{0.0f, 0.0f, 0.0f, 0.0f}};
  float a_sum=0.0f, r_sum=0.0f;

  /* Perform simplified shadow test. */
  if(scene->cfg.epsilon) {  //TODO get switch from file
    // calculate intensities
    for(c=0; c<scene->nl; c++) {
      l = &scene->l[c];

      df = rf = 0.0f;
      rtVectorRay(rnew, onew, l->p);
      n_dot_lo = rtVectorDotp(norm, rnew);

      // diffusion factor
      df = nearest->s->kd * n_dot_lo;
      if(df < 0.0f && nearest->s->kt > 0.0f) {
        df = -df;
      }

      // reflection factor
      if(nearest->s->ks > 0.0f) {
        rf = nearest->s->ks * pow(rtVectorDotp(r, rtVectorRayReflected2(tmpv, norm, rnew, n_dot_lo)), nearest->s->g);
        if(rf < 0.0f && nearest->s->kt > 0.0f) {
          rf = -rf;
        }
      }

      // calculate luminance and add to buffer
      scene->lbuf[c] = l->flux*(df+rf)/(rtVectorDistance(onew, l->p)+scene->cfg.distmod);
    }

    // sort luminance buffer in descending order
    qsort(scene->lbuf, scene->nl, sizeof(float), lbuf_cmp);
    
    // calculate maximal available luminance
    for(c=0; c<scene->nl; c++) {
      r_sum += scene->lbuf[c];
    }

    if(r_sum < 0.0f) {
      a_sum = -0.000001f;
    } else {
      a_sum = 0.000001f;
    }

    // process lights
    for(c=0; c<scene->nl; c++) {
      l = &scene->l[c];

      if(r_sum/a_sum < scene->cfg.epsilon)  //TODO: get param from file
        break;

      if(!rtUddFindShadow(udd, scene, nearest, onew, l, c, &ts)) {
        a_sum += scene->lbuf[c];
        scene->lc[c] += 1.0f;
        // calculate color
        rtVectorAdd(tmp.c, l->color.c, nearest->s->color.c);
        rtVectorMul(tmp.c, tmp.c, ts*scene->lbuf[c]);
        rtVectorAdd(res.c, res.c, tmp.c);
      }
      
      scene->tc[c] += 1.0f;
      r_sum -= scene->lbuf[c];
    }

    // proces other lights in simplified way
    for(; c<scene->nl; c++) {
      l = &scene->l[c];
      rtVectorAdd(tmp.c, l->color.c, nearest->s->color.c);
      rtVectorMul(tmp.c, tmp.c, scene->lbuf[c]*scene->lc[c]/scene->tc[c]);
      rtVectorAdd(res.c, res.c, tmp.c);
    }

  /* Perform normal shadow test. */
  } else {
    a_sum = 0.0f;
    for(c=0; c<scene->nl; c++) {
      l = &scene->l[c];
      df = rf = 0.0f;
      rtVectorRay(rnew, onew, l->p);

      if(!rtUddFindShadow(udd, scene, nearest, onew, l, c, &ts)) {
        n_dot_lo = rtVectorDotp(norm, rnew);

        // diffusion factor
        df = nearest->s->kd * n_dot_lo;
        if(df < 0.0f && nearest->s->kt > 0.0f) {
          df = -df;
        }

        // reflection factor
        if(nearest->s->ks > 0.0f) {
          rf = nearest->s->ks * pow(rtVectorDotp(r, rtVectorRayReflected2(tmpv, norm, rnew, n_dot_lo)), nearest->s->g);
          if(rf < 0.0f && nearest->s->kt > 0.0f) {
            rf = -rf;
          }
        }

        // calculate color
        rtVectorAdd(tmp.c, l->color.c, nearest->s->color.c);
        rtVectorMul(tmp.c, tmp.c, ts*l->flux*(df+rf)/(rtVectorDistance(onew, l->p)+scene->cfg.distmod));
        rtVectorAdd(res.c, res.c, tmp.c);
      }
    }
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
  if(scene->cfg.gamma > 0.0f) {
    res->gamma = scene->cfg.gamma;
  } else {
    res->gamma = 2.5f;
  }

  /* At this point constant triangle coefficients are
   * calculated and correct ray->triangle intersection function is assigned. */
  rtScenePreprocess(scene, camera);

  /* Calculate light total flux (used to determine ambient light amount). Also
   * increase domain minimal and maximal size if light position is beyond
   * minimal and maximal coords calculated at scene load step. */
  res->total_flux = 0.0f;
  for(k=0; k<scene->nl; k++) {
    res->total_flux += scene->l[k].flux;
    for(i=0; i<3; i++) {
      if(scene->l[k].p[i] < scene->dmin[i]) 
        scene->dmin[i] = scene->l[k].p[i] - 0.001f;
      if(scene->l[k].p[i] > scene->dmax[i])
        scene->dmax[i] = scene->l[k].p[i] + 0.001f;
    }
  }

  /* At this step scene is divided into voxels and each triangle in scene is
   * assigned to all voxels it belongs to. */
  RT_Udd *udd = rtUddCreate(scene);
  if(!udd) {
    rtVisualizedSceneDestroy(&res);
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
        camera->ob, ray, res->total_flux, 5,
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
RT_Bitmap* rtVisualizedSceneToBitmap(RT_VisualizedScene *s, int flags, void* param1) {
  RT_Bitmap *res = rtBitmapCreate(s->width, s->height, 0);
  if(!res)
    return NULL;

  int k;
  float delta[3];
  uint32_t *p=res->pixels, *maxp=(res->pixels + s->width*s->height);
  RT_Color *m=s->map;

  // calculate differences between minimal and maximal colors
  for(k=0; k<3; k++) {
    delta[k] = 1.0f / (s->max.c[k] - s->min.c[k]);
  }

  if(flags & F_HDR) {
    // make array of gamma values
    float default_gammas[] = {s->gamma, 0.0f};
    float *gammas = param1? (float*)param1: default_gammas;
    float *gptr = gammas;
    int32_t total_gammas=0;

    // calculate total number of gammas
    while(*(gptr++) > 0.0f) {
      total_gammas++;
    }

    // iterate through each pixel of image and calculate color by scaling to
    // 0..255 range
    while(p < maxp) {
      float r=0.0f, g=0.0f, b=0.0f;
      for(k=0; k<total_gammas; k++) {
        r += pow((m->c[0] - s->min.c[0]) * delta[0], gammas[k]) * 255.0f;
        g += pow((m->c[1] - s->min.c[1]) * delta[1], gammas[k]) * 255.0f;
        b += pow((m->c[2] - s->min.c[2]) * delta[2], gammas[k]) * 255.0f;
      }
      *p = rtColorBuildRGBA(r/(float)total_gammas, g/(float)total_gammas, b/(float)total_gammas, 0);
      p++; m++;
    }
  }

  return res;
}

// vim: tabstop=2 shiftwidth=2 softtabstop=2
