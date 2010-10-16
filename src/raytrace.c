#include <float.h>
#include "raytrace.h"
#include "vectormath.h"


//// HELPER FUNCTIONS /////////////////////////////////////////

/* RayTracing algorithm implementation. 

 @param: t: pointer to scene's triangle array 
 @param: maxt: pointer to first element above scene's triangle array 
 @param: o: ray origin point
 @param: r: ray vector (normalized) */
static int32_t raytrace(SCN_Triangle *t, SCN_Triangle *maxt, SCN_Vertex *o, SCN_Vertex *r) {
    /* Find nearest triangle that intersects with given ray. */
    SCN_Triangle *nearest=t;
    float d, dmin=FLT_MAX, ray_dotp_n;
    while(t < maxt) {
        ray_dotp_n = vec_dotproduct(r, &t->n); // vec_dotproduct(ray, &t->n);
        if(ray_dotp_n == 0.0f) {  // intersection point is somewhere in infinity (ray is parallel to triangle)
            t++;
            continue;
        }
        d = -(vec_dotproduct(o, &t->n) + t->d)/ray_dotp_n; //-(vec_dotproduct(o, &t->n) + t->d)/ray_dotp_n;
        if(d <= 0.0f) {  // TODO: t <= 0.0f or t < 0.0f ?
            t++;
            continue;  // intersection point is "behind" current ray
        }
        //printf("%f\n", d);
        t++;
    }
}


/* Performs scene preprocessing. Calculates all constant coefficients for each
 * triangle in scene. */
static SCN_Scene* preprocess_scene(SCN_Scene *scene, SCN_Camera *camera) {
    SCN_Triangle *t=scene->t, *maxt=(SCN_Triangle*)(scene->t + scene->tsize);
    printf("%ld\n", scene->tsize);
    while(t < maxt) {
        /* calculate normal vector and orient it towards observer */
        SCN_Vertex ij=vec_make_vector(t->i, t->j), ik=vec_make_vector(t->i, t->k);
        SCN_Vertex oi=vec_make_vector(&camera->ob, t->i);
        SCN_Vertex norm = vec_crossproduct(&ij, &ik);
        oi = vec_normalize(&oi);  // vector from observer towards current triangle (normalized)
        norm = vec_normalize(&norm);  // normal vector of current triangle (normalized)
        if(vec_dotproduct(&oi, &norm) < 0.0f) {
            norm = vec_mul(&norm, -1.0f);  // point normal vector towards observer
        }
        t->n = norm;

        /* calculate `d` parameter of triangle's plane equation: i*n+d=0 ->
         * d=-i*n, where: i - one of triangle's vertices, n - normal vector */
        t->d = -vec_dotproduct(t->i, &t->n);

        t++;
    }

    return scene;
}


//// INTERFACE FUNCTIONS //////////////////////////////////////

IML_Bitmap* rtr_execute(SCN_Scene *scene, SCN_Camera *camera) {
    uint32_t color;
    float x, y, w=camera->sw, h=camera->sh;
    SCN_Vertex *a=&camera->ul, *b=&camera->ur, *c=&camera->bl, *o=&camera->ob;
    IML_Bitmap *res=iml_bitmap_create(camera->sw, camera->sh, 0);
    
    // preprocess scene (calculate all needed coefficients etc.)
    preprocess_scene(scene, camera);

    // main loop
    for(y=0.5f; y<h; y+=1.0f) {
        for(x=0.5f; x<w; x+=1.0f) {
            /* Calculate primary ray direction vector and normalize it. */
            float x_coef=x/w, y_coef=y/h;
            SCN_Vertex ray = {
                x_coef*(b->x - a->x) + y_coef*(c->x - a->x) + a->x - o->x,
                x_coef*(b->y - a->y) + y_coef*(c->y - a->y) + a->y - o->y,
                x_coef*(b->z - a->z) + y_coef*(c->z - a->z) + a->z - o->z
            };
            ray = vec_normalize(&ray);

            /* Trace current ray and calculate color of current pixel. */
            color = raytrace(scene->t, (SCN_Triangle*)(scene->t+scene->tsize), o, &ray);
            iml_bitmap_setpixel(res, (int32_t)x, (int32_t)y, color);    
        }
    }

    return res;
}
