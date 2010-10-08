#ifndef __SCENE_H
#define __SCENE_H

#include "types.h"

//// STRUCTURES ///////////////////////////////////////////////

/* Definition of single vertex in 3D space. 

 @attr: x, y, z: vertex coordinates */
typedef struct _SCN_Vertex {
    float x, y, z;  //vertex coords
} SCN_Vertex;

/* Definition of surface. A surface is a group of triangles that have the same
 * properties. */
typedef struct _SCN_Surface {
} SCN_Surface;

/* Definition of triangle. 

 @attr: i, j, k: pointers to SCN_Vertex objects that create this triangle 
 @attr: s: pointer to surface represented by this triangle */
typedef struct _SCN_Triangle {
    SCN_Vertex *i, *j, *k;  //pointers to triangle's vertices
    SCN_Surface *s;  //pointer to surface description of this triangle
} SCN_Triangle;

/* Groups vertices and triangles in one place creating scene. This object will
 * contain only geometrical definition of scene. Full scene definition
 * requires also lights and surface to be desribed. 
 
 @attr: vsize: number of vertices
 @attr: tsize: number of triangles 
 @attr: v: array of vertices 
 @attr: t: array of triangles */
typedef struct _SCN_Scene {
    int32_t vsize;   //number of vertices in scene
    int32_t tsize;   //number of triangles in scene
    SCN_Vertex *v;   //array of vertices
    SCN_Triangle *t; //array of triangles
} SCN_Scene;

/* Definition of camera. All coordinates (except screen size) are in scene's
 * space.

 @attr: vx, vy, vz: observer's "eye" location
 @attr: ul_x, ul_y, ul_z: coordinates of screen's upper-left corner 
 @attr: bl_x, bl_y, bl_z: coordinates of screen's bottom-left corner 
 @attr: ur_x, ur_y, ur_z: coordinates of screen's upper-right corner */
typedef struct _SCN_Camera {
} SCN_Camera;

/* Definition of lights. */
typedef struct _SCN_Lights {
} SCN_Lights;

//// FUNCTIONS ////////////////////////////////////////////////

/* Loads scene geometry description from given *.brp file.
 
 @param: filename: absolute path to geometry file */
SCN_Scene* scn_scene_load(const char *filename);

#endif
