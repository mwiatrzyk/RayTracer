#include "scene.h"
#include "error.h"
#include "vectormath.h"
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
        if(strstr(buffer, "//") == buffer)
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
    SCN_Vertex4f *v=NULL;
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
            v = malloc(vcount*sizeof(SCN_Vertex4f));  //create array of vertices
            if (!v) {
                free(res);  //unable to allocate memory for vertices - NULL must be returned
                res = NULL;
                errno = E_MEMORY;
                goto cleanup;
            }

        /* reading vertices into array */
        } else if (vcount > 0) {
            sscanf(line, "%f %f %f", &v[i][0], &v[i][1], &v[i][2]);
            i++; vcount--;

        /* reading number of triangles */
        } else if (tcount == -1) {
            sscanf(line, "%d", &tcount);
            i = 0;
            res->tsize = tcount;
            res->t = malloc(tcount*sizeof(SCN_Triangle));  //create array of triangles
            if (!res->t) {
                free(res);
                res = NULL;
                errno = E_MEMORY;
                goto cleanup;
            }

        /* reading triangles into array */
        } else if (tcount > 0) {
            sscanf(line, "%d %d %d", &_i, &j, &k);
            //res->t[i].i = v[_i];  //assign address of correct vertex
            vec_vector_copy(v[_i], res->t[i].i);
            //res->t[i].j = v[j];
            vec_vector_copy(v[j], res->t[i].j);
            //res->t[i].k = v[k];
            vec_vector_copy(v[k], res->t[i].k);
            i++; tcount--;
        
        /* initialize pcount variable and read first line of part assignment */
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
        if(v) free(v);
        if(fd) fclose(fd);

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
            sscanf(line, "%f %f %f %f %f %f %f", &self->l[i].p[0], &self->l[i].p[1], &self->l[i].p[2], 
                                                 &self->l[i].flux, 
                                                 &self->l[i].color.r, &self->l[i].color.g, &self->l[i].color.b);
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
    char *line=NULL, *pch;
    int32_t scount=-1, i, j;
    float kd, ks, g, ka, R, G, B;
    float kt, eta, kr, tmp;
    
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
            j = 0;
            pch = strtok(line, " \t");
            while(pch != NULL) {
                sscanf(pch, "%f", &tmp);
                pch = strtok(NULL, " \t");
                switch(j) {
                    case 0:
                        self->s[i].kd = tmp;
                        break;
                    case 1:
                        self->s[i].ks = tmp;
                        break;
                    case 2:
                        self->s[i].g = tmp;
                        break;
                    case 3:
                        self->s[i].ka = tmp;
                        break;
                    case 4:
                        self->s[i].color.r = tmp;
                        break;
                    case 5:
                        self->s[i].color.g = tmp;
                        break;
                    case 6:
                        self->s[i].color.b = tmp;
                        break;
                    case 7: 
                        self->s[i].kt = tmp;
                        break;
                    case 8:
                        self->s[i].eta = tmp;
                        break;
                    case 9:
                        self->s[i].kr = tmp;
                        break;
                }
                if(++j >= 10)
                    break;
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
    SCN_Vertex4f tmp;

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
            sscanf(line, "%f %f %f", &res->ob[0], &res->ob[1], &res->ob[2]);
            vp--;
        /* screen position */
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
    if(self->t)
        free(self->t);
    if(self->l)
        free(self->l);
    if(self->s)
        free(self->s);
    free(self);
}
