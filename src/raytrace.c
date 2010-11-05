// vim: tabstop=2 shiftwidth=2 softtabstop=2
#include <float.h>
#include "raytrace.h"
#include "vectormath.h"
#include "rdtsc.h"

#define EPSILON 0.000001f
#define BENCHMARK

#ifdef BENCHMARK
#include <time.h>
#endif


//// GLOBALS //////////////////////////////////////////////////

#ifdef BENCHMARK
uint32_t intersection_test_count = 0;
#endif

//// SHADOW TEST //////////////////////////////////////////////

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


//// MAIN FUNCTIONS ///////////////////////////////////////////

/* RayTracing algorithm implementation. 

   @param: t: pointer to scene's triangle array 
   @param: maxt: pointer to first element beyond triangle array
   @param: l: pointer to lights array
   @param: maxl: pointer to first element beyond light array
   @param: o: ray origin point
   @param: r: ray vector (normalized) */
static RT_Color raytrace(RT_Triangle *t, RT_Triangle *maxt, RT_Triangle *skip, 
    RT_Light *l, RT_Light *maxl, 
    float *o, float *r, 
    float total_flux, uint32_t level) 
{
  RT_Color res={0.0f, 0.0f, 0.0f, 0.0f}, tmp, rcolor;
  RT_Vertex4f onew, rnew, rray, tmpv;
  RT_Triangle *nearest=NULL, *tt=t;
  float d, dmin=FLT_MAX, df, rf, n_dot_lo, dm;

  if(level == 0) {
    return res;
  }

  while(tt < maxt) {
    if(tt != skip) {
      if(tt->isint(tt, o, r, &d, &dmin)) {
        if(d < dmin) {
          dmin = d;
          nearest = tt;
        }
      }
    }
    tt++;
  }

  if(nearest) {
    // initialize result color with ambient color
    iml_color_scale(&res, &nearest->s->color, nearest->s->ka * total_flux);

    // calculate intersection point with nearest triangle
    rtVectorRaypoint(onew, o, r, dmin);

    // raytrace reflected ray
    if(nearest->s->kr > 0.0f) {
      rtVectorRayReflected(rray, nearest->n, rtVectorInverse(tmpv, r));
      rcolor = raytrace(t, maxt, nearest, l, maxl, onew, rray, total_flux, level-1);
      iml_color_add(&res, &res, iml_color_scale(&rcolor, &rcolor, nearest->s->kr));
    }

    // raytrace refracted ray
    if(nearest->s->kt > 0.0f) {
      rtVectorRayRefracted(rray, nearest->n, rtVectorInverse(tmpv, r), nearest->s->eta);
      rcolor = raytrace(t, maxt, nearest, l, maxl, onew, rray, total_flux, level-1);
      iml_color_add(&res, &res, iml_color_scale(&rcolor, &rcolor, nearest->s->kt));
    }

    // calculate color at intersection point
    while(l < maxl) {
      df = rf = 0.0f;
      rtVectorRay(rnew, onew, l->p);

      if(!is_shadow(t, maxt, nearest, onew, rnew, l->p)) {
        dm = rtVectorDistance(onew, l->p);
        n_dot_lo = rtVectorDotp(nearest->n, rnew);

        // diffusion factor
        df = nearest->s->kd * n_dot_lo;

        // reflection factor
        rf = nearest->s->ks * pow(rtVectorDotp(r, rtVectorRayReflected2(tmpv, nearest->n, rnew, n_dot_lo)), nearest->s->g);

        // calculate color
        iml_color_add(&tmp, &l->color, &nearest->s->color);
        iml_color_scale(&tmp, &tmp, l->flux*(df+rf)/(dm+0.001f));
        iml_color_add(&res, &res, &tmp);
      }
      l++;
    }
  }

  return res;
}


//// INTERFACE FUNCTIONS //////////////////////////////////////

/* Executes raytracing algorithm on given scene and camera configuration
 * producing output bitmap. 

 @param: scene: pointer to scene object
 @param: camera: pointer to camera object */
RT_Bitmap* rtr_execute(RT_Scene *scene, RT_Camera *camera) {
  RT_Color color;
  int i;
  float x, y, dx, dy, w=camera->sw, h=camera->sh, total_flux=3000.0f, samples=1.0f;
  float *a=camera->ul, *b=camera->ur, *c=camera->bl, *o=camera->ob;
  RT_Bitmap *res = rtBitmapCreate(camera->sw, camera->sh, 0);

  // run scene preprocessor
  rtScenePreprocess(scene, camera);

#ifdef BENCHMARK
  clock_t start = clock();
#endif

  // calculate total flux of all lights
  /*for(i=0; i<scene->nt; i++) {
    total_flux += scene->l[i].flux;
    }*/

  // main loop
  for(y=0.0f; y<h; y+=1.0f) {
    for(x=0.0f; x<w; x+=1.0f) {
      RT_Color sum={0.0f, 0.0f, 0.0f, 0.0f};

      for(i=0, dy=0.0f; dy<1.0f; dy+=samples) {
        for(dx=0.0f; dx<1.0f; dx+=samples, i++) {
          /* Calculate primary ray direction vector and normalize it. */
          float x_coef=(x+dx)/w, y_coef=(y+dy)/h;
          float ray[4] = {
            x_coef*(b[0] - a[0]) + y_coef*(c[0] - a[0]) + a[0] - o[0],
            x_coef*(b[1] - a[1]) + y_coef*(c[1] - a[1]) + a[1] - o[1],
            x_coef*(b[2] - a[2]) + y_coef*(c[2] - a[2]) + a[2] - o[2],
            0.0f
          };
          rtVectorNorm(ray);

          /* Trace current ray and calculate color of current pixel. */
          color = raytrace(scene->t, (RT_Triangle*)(scene->t+scene->nt), NULL,
              scene->l, (RT_Light*)(scene->l+scene->nl),
              o, ray, total_flux, 10);

          /* Normalize color */
          iml_color_scale(&color, &color, 255.0f/total_flux);

          /* Add to total samples */
          iml_color_add(&sum, &sum, &color);
        }
      }

      /* Calculate average pixel color */
      iml_color_scale(&sum, &sum, 1.0f/(float)i);

      /* Write pixel onto bitmap */
      rtBitmapSetPixel(res, (int32_t)x, (int32_t)y,
          rtColorBuildRGBA(sum.r>=0.0f ? (sum.r<=255.0f ? sum.r : 255.0f) : 0.0f,
                           sum.g>=0.0f ? (sum.g<=255.0f ? sum.g : 255.0f) : 0.0f,
                           sum.b>=0.0f ? (sum.b<=255.0f ? sum.b : 255.0f) : 0.0f, 0));
    }
  }

#ifdef BENCHMARK
  double total_time = (double)(clock()-start)/CLOCKS_PER_SEC;
  printf("Intersection tests per second: %lu\n", (uint32_t)(intersection_test_count/total_time));
#endif

  return res;
}
