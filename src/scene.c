// vim: tabstop=2 shiftwidth=2 softtabstop=2
#include "scene.h"
#include "error.h"
#include "vectormath.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/* Helper function for reading lines from scene files. Sequential calls to this
 * function will return pointer to next line in file filtered from whitechars
 * and comments.

:param: self: pointer to FILE object */
static char* rtReadline(FILE *self) {
  static char buffer[1024];
  int32_t i, len;
  char *res=NULL, c;
  while((res=fgets(buffer, 1024, self)) != NULL) {
    len = strlen(buffer);
    for(i=0; i<len; i++) {
      c = buffer[i];
      if(c!=' ' && c!='\t' && c!='\n' && c!='\r')
        break;  //other character than white one
    }
    if(i == len)
      continue;  // white chars only found
    if(strstr(buffer, "//") == buffer)
      continue;  //comment found
    return res;
  }
  return NULL;
}


///////////////////////////////////////////////////////////////
RT_Scene* rtSceneLoad(const char *filename) {
  int32_t i=0, _i, j, k, vcount=-1, tcount=-1, pcount=-1;
  char *pch;
  FILE *fd=NULL;
  char *line=NULL;
  RT_Vertex4f *v=NULL;
  RT_Scene *res=NULL;

  fd=fopen(filename, "r");
  if (!fd) {
    errno = E_IO;
    goto cleanup;
  }

  while((line=rtReadline(fd)) != NULL) {
    /*---------------------------
      vertices reading startup
     ----------------------------*/
    if(vcount == -1) {
      sscanf(line, "%d", &vcount);

      // create RT_Scene object
      res = malloc(sizeof(RT_Scene));
      if (!res) {
        errno = E_MEMORY;
        goto cleanup;
      }
      memset(res, 0, sizeof(RT_Scene));

      // create placeholder for vertices
      v = malloc(vcount*sizeof(RT_Vertex4f));
      if (!v) {
        rtSceneDestroy(&res);
        errno = E_MEMORY;
        goto cleanup;
      }

      // initialize variables
      i = 0;

    /*----------------------------
      reading vertices into array
     -----------------------------*/
    } else if (vcount > 0) {
      sscanf(line, "%f %f %f", &v[i][0], &v[i][1], &v[i][2]);
      i++; vcount--;

    /*----------------------------
      triangles reading startup
     -----------------------------*/
    } else if (tcount == -1) {
      sscanf(line, "%d", &tcount);
      
      // create array of triangles
      res->nt = tcount;
      res->t = malloc(tcount*sizeof(RT_Triangle));
      if (!res->t) {
        rtSceneDestroy(&res);
        errno = E_MEMORY;
        goto cleanup;
      }
      
      // initialize variables
      i = 0;

    /*-----------------------------
      reading triangles into array
     ------------------------------*/
    } else if (tcount > 0) {
      sscanf(line, "%d %d %d", &_i, &j, &k);

      vec_vector_copy(v[_i], res->t[i].i);
      vec_vector_copy(v[j], res->t[i].j);
      vec_vector_copy(v[k], res->t[i].k);

      i++; tcount--;

    /*-----------------------------------
      surface assignment reading startup 
     ------------------------------------*/
    } else if (pcount == -1) {
      i = 0;
      pcount = res->nt;
      pch = strtok(line, " \t\n\r");
      while(pch != NULL) {
        sscanf(pch, "%d", &_i);
        res->t[i].sid = _i;
        res->t[i].s = NULL;
        pch = strtok(NULL, " \t\n\r");
        i++; pcount--;
      }

    /*-------------------------------------
      reading part assignment of triangles
     --------------------------------------*/
    } else if (pcount > 0) {
      pch = strtok(line, " \t\n\r");
      while(pch != NULL) {
        sscanf(pch, "%d", &_i);
        res->t[i].sid = _i;
        res->t[i].s = NULL;
        pch = strtok(NULL, " \t\n\r");
        i++; pcount--;
      }
    }
  }

cleanup:
  if(v) free(v);
  if(fd) fclose(fd);

  return res;
}


///////////////////////////////////////////////////////////////
RT_Scene* rtSceneSetSurfaces(RT_Scene* self, RT_Surface* s, uint32_t ns) {
  int32_t i;

  self->ns = ns;
  self->s = s;

  //update pointer to surface for each triangle
  for(i=0; i<self->nt; i++) {
    // check if number of surfaces is sufficient
    if(self->t[i].sid >= ns) {
      errno = E_NOT_ENOUGH_SURFACES;
      return NULL;
    }
    // assign surface address to triangle
    self->t[i].s = &s[self->t[i].sid];
  }

  return self;
}


///////////////////////////////////////////////////////////////
void rtSceneDestroy(RT_Scene **self) {
  RT_Scene *ptr=*self;
  if(!ptr)
    return;
  if(ptr->t)
    free(ptr->t);
  if(ptr->l)
    free(ptr->l);
  if(ptr->s)
    free(ptr->s);
  free(ptr);
  *self = NULL;
}


