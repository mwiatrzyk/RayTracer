#include <float.h>
#include "raytrace.h"
#include "vectormath.h"

#define BENCHMARK

#ifdef BENCHMARK
#include <time.h>
#endif


//// GLOBALS //////////////////////////////////////////////////

#ifdef BENCHMARK
uint32_t intersection_test_count = 0;
#endif

//// HELPER FUNCTIONS /////////////////////////////////////////

/* Performs ray->triangle intersection test. Returns true if ray intersects
 * with triangle or false otherwise. 
 
 @param: t: triangle pointer
 @param: o: ray origin
 @param: r: ray direction (normalized)
 @param: d: when triangle intersects with ray, distance from ray origin to
    ray->triangle intersection point is stored here */
static int ray_triangle_intersection(SCN_Triangle *t, SCN_Vertex *o, SCN_Vertex *r, float *d) {
    #define EPSILON 0.000001f
    SCN_Vertex ij, ik, pvec, tvec, qvec;
    float det, inv_det, u, v;
    
    vec_vector_make(&ij, t->i, t->j);
    vec_vector_make(&ik, t->i, t->k);

    vec_vector_crossp(&pvec, r, &ik);

    det = vec_vector_dotp(&ij, &pvec);

    if(det > -EPSILON && det < EPSILON) {
        return 0;
    }
    
    inv_det = 1.0f / det;

    vec_vector_make(&tvec, t->i, o);

    u = vec_vector_dotp(&tvec, &pvec) * inv_det;
    if(u < 0.0f || u > 1.0f) {
        return 0;
    }

    vec_vector_crossp(&qvec, &tvec, &ij);
    v = vec_vector_dotp(r, &qvec) * inv_det;
    if(v < 0.0f || u + v > 1.0f) {
        return 0;
    }
    
    *d = vec_vector_dotp(&ik, &qvec) * inv_det;

    return 1;
}


/* RayTracing algorithm implementation. 

 @param: t: pointer to scene's triangle array 
 @param: maxt: pointer to first element above scene's triangle array 
 @param: o: ray origin point
 @param: r: ray vector (normalized) */
static int32_t raytrace(SCN_Triangle *t, SCN_Triangle *maxt, SCN_Vertex *o, SCN_Vertex *r) {
    /* Find nearest triangle that intersects with given ray. */
    SCN_Triangle *nearest=NULL;
    float d, dmin=FLT_MAX, ray_dotp_n;
    while(t < maxt) {
        if(ray_triangle_intersection(t, o, r, &d)) {
            if(d < dmin) {
                dmin = d;
                nearest = t;
            }
        }
        #ifdef BENCHMARK
            intersection_test_count++;
        #endif
        t++;
    }
    if(nearest) {
        uint32_t r, g, b;
        r = nearest->s->R * 255.0f;
        g = nearest->s->G * 255.0f;
        b = nearest->s->B * 255.0f;
        return iml_rgba(r, g, b, 0);
    } else {
        return 0;
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

    #ifdef BENCHMARK
        clock_t start = clock();
    #endif

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
            vec_vector_normalize(&ray);

            /* Trace current ray and calculate color of current pixel. */
            color = raytrace(scene->t, (SCN_Triangle*)(scene->t+scene->tsize), o, &ray);
            iml_bitmap_setpixel(res, (int32_t)x, (int32_t)y, color);    
        }
    }

    #ifdef BENCHMARK
        double total_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        printf("Intersection tests per second: %lu\n", (uint32_t)(intersection_test_count/total_time));
    #endif
    
    return res;
}
