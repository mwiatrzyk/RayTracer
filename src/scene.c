#include "scene.h"
#include "error.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


SCN_Scene* scn_scene_load(const char *filename) {
    char c;
    int32_t i, _i, j, k, len, vcount=-1, tcount=-1, pcount=-1;
    char *pch;
    FILE *fd=NULL;
    char *buffer=NULL;
    SCN_Scene *res=NULL;

    fd=fopen(filename, "r");
    if (!fd) {
        errno = E_IO;
        goto cleanup;
    }
    
    uint16_t buf_size=1024;
    buffer = malloc(buf_size);
    if (!buffer) {
        errno = E_MEMORY;
        goto cleanup;
    }
    
    while(!feof(fd)) {
        if(!fgets(buffer, buf_size, fd))
            break;
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

        /* reading number of vertices */
        if(vcount == -1) {
            sscanf(buffer, "%d", &vcount);  //read number of vertices
            res = malloc(sizeof(SCN_Scene));  //create SCN_Scene object
            if (!res) {
                errno = E_MEMORY;
                goto cleanup;
            }
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
            sscanf(buffer, "%f %f %f", &res->v[i].x, &res->v[i].y, &res->v[i].z);
            i++; vcount--;

        /* reading number of triangles */
        } else if (tcount == -1) {
            sscanf(buffer, "%d", &tcount);
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
            sscanf(buffer, "%d %d %d", &_i, &j, &k);
            res->t[i].i = &res->v[_i];  //assign address of correct vertex
            res->t[i].j = &res->v[j];
            res->t[i].k = &res->v[k];
            i++; tcount--;
        
        /* initialize pcount variable */
        } else if (pcount == -1) {
            i = 0;
            pcount = res->tsize;

        /* reading part assignment of triangles */
        } else if (pcount > 0) {
            pch = strtok(buffer, " \t\n\r");  //FIXME: use another function here (strtok is not recommended)
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
        if(fd)
            fclose(fd);
        if(buffer)
            free(buffer);

    return res;
}
