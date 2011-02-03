#include "scene.h"
#include "error.h"
#include "vectormath.h"
#include "common.h"
#include <float.h>
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
      for(k=0; k<3; k++) {
        res->dmin[k] = FLT_MAX;
        res->dmax[k] = FLT_MIN;
      }

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
      
      // update minimal and maximal domain size
      for(k=0; k<3; k++) {
        //FIXME: result is filled with salt & pepper without if..else below
        /*if(v[i][k] > 0.0f) {    
          v[i][k] += 0.0001f;
        } else {
          v[i][k] -= 0.0001f;
        }*/
        if(v[i][k] < res->dmin[k]) res->dmin[k]=v[i][k];
        if(v[i][k] > res->dmax[k]) res->dmax[k]=v[i][k];
      }

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

      rtVectorCopy(v[_i], res->t[i].i);
      rtVectorCopy(v[j], res->t[i].j);
      rtVectorCopy(v[k], res->t[i].k);
      res->t[i].ti[0] = 0.0f;
      res->t[i].ti[1] = 0.0f;
      res->t[i].tj[0] = 1.0f;
      res->t[i].tj[1] = 0.0f;
      res->t[i].tk[0] = 0.0f;
      res->t[i].tk[1] = 1.0f;
      
      RT_Vertex4f tmp;
      RT_Triangle *t = &res->t[i];
      const float delta = -0.0000001f;

      // calculate centroid vertex as average of all vertices (used to enlarge
      // triangle)
      RT_Vertex4f cent;
      for(k=0; k<3; k++) {
        cent[k] = (t->i[k] + t->j[k] + t->k[k]) / 3.0f;
      }
      
      // modify vertex `i` according to direction of cent->i vector
      rtVectorRay(tmp, cent, t->i);
      for(k=0; k<3; k++) {
        if(tmp[k] < 0.0f) {
          t->i[k] += -delta;
        } else if(tmp[k] > 0.0f) {
          t->i[k] += delta;
        }
        if(t->i[k] < res->dmin[k]) res->dmin[k]=t->i[k];
        if(t->i[k] > res->dmax[k]) res->dmax[k]=t->i[k];
      }

      // modify vertex `j` according to direction of cent->j vector
      rtVectorRay(tmp, cent, t->j);
      for(k=0; k<3; k++) {
        if(tmp[k] < 0.0f) {
          t->j[k] += -delta;
        } else if(tmp[k] > 0.0f) {
          t->j[k] += delta;
        }
        if(t->j[k] < res->dmin[k]) res->dmin[k]=t->j[k];
        if(t->j[k] > res->dmax[k]) res->dmax[k]=t->j[k];
      }

      // modify vertex `k` according to direction of cent->k vector
      rtVectorRay(tmp, cent, t->k);
      for(k=0; k<3; k++) {
        if(tmp[k] < 0.0f) {
          t->k[k] += -delta;
        } else if(tmp[k] > 0.0f) {
          t->k[k] += delta;
        }
        if(t->k[k] < res->dmin[k]) res->dmin[k]=t->k[k];
        if(t->k[k] > res->dmax[k]) res->dmax[k]=t->k[k];
      }

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
  
  // set default config values
  res->cfg.epsilon = 0.0f;
  res->cfg.gamma = 2.5f;
  res->cfg.distmod = 2.0f;
  res->cfg.vmode = VOX_DEFAULT;

  return res;
}


