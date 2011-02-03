#include <float.h>
#include <math.h>
#include <errno.h>
#include <stdlib.h>
#include "error.h"
#include "voxelize.h"
#include "raytrace.h"
#include "texture.h"
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


/* Get color by bluring pixels lying on the same plane. */
static RT_Color rtVisualizedSceneGetBluredPixel(RT_VisualizedScene* s, int x, int y) {
  const int radius = 2;  // neighbourhood radius
  const float delta = 0.00001f;
  int i, j, t;
  float tmp;
  int miny=y-radius, maxy=y+radius;
  int minx=x-radius, maxx=x+radius;
  RT_Color r = {{0.0f, 0.0f, 0.0f, 0.0f}};
  
  RT_VisualizedScenePixel *ref = rtVisualizedSceneGetPixel(s, x, y);  // reference pixel
  return ref->c;
  if(!ref->t) {
    return r;
  }

  for(j=miny, t=0; j<=maxy; j++) {
    for(i=minx; i<=maxx; i++) {
      if(i < 0 || j < 0)
        continue;
      if(i >= s->width || j >= s->height)
        continue;
      RT_VisualizedScenePixel *p = rtVisualizedSceneGetPixel(s, i, j);
      if(!p->t) 
        continue;
      
      // check if this pixel lies in the same plane as reference pixel
      tmp = p->t->d - ref->t->d;
      if(tmp < -delta || tmp > delta)
        continue;
      tmp = p->t->n[0] - ref->t->n[0];
      if(tmp < -delta || tmp > delta)
        continue;
      tmp = p->t->n[1] - ref->t->n[1];
      if(tmp < -delta || tmp > delta)
        continue;
      tmp = p->t->n[2] - ref->t->n[2];
      if(tmp < -delta || tmp > delta)
        continue;

      t++;
      rtVectorAdd(r.c, r.c, p->c.c);
    }
  }

  rtVectorMul(r.c, r.c, 1.0f/t);

  return r;
}


