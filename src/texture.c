#include <float.h>
#include <math.h>
#include <errno.h>
#include <stdlib.h>
#include "error.h"
#include "voxelize.h"
#include "raytrace.h"
#include "texture.h"
#include "vectormath.h"
#include "rdtsc.h"
#include "common.h"

static int myPerlin[] = { 151,160,137,91,90,15,
      131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
      190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
      88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,134,139,48,27,166,
      77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
      102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208,89,18,169,200,196,
      135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,250,124,123,
      5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
      23,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167,43,172,9,
      129,22,39,253,19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
      251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,107,
      49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127,4,150,254,
      138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
      151,160,137,91,90,15,
      131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
      190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
      88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,134,139,48,27,166,
      77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
      102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208,89,18,169,200,196,
      135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,250,124,123,
      5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
      23,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167,43,172,9,
      129,22,39,253,19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
      251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,107,
      49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127,4,150,254,
      138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
    };

static double fade(double t)
{ 
  return t * t * t * (t * (t * 6 - 15) + 10);
}

static double lerp(double t, double a, double b) 
{ 
  return a + t * (b - a);
}

static double grad(int hash, double x, double y, double z) 
{
  int h = hash & 15;
  // CONVERT LO 4 BITS OF HASH CODE
  double u = h<8||h==12||h==13 ? x : y, // INTO 12 GRADIENT DIRECTIONS.
  v = h < 4||h == 12||h == 13 ? y : z;
  return ((h & 1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
}

double noise(double x, double y, double z) 
{
  int X = (int)floor(x) & 255, // FIND UNIT CUBE THAT
      Y = (int)floor(y) & 255, // CONTAINS POINT.
      Z = (int)floor(z) & 255;
  x -= floor(x);                   // FIND RELATIVE X,Y,Z
  y -= floor(y);                   // OF POINT IN CUBE.
  z -= floor(z);
  double u = fade(x),              // COMPUTE FADE CURVES
         v = fade(y),              // FOR EACH OF X,Y,Z.
         w = fade(z);
  int A = myPerlin[X]+Y,    // HASH COORDINATES OF
      AA = myPerlin[A]+Z,   // THE 8 CUBE CORNERS,
      AB = myPerlin[A+1]+Z, 
      B = myPerlin[X+1]+Y, 
      BA = myPerlin[B]+Z, 
      BB = myPerlin[B+1]+Z;

  return 
    lerp(w, lerp(v, lerp(u, grad(myPerlin[AA], x, y, z),      // AND ADD  
                           grad(myPerlin[BA], x-1, y, z)),    // BLENDED
                   lerp(u, grad(myPerlin[AB], x, y-1, z),     // RESULTS
                           grad(myPerlin[BB], x-1, y-1, z))), // FROM 8
           lerp(v, lerp(u, grad(myPerlin[AA+1], x, y, z-1),   // CORNERS
                           grad(myPerlin[BA+1], x-1, y, z-1)),// OF CUBE
                   lerp(u, grad(myPerlin[AB+1], x, y-1, z-1 ),
                           grad(myPerlin[BB+1], x-1, y-1, z-1 ))));
}

RT_Color bricks(float x, float y, float bheight, float bwidth, float filling, float rfactor, float gfactor, float bfactor, float brickpos, float* vectormod, float smoothRadius) {           
    RT_Color color;
    float w = 2*filling+bwidth;         
    float h = 2*filling+bheight; 
    
    RT_Color brickColor = {{173 / 255.0f, 106 / 255.0f, 64 / 255.0f, 0.0f}};
    RT_Color fillColor = {{215 / 255.0f, 205 / 255.0f, 178 / 255.0f, 0.0f}};
    float basef = 0.7f, derf = 0.4f;       
       
    double ay = y / h;       
    int row = floor(ay);
    double ax = (x / w) + ((row & 1) ? 0.5 : 0) ;
    int col = floor(ax);
  
    ax = ax - col;
    ay = ay - row;
        
    float posmod[4];
    posmod[0] = 0.2f*noise(brickpos * row, brickpos * col, 0.435);
    posmod[1] = 0.2f*noise(brickpos * row, brickpos * col, 0.645);
    posmod[2] = 0.2f*noise(brickpos * row, brickpos * col, 0.354);
    posmod[3] = 0.2f*noise(brickpos * row, brickpos * col, 0.768);   
    
    float boundleft = filling/w + posmod[0] * filling/w;
    float boundright = (w - filling)/w + posmod[1] * (w - filling)/w;
    float boundtop = filling/h + posmod[2] * filling/h;
    float boundbottom = (h - filling)/h + posmod[3] * (h - filling)/h;
    
    if (ax < boundleft) {
       color = fillColor;
    } else if ( ax > boundright) {
       color = fillColor;  
    } else if ( ay < boundtop){
       color = fillColor; 
    } else if ( ay > boundbottom){
       color = fillColor;  
    } else {
       color = brickColor;    
       color.c[0] += basef * (float)noise(row*x, col*y, row*col);
       color.c[1] += basef * (float)noise(row*x, col*y, row*col);
       color.c[2] += basef * (float)noise(row*x, col*y, row*col); 
    }
    
    if (ay > boundtop && ay < boundbottom){
        if ( ax > boundleft - smoothRadius && ax < boundleft + smoothRadius){
           vectormod[0] = - cos(boundleft-ax/smoothRadius);
        }
        if ( ax > boundright - smoothRadius && ax < boundright + smoothRadius){
           vectormod[0] = cos(boundright - ax/smoothRadius);
        }
    }
    
    if (ax > boundleft && ay < boundright){
       if ( ay > boundtop - smoothRadius && ay < boundtop + smoothRadius){
           vectormod[1] = - cos(boundtop-ay/smoothRadius);
       }
       if ( ay > boundbottom - smoothRadius && ay < boundbottom + smoothRadius){
           vectormod[1] = cos(boundbottom-ay/smoothRadius);
       }
    }

    color.c[0] += derf * (float)noise(rfactor * x, rfactor * y, row * col);
    color.c[1] += derf * (float)noise(gfactor * x, gfactor*y, row * col);
    color.c[2] += derf * (float)noise(bfactor * x, bfactor*y, row * col);
    
    return color;
}
