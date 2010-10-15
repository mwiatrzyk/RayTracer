#include "raytrace.h"
#include "vectormath.h"


//// HELPER FUNCTIONS /////////////////////////////////////////


//// INTERFACE FUNCTIONS //////////////////////////////////////

IML_Bitmap* rtr_execute(SCN_Scene *scene, SCN_Camera *camera) {
    /* Vector from upper-left towards upper-right screen corner. Length of this
     * vector matches "width" of single virtual screen pixel. */
    SCN_Vertex vx = {  
        (camera->ur.x - camera->ul.x) / camera->sw,
        (camera->ur.y - camera->ul.y) / camera->sw,
        (camera->ur.z - camera->ul.z) / camera->sw
    };

    /* Vector from upper-left towards bottom-left screen corner. Length of this
     * vector matches "height" of single virtual screen pixel. */
    SCN_Vertex vy = {  
        (camera->bl.x - camera->ul.x) / camera->sh,
        (camera->bl.y - camera->ul.y) / camera->sh,
        (camera->bl.z - camera->ul.z) / camera->sh
    };
    
    float x, y, w=camera->sw, h=camera->sh;
    SCN_Vertex vx_mul_x, vy_mul_y;
    SCN_Vertex dir;
    SCN_Vertex cp, ray;

    // main loop
    for(y=0.5f; y<h; y+=1.0f) {
        for(x=0.5f; x<w; x+=1.0f) {
            vx_mul_x = vec_mul(&vx, x);
            vy_mul_y = vec_mul(&vy, y);
            dir = vec_sum(&vx_mul_x, &vy_mul_y);
            cp = vec_sum(&camera->ul, &dir);
            ray = vec_make_vector(&camera->ob, &cp);
            ray = vec_normalize(&ray);
            //printf("%f %f %f\n", ray.x, ray.y, ray.z);
        }
    }

    return NULL;
}