///////////////////////////////////////////////////////////////
RT_Scene* rtSceneConfigureRenderer(RT_Scene* self, const char *filename) {
  char *line, *pch;
  char buf[1024];

  FILE *fd = fopen(filename, "r");
  if (!fd) {
    errno = E_IO;
    return NULL;
  }

  while((line=rtReadline(fd)) != NULL) {
    pch = strtok(line, " \t");
    while(pch != NULL) {
      if(!strcmp(pch, "epsilon")) {
        pch = strtok(NULL, " \t");
        sscanf(pch, "%f", &self->cfg.epsilon);
      } else if(!strcmp(pch, "gamma")) {
        pch = strtok(NULL, " \t");
        sscanf(pch, "%f", &self->cfg.gamma);
      } else if(!strcmp(pch, "distmod")) {
        pch = strtok(NULL, " \t");
        sscanf(pch, "%f", &self->cfg.distmod);
      } else if(!strcmp(pch, "voxmode")) {
        pch = strtok(NULL, " \t");
        sscanf(pch, "%s", buf);
        if(!strcmp(buf, "DEFAULT")) {
          self->cfg.vmode = VOX_DEFAULT;
        } else if(!strcmp(buf, "MODIFIED_DEFAULT")) {
          self->cfg.vmode = VOX_MODIFIED_DEFAULT;
        } else if(!strcmp(buf, "FIXED")) {
          self->cfg.vmode = VOX_FIXED;
        } else {
          RT_WARN("%s: no such voxelization mode - using VOX_DEFAULT", pch)
          self->cfg.vmode = VOX_DEFAULT;
        }
      } else if(!strcmp(pch, "voxparams")) {
        pch = strtok(NULL, " \t");
        sscanf(pch, "%f", &self->cfg.vcoeff[0]);
        pch = strtok(NULL, " \t");
        sscanf(pch, "%f", &self->cfg.vcoeff[1]);
        pch = strtok(NULL, " \t");
        sscanf(pch, "%f", &self->cfg.vcoeff[2]);
      }
      pch = strtok(NULL, " \t");
    }
  }

  return self;
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
RT_Scene* rtSceneSetLights(RT_Scene* self, RT_Light* l, uint32_t nl) {
  int32_t k;
  self->nl = nl;
  self->l = l;
  
  self->lbuf = malloc(nl*sizeof(float));
  if(!self->lbuf) {
    errno = E_MEMORY;
    return NULL;
  }
  
  self->tc = malloc(nl*sizeof(float));
  if(!self->tc) {
    errno = E_MEMORY;
    return NULL;
  }
  for(k=0; k<nl; k++) {
    self->tc[k] = 1.0f;
  }
  
  self->lc = malloc(nl*sizeof(float));
  if(!self->lc) {
    errno = E_MEMORY;
    return NULL;
  }
  for(k=0; k<nl; k++) {
    self->lc[k] = 1.0f;
  }

  memset(self->lbuf, 0, nl*sizeof(float));
  
  for(k=0; k<self->nt; k++) {
    self->t[k].shadow_cache = malloc(nl*sizeof(RT_Triangle*));
    if(!self->t[k].shadow_cache) {
      errno = E_MEMORY;
      return NULL;
    }
    memset(self->t[k].shadow_cache, 0, nl*sizeof(RT_Triangle*));
  }
  return self;
}

///////////////////////////////////////////////////////////////
RT_Scene* rtSceneSetPlanarLights(RT_Scene* self, RT_PlanarLight* pl, uint32_t npl) {
  self->pl = pl;
  self->npl = npl;
  return self;
}

///////////////////////////////////////////////////////////////
void rtSceneDestroy(RT_Scene **self) {
  int32_t k;
  RT_Scene *ptr=*self;
  if(!ptr)
    return;
  if(ptr->t) {
    for(k=0; k<ptr->nt; k++) {
      free(ptr->t[k].shadow_cache);
    }
    free(ptr->t);
  }
  if(ptr->tc)
    free(ptr->tc);
  if(ptr->lc)
    free(ptr->lc);
  if(ptr->lbuf)
    free(ptr->lbuf);
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
      memset(res, 0, lcount*sizeof(RT_Light));
      
      // initialize variables
      i = 0;

    /*------------------
      read single light 
     -------------------*/
    } else {
      sscanf(line, 
             "%f %f %f %f %f %f %f", 
             &res[i].p[0], &res[i].p[1], &res[i].p[2], &res[i].flux, &res[i].color.c[0], &res[i].color.c[1], &res[i].color.c[2]);
      i++;
    }
  }

cleanup:
  if(fd)
    fclose(fd);

  return res;
}

///////////////////////////////////////////////////////////////
RT_PlanarLight* rtPlanarLightLoad(const char *filename, uint32_t *n) {
  FILE *fd=NULL;
  char *line=NULL;
  int32_t i=0, tmp, lcount=-1;
  RT_PlanarLight *res=NULL;

  fd = fopen(filename, "r");
  if(!fd) {
    errno = E_IO;
    goto cleanup;
  }
  
  while((line=rtReadline(fd)) != NULL) {
    /*----------------------
      read number of lights 
     -----------------------*/
    if (lcount == -1) {
      sscanf(line, "%d", &lcount);
      
      // allocate memory for array of planar lights
      *n = lcount;
      res = malloc(lcount*sizeof(RT_PlanarLight));
      if (!res) {
        errno = E_MEMORY;
        res = NULL;
        goto cleanup;
      }
      memset(res, 0, lcount*sizeof(RT_PlanarLight));
      
      // initialize variables
      i = 0;

    /*------------------
      read single light 
     -------------------*/
    } else {
      tmp = i / 4;
      switch(i % 4) {
        // flux, R, G, B
        case 0:
          sscanf(line, "%f %f %f %f", 
            &res[tmp].flux, 
            &res[tmp].color.c[0], 
            &res[tmp].color.c[1],
            &res[tmp].color.c[2]);
          break;
        // origin point
        case 1:
          sscanf(line, "%f %f %f", &res[tmp].a[0], &res[tmp].a[1], &res[tmp].a[2]);
          break;
        // "top" point
        case 2:
          sscanf(line, "%f %f %f", &res[tmp].b[0], &res[tmp].b[1], &res[tmp].b[2]);
          break;
        // "right" point
        case 3:
          sscanf(line, "%f %f %f", &res[tmp].c[0], &res[tmp].c[1], &res[tmp].c[2]);
          break;
      }
      i++;
    }
  }
  
  // calculate other parameters of surface light (such as f.e. normal vector)
  for(i=0; i<lcount; i++) {
    rtVectorMake(res[i].ab, res[i].a, res[i].b);
    rtVectorMake(res[i].ac, res[i].a, res[i].c);
    rtVectorCrossp(res[i].n, res[i].ab, res[i].ac);
    rtVectorNorm(res[i].n);
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
            res[i].color.c[0] = tmp<=1.0f? tmp: tmp/255.0f;
            break;
          case 5:
            res[i].color.c[1] = tmp<=1.0f? tmp: tmp/255.0f;
            break;
          case 6:
            res[i].color.c[2] = tmp<=1.0f? tmp: tmp/255.0f;
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
          rtVectorCopy(tmp, res->ul);
          break;
        //bottom left screen corner
        case 2:
          rtVectorCopy(tmp, res->bl);
          break;
        //upper right screen corner
        case 1:
          rtVectorCopy(tmp, res->ur);
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

// vim: tabstop=2 shiftwidth=2 softtabstop=2
