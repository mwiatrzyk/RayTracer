#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include "error.h"
#include "stringtools.h"
#include "bitmap.h"
#include "scene.h"
#include "rdtsc.h"
#include "common.h"
#include "raytrace.h"


/* Print command line options help. 

:param: executable: name of executable file */
void print_help(const char *executable) {
  printf("usage: %s [options]\n\n", executable);
  printf("options:\n"
      "    Input data options:\n"
      "    -g PATH     use geometry file PATH\n"
      "    -l PATH     use light file PATH\n"
      "    -a PATH     use attribute file PATH\n"
      "    -c PATH     use camera file PATH\n"
      "    -C PATH     use renderer config file PATH\n"
      "    -s PATH     use PATH as prefix that will be appended with file extensions.\n"
      "                This argument allows to pass all files (*.brs, *.atr, *.cam, *.lgt)\n"
      "                at once (-g, -l, -a, -c can be used to override some of them)\n"
      "\n"
      "    Output image options:\n"
      "    -o PATH     store rendered image in file PATH\n");
}


/* Parse command line arguments. */
int parse_args(
    int argc, char* argv[], 
    char **g, char **l, char **a, char **c,
    char **s, char **o, float *gamma, float *epsilon, float *distmod, char **C, char **L) {

  int i=1, alen;
  char *tmp, **dst=NULL;
  if(argc <= 1) {
    print_help(argv[0]);
    return 0;
  } else {
    while(i < argc) {
      tmp = argv[i];
      alen = strlen(tmp);
      if(rtStringStartsWith(tmp, "-g")) {
        dst = g;
      } else if(rtStringStartsWith(tmp, "-l")) {
        dst = l;
      } else if(rtStringStartsWith(tmp, "-L")) {
        dst = L;
      } else if(rtStringStartsWith(tmp, "-a")) {
        dst = a;
      } else if(rtStringStartsWith(tmp, "-c")) {
        dst = c;
      } else if(rtStringStartsWith(tmp, "-C")) {
        dst = C;
      } else if(rtStringStartsWith(tmp, "-s")) {
        dst = s;
      } else if(rtStringStartsWith(tmp, "-o")) {
        dst = o;
      } else if(rtStringStartsWith(tmp, "-G")) {
        if(alen == 2) {
          sscanf(argv[++i], "%f", gamma);
        } else {
          sscanf((char*)(tmp+2), "%f", gamma);
        }
        i++;
        continue;
      } else if(rtStringStartsWith(tmp, "-E")) {
        if(alen == 2) {
          sscanf(argv[++i], "%f", epsilon);
        } else {
          sscanf((char*)(tmp+2), "%f", epsilon);
        }
        i++;
        continue;
      } else if(rtStringStartsWith(tmp, "-D")) {
        if(alen == 2) {
          sscanf(argv[++i], "%f", distmod);
        } else {
          sscanf((char*)(tmp+2), "%f", distmod);
        }
        i++;
        continue;
      }
      if(alen == 2) {
        *dst = rtStringCopy(argv[++i]);
      } else {
        *dst = rtStringCopy((char*)(tmp+2));
      }
      i++;
    }
  }
  if((!*s && (!*g || (!*l && !*L) || !*a || !*c)) || !*o) {
    RT_EERROR("some of required options are missing")
    return 0;
  }
  return 1;
}


