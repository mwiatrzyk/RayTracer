#include <stdio.h>
#include <time.h>
#include <errno.h>
#include "imagelib.h"
#include "scene.h"
#include "rdtsc.h"
#include "raytrace.h"

int main(int argc, char* argv[]) {
    SCN_Scene *scene=scn_scene_load("./scenes/pokoj/s2.brs");
    scn_scene_load_lights(scene, "./scenes/pokoj/s2.lgt");
    scn_scene_load_surface(scene, "./scenes/pokoj/s2.atr");
    SCN_Camera *cam=scn_camera_load("./scenes/pokoj/s2.cam");
    
    clock_t start = clock();
    IML_Bitmap *bmp=rtr_execute(scene, cam);
    printf("Time taken: %f seconds\n", (double)(clock()-start)/CLOCKS_PER_SEC);

    iml_bitmap_save(bmp, "./result.bmp", 24);
    iml_bitmap_destroy(bmp);

    return 0;
}
