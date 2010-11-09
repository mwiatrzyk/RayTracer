#include <float.h>
#include <errno.h>
#include "error.h"
#include "voxelize.h"
#include "raytrace.h"
#include "vectormath.h"
#include "rdtsc.h"
#include "common.h"


/* Shadow test. */
static int is_shadow(RT_Triangle *t, RT_Triangle *maxt, RT_Triangle *current, float *o, float *r, float *l) {
  int i;
  float d, dmax=rtVectorDistance(o, l), dmin=FLT_MAX;
  float min[4];
  float max[4];

  for(i=0; i<3; i++) {
    if(o[i] < l[i]) {
      min[i]=o[i]; max[i]=l[i];
    } else {
      min[i]=l[i]; max[i]=o[i];
    }
  }

  while(t < maxt) {
    if(t != current) {
      if(t->i[0]<min[0] && t->j[0]<min[0] && t->k[0]<min[0]) {t++; continue;}
      if(t->i[0]>max[0] && t->j[0]>max[0] && t->k[0]>max[0]) {t++; continue;}
      if(t->i[1]<min[1] && t->j[1]<min[1] && t->k[1]<min[0]) {t++; continue;}
      if(t->i[1]>max[1] && t->j[1]>max[1] && t->k[1]>max[0]) {t++; continue;}
      if(t->i[2]<min[2] && t->j[2]<min[2] && t->k[2]<min[0]) {t++; continue;}
      if(t->i[2]>max[2] && t->j[2]>max[2] && t->k[2]>max[0]) {t++; continue;}
      if(t->isint(t, o, r, &d, &dmin)) {
        if(d < dmax) {
          return 1;
        }
      }
    }
    t++;
  }

  return 0;
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
  RT_Color res={0.0f, 0.0f, 0.0f, 0.0f}, tmp, rcolor;
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
  iml_color_scale(&res, &nearest->s->color, nearest->s->ka * total_flux);

  // rtRayTrace reflected ray
  /*if(nearest->s->kr > 0.0f) {
    rtVectorRayReflected(rray, nearest->n, rtVectorInverse(tmpv, r));
    rcolor = rtRayTrace(scene, udd, t, maxt, nearest, l, maxl, onew, rray, total_flux, level-1, i, j, k);
    iml_color_add(&res, &res, iml_color_scale(&rcolor, &rcolor, nearest->s->kr));
  }

  // rtRayTrace refracted ray
  if(nearest->s->kt > 0.0f) {
    rtVectorRayRefracted(rray, nearest->n, rtVectorInverse(tmpv, r), nearest->s->eta);
    rcolor = rtRayTrace(scene, udd, t, maxt, nearest, l, maxl, onew, rray, total_flux, level-1, i, j, k);
    iml_color_add(&res, &res, iml_color_scale(&rcolor, &rcolor, nearest->s->kt));
  }*/

  // calculate color at intersection point
  while(l < maxl) {
    df = rf = 0.0f;
    rtVectorRay(rnew, onew, l->p);

    if(!is_shadow(t, maxt, nearest, onew, rnew, l->p)) {
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
      iml_color_add(&tmp, &l->color, &nearest->s->color);
      iml_color_scale(&tmp, &tmp, l->flux*(df+rf)/(dm+0.001f));
      iml_color_add(&res, &res, &tmp);
    }
    l++;
  }

  return res;
}


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
RT_Bitmap* rtSceneVisualize(RT_Scene *scene, RT_Camera *camera) {
  int32_t i, j, k;
  float x, y, dx, dy, w=camera->sw, h=camera->sh, total_flux=3000.0f, samples=1.0f;
  float *a=camera->ul, *b=camera->ur, *c=camera->bl, *o=camera->ob;
  RT_Bitmap *res = rtBitmapCreate(camera->sw, camera->sh, 0);
  
  /* 1st step: Preprocess scene. 
   * At this point constant triangle coefficients are calculated and correct
   * ray->triangle intersection function is assigned. */
  rtScenePreprocess(scene, camera);

  /* 2nd step: Calculate uniform domain division.
   * At this step scene is divided into voxels and each triangle in scene is
   * assigned to all voxels it belongs to. */
  RT_Udd *udd = rtUddCreate(scene);
  if(!udd) {
    errno = E_MEMORY;
    rtBitmapDestroy(&res);
    return NULL;
  }
  
  RT_IINFO("starting voxelization...");
  rtUddVoxelize(udd, scene);
  RT_IINFO("...voxelization finished");
  
  // calculate total flux of all lights
  /*for(i=0; i<scene->nt; i++) {
    total_flux += scene->l[i].flux;
    }*/

  /* 3rd step: Generate primary rays and execute rtRayTrace procedure for each of
   * generated primary rays. */
  for(y=0.0f; y<h; y+=1.0f) {
    for(x=0.0f; x<w; x+=1.0f) {
      /* Calculate primary ray direction vector and normalize it. */
      float x_coef=(x+dx)/w, y_coef=(y+dy)/h;
      float ray[4] = {
        x_coef*(b[0] - a[0]) + y_coef*(c[0] - a[0]) + a[0] - o[0],
        x_coef*(b[1] - a[1]) + y_coef*(c[1] - a[1]) + a[1] - o[1],
        x_coef*(b[2] - a[2]) + y_coef*(c[2] - a[2]) + a[2] - o[2],
        0.0f
      };
      rtVectorNorm(ray);
      
      /* Calculate startup (i,j,k) voxel coordinates for current primary ray. */
      if(!rtUddFindStartupVoxel(udd, scene, o, ray, &i, &j, &k))
        continue;

      /* Trace current ray and calculate color of current pixel. */
      RT_Color sum = rtRayTrace(
        scene, udd,
        scene->t, (RT_Triangle*)(scene->t+scene->nt), NULL,
        scene->l, (RT_Light*)(scene->l+scene->nl),
        o, ray, total_flux, 10,
        i, j, k
      );

      /* Normalize color */
      iml_color_scale(&sum, &sum, 255.0f/total_flux);

      /* Write pixel onto bitmap */
      rtBitmapSetPixel(res, (int32_t)x, (int32_t)y,
          rtColorBuildRGBA(sum.r>=0.0f ? (sum.r<=255.0f ? sum.r : 255.0f) : 0.0f,
                           sum.g>=0.0f ? (sum.g<=255.0f ? sum.g : 255.0f) : 0.0f,
                           sum.b>=0.0f ? (sum.b<=255.0f ? sum.b : 255.0f) : 0.0f, 0));
    }
  }
  
  // release memory occupied by domain division structures
  rtUddDestroy(&udd);

  return res;
}

// vim: tabstop=2 shiftwidth=2 softtabstop=2
