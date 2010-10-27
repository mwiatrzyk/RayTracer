#include <stdio.h>
#include <time.h>
#include <errno.h>
#include "stringtools.h"
#include "imagelib.h"
#include "scene.h"
#include "rdtsc.h"
#include "raytrace.h"


/* Print command line options help. */
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

    /* parse command line arguments */
    if(!parse_args(argc, argv, &g, &l, &a, &c, &s, &o)) {
        goto garbage_collect;
    }
    if(errno>0) {  // parse_args allocates memory
        printf("CRITICAL: unable to parse args: %s\n", err_desc());
        goto garbage_collect;
    }

    /* prepare data */
    if(!g) g = str_string_concat(s, ".brs");
    if(!l) l = str_string_concat(s, ".lgt");
    if(!a) a = str_string_concat(s, ".atr");
    if(!c) c = str_string_concat(s, ".cam");

    /* load scene geometry */
    SCN_Scene *scene=scn_scene_load(g);
    if(errno>0) {
        printf("ERROR: unable to load scene geometry: %s\n", err_desc());
        goto garbage_collect;
    }

    /* load scene lights */
    scn_scene_load_lights(scene, l);
    if(errno>0) {
        printf("ERROR: unable to load scene's lights: %s\n", err_desc());
        goto garbage_collect;
    }

    /* load surface attributes */
    scn_scene_load_surface(scene, a);
    if(errno>0) {
        printf("ERROR: unable to load scene's attributes: %s\n", err_desc());
        goto garbage_collect;
    }

    /* load camera configuration */
    SCN_Camera *cam=scn_camera_load(c);
    if(errno>0) {
        printf("ERROR: unable to load camera info: %s\n", err_desc());
        goto garbage_collect;
    }
    
    /* execute raytrace process */
    clock_t start = clock();
    IML_Bitmap *bmp=rtr_execute(scene, cam);
    printf("Time taken: %f seconds\n", (double)(clock()-start)/CLOCKS_PER_SEC);
    
    /* save result bitmap */
    iml_bitmap_save(bmp, o, 24);
    iml_bitmap_destroy(bmp);
    if(errno>0) {
        printf("ERROR: problem while creating result image: %s\n", err_desc());
        goto garbage_collect;
    }
    
    /* perform some cleanup actions */
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
