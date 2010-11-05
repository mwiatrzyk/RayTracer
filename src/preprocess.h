#ifndef __PREPROCESS_H
#define __PREPROCESS_H

#include "scene.h"


/* Performs scene preprocessing: calculations of all coefficients, domain
 * division, intersection algorithms setup etc. 
 
:param: scene: pointer to scene object
:param: camera: pointer to camera object */
RT_Scene* rtScenePreprocess(RT_Scene *scene, RT_Camera *camera);

#endif

// vim: tabstop=2 shiftwidth=2 softtabstop=2