/* Apply texture. */
static rtApplyTexture(RT_Triangle* nearest, RT_Vertex4f norm, RT_Color* out, float *o, float u, float v) {
  float px = nearest->ti[0] + (nearest->tj[0]-nearest->ti[0])*u + (nearest->tk[0]-nearest->ti[0])*v;
  float py = nearest->ti[1] + (nearest->tj[1]-nearest->ti[1])*u + (nearest->tk[1]-nearest->ti[1])*v;
  
  float bheight=0.04f, bwidth=0.10f, filling=0.005f, radius=0.005f;
  float delta=0.002f; 
  float vectormod[2];
  float rfactor=2160.0f, gfactor=0.0f, bfactor=0.0f;
  int blur=0;
  
  RT_Color cx1, cx2, cy1, cy2;
  RT_Color avg = bricks(px, py, bheight, bwidth, filling, rfactor, gfactor, bfactor, 33, vectormod, radius);
  if(delta > 0.0f) {
    float tmp;
    if((tmp=px-delta) >= 0.0f) {
      cx1 = bricks(tmp, py, bheight, bwidth, filling, rfactor, gfactor, bfactor, 33, vectormod, radius);
    }
    if((tmp=px+delta) <= 1.0f) {
      cx2 = bricks(tmp, py, bheight, bwidth, filling, rfactor, gfactor, bfactor, 33, vectormod, radius);
    }
    if((tmp=py-delta) >= 0.0f) {
      cy1 = bricks(px, tmp, bheight, bwidth, filling, rfactor, gfactor, bfactor, 33, vectormod, radius);
    }
    if((tmp=py+delta) <= 1.0f) {
      cy2 = bricks(px, tmp, bheight, bwidth, filling, rfactor, gfactor, bfactor, 33, vectormod, radius);
    }
    if(blur) {
      rtVectorAdd(avg.c, avg.c, cx1.c);
      rtVectorAdd(avg.c, avg.c, cx2.c);
      rtVectorAdd(avg.c, avg.c, cy1.c);
      rtVectorAdd(avg.c, avg.c, cy2.c);
      rtVectorMul(avg.c, avg.c, 0.25f);
    }
  }
    
  out->c[0] = avg.c[0];
  out->c[1] = avg.c[1];
  out->c[2] = avg.c[2];
  
  float ugrad = (cx2.c[0]+cx2.c[1]+cx2.c[2])*0.333f - (cx1.c[0]+cx1.c[1]+cx1.c[2])*0.333f;
  float vgrad = (cy2.c[0]+cy2.c[1]+cy2.c[2])*0.333f - (cy1.c[0]+cy1.c[1]+cy1.c[2])*0.333f;
  RT_Vertex4f U, V;

  rtVectorMul(U, nearest->ij, ugrad);
  rtVectorMul(V, nearest->ik, -vgrad);
  rtVectorAdd(norm, norm, U);
  rtVectorAdd(norm, norm, V);
  rtVectorNorm(norm);
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
    int32_t i, int32_t j, int32_t k,
    RT_Triangle **visible) 
{
  RT_Color res={{0.0f, 0.0f, 0.0f, 0.0f}}, rcolor, nc;
  RT_Vertex4f onew, rnew, rray, tmpv, norm;
  float dmin, df, rf, n_dot_lo, dm, ts=1.0f, u, v;
  int32_t c, d;

  /* Terminate if we reached limit of recurrency level. */
  if(level == 0) {
    return res;
  }
  
  /* Traverse through grid of voxels to find nearest triangle for further
   * shading processing. */
  RT_Triangle *nearest = rtUddFindNearestTriangle(udd, scene, current, onew, &dmin, o, r, &i, &j, &k, &u, &v);
  if(!nearest) {
    return res;
  }
  
  if(!*visible) {
    *visible = nearest;
  }

  // point normal towards current observer
  rtVectorCopy(nearest->n, norm);
  if(rtVectorDotp(r, norm) > 0.0f) {
    rtVectorInverse(norm, norm);
  }

  // bump mapping: http://www.cs.jhu.edu/~cohen/rendtech99/lectures
  // apply texture
  rtVectorCopy(nearest->s->color.c, nc.c);
  if(nearest->sid == 7 && nearest->texture) {
    rtApplyTexture(nearest, norm, &nc, onew, u, v);
  }

  // initialize result color with ambient color
  if(nearest->s->ka > 0.0f) {
    rtVectorMul(res.c, nc.c, nearest->s->ka * total_flux);
  } 

  // rtRayTrace reflected ray
  if(nearest->s->kr > 0.0f) {
    rtVectorRayReflected(rray, norm, rtVectorInverse(tmpv, r));
    rcolor = rtRayTrace(scene, udd, t, maxt, nearest, l, maxl, onew, rray, total_flux, level-1, i, j, k, visible);
    rtVectorAdd(res.c, res.c, rtVectorMul(rcolor.c, rcolor.c, nearest->s->kr));
  }

  // rtRayTrace refracted ray
  if(nearest->s->kt > 0.0f) {
    rtVectorRayRefracted(rray, norm, rtVectorInverse(tmpv, r), nearest->s->eta);
    rcolor = rtRayTrace(scene, udd, t, maxt, nearest, l, maxl, onew, rray, total_flux, level-1, i, j, k, visible);
    rtVectorAdd(res.c, res.c, rtVectorMul(rcolor.c, rcolor.c, nearest->s->kt));
  }
  
  // some variables
  RT_Color tmp={{0.0f, 0.0f, 0.0f, 0.0f}};
  float a_sum=0.0f, r_sum=0.0f;

  /* Process point lights. */
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
      rtVectorAdd(tmp.c, l->color.c, nc.c);
      rtVectorMul(tmp.c, tmp.c, ts*l->flux*(df+rf)/(rtVectorDistance(onew, l->p)+scene->cfg.distmod));
      rtVectorAdd(res.c, res.c, tmp.c);
    }
  }

  /* Process planar lights. */
  for(c=0; c<scene->npl; c++) {
    int32_t nsamples=16;
    RT_Color sum={{0.0f, 0.0f, 0.0f, 0.0f}};

    for(d=0; d<nsamples; d++) {  // how many samples to take
      RT_PlanarLight *pl = &scene->pl[c];
      RT_Vertex4f ab, ac;
      float dotp;

      float eta = rand() / (float)RAND_MAX;
      float psi = rand() / (float)RAND_MAX;

      RT_Light chosen;
      chosen.flux = pl->flux / nsamples;
      rtVectorCopy(pl->color.c, chosen.color.c);
      rtVectorCopy(pl->a, chosen.p);

      rtVectorMul(ab, pl->ab, eta);
      rtVectorMul(ac, pl->ac, psi);
      rtVectorAdd(chosen.p, chosen.p, ab);
      rtVectorAdd(chosen.p, chosen.p, ac);

      df = rf = 0.0f;
      rtVectorRay(rnew, onew, chosen.p);
      if((dotp=rtVectorDotp(rnew, pl->n)) <= 0) {
        //break;
      }
      
      //printf("%.3f\n", dotp);
      if(!rtUddFindShadow(udd, scene, nearest, onew, &chosen, -1, &ts)) {
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
        rtVectorAdd(sum.c, chosen.color.c, nc.c);
        rtVectorMul(sum.c, sum.c, /*pow(dotp, 0.7f)**/ts*chosen.flux*(df+rf)/(rtVectorDistance(onew, chosen.p)+scene->cfg.distmod));
      }
      
      rtVectorMul(sum.c, sum.c, 1.0f/nsamples);
      rtVectorAdd(tmp.c, tmp.c, sum.c);
      rtVectorAdd(res.c, res.c, tmp.c);
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

    // allocate memory for raytraced raw color buffer
    res->map = malloc(camera->sw*camera->sh*sizeof(RT_VisualizedScenePixel));
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
      RT_Triangle *visible = NULL;  // holds triangle intersected by primary ray
      color = rtRayTrace(
        scene, udd,
        scene->t, (RT_Triangle*)(scene->t+scene->nt), NULL,
        scene->l, (RT_Light*)(scene->l+scene->nl),
        camera->ob, ray, res->total_flux, 5,
        i, j, k,
        &visible
      );
      
      // update minimal and maximal color
      for(k=0; k<3; k++) {
        if(color.c[k] > res->max.c[k]) res->max.c[k]=color.c[k];
        if(color.c[k] < res->min.c[k]) res->min.c[k]=color.c[k];
      }

      // save pixel color (not normalized)
      rtVisualizedSceneSetPixel(res, x, y, &color, visible);
    }
  }
  
  RT_INFO("minimal color (not normalized): R=%.3f, G=%.3f, B=%.3f", res->min.c[0], res->min.c[1], res->min.c[2]);
  RT_INFO("maximal color (not normalized): R=%.3f, G=%.3f, B=%.3f", res->max.c[0], res->max.c[1], res->max.c[2]);

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

  int k, x, y;
  float delta[3];

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
    for(y=0; y<s->height; y++) {
      for(x=0; x<s->width; x++) {
        RT_Color m = rtVisualizedSceneGetBluredPixel(s, x, y);

        float r=0.0f, g=0.0f, b=0.0f;
        for(k=0; k<total_gammas; k++) {
          r += pow((m.c[0] - s->min.c[0]) * delta[0], gammas[k]) * 255.0f;
          g += pow((m.c[1] - s->min.c[1]) * delta[1], gammas[k]) * 255.0f;
          b += pow((m.c[2] - s->min.c[2]) * delta[2], gammas[k]) * 255.0f;
        }

        rtBitmapSetPixel(
          res, x, y, rtColorBuildRGBA(
            r/(float)total_gammas,
            g/(float)total_gammas, 
            b/(float)total_gammas, 0));
      }
    }
  }

  return res;
}

// vim: tabstop=2 shiftwidth=2 softtabstop=2