/* Bootstrap function */
int main(int argc, char* argv[]) {
  char *g=NULL, *l=NULL, *a=NULL, *c=NULL, *s=NULL, *o=NULL, *C=NULL, *L=NULL;
  float gamma=2.5f, epsilon=0.0f, distmod=2.0f;
  uint32_t n;

  // parse command line arguments
  if(!parse_args(argc, argv, &g, &l, &a, &c, &s, &o, &gamma, &epsilon, &distmod, &C, &L)) {
    goto garbage_collect;
  }
  if(errno>0) {
    RT_CRITICAL("unable to parse args: %s\n", rtGetErrorDesc());
    goto garbage_collect;
  }

  // prepare data
  if(s) {
    if(!g) g = rtStringConcat(s, ".brs");
    if(!l) l = rtStringConcat(s, ".lgt");
    if(!a) a = rtStringConcat(s, ".atr");
    if(!c) c = rtStringConcat(s, ".cam");
    if(!C) C = rtStringConcat(s, ".cfg");
    if(!L) L = rtStringConcat(s, ".pnr");
  }

  // load scene geometry
  RT_INFO("loading scene geometry: %s", g);
  RT_Scene *scene = rtSceneLoad(g);
  if(errno>0) {
    RT_ERROR("unable to load scene geometry: %s", rtGetErrorDesc())
    goto garbage_collect;
  }
  scene->cfg.epsilon = epsilon;
  scene->cfg.gamma = gamma;
  scene->cfg.distmod = distmod;
  RT_INFO("loading renderer configuration file: %s", C)
  rtSceneConfigureRenderer(scene, C);
  if(errno > 0) {
    RT_WARN("unable to load renderer configuration file: %s", rtGetErrorDesc())
    errno = 0;
  }

  // load lights and add to scene
  RT_INFO("loading lights: %s", l);
  RT_Light *lgt = rtLightLoad(l, &n);
  if(errno>0) {
    RT_WARN("unable to load scene's lights: %s", rtGetErrorDesc());
    errno=0;
  } else {
    rtSceneSetLights(scene, lgt, n);
  }

  // load planar lights and add to scene
  RT_INFO("loading planar lights: %s", L)
  RT_PlanarLight *pl = rtPlanarLightLoad(L, &n);
  if(errno>0) {
    RT_WARN("unable to load planar lights: %s", rtGetErrorDesc());
    errno = 0;
  } else {
    rtSceneSetPlanarLights(scene, pl, n);
  }

  // load surface attributes and add to scene
  RT_INFO("loading surface attributes: %s", a);
  RT_Surface *surf = rtSurfaceLoad(a, &n);
  if(errno>0) {
    RT_ERROR("unable to load scene's attributes: %s", rtGetErrorDesc());
    goto garbage_collect;
  }
  rtSceneSetSurfaces(scene, surf, n);

  // load camera configuration
  RT_INFO("loading camera configuration: %s", c);
  RT_Camera *cam = rtCameraLoad(c);
  if(errno>0) {
    RT_ERROR("unable to load camera RT_INFO: %s", rtGetErrorDesc());
    goto garbage_collect;
  }

  // execute raytrace process
  RT_IINFO("ray-tracing in progress...");
  clock_t start = clock();
  RT_VisualizedScene *vs = rtVisualizedSceneRaytrace(scene, cam);
  if(errno>0) {
    RT_WARN("errno set by ray-trace process: %d, %s", errno, rtGetErrorDesc());
    errno = 0;
  }
  if(!vs) {
    goto garbage_collect;
  }
  RT_INFO("...ray-tracing done. Time taken: %.3f seconds", (double)(clock()-start)/CLOCKS_PER_SEC);

  // create and save result bitmap
  RT_INFO("creating result image: %s", o);
  RT_Bitmap *bmp = rtVisualizedSceneToBitmap(vs, F_HDR, NULL);
  rtBitmapSave(bmp, o, 24);
  rtBitmapDestroy(&bmp);
  rtVisualizedSceneDestroy(&vs);
  if(errno>0) {
    RT_ERROR("problem while creating result image: %d, %s", errno, rtGetErrorDesc());
    goto garbage_collect;
  }
  RT_IINFO("all done.")

garbage_collect:
  rtStringDestroy(&g);
  rtStringDestroy(&l);
  rtStringDestroy(&a);
  rtStringDestroy(&c);
  rtStringDestroy(&s);
  rtStringDestroy(&o);
  if(errno>0) {
    return 1;
  } else {
    return 0;
  }
}

// vim: tabstop=2 shiftwidth=2 softtabstop=2
