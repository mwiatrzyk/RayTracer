#ifndef __TEXTURE_H
#define __TEXTURE_H



#include "scene.h"
#include "bitmap.h"


//// STRUCTURES ///////////////////////////////////////////////

typedef struct _perlin
{
  int p[512];
} perlin;

//// INLINE FUNCTIONS /////////////////////////////////////////

static inline double fade(double t);
static inline double lerp(double t, double a, double b);
static inline double grad(int hash, double x, double y, double z);

//// FUNCTIONS ////////////////////////////////////////////////

perlin initPerlin();
double noise(double x, double y, double z);

RT_Color bricks(float x, float y, float bheight, float bwidth, float filling, 
                float rfactor, float gfactor, float bfactor, float brickpos, 
                float* vectormod, float smoothRadius);

#endif
