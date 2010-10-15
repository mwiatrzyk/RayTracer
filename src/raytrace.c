#include "raytrace.h"
#include "vectormath.h"


//// HELPER FUNCTIONS /////////////////////////////////////////

/* RayTracing algorithm implementation. 

 @param: t: pointer to scene's triangle array 
 @param: maxt: pointer to first element above scene's triangle array 
 @param: o: ray origin point
 @param: ray: ray vector (normalized) */
static int32_t raytrace(SCN_Triangle *t, SCN_Triangle *maxt, SCN_Vertex *o, SCN_Vertex *ray) {
    // find nearest triangle that intersects with given ray
    SCN_Triangle *nearest=t;
    float d, dmin, ray_dotp_n;
    //float rx=ray->x, ry=ray->y, rz=ray->z;
    //float nx, ny, nz;
    while(t < maxt) {
        ray_dotp_n = vec_dotproduct(ray, &t->n);
        if(ray_dotp_n == 0.0f) {  // intersection point is somewhere in infinity (ray is parallel to triangle)
            t++;
            continue;
        }
        d = -(vec_dotproduct(o, &t->n) + t->d)/ray_dotp_n;
        if(d <= 0.0f) {  // TODO: t <= 0.0f or t < 0.0f ?
            t++;
            continue;  // intersection point is "behind" current ray
        }
        //printf("%f\n", d);
        t++;
    }
}



//// INTERFACE FUNCTIONS //////////////////////////////////////

IML_Bitmap* rtr_execute(SCN_Scene *scene, SCN_Camera *camera) {
    float x, y, w=camera->sw, h=camera->sh;
    SCN_Vertex *a=&camera->ul, *b=&camera->ur, *c=&camera->bl, *o=&camera->ob;

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
            //printf("%f %f %f\n", ray.x, ray.y, ray.z);
        }
    }

    return NULL;
}
