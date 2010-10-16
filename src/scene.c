#include "scene.h"
#include "error.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/* Helper function for reading lines from scene files. Sequential calls to this
 * function will return pointer to next line in file filtered from whitechars
 * and comments.
 
 @param: self: pointer to FILE object */
static char* scn_file_readline(FILE *self) {
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
        if(strstr(buffer, "//"))
            continue;  //comment found
        return res;
    }
    return NULL;
}


SCN_Scene* scn_scene_load(const char *filename) {
    int32_t i, _i, j, k, vcount=-1, tcount=-1, pcount=-1;
    char *pch;
    FILE *fd=NULL;
    char *line=NULL;
    SCN_Scene *res=NULL;

    fd=fopen(filename, "r");
    if (!fd) {
        errno = E_IO;
        goto cleanup;
    }
    
    while((line=scn_file_readline(fd)) != NULL) {
        /* reading number of vertices */
        if(vcount == -1) {
            sscanf(line, "%d", &vcount);  //read number of vertices
            res = malloc(sizeof(SCN_Scene));  //create SCN_Scene object
            if (!res) {
                errno = E_MEMORY;
                goto cleanup;
            }
            memset(res, 0, sizeof(SCN_Scene));
            i = 0;
            res->vsize = vcount;
            res->v = malloc(vcount*sizeof(SCN_Vertex));  //create array of vertices
            if (!res->v) {
                free(res);  //unable to allocate memory for vertices - NULL must be returned
                res = NULL;
                errno = E_MEMORY;
                goto cleanup;
            }

        /* reading vertices into array */
        } else if (vcount > 0) {
            sscanf(line, "%f %f %f", &res->v[i].x, &res->v[i].y, &res->v[i].z);
            i++; vcount--;

        /* reading number of triangles */
        } else if (tcount == -1) {
            sscanf(line, "%d", &tcount);
            i = 0;
            res->tsize = tcount;
            res->t = malloc(tcount*sizeof(SCN_Triangle));  //create array of triangles
            if (!res->t) {
                free(res->v);  //res->v is allocated so it need to be freed before call to free(res)
                free(res);
                res = NULL;
                errno = E_MEMORY;
                goto cleanup;
            }

        /* reading triangles into array */
        } else if (tcount > 0) {
            sscanf(line, "%d %d %d", &_i, &j, &k);
            res->t[i].i = &res->v[_i];  //assign address of correct vertex
            res->t[i].j = &res->v[j];
            res->t[i].k = &res->v[k];
            i++; tcount--;
        
        /* initialize pcount variable */
        } else if (pcount == -1) {
            i = 0;
            pcount = res->tsize;
            pch = strtok(line, " \t\n\r");  //FIXME: use another function here (strtok is not recommended)
            while(pch != NULL) {
                sscanf(pch, "%d", &_i);
                res->t[i].sid = _i;
                res->t[i].s = NULL;
                pch = strtok(NULL, " \t\n\r");
                i++; pcount--;
            }
        /* reading part assignment of triangles */
        } else if (pcount > 0) {
            pch = strtok(line, " \t\n\r");  //FIXME: use another function here (strtok is not recommended)
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
        if (fd)
            fclose(fd);

    return res;
}


SCN_Scene* scn_scene_load_lights(SCN_Scene* self, const char *filename) {
    int32_t i, lcount=-1;
    FILE *fd=NULL;
    char *line=NULL;
    SCN_Scene *res=self;

    fd=fopen(filename, "r");
    if (!fd) {
        errno = E_IO;
        goto cleanup;
    }
    
    while((line=scn_file_readline(fd)) != NULL) {
        /* get number of lights */
        if (lcount == -1) {
            sscanf(line, "%d", &lcount);  //read number of lights
            i = 0;
            self->lsize = lcount;
            self->l = malloc(lcount*sizeof(SCN_Light));
            if (!self->l) {
                errno = E_MEMORY;
                res = NULL;  //return NULL to notify error (please note that original object is not destroyed)
                goto cleanup;
            }

        /* get single light */
        } else {
            sscanf(line, "%f %f %f %f %f %f %f", &self->l[i].x, &self->l[i].y, &self->l[i].z, 
                                                 &self->l[i].p, 
                                                 &self->l[i].R, &self->l[i].G, &self->l[i].B);
            i++;
        }
    }

    cleanup:
        if(fd)
            fclose(fd);

    return res;
}


SCN_Scene* scn_scene_load_surface(SCN_Scene* self, const char *filename) {
    SCN_Scene *res=self;
    FILE *fd=NULL;
    char *line=NULL;
    int32_t scount=-1, i;
    float kd, ks, g, ka, R, G, B;
    float kt, eta, kr;
    
    fd = fopen(filename, "r");
    if(!fd) {
        errno = E_IO;
        goto cleanup;
    }

    while((line=scn_file_readline(fd)) != NULL) {
        if(scount == -1) {
            sscanf(line, "%d", &scount);
            i = 0;
            self->ssize = scount;
            self->s = malloc(scount*sizeof(SCN_Surface));
            if(!self->s) {
                errno = E_MEMORY;
                res = NULL;
                goto cleanup;
            }
        } else {
            if(i%2 == 0) {
                sscanf(line, "%f %f %f %f %f %f %f", &kd, &ks, &g, &ka, &R, &G, &B);
            } else {
                sscanf(line, "%f %f %f", &kt, &eta, &kr);
                self->s[i/2].kd = kd;
                self->s[i/2].ks = ks;
                self->s[i/2].g = g;
                self->s[i/2].ka = ka;
                self->s[i/2].R = R;
                self->s[i/2].G = G;
                self->s[i/2].B = B;
                self->s[i/2].kt = kt;
                self->s[i/2].eta = eta;
                self->s[i/2].kr = kr;
            }
            i++;
        }
    }

    //update pointer to surface for each triangle
    for(i=0; i<self->tsize; i++) {
        if(self->t[i].sid >= self->ssize) {  //need more surface data
            errno = E_NOT_ENOUGH_SURFACES;
            res = NULL;
            goto cleanup;
        }
        self->t[i].s = &self->s[self->t[i].sid];
    }

    cleanup:
        if(fd)
            fclose(fd);
    
    return res;
}


SCN_Camera* scn_camera_load(const char *filename) {
    FILE *fd=NULL;
    SCN_Camera *res=NULL;
    char *line=NULL;
    int16_t vp=1, sc=3, sr=1;
    SCN_Vertex tmp;

    fd=fopen(filename, "r");
    if (!fd) {
        errno = E_IO;
        goto cleanup;
    }

    while((line=scn_file_readline(fd)) != NULL) {
        /* viewpoint position */
        if(vp > 0) {
            res = malloc(sizeof(SCN_Camera));
            if(!res) {
                errno = E_MEMORY;
                goto cleanup;
            }
            memset(res, 0, sizeof(SCN_Camera));
            sscanf(line, "%f %f %f", &res->ob.x, &res->ob.y, &res->ob.z);
            vp--;
        /* screen position */
        } else if(sc > 0) {
            sscanf(line, "%f %f %f", &tmp.x, &tmp.y, &tmp.z);
            switch(sc) {
                //upper left screen corner
                case 3:
                    res->ul = tmp;
                    break;
                //bottom left screen corner
                case 2:
                    res->bl = tmp;
                    break;
                //upper right screen corner
                case 1:
                    res->ur = tmp;
                    break;
            }
            sc--;
        /* screen resolution */
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


void scn_camera_destroy(SCN_Camera *self) {
    free(self);
}


void scn_scene_destroy(SCN_Scene *self) {
    if(self->v)
        free(self->v);
    if(self->t)
        free(self->t);
    if(self->l)
        free(self->l);
    if(self->s)
        free(self->s);
    free(self);
}