///////////////////////////////////////////////////////////////
RT_Light* rtLightLoad(const char *filename, uint32_t *n) {
  int32_t i=0, lcount=-1;
  FILE *fd=NULL;
  char *line=NULL;
  RT_Light *res=NULL;

  fd=fopen(filename, "r");
  if (!fd) {
    errno = E_IO;
    goto cleanup;
  }

  while((line=rtReadline(fd)) != NULL) {
    /*----------------------
      read number of lights 
     -----------------------*/
    if (lcount == -1) {
      sscanf(line, "%d", &lcount);
      
      // allocate memory for array of lights
      *n = lcount;
      res = malloc(lcount*sizeof(RT_Light));
      if (!res) {
        errno = E_MEMORY;
        res = NULL;
        goto cleanup;
      }
      
      // initialize variables
      i = 0;

    /*------------------
      read single light 
     -------------------*/
    } else {
      sscanf(line, 
             "%f %f %f %f %f %f %f", 
             &res[i].p[0], &res[i].p[1], &res[i].p[2], &res[i].flux, &res[i].color.r, &res[i].color.g, &res[i].color.b);
      i++;
    }
  }

cleanup:
  if(fd)
    fclose(fd);

  return res;
}


///////////////////////////////////////////////////////////////
RT_Surface* rtSurfaceLoad(const char *filename, uint32_t *n) {
  RT_Surface *res=NULL;
  FILE *fd=NULL;
  char *line=NULL, *pch;
  int32_t scount=-1, i=0, j=0;
  float tmp;

  fd = fopen(filename, "r");
  if(!fd) {
    errno = E_IO;
    goto cleanup;
  }

  while((line=rtReadline(fd)) != NULL) {
    /*------------------------
      read number of surfaces
     -------------------------*/
    if(scount == -1) {
      sscanf(line, "%d", &scount);

      // create surface array
      *n = scount;
      res = malloc(scount*sizeof(RT_Surface));
      if(!res) {
        errno = E_MEMORY;
        res = NULL;
        goto cleanup;
      }

      // initialize variables
      i = 0;

    /*-------------
      read surface
     --------------*/
    } else {
      j = 0;
      pch = strtok(line, " \t");
      while(pch != NULL) {
        sscanf(pch, "%f", &tmp);
        pch = strtok(NULL, " \t");
        switch(j) {
          case 0:
            res[i].kd = tmp;
            break;
          case 1:
            res[i].ks = tmp;
            break;
          case 2:
            res[i].g = tmp;
            break;
          case 3:
            res[i].ka = tmp;
            break;
          case 4:
            res[i].color.r = tmp;
            break;
          case 5:
            res[i].color.g = tmp;
            break;
          case 6:
            res[i].color.b = tmp;
            break;
          case 7: 
            res[i].kt = tmp;
            break;
          case 8:
            res[i].eta = tmp;
            break;
          case 9:
            res[i].kr = tmp;
            break;
        }
        if(++j >= 10)
          break;
      }
      i++;
    }
  }

cleanup:
  if(fd)
    fclose(fd);

  return res;
}


///////////////////////////////////////////////////////////////
RT_Camera* rtCameraLoad(const char *filename) {
  FILE *fd=NULL;
  RT_Camera *res=NULL;
  char *line=NULL;
  int16_t vp=1, sc=3, sr=1;
  RT_Vertex4f tmp;

  fd=fopen(filename, "r");
  if (!fd) {
    errno = E_IO;
    goto cleanup;
  }

  while((line=rtReadline(fd)) != NULL) {
    /*-------------------
      observer position 
     --------------------*/
    if(vp > 0) {
      // create camera object
      res = malloc(sizeof(RT_Camera));
      if(!res) {
        errno = E_MEMORY;
        goto cleanup;
      }
      memset(res, 0, sizeof(RT_Camera));

      // read observer position from file
      sscanf(line, "%f %f %f", &res->ob[0], &res->ob[1], &res->ob[2]);

      vp--;

    /*----------------
      screen position 
     -----------------*/
    } else if(sc > 0) {
      sscanf(line, "%f %f %f", &tmp[0], &tmp[1], &tmp[2]);
      switch(sc) {
        //upper left screen corner
        case 3:
          vec_vector_copy(tmp, res->ul);
          break;
        //bottom left screen corner
        case 2:
          vec_vector_copy(tmp, res->bl);
          break;
        //upper right screen corner
        case 1:
          vec_vector_copy(tmp, res->ur);
          break;
      }
      sc--;

    /*------------------
      screen resolution 
     -------------------*/
    } else if(sr > 0) {
      sscanf(line, "%d %d", &res->sw, &res->sh);
      sr--;
    }
  }

cleanup:
  if (fd) 
    fclose(fd);

  return res;
}


///////////////////////////////////////////////////////////////
void rtCameraDestroy(RT_Camera **self) {
  free(*self);
  *self = NULL;
}
