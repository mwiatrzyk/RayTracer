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
static int is_intersection(SCN_Triangle *t, SCN_Vertex *o, SCN_Vertex *r, float *d) {
    #define EPSILON 0.000001f
    SCN_Vertex pvec, tvec, qvec;
    float det, inv_det, u, v;
    
    vec_vector_crossp(&pvec, r, &t->ik);

    det = vec_vector_dotp(&t->ij, &pvec);

    if(det > -EPSILON && det < EPSILON) {
        return 0;
    }
    
    inv_det = 1.0f / det;

    vec_vector_make(&tvec, t->i, o);

    u = vec_vector_dotp(&tvec, &pvec) * inv_det;
    if(u < 0.0f || u > 1.0f) {
        return 0;
    }

    vec_vector_crossp(&qvec, &tvec, &t->ij);
    v = vec_vector_dotp(r, &qvec) * inv_det;
    if(v < 0.0f || u + v > 1.0f) {
        return 0;
    }
    
    *d = vec_vector_dotp(&t->ik, &qvec) * inv_det;
    if(*d < 0.0f)
        return 0;

    return 1;
}


static int is_shadow(SCN_Triangle *t, SCN_Triangle *maxt, SCN_Triangle *current, SCN_Vertex *o, SCN_Vertex *r, SCN_Vertex *lpos) {
    float d, dmax=vec_vector_distance(o, lpos);
    SCN_Vertex cl, ct;
    
    //vec_vector_ray(&r, o, lpos);    // create ray vector pointing towards light
    
    while(t < maxt) {
        if(t != current) {
            if(is_intersection(t, o, r, &d)) {
                if(d < dmax) {
                    return 1;
                }
            }
        }
        t++;
    }

    return 0;
}


/* RayTracing algorithm implementation. 

 @param: t: pointer to scene's triangle array 
 @param: maxt: pointer to first element beyond triangle array
 @param: l: pointer to lights array
 @param: maxl: pointer to first element beyond light array
 @param: o: ray origin point
 @param: r: ray vector (normalized) */
static IML_Color raytrace(SCN_Triangle *t, SCN_Triangle *maxt, SCN_Triangle *skip, 
                          SCN_Light *l, SCN_Light *maxl, 
                          SCN_Vertex *o, SCN_Vertex *r, 
                          float total_flux, uint32_t level) 
{
    IML_Color res={0.0f, 0.0f, 0.0f, 0.0f}, tmp, rcolor;
    SCN_Vertex onew, rnew, rray, tmpv;
    SCN_Triangle *nearest=NULL, *tt=t;
    float d, dmin=FLT_MAX, df, rf, n_dot_lo;
    
    if(level == 0) {
        return res;
    }

    while(tt < maxt) {
        if(tt != skip) {
            if(is_intersection(tt, o, r, &d)) {
                if(d < dmin) {
                    dmin = d;
                    nearest = tt;
                }
            }
        }
        #ifdef BENCHMARK
            intersection_test_count++;
        #endif
        tt++;
    }
    
    //if(level < 10) 
    //    printf("%p\n", nearest);

    if(nearest) {
        /* Initialize result color with ambient color */
        iml_color_scale(&res, &nearest->s->color, nearest->s->ka * total_flux);

        /* calculate intersection point with nearest triangle */
        vec_vector_raypoint(&onew, o, r, dmin);
        
        /* raytrace reflected ray */
        if(nearest->s->ks > 0.0f) {
            vec_vector_ray_reflected(&rray, &nearest->n, vec_vector_inverse(&tmpv, r));
            rcolor = raytrace(t, maxt, nearest,
                              l, maxl, 
                              &onew, &rray, total_flux, level-1);
            iml_color_add(&res, &res, iml_color_scale(&rcolor, &rcolor, nearest->s->ks));
        }

        /* raytrace refracted ray */
        if(nearest->s->kr > 0.0f) {
            vec_vector_ray_refracted(&rray, &nearest->n, vec_vector_inverse(&tmpv, r), nearest->s->eta);
            rcolor = raytrace(t, maxt, nearest,
                              l, maxl,
                              &onew, &rray, total_flux, level-1);
            iml_color_add(&res, &res, iml_color_scale(&rcolor, &rcolor, nearest->s->kr));
        }

        /* calculate color at intersection point */
        while(l < maxl) {
            df = rf = 0.0f;
            vec_vector_ray(&rnew, &onew, &l->p);
             
            if(!is_shadow(t, maxt, nearest, &onew, &rnew, &l->p)) {
                n_dot_lo = vec_vector_dotp(&nearest->n, &rnew);

                /* diffusion factor */
                df = nearest->s->kd * n_dot_lo;

                /* reflection factor */
                rf = nearest->s->ks * pow(vec_vector_dotp(r, vec_vector_ray_reflected2(&tmpv, &nearest->n, &rnew, n_dot_lo)), nearest->s->g);

                /* calculate color */
                iml_color_add(&tmp, &l->color, &nearest->s->color);
                iml_color_scale(&tmp, &tmp, l->flux*(df+rf));
                iml_color_add(&res, &res, &tmp);
            }
            l++;
        }
    }

    return res;
}


/* Performs scene preprocessing. Calculates all constant coefficients for each
 * triangle in scene. */
static SCN_Scene* preprocess_scene(SCN_Scene *scene, SCN_Camera *camera) {
    SCN_Triangle *t=scene->t, *maxt=(SCN_Triangle*)(scene->t + scene->tsize);
    SCN_Vertex ij, ik, io, n;
    while(t < maxt) {
        /* make vectors i->j and i->k (used by intersection test function) */
        vec_vector_make(&t->ij, t->i, t->j);
        vec_vector_make(&t->ik, t->i, t->k);
        
        /* make vector from observer towards one of triangle vertices */
        vec_vector_normalize(vec_vector_make(&io, t->i, &camera->ob));

        /* create and normalize normal vector and point it towards camera */
        vec_vector_normalize(vec_vector_crossp(&t->n, &t->ij, &t->ik));
        if(vec_vector_dotp(&t->n, &io) < 0.0f) {
            vec_vector_inverse(&t->n, &t->n);
        }

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
            color = raytrace(scene->t, (SCN_Triangle*)(scene->t+scene->tsize),
                             scene->l, (SCN_Light*)(scene->l+scene->lsize),
                             o, &ray);
            iml_bitmap_setpixel(res, (int32_t)x, (int32_t)y, color);    
        }
    }

    #ifdef BENCHMARK
        double total_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        printf("Intersection tests per second: %lu\n", (uint32_t)(intersection_test_count/total_time));
    #endif
    
    return res;
}
