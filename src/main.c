// vim: tabstop=2 shiftwidth=2 softtabstop=2
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include "stringtools.h"
#include "imagelib.h"
#include "scene.h"
#include "rdtsc.h"
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
      "    -s PATH     use PATH as prefix that will be appended with file extensions.\n"
      "                This argument allows to pass all files (*.brs, *.atr, *.cam, *.lgt)\n"
      "                at once (-g, -l, -a, -c can be used to override some of them)\n"
      "\n"
      "    Output image options:\n"
      "    -o PATH     store rendered image in file PATH\n");
}


/* Parse command line arguments. */
int parse_args(int argc, char* argv[], char **g, char **l, char **a, char **c, char **s, char **o) {
  int i=1, alen;
  char *tmp, **dst;
  if(argc <= 1) {
    print_help(argv[0]);
    return 0;
  } else {
    while(i < argc) {
      tmp = argv[i];
      alen = strlen(tmp);
      if(str_string_startswith(tmp, "-g")) {
        dst = g;
      } else if(str_string_startswith(tmp, "-l")) {
        dst = l;
      } else if(str_string_startswith(tmp, "-a")) {
        dst = a;
      } else if(str_string_startswith(tmp, "-c")) {
        dst = c;
      } else if(str_string_startswith(tmp, "-s")) {
        dst = s;
      } else if(str_string_startswith(tmp, "-o")) {
        dst = o;
      }
      if(alen == 2) {
        *dst = str_string_copy(argv[++i]);
      } else {
        *dst = str_string_copy((char*)(tmp+2));
      }
      i++;
    }
  }
  if((!*s && (!*g || !*l || !*a || !*c)) || !*o) {
    printf("ERROR: some of required arguments are missing\n");
    return 0;
  }
  return 1;
}


/* Bootstrap function */
int main(int argc, char* argv[]) {
  char *tmp=NULL;
  char *g=NULL, *l=NULL, *a=NULL, *c=NULL, *s=NULL, *o=NULL;
  uint32_t n;

  // parse command line arguments
  if(!parse_args(argc, argv, &g, &l, &a, &c, &s, &o)) {
    goto garbage_collect;
  }
  if(errno>0) {
    printf("CRITICAL: unable to parse args: %s\n", rtGetErrorDesc());
    goto garbage_collect;
  }

  // prepare data
  if(!g) g = str_string_concat(s, ".brs");
  if(!l) l = str_string_concat(s, ".lgt");
  if(!a) a = str_string_concat(s, ".atr");
  if(!c) c = str_string_concat(s, ".cam");

  // load scene geometry
  printf("INFO: loading scene geometry: %s\n", g);
  RT_Scene *scene = rtSceneLoad(g);
  if(errno>0) {
    printf("ERROR: unable to load scene geometry: %s\n", rtGetErrorDesc());
    goto garbage_collect;
  }

  // load lights and add to scene
  printf("INFO: loading lights: %s\n", l);
  RT_Light *lgt = rtLightLoad(l, &n);
  if(errno>0) {
    printf("ERROR: unable to load scene's lights: %s\n", rtGetErrorDesc());
    goto garbage_collect;
  }
  rtSceneSetLights(scene, lgt, n);

  // load surface attributes and add to scene
  printf("INFO: loading surface attributes: %s\n", a);
  RT_Surface *surf = rtSurfaceLoad(a, &n);
  if(errno>0) {
    printf("ERROR: unable to load scene's attributes: %s\n", rtGetErrorDesc());
    goto garbage_collect;
  }
  rtSceneSetSurfaces(scene, surf, n);

  // load camera configuration
  printf("INFO: loading camera configuration: %s\n", c);
  RT_Camera *cam = rtCameraLoad(c);
  if(errno>0) {
    printf("ERROR: unable to load camera info: %s\n", rtGetErrorDesc());
    goto garbage_collect;
  }

  // execute raytrace process
  printf("INFO: ray-tracing...\n");
  clock_t start = clock();
  IML_Bitmap *bmp=rtr_execute(scene, cam);
  printf("INFO: ...done. Time taken: %f seconds\n", (double)(clock()-start)/CLOCKS_PER_SEC);

  // save result bitmap
  printf("INFO: creating result image: %s\n", o);
  iml_bitmap_save(bmp, o, 24);
  iml_bitmap_destroy(bmp);
  if(errno>0) {
    printf("ERROR: problem while creating result image: %s\n", rtGetErrorDesc());
    goto garbage_collect;
  }

garbage_collect:
  str_string_destroy(&g);
  str_string_destroy(&l);
  str_string_destroy(&a);
  str_string_destroy(&c);
  str_string_destroy(&s);
  str_string_destroy(&o);
  if(errno>0) {
    return 1;
  } else {
    return 0;
  }
}
