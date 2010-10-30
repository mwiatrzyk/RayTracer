#include <float.h>
#include "raytrace.h"
#include "vectormath.h"

#define EPSILON 0.000001f
#define BENCHMARK

#ifdef BENCHMARK
#include <time.h>
#endif


//// GLOBALS //////////////////////////////////////////////////

#ifdef BENCHMARK
uint32_t intersection_test_count = 0;
#endif

//// INTERSECTION ALGORITHMS //////////////////////////////////

//// 1st algorithm

#if !defined(INT_ALG) || INT_ALG == 1
/* First intersection algorithm. Works by solving triangle parametric equation
 * and ray parametric function system. Gives 3 parameters in output: `u`, `v`
 * (coords relative to triangle) and `d` (distance to intersection point). */
static int is_intersection(SCN_Triangle *t, float *o, float *r, float *d, float *dmin) {
    float pvec, tvec, qvec, tmp;
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
#endif  //INT_ALG == 1


//// 2nd algorithm

#if INT_ALG == 2
/* Second intersection algorithm. This algorithm works performs projection of
 * 3D triangles into 2D plane in a way that won't cause triangles to be reduced
 * to segments. Gives only `d` parameter (distance to intersection point). */
static int is_intersection(SCN_Triangle *t, float *o, float *r, float *d, float *dmin) {
    #ifdef BENCHMARK
    intersection_test_count++;
    #endif
    float rdn = vec_vector_dotp(r, t->n);
    if(rdn == 0.0f)
        return 0;
    *d = -(vec_vector_dotp(o, t->n) + t->d) / rdn;
    if(*d < 0.0f || *d > *dmin)
        return 0;
    float x, y;
    SCN_ProjectionPlane pplane = t->pplane;
    if(pplane == PP_XOY) {
        x = o[0] + (*d)*r[0];
        y = o[1] + (*d)*r[1];
    } else if(pplane == PP_XOZ) {
        x = o[0] + (*d)*r[0];
        y = o[2] + (*d)*r[2];
    } else {  // PP_ZOY
        x = o[2] + (*d)*r[2];
        y = o[1] + (*d)*r[1];
    }
    if(x < t->minx) return 0;
    if(x > t->maxx) return 0;
    if(y < t->miny) return 0;
    if(y > t->maxy) return 0;
    if(t->A[0]*x + t->B[0]*y < -t->C[0]) return 0;
    if(t->A[1]*x + t->B[1]*y < -t->C[1]) return 0;
    if(t->A[2]*x + t->B[2]*y < -t->C[2]) return 0;
    return 1;
}


/* Tests if given triangle can be projected onto given plane without being
 * reduced to segment. This is helper funtion, executed once for each triangle
 * in preprocessing step. */
static int is_projection_possible(SCN_Triangle *t, SCN_ProjectionPlane pplane) {
    float x1, x2, x3, y1, y2, y3, p1, q1, p2, q2, l1, l2, dotp;
    switch(pplane) {
        case PP_XOY:
            x1 = t->i[0]; x2 = t->j[0]; x3 = t->k[0];
            y1 = t->i[1]; y2 = t->j[1]; y3 = t->k[1];
            break;
        case PP_XOZ:
            x1 = t->i[0]; x2 = t->j[0]; x3 = t->k[0];
            y1 = t->i[2]; y2 = t->j[2]; y3 = t->k[2];
            break;
        default:  //PP_ZOY
            x1 = t->i[2]; x2 = t->j[2]; x3 = t->k[2];
            y1 = t->i[1]; y2 = t->j[1]; y3 = t->k[1];
            break;
    }
    p1 = x2 - x1; q1 = y2 - y1;  // build vector from (x1, y1) to (x2, y2)
    p2 = x3 - x1; q2 = y3 - y1;  // build vector from (x1, y1) to (x2, y2)
    if((p1 == 0.0f && q1 == 0.0f) || (p2 == 0.0f && q2 == 0.0f))  // [0,0] vectors - projection not possible
        return 0;
    l1 = sqrt(p1*p1 + q1*q1);   
    l2 = sqrt(p2*p2 + q2*q2);
    p1 /= l1; q1 /= l1;  // normalize vectors
    p2 /= l2; q2 /= l2;
    if((p1 == p2 && q1 == q2) || (p1+p2 == 0.0f && q1+q2 == 0.0f)) {
        return 0;
    } else {
        return 1;
    }
}


/* Calculates coefficients of triangle's bounding lines after projection onto
 * 2D plane. */
static void calc_line_coefficients(float *a, float *b, float *c, 
                                   SCN_ProjectionPlane pplane, 
                                   float *A, float *B, float *C) {
    float x1, x2, y1, y2, x3, y3;
    switch(pplane) {
        case PP_XOY:
            x1 = a[0]; x2 = b[0]; x3 = c[0];
            y1 = a[1]; y2 = b[1]; y3 = c[1];
            break;
        case PP_XOZ:
            x1 = a[0]; x2 = b[0]; x3 = c[0];
            y1 = a[2]; y2 = b[2]; y3 = c[2];
            break;
        default:  //PP_ZOY
            x1 = a[2]; x2 = b[2]; x3 = c[2];
            y1 = a[1]; y2 = b[1]; y3 = c[1];
            break;
    }
    *A = y2 - y1;
    *B = x1 - x2;    // equals to: -(x2 - x1)
    *C = -((*A)*x1 + (*B)*y1);  // Ax+By+C=0 => C=-(Ax+By)
    if((*A)*x3 + (*B)*y3 + *C < 0.0f) {
        *A = -(*A);
        *B = -(*B);
        *C = -(*C);
    }
}
#endif  //INT_ALG == 2


//// SHADOW TEST //////////////////////////////////////////////

static int is_shadow(SCN_Triangle *t, SCN_Triangle *maxt, SCN_Triangle *current, float *o, float *r, float *l) {
    int i;
    float d, dmax=vec_vector_distance(o, l), dmin=FLT_MAX;
    float min[4];
    float max[4];

    for(i=0; i<3; i++) {
        if(o[i] < l[i]) {
            min[i]=o[i]; max[i]=l[i];
        } else {
            min[i]=l[i]; max[i]=o[i];
        }
    }
    
    while(t < maxt) {
        if(t != current) {
            if(t->i[0]<min[0] && t->j[0]<min[0] && t->k[0]<min[0]) {t++; continue;}
            if(t->i[0]>max[0] && t->j[0]>max[0] && t->k[0]>max[0]) {t++; continue;}
            if(t->i[1]<min[1] && t->j[1]<min[1] && t->k[1]<min[0]) {t++; continue;}
            if(t->i[1]>max[1] && t->j[1]>max[1] && t->k[1]>max[0]) {t++; continue;}
            if(t->i[2]<min[2] && t->j[2]<min[2] && t->k[2]<min[0]) {t++; continue;}
            if(t->i[2]>max[2] && t->j[2]>max[2] && t->k[2]>max[0]) {t++; continue;}
            if(is_intersection(t, o, r, &d, &dmin)) {
                if(d < dmax) {
                    return 1;
                }
            }
        }
        t++;
    }

    return 0;
}


//// MAIN FUNCTIONS ///////////////////////////////////////////

/* RayTracing algorithm implementation. 

 @param: t: pointer to scene's triangle array 
 @param: maxt: pointer to first element beyond triangle array
 @param: l: pointer to lights array
 @param: maxl: pointer to first element beyond light array
 @param: o: ray origin point
 @param: r: ray vector (normalized) */
static IML_Color raytrace(SCN_Triangle *t, SCN_Triangle *maxt, SCN_Triangle *skip, 
                          SCN_Light *l, SCN_Light *maxl, 
                          float *o, float *r, 
                          float total_flux, uint32_t level) 
{
    IML_Color res={0.0f, 0.0f, 0.0f, 0.0f}, tmp, rcolor;
    float onew[4], rnew[4], rray[4], tmpv[4];
    SCN_Triangle *nearest=NULL, *tt=t;
    float d, dmin=FLT_MAX, df, rf, n_dot_lo, dm;
    
    if(level == 0) {
        return res;
    }

    while(tt < maxt) {
        if(tt != skip) {
            if(is_intersection(tt, o, r, &d, &dmin)) {
                if(d < dmin) {
                    dmin = d;
                    nearest = tt;
                }
            }
        }
        tt++;
    }
    
    if(nearest) {
        // initialize result color with ambient color
        iml_color_scale(&res, &nearest->s->color, nearest->s->ka * total_flux);

        // calculate intersection point with nearest triangle
        vec_vector_raypoint(onew, o, r, dmin);
        
        // raytrace reflected ray
        if(nearest->s->kr > 0.0f) {
            vec_vector_ray_reflected(rray, nearest->n, vec_vector_inverse(tmpv, r));
            rcolor = raytrace(t, maxt, nearest, l, maxl, onew, rray, total_flux, level-1);
            iml_color_add(&res, &res, iml_color_scale(&rcolor, &rcolor, nearest->s->kr));
        }

        // raytrace refracted ray
        if(nearest->s->kt > 0.0f) {
            vec_vector_ray_refracted(rray, nearest->n, vec_vector_inverse(tmpv, r), nearest->s->eta);
            rcolor = raytrace(t, maxt, nearest, l, maxl, onew, rray, total_flux, level-1);
            iml_color_add(&res, &res, iml_color_scale(&rcolor, &rcolor, nearest->s->kt));
        }

        // calculate color at intersection point
        while(l < maxl) {
            df = rf = 0.0f;
            vec_vector_ray(rnew, onew, l->p);
             
            if(!is_shadow(t, maxt, nearest, onew, rnew, l->p)) {
                dm = vec_vector_distance(onew, l->p);
                n_dot_lo = vec_vector_dotp(nearest->n, rnew);

                // diffusion factor
                df = nearest->s->kd * n_dot_lo;

                // reflection factor
                rf = nearest->s->ks * pow(vec_vector_dotp(r, vec_vector_ray_reflected2(tmpv, nearest->n, rnew, n_dot_lo)), nearest->s->g);

                // calculate color
                iml_color_add(&tmp, &l->color, &nearest->s->color);
                iml_color_scale(&tmp, &tmp, l->flux*(df+rf)/(dm+0.001f));
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
    #if INT_ALG == 2
    float A, B, C;
    #endif
    SCN_Triangle *t=scene->t, *maxt=(SCN_Triangle*)(scene->t + scene->tsize);
    float ij[4], ik[4], io[4], n[4];
    while(t < maxt) {
        // calculate vectors used to calculate normal
        vec_vector_make(t->ij, t->i, t->j);
        vec_vector_make(t->ik, t->i, t->k);
        
        // make vector from observer towards one of triangle vertices
        vec_vector_normalize(vec_vector_make(io, t->i, camera->ob));

        // create and normalize normal vector and point it towards camera
        vec_vector_normalize(vec_vector_crossp(t->n, t->ij, t->ik));
        if(vec_vector_dotp(t->n, io) < 0.0f) {
            vec_vector_inverse(t->n, t->n);
        }

        #if INT_ALG == 2
        // calculate d coefficient of plane equation
        t->d = -vec_vector_dotp(t->i, t->n);
        
        // project triangle onto coordinate system (one of XOY, XOZ, ZOY) that
        // won't cause reduction of triangle to segment
        if(is_projection_possible(t, PP_XOY)) {
            t->pplane = PP_XOY;
            t->minx = MIN(t->i[0], t->j[0], t->k[0]);
            t->miny = MIN(t->i[1], t->j[1], t->k[1]);
            t->maxx = MAX(t->i[0], t->j[0], t->k[0]);
            t->maxy = MAX(t->i[1], t->j[1], t->k[1]);
        } else if(is_projection_possible(t, PP_XOZ)) {
            t->pplane = PP_XOZ;
            t->minx = MIN(t->i[0], t->j[0], t->k[0]);
            t->miny = MIN(t->i[2], t->j[2], t->k[2]);
            t->maxx = MAX(t->i[0], t->j[0], t->k[0]);
            t->maxy = MAX(t->i[2], t->j[2], t->k[2]);
        } else {
            t->pplane = PP_ZOY;
            t->minx = MIN(t->i[2], t->j[2], t->k[2]);
            t->miny = MIN(t->i[1], t->j[1], t->k[1]);
            t->maxx = MAX(t->i[2], t->j[2], t->k[2]);
            t->maxy = MAX(t->i[1], t->j[1], t->k[1]);
        }
        calc_line_coefficients(t->i, t->j, t->k, t->pplane, &t->A[0], &t->B[0], &t->C[0]);
        calc_line_coefficients(t->j, t->k, t->i, t->pplane, &t->A[1], &t->B[1], &t->C[1]);
        calc_line_coefficients(t->i, t->k, t->j, t->pplane, &t->A[2], &t->B[2], &t->C[2]);
        #endif

        t++;
    }
    return scene;
}


//// INTERFACE FUNCTIONS //////////////////////////////////////

/* Executes raytracing algorithm on given scene and camera configuration
 * producing output bitmap. 

 @param: scene: pointer to scene object
 @param: camera: pointer to camera object */
IML_Bitmap* rtr_execute(SCN_Scene *scene, SCN_Camera *camera) {
    IML_Color color;
    int i;
    float x, y, dx, dy, w=camera->sw, h=camera->sh, total_flux=3000.0f, samples=1.0f;
    float *a=camera->ul, *b=camera->ur, *c=camera->bl, *o=camera->ob;
    IML_Bitmap *res=iml_bitmap_create(camera->sw, camera->sh, 0);
    
    // preprocess scene (calculate all needed coefficients etc.)
    preprocess_scene(scene, camera);

    #ifdef BENCHMARK
        clock_t start = clock();
    #endif
    
    // calculate total flux of all lights
    /*for(i=0; i<scene->lsize; i++) {
        total_flux += scene->l[i].flux;
    }*/

    // main loop
    for(y=0.0f; y<h; y+=1.0f) {
        for(x=0.0f; x<w; x+=1.0f) {
            IML_Color sum={0.0f, 0.0f, 0.0f, 0.0f};

            for(i=0, dy=0.0f; dy<1.0f; dy+=samples) {
                for(dx=0.0f; dx<1.0f; dx+=samples, i++) {
                    /* Calculate primary ray direction vector and normalize it. */
                    float x_coef=(x+dx)/w, y_coef=(y+dy)/h;
                    float ray[4] = {
                        x_coef*(b[0] - a[0]) + y_coef*(c[0] - a[0]) + a[0] - o[0],
                        x_coef*(b[1] - a[1]) + y_coef*(c[1] - a[1]) + a[1] - o[1],
                        x_coef*(b[2] - a[2]) + y_coef*(c[2] - a[2]) + a[2] - o[2],
                        0.0f
                    };
                    vec_vector_normalize(ray);

                    /* Trace current ray and calculate color of current pixel. */
                    color = raytrace(scene->t, (SCN_Triangle*)(scene->t+scene->tsize), NULL,
                                     scene->l, (SCN_Light*)(scene->l+scene->lsize),
                                     o, ray, total_flux, 10);

                    /* Normalize color */
                    iml_color_scale(&color, &color, 255.0f/total_flux);

                    /* Add to total samples */
                    iml_color_add(&sum, &sum, &color);
                }
            }

            /* Calculate average pixel color */
            iml_color_scale(&sum, &sum, 1.0f/(float)i);
            
            /* Write pixel onto bitmap */
            iml_bitmap_setpixel(res, (int32_t)x, (int32_t)y,
                                iml_rgba(sum.r>=0.0f ? (sum.r<=255.0f ? sum.r : 255.0f) : 0.0f,
                                         sum.g>=0.0f ? (sum.g<=255.0f ? sum.g : 255.0f) : 0.0f,
                                         sum.b>=0.0f ? (sum.b<=255.0f ? sum.b : 255.0f) : 0.0f, 0));
        }
    }

    #ifdef BENCHMARK
        double total_time = (double)(clock()-start)/CLOCKS_PER_SEC;
        printf("Intersection tests per second: %lu\n", (uint32_t)(intersection_test_count/total_time));
    #endif
    
    return res;
}
