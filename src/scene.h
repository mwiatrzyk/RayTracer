#ifndef __SCENE_H
#define __SCENE_H

#include "types.h"

//// STRUCTURES ///////////////////////////////////////////////

typedef struct _SCN_Vertex {
    float x, y, z;  //vertex coords
} SCN_Vertex;

typedef struct _SCN_Triangle {
    SCN_Vertex *i, *j, *k;  //pointers to triangle's vertices
} SCN_Triangle;

typedef struct _SCN_Scene {
    int32_t vsize;   //number of vertices in scene
    int32_t tsize;   //number of triangles in scene
    SCN_Vertex *v;   //array of vertices
    SCN_Triangle *t; //array of triangles
} SCN_Scene;

typedef struct _SCN_Camera {
} SCN_Camera;

typedef struct _SCN_Lights {
} SCN_Lights;

typedef struct _SCN_SurfaceAttribs {
} SCN_SurfaceAttribs;

//// FUNCTIONS ////////////////////////////////////////////////

/* Loads scene geometry description from given *.brp file.
 
 @param: filename: absolute path to geometry file */
SCN_Scene* scn_scene_load(const char *filename);

#endif
