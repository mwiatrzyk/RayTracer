#include "vectormath.h"
#include <math.h>


inline SCN_Vertex vec_make_vector(SCN_Vertex *a, SCN_Vertex *b) {
    SCN_Vertex res = {
        b->x - a->x,
        b->y - a->y,
        b->z - a->z
    };
    return res;
}


inline SCN_Vertex vec_normalize(SCN_Vertex *v) {
    float len=sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
    SCN_Vertex res = {
        v->x / len,
        v->y / len,
        v->z / len
    };
    return res;
}


inline SCN_Vertex vec_mul(SCN_Vertex *v, float t) {
    SCN_Vertex res = {
        v->x * t,
        v->y * t,
        v->z * t
    };
    return res;
}


inline SCN_Vertex vec_sum(SCN_Vertex *v1, SCN_Vertex *v2) {
    SCN_Vertex res = {
        v1->x + v2->x,
        v1->y + v2->y,
        v1->z + v2->z
    };
    return res;
}


inline SCN_Vertex vec_crossproduct(SCN_Vertex *v1, SCN_Vertex *v2) {
    SCN_Vertex res = {
        v1->y*v2->z - v1->z*v2->y,
        v1->z*v2->x - v1->x*v2->z,
        v1->x*v2->y - v1->y*v2->x
    };
    return res;
}
