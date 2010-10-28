#ifndef __RAYTRACE_H
#define __RAYTRACE_H

#include "scene.h"
#include "imagelib.h"

#define MIN(a,b,c) ((c)<((a)<(b)? (a): (b))? (c): ((a)<(b)? (a): (b)))
#define MAX(a,b,c) ((c)>((a)>(b)? (a): (b))? (c): ((a)>(b)? (a): (b)))

IML_Bitmap* rtr_execute(SCN_Scene *scene, SCN_Camera *camera);

#endif
