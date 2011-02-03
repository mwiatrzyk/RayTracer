#include "common.h"
#include "preprocess.h"
#include "vectormath.h"
#include "intersection.h"


///////////////////////////////////////////////////////////////
RT_Scene* rtScenePreprocess(RT_Scene *scene, RT_Camera *camera) {
  RT_Triangle *t=scene->t, *maxt=(RT_Triangle*)(scene->t + scene->nt);
  RT_Vertex4f io;
  
  // XXX: remove me
  RT_Bitmap *tex = rtBitmapLoad("textures/brickwall.bmp");
  RT_INFO("Texture loaded: %p", tex)
  // XXX: end

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
  
  // XXX: remove me (start from here)
  t = scene->t;
  int id=0;
  while(t < maxt) {
    t->texture = NULL;
    if(t->sid == 7) {
      switch(id) {
        case 0:
          t->ti[0]=0.0f; t->ti[1]=1.0f;
          t->tj[0]=1.0f; t->tj[1]=0.0f;
          t->tk[0]=0.0f; t->tk[1]=0.0f;
          t->texture = tex;
          break;
        case 1:
          t->ti[0]=0.0f; t->ti[1]=1.0f;
          t->tj[0]=1.0f; t->tj[1]=1.0f;
          t->tk[0]=1.0f; t->tk[1]=0.0f;
          t->texture = tex;
          break;
        case 6:
          t->ti[0]=0.0f; t->ti[1]=1.0f;
          t->tj[0]=0.8f; t->tj[1]=0.0f;
          t->tk[0]=0.0f; t->tk[1]=0.0f;
          t->texture = tex;
          break;
        case 7:
          t->ti[0]=0.0f; t->ti[1]=1.0f;
          t->tj[0]=0.8f; t->tj[1]=1.0f;
          t->tk[0]=0.8f; t->tk[1]=0.0f;
          t->texture = tex;
          break;
      }
      /*printf("%d: n.x=%.3f, n.y=%.3f, n.z=%.3f, d=%.3f\n", id, t->n[0], t->n[1], t->n[2], t->d);
      printf("i.x=%.3f, j.x=%.3f, k.x=%.3f\n", t->i[0], t->j[0], t->k[0]);
      printf("i.y=%.3f, j.y=%.3f, k.y=%.3f\n", t->i[1], t->j[1], t->k[1]);
      printf("i.z=%.3f, j.z=%.3f, k.z=%.3f\n\n", t->i[2], t->j[2], t->k[2]);*/
      id++;
    }
    t++;
  }
  // XXX: end

  return scene;
}


// vim: tabstop=2 shiftwidth=2 softtabstop=2
