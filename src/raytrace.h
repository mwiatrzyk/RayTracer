#ifndef __RAYTRACE_H
#define __RAYTRACE_H

#include "scene.h"
#include "imagelib.h"

IML_Bitmap* rtr_execute(SCN_Scene *scene, SCN_Camera *camera);

#endif
