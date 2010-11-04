// vim: tabstop=2 shiftwidth=2 softtabstop=2
#ifndef __RAYTRACE_H
#define __RAYTRACE_H

#include "scene.h"
#include "bitmap.h"

#define MIN(a,b,c) ((c)<((a)<(b)? (a): (b))? (c): ((a)<(b)? (a): (b)))
#define MAX(a,b,c) ((c)>((a)>(b)? (a): (b))? (c): ((a)>(b)? (a): (b)))

RT_Bitmap* rtr_execute(RT_Scene *scene, RT_Camera *camera);

#endif
