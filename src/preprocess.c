#include "preprocess.h"
#include "vectormath.h"
#include "intersection.h"


///////////////////////////////////////////////////////////////
RT_Scene* rtScenePreprocess(RT_Scene *scene, RT_Camera *camera) {
  RT_Triangle *t=scene->t, *maxt=(RT_Triangle*)(scene->t + scene->nt);
  RT_Vertex4f io;
  while(t < maxt) {
    // calculate vectors used to calculate normal
    rtVectorMake(t->ij, t->i, t->j);
    rtVectorMake(t->ik, t->i, t->k);

    // make vector from observer towards one of triangle vertices
    rtVectorNorm(rtVectorMake(io, t->i, camera->ob));

    // create and normalize normal vector and point it towards camera
    rtVectorNorm(rtVectorCrossp(t->n, t->ij, t->ik));
    if(rtVectorDotp(t->n, io) < 0.0f) {
      rtVectorInverse(t->n, t->n);
    }

    // calculate d coefficient of plane equation
    t->d = -rtVectorDotp(t->i, t->n);
    
    // choose intersection test algorithm for triangle
    rtInt1CoeffsPrecalc(t);

    t++;
  }
  return scene;
}


// vim: tabstop=2 shiftwidth=2 softtabstop=2
